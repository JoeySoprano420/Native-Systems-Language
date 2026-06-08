// NSL Semantic Analyzer Implementation

#include "sema.hpp"
#include <algorithm>
#include <sstream>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Scope
// ──────────────────────────────────────────────────────────────────────────
void Scope::define(const std::string& name, Symbol sym) {
    m_symbols[name] = std::move(sym);
}

Symbol* Scope::lookup(const std::string& name) {
    auto it = m_symbols.find(name);
    return it != m_symbols.end() ? &it->second : nullptr;
}

Symbol* Scope::lookupAll(const std::string& name) {
    if (auto* s = lookup(name)) return s;
    if (m_parent) return m_parent->lookupAll(name);
    return nullptr;
}

// ──────────────────────────────────────────────────────────────────────────
// SemanticAnalyzer
// ──────────────────────────────────────────────────────────────────────────
SemanticAnalyzer::SemanticAnalyzer(DiagnosticEngine& diag) : m_diag(diag) {}

Scope* SemanticAnalyzer::currentScope() {
    return m_scopeStack.empty() ? nullptr : &m_scopeStack.back();
}

void SemanticAnalyzer::pushScope() {
    m_scopeStack.emplace_back();
    if (m_scopeStack.size() > 1)
        m_scopeStack.back().setParent(&m_scopeStack[m_scopeStack.size() - 2]);
}

void SemanticAnalyzer::popScope() {
    if (!m_scopeStack.empty()) m_scopeStack.pop_back();
}

bool SemanticAnalyzer::analyse(Program& prog) {
    pushScope();
    firstPass(prog);
    visitProgram(prog);
    popScope();
    return !m_diag.hasErrors();
}

// ──────────────────────────────────────────────────────────────────────────
// First pass: register all top-level symbols so forward references work.
// ──────────────────────────────────────────────────────────────────────────
void SemanticAnalyzer::firstPass(Program& prog) {
    for (auto& d : prog.decls) {
        if (auto* fn = dynamic_cast<FunctionDecl*>(d.get())) {
            Symbol sym;
            sym.name       = fn->name;
            sym.isFunction = true;
            sym.declLoc    = fn->loc;
            sym.type       = resolveTypeAnnotation(fn->returnType);
            currentScope()->define(fn->name, sym);
        }
        if (auto* td = dynamic_cast<TypeDecl*>(d.get())) {
            auto userType = std::make_shared<Type>();
            userType->kind = TypeKind::Struct;
            userType->name = td->name;
            for (auto& f : td->fields) {
                Type::Field tf;
                tf.name     = f.name;
                tf.type     = resolveTypeAnnotation(f.typeAnnotation);
                tf.isPublic = f.isPublic || !f.isPrivate;
                userType->fields.push_back(tf);
            }
            m_types.registerType(td->name, userType);
            Symbol sym;
            sym.name    = td->name;
            sym.type    = userType;
            sym.declLoc = td->loc;
            currentScope()->define(td->name, sym);
        }
    }
}

TypePtr SemanticAnalyzer::resolveTypeAnnotation(const TypeExpr& te) {
    TypePtr t = m_types.resolve(te);
    return t ? t : Type::makeUnknown();
}

void SemanticAnalyzer::checkType(const TypePtr& actual, const TypePtr& expected,
                                 const SourceLocation& loc, const std::string& context) {
    if (!actual || !expected) return;
    if (!m_types.isCompatible(actual, expected)) {
        m_diag.error("Type mismatch in " + context +
                     ": expected '" + expected->toString() +
                     "' but got '"  + actual->toString() + "'", loc);
    }
}

Symbol* SemanticAnalyzer::requireSymbol(const std::string& name, const SourceLocation& loc) {
    if (auto* s = currentScope()->lookupAll(name)) return s;
    m_diag.error("Undefined symbol '" + name + "'", loc);
    return nullptr;
}

// ──────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────
void SemanticAnalyzer::analyseStmts(std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts) if (s) s->accept(*this);
}

TypePtr SemanticAnalyzer::inferExprType(Expr& e) {
    e.accept(*this);
    if (e.resolvedType.empty()) return Type::makeUnknown();
    return m_types.lookup(e.resolvedType);
}

