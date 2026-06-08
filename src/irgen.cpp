// NSL IR Generation Pass

#include "irgen.hpp"
#include <stdexcept>

namespace nsl {

IRGen::IRGen(DiagnosticEngine& diag, SemanticAnalyzer& sema)
    : m_diag(diag), m_sema(sema), m_builder(m_mod) {}

ir::Module IRGen::generate(Program& prog) {
    m_mod.name = prog.programName;
    visitProgram(prog);
    return std::move(m_mod);
}

// ── Variable scope helpers ─────────────────────────────────────────────────
void IRGen::defineVar(const std::string& name, ir::Value ptr, TypePtr t) {
    if (!m_varScopes.empty())
        m_varScopes.back()[name] = {ptr, t};
}

IRGen::VarEntry* IRGen::lookupVar(const std::string& name) {
    for (int i = static_cast<int>(m_varScopes.size()) - 1; i >= 0; --i) {
        auto it = m_varScopes[i].find(name);
        if (it != m_varScopes[i].end()) return &it->second;
    }
    return nullptr;
}

TypePtr IRGen::resolveType(const std::string& name) {
    return m_sema.typeRegistry().lookup(name);
}

TypePtr IRGen::resolveType(const TypeExpr& te) {
    return m_sema.typeRegistry().resolve(te);
}

ir::Value IRGen::emitExpr(Expr& e) {
    m_exprValue = std::nullopt;
    e.accept(*this);
    if (m_exprValue) return *m_exprValue;
    // Return a null-typed dummy so code generation continues
    ir::Value dummy; dummy.name = "undef"; dummy.type = Type::makeUnknown();
    return dummy;
}

void IRGen::emitStmts(std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts) if (s) s->accept(*this);
}

// ── Opcode mapping ─────────────────────────────────────────────────────────
ir::Opcode IRGen::binOpcode(const std::string& op) {
    if (op == "+")  return ir::Opcode::Add;
    if (op == "-")  return ir::Opcode::Sub;
    if (op == "*")  return ir::Opcode::Mul;
    if (op == "/")  return ir::Opcode::Div;
    if (op == "%")  return ir::Opcode::Mod;
    if (op == "==") return ir::Opcode::CmpEq;
    if (op == "!=") return ir::Opcode::CmpNe;
    if (op == "<")  return ir::Opcode::CmpLt;
    if (op == "<=") return ir::Opcode::CmpLe;
    if (op == ">")  return ir::Opcode::CmpGt;
    if (op == ">=") return ir::Opcode::CmpGe;
    if (op == "and")return ir::Opcode::LogAnd;
    if (op == "or") return ir::Opcode::LogOr;
    return ir::Opcode::Add; // fallback
}

// ══════════════════════════════════════════════════════════════════════════
// Program
// ══════════════════════════════════════════════════════════════════════════
void IRGen::visitProgram(Program& n) {
    for (auto& d : n.decls) if (d) d->accept(*this);
}

void IRGen::visitUseDecl(UseDecl& n) {
    std::string modName;
    for (std::size_t i = 0; i < n.modulePath.size(); ++i) {
        if (i) modName += ".";
        modName += n.modulePath[i];
    }
    m_mod.imports.push_back(modName.empty() ? n.name : modName);
}

void IRGen::visitTypeDecl(TypeDecl& n) { /* struct types are handled during sema */ }

void IRGen::visitConstDecl(ConstDecl& n) {
    ir::GlobalVar g;
    g.name    = n.name;
    g.type    = resolveType(n.typeAnnotation);
    g.isConst = true;
    if (n.init) {
        ir::Value v = emitExpr(*n.init);
        g.initValue = v.name; // placeholder
    }
    m_mod.globals.push_back(g);
}

// ══════════════════════════════════════════════════════════════════════════
// Functions
// ══════════════════════════════════════════════════════════════════════════
void IRGen::visitFunctionDecl(FunctionDecl& n) {
    std::vector<ir::Parameter> params;
    for (auto& p : n.params) {
        params.push_back({p.name, resolveType(p.typeAnnotation)});
    }
    TypePtr retType = resolveType(n.returnType);

    m_builder.beginFunction(n.name, std::move(params), retType, n.isAsync, n.isPublic);
    pushVarScope();

    // Store parameters as addressable alloca slots
    for (auto& p : n.params) {
        TypePtr pt = resolveType(p.typeAnnotation);
        ir::Value alloc = m_builder.emitAlloca(pt);
        ir::Value paramVal; paramVal.name = p.name; paramVal.type = pt;
        m_builder.emitStore(pt, paramVal, alloc);
        defineVar(p.name, alloc, pt);
    }

    emitStmts(n.body);

    // Implicit void return
    if (retType && retType->isVoid())
        m_builder.emitReturn();

    popVarScope();
    m_builder.endFunction();
}

// ══════════════════════════════════════════════════════════════════════════
// Statements
// ══════════════════════════════════════════════════════════════════════════
void IRGen::visitLetDecl(LetDecl& n) {
    TypePtr t = n.typeAnnotation
        ? resolveType(*n.typeAnnotation)
        : Type::makeUnknown();

    ir::Value initVal;
    bool hasInit = false;
    if (n.init) {
        initVal = emitExpr(*n.init);
        hasInit = true;
        if (!t || t->kind == TypeKind::Unknown) t = initVal.type;
    }

    // Allocate based on memory region
    ir::Value alloc;
    switch (n.memSpec.region) {
        case MemRegion::Heap:
            alloc = m_builder.emitHeapAlloc(t, hasInit ? std::vector<ir::Value>{initVal}
                                                       : std::vector<ir::Value>{});
            break;
        case MemRegion::Stack:
            alloc = m_builder.emitStackAlloc(t, hasInit ? std::vector<ir::Value>{initVal}
                                                        : std::vector<ir::Value>{});
            break;
        case MemRegion::Arena:
            alloc = m_builder.emitArenaAlloc(t, n.memSpec.label,
                                             hasInit ? std::vector<ir::Value>{initVal}
                                                     : std::vector<ir::Value>{});
            break;
        case MemRegion::Zone:
            alloc = m_builder.emitZoneAlloc(t, n.memSpec.label,
                                            hasInit ? std::vector<ir::Value>{initVal}
                                                    : std::vector<ir::Value>{});
            break;
        default:
            alloc = m_builder.emitAlloca(t);
            if (hasInit) m_builder.emitStore(t, initVal, alloc);
            break;
    }
    defineVar(n.name, alloc, t);
}

void IRGen::visitSetStmt(SetStmt& n) {
    if (!n.value) return;
    ir::Value val = emitExpr(*n.value);

    if (!n.target) return;
    if (auto* id = dynamic_cast<IdentExpr*>(n.target.get())) {
        VarEntry* ve = lookupVar(id->id);
        if (ve) m_builder.emitStore(ve->type, val, ve->ptr);
    } else if (auto* mem = dynamic_cast<MemberExpr*>(n.target.get())) {
        ir::Value obj = emitExpr(*mem->object);
        m_builder.emitSetField(obj, mem->member, val);
    } else if (auto* idx = dynamic_cast<IndexExpr*>(n.target.get())) {
        ir::Value arr    = emitExpr(*idx->object);
        ir::Value idxVal = emitExpr(*idx->index);
        m_builder.emitSetIndex(arr, idxVal, val);
    }
}

void IRGen::visitReturnStmt(ReturnStmt& n) {
    if (n.value) {
        ir::Value v = emitExpr(*n.value);
        m_builder.emitReturn(v);
    } else {
        m_builder.emitReturn();
    }
}

void IRGen::visitBreakStmt(BreakStmt& n) {
    if (!m_loopStack.empty())
        m_builder.emitJump(m_loopStack.back().breakLabel);
}

void IRGen::visitContinueStmt(ContinueStmt& n) {
    if (!m_loopStack.empty())
        m_builder.emitJump(m_loopStack.back().continueLabel);
}

void IRGen::visitFreeStmt(FreeStmt& n) {
    if (n.ptr) {
        ir::Value v = emitExpr(*n.ptr);
        m_builder.emitFree(v);
    }
}

void IRGen::visitThrowStmt(ThrowStmt& n) {
    ir::Value v = n.value ? emitExpr(*n.value) : m_builder.emitConstNull();
    ir::Instruction i; i.opcode = ir::Opcode::Throw_;
    i.operands.push_back(v);
    m_builder.currentBlock().emit(i);
}