// ──────────────────────────────────────────────────────────────────────────
// Program
// ──────────────────────────────────────────────────────────────────────────
void SemanticAnalyzer::visitProgram(Program& n) {
    for (auto& d : n.decls) if (d) d->accept(*this);
}

// ──────────────────────────────────────────────────────────────────────────
// Declarations
// ──────────────────────────────────────────────────────────────────────────
void SemanticAnalyzer::visitUseDecl(UseDecl& n) {
    // Mark the imported module/symbol as a known symbol (type unknown for now)
    std::string sym = n.alias.empty()
        ? (n.isFromImport ? n.fromSymbol : (n.modulePath.empty() ? n.name : n.modulePath[0]))
        : n.alias;
    Symbol s;
    s.name    = sym;
    s.type    = Type::makeUnknown();
    s.declLoc = n.loc;
    currentScope()->define(sym, s);
}

void SemanticAnalyzer::visitExportDecl(ExportDecl& n) {}
void SemanticAnalyzer::visitContextDecl(ContextDecl& n) {}
void SemanticAnalyzer::visitMacroDecl(MacroDecl& n) {}

void SemanticAnalyzer::visitFunctionDecl(FunctionDecl& n) {
    pushScope();
    TypePtr savedReturn = m_currentReturnType;
    m_currentReturnType = resolveTypeAnnotation(n.returnType);

    // Register parameters
    for (auto& p : n.params) {
        Symbol sym;
        sym.name    = p.name;
        sym.type    = resolveTypeAnnotation(p.typeAnnotation);
        sym.declLoc = p.loc;
        currentScope()->define(p.name, sym);
    }

    bool savedAsync = m_inAsync;
    m_inAsync = n.isAsync;
    analyseStmts(n.body);
    m_inAsync = savedAsync;
    m_currentReturnType = savedReturn;
    popScope();
}

void SemanticAnalyzer::visitTypeDecl(TypeDecl& n) {
    // Types are already registered in first pass; analyse methods
    pushScope();
    for (auto& m : n.methods) if (m) visitFunctionDecl(*m);
    popScope();
}

void SemanticAnalyzer::visitConstDecl(ConstDecl& n) {
    TypePtr declType = resolveTypeAnnotation(n.typeAnnotation);
    TypePtr initType = n.init ? inferExprType(*n.init) : Type::makeUnknown();
    if (n.init) checkType(initType, declType, n.loc, "const '" + n.name + "'");
    Symbol sym;
    sym.name    = n.name;
    sym.type    = declType;
    sym.isConst = true;
    sym.declLoc = n.loc;
    currentScope()->define(n.name, sym);
}

// ── Statements ─────────────────────────────────────────────────────────────
void SemanticAnalyzer::visitLetDecl(LetDecl& n) {
    TypePtr declType = n.typeAnnotation
        ? resolveTypeAnnotation(*n.typeAnnotation)
        : Type::makeUnknown();

    TypePtr initType = Type::makeUnknown();
    if (n.init) {
        initType = inferExprType(*n.init);
        if (n.typeAnnotation)
            checkType(initType, declType, n.loc, "let '" + n.name + "'");
        else
            declType = initType; // type inference
    }

    Symbol sym;
    sym.name    = n.name;
    sym.type    = declType;
    sym.memSpec = n.memSpec;
    sym.declLoc = n.loc;
    currentScope()->define(n.name, sym);
}

void SemanticAnalyzer::visitSetStmt(SetStmt& n) {
    if (!n.target || !n.value) return;
    TypePtr lhsType = inferExprType(*n.target);
    TypePtr rhsType = inferExprType(*n.value);

    // Check that target is mutable
    if (auto* id = dynamic_cast<IdentExpr*>(n.target.get())) {
        if (auto* sym = currentScope()->lookupAll(id->id)) {
            if (sym->isConst)
                m_diag.error("Cannot mutate constant '" + id->id + "'", n.loc);
        }
    }
    checkType(rhsType, lhsType, n.loc, "set statement");
}

void SemanticAnalyzer::visitReturnStmt(ReturnStmt& n) {
    TypePtr retType = n.value ? inferExprType(*n.value) : Type::makeVoid();
    if (m_currentReturnType)
        checkType(retType, m_currentReturnType, n.loc, "return");
}

void SemanticAnalyzer::visitBreakStmt(BreakStmt& n) {
    if (!m_inLoop)
        m_diag.error("'break' outside of loop", n.loc);
}

void SemanticAnalyzer::visitContinueStmt(ContinueStmt& n) {
    if (!m_inLoop)
        m_diag.error("'continue' outside of loop", n.loc);
}

void SemanticAnalyzer::visitFreeStmt(FreeStmt& n) {
    if (!n.ptr) return;
    TypePtr t = inferExprType(*n.ptr);
    if (t && t->kind != TypeKind::Ptr && t->kind != TypeKind::Unknown)
        m_diag.warning("'free' applied to a non-pointer type '" + t->toString() + "'", n.loc);
}

void SemanticAnalyzer::visitThrowStmt(ThrowStmt& n) {
    if (n.value) inferExprType(*n.value);
}

void SemanticAnalyzer::visitIfStmt(IfStmt& n) {
    if (n.condition) {
        TypePtr cond = inferExprType(*n.condition);
        if (cond && cond->kind != TypeKind::Bool && cond->kind != TypeKind::Unknown
            && !cond->isNumeric())
            m_diag.warning("Condition is not a boolean expression", n.condition->loc);
    }
    pushScope(); analyseStmts(n.thenBlock); popScope();
    if (n.elseIf) {
        visitIfStmt(*n.elseIf);
    } else if (!n.elseBlock.empty()) {
        pushScope(); analyseStmts(n.elseBlock); popScope();
    }
}

void SemanticAnalyzer::visitMatchStmt(MatchStmt& n) {
    if (n.subject) inferExprType(*n.subject);
    for (auto& mc : n.cases) {
        if (mc.pattern) inferExprType(*mc.pattern);
        pushScope(); analyseStmts(mc.body); popScope();
    }
}

void SemanticAnalyzer::visitForStmt(ForStmt& n) {
    TypePtr iterType = n.iterable ? inferExprType(*n.iterable) : Type::makeUnknown();
    TypePtr elemType = Type::makeUnknown();
    if (iterType && iterType->isArray() && iterType->inner)
        elemType = iterType->inner;

    pushScope();
    bool saved = m_inLoop; m_inLoop = true;
    Symbol sym; sym.name = n.iterVar; sym.type = elemType; sym.declLoc = n.loc;
    currentScope()->define(n.iterVar, sym);
    analyseStmts(n.body);
    m_inLoop = saved;
    popScope();
}

void SemanticAnalyzer::visitWhileStmt(WhileStmt& n) {
    if (n.condition) inferExprType(*n.condition);
    pushScope();
    bool saved = m_inLoop; m_inLoop = true;
    analyseStmts(n.body);
    m_inLoop = saved;
    popScope();
}

void SemanticAnalyzer::visitLoopStmt(LoopStmt& n) {
    pushScope();
    bool saved = m_inLoop; m_inLoop = true;
    analyseStmts(n.body);
    m_inLoop = saved;
    popScope();
}

void SemanticAnalyzer::visitTryCatchStmt(TryCatchStmt& n) {
    pushScope(); analyseStmts(n.tryBlock); popScope();
    pushScope();
    if (n.catchVar) {
        Symbol sym;
        sym.name    = *n.catchVar;
        sym.type    = n.catchType ? resolveTypeAnnotation(*n.catchType) : Type::makeUnknown();
        sym.declLoc = n.loc;
        currentScope()->define(*n.catchVar, sym);
    }
    analyseStmts(n.catchBlock);
    popScope();
}

void SemanticAnalyzer::visitExprStmt(ExprStmt& n) {
    if (n.expr) inferExprType(*n.expr);
}

// ── Expressions ─────────────────────────────────────────────────────────────
void SemanticAnalyzer::visitIdentExpr(IdentExpr& n) {
    if (auto* sym = currentScope()->lookupAll(n.id)) {
        n.resolvedType = sym->type ? sym->type->toString() : "unknown";
    } else {
        // Soft error for undefined — could be a module or builtin
        n.resolvedType = "unknown";
    }
}

void SemanticAnalyzer::visitIntLitExpr(IntLitExpr& n) {
    n.resolvedType = "int";
}