void IRGen::visitIfStmt(IfStmt& n) {
    auto trueLabel  = m_builder.freshLabel("if_true");
    auto falseLabel = m_builder.freshLabel("if_false");
    auto endLabel   = m_builder.freshLabel("if_end");

    ir::Value cond = n.condition ? emitExpr(*n.condition) : m_builder.emitConstBool(false);
    m_builder.emitCondJump(cond, trueLabel.name, falseLabel.name);

    // Then block
    m_builder.setInsertBlock(trueLabel.name);
    m_builder.emitLabel(trueLabel.name);
    pushVarScope();
    emitStmts(n.thenBlock);
    popVarScope();
    m_builder.emitJump(endLabel.name);

    // Else / else-if block
    m_builder.setInsertBlock(falseLabel.name);
    m_builder.emitLabel(falseLabel.name);
    if (n.elseIf) {
        visitIfStmt(*n.elseIf);
    } else if (!n.elseBlock.empty()) {
        pushVarScope();
        emitStmts(n.elseBlock);
        popVarScope();
    }
    m_builder.emitJump(endLabel.name);

    // End
    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitMatchStmt(MatchStmt& n) {
    ir::Value subj = n.subject ? emitExpr(*n.subject) : m_builder.emitConstNull();
    auto endLabel  = m_builder.freshLabel("match_end");

    for (auto& mc : n.cases) {
        auto caseLabel = m_builder.freshLabel("case");
        auto nextLabel = m_builder.freshLabel("case_next");

        if (mc.pattern) {
            ir::Value pat = emitExpr(*mc.pattern);
            ir::Value cmp = m_builder.emitCmp(ir::Opcode::CmpEq, subj, pat);
            m_builder.emitCondJump(cmp, caseLabel.name, nextLabel.name);
        }
        m_builder.setInsertBlock(caseLabel.name);
        m_builder.emitLabel(caseLabel.name);
        pushVarScope();
        emitStmts(mc.body);
        popVarScope();
        m_builder.emitJump(endLabel.name);

        m_builder.setInsertBlock(nextLabel.name);
        m_builder.emitLabel(nextLabel.name);
    }
    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitForStmt(ForStmt& n) {
    // for <var> in <iterable> … end
    // Lower to: alloca idx=0; while idx < len(iter) { var = iter[idx]; body; idx++ }
    auto condLabel = m_builder.freshLabel("for_cond");
    auto bodyLabel = m_builder.freshLabel("for_body");
    auto endLabel  = m_builder.freshLabel("for_end");

    ir::Value iterVal = n.iterable ? emitExpr(*n.iterable) : m_builder.emitConstNull();

    // idx = 0
    TypePtr intType = Type::makeInt();
    ir::Value idxAlloc = m_builder.emitAlloca(intType);
    ir::Value zero     = m_builder.emitConstInt(0, intType);
    m_builder.emitStore(intType, zero, idxAlloc);

    m_builder.emitJump(condLabel.name);

    // for_cond: if idx < len(iter) goto body else goto end
    m_builder.setInsertBlock(condLabel.name);
    m_builder.emitLabel(condLabel.name);
    ir::Value idxVal = m_builder.emitLoad(intType, idxAlloc);
    // Emit a call to len() – in the C backend this becomes array length
    ir::Value lenVal = m_builder.emitCall(
        ir::Value{"nsl_len", Type::makeInt(), false},
        {iterVal}, intType);
    ir::Value cond = m_builder.emitCmp(ir::Opcode::CmpLt, idxVal, lenVal);
    m_builder.emitCondJump(cond, bodyLabel.name, endLabel.name);

    // for_body
    m_builder.setInsertBlock(bodyLabel.name);
    m_builder.emitLabel(bodyLabel.name);
    pushVarScope();

    // iter_var = iter[idx]
    TypePtr elemType = Type::makeUnknown();
    ir::Value elemAlloc = m_builder.emitAlloca(elemType);
    ir::Value elem      = m_builder.emitGetIndex(iterVal, idxVal, elemType);
    m_builder.emitStore(elemType, elem, elemAlloc);
    defineVar(n.iterVar, elemAlloc, elemType);

    m_loopStack.push_back({condLabel.name, endLabel.name});
    emitStmts(n.body);
    m_loopStack.pop_back();

    popVarScope();

    // idx = idx + 1
    ir::Value one     = m_builder.emitConstInt(1, intType);
    ir::Value idxNew  = m_builder.emitBinOp(ir::Opcode::Add, idxVal, one, intType);
    ir::Value idxLoad = m_builder.emitLoad(intType, idxAlloc);
    m_builder.emitStore(intType, idxNew, idxAlloc);
    m_builder.emitJump(condLabel.name);

    // for_end
    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitWhileStmt(WhileStmt& n) {
    auto condLabel = m_builder.freshLabel("while_cond");
    auto bodyLabel = m_builder.freshLabel("while_body");
    auto endLabel  = m_builder.freshLabel("while_end");

    m_builder.emitJump(condLabel.name);
    m_builder.setInsertBlock(condLabel.name);
    m_builder.emitLabel(condLabel.name);

    ir::Value cond = n.condition ? emitExpr(*n.condition) : m_builder.emitConstBool(false);
    m_builder.emitCondJump(cond, bodyLabel.name, endLabel.name);

    m_builder.setInsertBlock(bodyLabel.name);
    m_builder.emitLabel(bodyLabel.name);
    pushVarScope();
    m_loopStack.push_back({condLabel.name, endLabel.name});
    emitStmts(n.body);
    m_loopStack.pop_back();
    popVarScope();
    m_builder.emitJump(condLabel.name);

    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitLoopStmt(LoopStmt& n) {
    auto bodyLabel = m_builder.freshLabel("loop_body");
    auto endLabel  = m_builder.freshLabel("loop_end");

    m_builder.emitJump(bodyLabel.name);
    m_builder.setInsertBlock(bodyLabel.name);
    m_builder.emitLabel(bodyLabel.name);

    pushVarScope();
    m_loopStack.push_back({bodyLabel.name, endLabel.name});
    emitStmts(n.body);
    m_loopStack.pop_back();
    popVarScope();
    m_builder.emitJump(bodyLabel.name);

    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitTryCatchStmt(TryCatchStmt& n) {
    auto tryLabel   = m_builder.freshLabel("try");
    auto catchLabel = m_builder.freshLabel("catch");
    auto endLabel   = m_builder.freshLabel("try_end");

    m_builder.setInsertBlock(tryLabel.name);
    m_builder.emitLabel(tryLabel.name);
    pushVarScope();
    emitStmts(n.tryBlock);
    popVarScope();
    m_builder.emitJump(endLabel.name);

    m_builder.setInsertBlock(catchLabel.name);
    m_builder.emitLabel(catchLabel.name);
    pushVarScope();
    if (n.catchVar) {
        TypePtr ct = n.catchType ? resolveType(*n.catchType) : Type::makeUnknown();
        ir::Value lp; lp.name = "exn"; lp.type = ct;
        ir::Value exnAlloc = m_builder.emitAlloca(ct);
        m_builder.emitStore(ct, lp, exnAlloc);
        defineVar(*n.catchVar, exnAlloc, ct);
    }
    emitStmts(n.catchBlock);
    popVarScope();
    m_builder.emitJump(endLabel.name);

    m_builder.setInsertBlock(endLabel.name);
    m_builder.emitLabel(endLabel.name);
}

void IRGen::visitExprStmt(ExprStmt& n) {
    if (n.expr) emitExpr(*n.expr);
}

// ══════════════════════════════════════════════════════════════════════════
// Expressions
// ══════════════════════════════════════════════════════════════════════════
void IRGen::visitIdentExpr(IdentExpr& n) {
    VarEntry* ve = lookupVar(n.id);
    if (ve) {
        m_exprValue = m_builder.emitLoad(ve->type, ve->ptr);
    } else {
        // Treat as a global/function reference
        ir::Value v; v.name = n.id; v.isGlobal = false;
        v.type = resolveType(n.resolvedType.empty() ? "unknown" : n.resolvedType);
        m_exprValue = v;
    }
}

void IRGen::visitIntLitExpr(IntLitExpr& n) {
    m_exprValue = m_builder.emitConstInt(n.value);
}

void IRGen::visitFloatLitExpr(FloatLitExpr& n) {
    m_exprValue = m_builder.emitConstFloat(n.value);
}

void IRGen::visitStringLitExpr(StringLitExpr& n) {
    m_exprValue = m_builder.emitConstStr(n.value);
}

void IRGen::visitCharLitExpr(CharLitExpr& n) {
    m_exprValue = m_builder.emitConstInt(static_cast<int64_t>(n.value), Type::makeChar());
}

void IRGen::visitBoolLitExpr(BoolLitExpr& n) {
    m_exprValue = m_builder.emitConstBool(n.value);
}

void IRGen::visitNullLitExpr(NullLitExpr& n) {
    m_exprValue = m_builder.emitConstNull();
}

void IRGen::visitBinaryExpr(BinaryExpr& n) {
    ir::Value lhs = n.lhs ? emitExpr(*n.lhs) : m_builder.emitConstInt(0);
    ir::Value rhs = n.rhs ? emitExpr(*n.rhs) : m_builder.emitConstInt(0);

    ir::Opcode op = binOpcode(n.op);
    bool isCmp = (op >= ir::Opcode::CmpEq && op <= ir::Opcode::CmpGe) ||
                 op == ir::Opcode::LogAnd || op == ir::Opcode::LogOr;

    if (isCmp)
        m_exprValue = m_builder.emitCmp(op, lhs, rhs);
    else
        m_exprValue = m_builder.emitBinOp(op, lhs, rhs, lhs.type);
}

void IRGen::visitUnaryExpr(UnaryExpr& n) {
    ir::Value operand = n.operand ? emitExpr(*n.operand) : m_builder.emitConstInt(0);
    if (n.op == "-") {
        ir::Value zero = m_builder.emitConstInt(0, operand.type);
        m_exprValue = m_builder.emitBinOp(ir::Opcode::Sub, zero, operand, operand.type);
    } else if (n.op == "not") {
        ir::Value t = m_builder.emitConstBool(true);
        m_exprValue = m_builder.emitCmp(ir::Opcode::CmpEq, operand,
                                        m_builder.emitConstBool(false));
    } else {
        m_exprValue = operand;
    }
}

void IRGen::visitCallExpr(CallExpr& n) {
    ir::Value callee;
    if (auto* id = dynamic_cast<IdentExpr*>(n.callee.get())) {
        callee.name = id->id;
        callee.isGlobal = false;
    } else if (auto* mem = dynamic_cast<MemberExpr*>(n.callee.get())) {
        // module.function → "module_function"
        ir::Value obj = emitExpr(*mem->object);
        callee.name   = obj.name + "." + mem->member;
    } else {
        callee = n.callee ? emitExpr(*n.callee) : ir::Value{};
    }

    std::vector<ir::Value> args;
    for (auto& a : n.args) if (a) args.push_back(emitExpr(*a));

    TypePtr retType = resolveType("unknown");
    m_exprValue = m_builder.emitCall(callee, std::move(args), retType);
}

void IRGen::visitMemberExpr(MemberExpr& n) {
    ir::Value obj = n.object ? emitExpr(*n.object) : ir::Value{};
    TypePtr ft = resolveType("unknown");
    m_exprValue = m_builder.emitGetField(obj, n.member, ft);
}

void IRGen::visitIndexExpr(IndexExpr& n) {
    ir::Value arr = n.object ? emitExpr(*n.object) : ir::Value{};
    ir::Value idx = n.index  ? emitExpr(*n.index)  : m_builder.emitConstInt(0);
    TypePtr et = resolveType("unknown");
    m_exprValue = m_builder.emitGetIndex(arr, idx, et);
}

void IRGen::visitNewExpr(NewExpr& n) {
    TypePtr t = resolveType(n.typeExpr);
    std::vector<ir::Value> args;
    for (auto& a : n.args) if (a) args.push_back(emitExpr(*a));
    m_exprValue = m_builder.emitHeapAlloc(t, std::move(args));
}

void IRGen::visitMoveExpr(MoveExpr& n) {
    ir::Value src = n.source ? emitExpr(*n.source) : ir::Value{};
    m_exprValue = m_builder.emitMove(src, src.type);
}

void IRGen::visitCopyExpr(CopyExpr& n) {
    ir::Value src = n.source ? emitExpr(*n.source) : ir::Value{};
    m_exprValue = m_builder.emitCopy(src, src.type);
}

void IRGen::visitRefExpr(RefExpr& n) {
    ir::Value src = n.source ? emitExpr(*n.source) : ir::Value{};
    m_exprValue = m_builder.emitRef(src, src.type);
}

void IRGen::visitAwaitExpr(AwaitExpr& n) {
    ir::Value v = n.expr ? emitExpr(*n.expr) : m_builder.emitConstNull();
    // Await is modelled as a call to __nsl_await in the C backend
    m_exprValue = m_builder.emitCall(
        ir::Value{"__nsl_await", nullptr, false}, {v}, v.type);
}

} // namespace nsl