void SemanticAnalyzer::visitFloatLitExpr(FloatLitExpr& n) {
    n.resolvedType = "double";
}

void SemanticAnalyzer::visitStringLitExpr(StringLitExpr& n) {
    n.resolvedType = "string";
}

void SemanticAnalyzer::visitCharLitExpr(CharLitExpr& n) {
    n.resolvedType = "char";
}

void SemanticAnalyzer::visitBoolLitExpr(BoolLitExpr& n) {
    n.resolvedType = "bool";
}

void SemanticAnalyzer::visitNullLitExpr(NullLitExpr& n) {
    n.resolvedType = "null";
}

void SemanticAnalyzer::visitBinaryExpr(BinaryExpr& n) {
    TypePtr lhsT = n.lhs ? inferExprType(*n.lhs) : Type::makeUnknown();
    TypePtr rhsT = n.rhs ? inferExprType(*n.rhs) : Type::makeUnknown();

    const std::string& op = n.op;
    if (op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=" || op == "and" || op == "or" || op == "is") {
        n.resolvedType = "bool";
    } else if (lhsT && lhsT->kind == TypeKind::String && op == "+") {
        n.resolvedType = "string"; // string concatenation
    } else {
        n.resolvedType = lhsT ? lhsT->toString() : "int";
    }
}

void SemanticAnalyzer::visitUnaryExpr(UnaryExpr& n) {
    TypePtr t = n.operand ? inferExprType(*n.operand) : Type::makeUnknown();
    if (n.op == "not") n.resolvedType = "bool";
    else               n.resolvedType = t ? t->toString() : "unknown";
}

void SemanticAnalyzer::visitCallExpr(CallExpr& n) {
    if (n.callee) inferExprType(*n.callee);
    for (auto& a : n.args) if (a) inferExprType(*a);
    // Return type inference: if callee is a known function, use its return type.
    // For now, mark as unknown unless we can determine it.
    n.resolvedType = "unknown";
}

void SemanticAnalyzer::visitMemberExpr(MemberExpr& n) {
    TypePtr objType = n.object ? inferExprType(*n.object) : Type::makeUnknown();
    // Try to look up the field in the struct type
    if (objType && objType->kind == TypeKind::Struct) {
        for (auto& f : objType->fields) {
            if (f.name == n.member) {
                n.resolvedType = f.type ? f.type->toString() : "unknown";
                return;
            }
        }
    }
    n.resolvedType = "unknown";
}

void SemanticAnalyzer::visitIndexExpr(IndexExpr& n) {
    TypePtr arrType = n.object ? inferExprType(*n.object) : Type::makeUnknown();
    if (n.index) inferExprType(*n.index);
    if (arrType && arrType->isArray() && arrType->inner)
        n.resolvedType = arrType->inner->toString();
    else
        n.resolvedType = "unknown";
}

void SemanticAnalyzer::visitNewExpr(NewExpr& n) {
    TypePtr inner = resolveTypeAnnotation(n.typeExpr);
    for (auto& a : n.args) if (a) inferExprType(*a);
    n.resolvedType = "ptr<" + (inner ? inner->toString() : n.typeExpr.name) + ">";
}

void SemanticAnalyzer::visitMoveExpr(MoveExpr& n) {
    TypePtr t = n.source ? inferExprType(*n.source) : Type::makeUnknown();
    n.resolvedType = t ? t->toString() : "unknown";
}

void SemanticAnalyzer::visitCopyExpr(CopyExpr& n) {
    TypePtr t = n.source ? inferExprType(*n.source) : Type::makeUnknown();
    n.resolvedType = t ? t->toString() : "unknown";
}

void SemanticAnalyzer::visitRefExpr(RefExpr& n) {
    TypePtr t = n.source ? inferExprType(*n.source) : Type::makeUnknown();
    n.resolvedType = "ref<" + (t ? t->toString() : "unknown") + ">";
}

void SemanticAnalyzer::visitAwaitExpr(AwaitExpr& n) {
    if (!m_inAsync)
        m_diag.error("'await' used outside of an async function", n.loc);
    TypePtr t = n.expr ? inferExprType(*n.expr) : Type::makeUnknown();
    n.resolvedType = t ? t->toString() : "unknown";
}

} // namespace nsl
