// NSL IR Implementation

#include "ir.hpp"
#include <sstream>

namespace nsl {
namespace ir {

// ──────────────────────────────────────────────────────────────────────────
// Instruction::toString
// ──────────────────────────────────────────────────────────────────────────
std::string Instruction::toString() const {
    std::ostringstream os;
    auto typeStr = [](const TypePtr& t) -> std::string {
        return t ? t->toString() : "?";
    };
    auto destStr = [&]() -> std::string {
        return dest ? (dest->str() + " = ") : "";
    };
    auto opsStr = [&]() -> std::string {
        std::string s;
        for (std::size_t i = 0; i < operands.size(); ++i) {
            if (i) s += ", ";
            s += operands[i].str();
        }
        return s;
    };

    switch (opcode) {
        case Opcode::Label:
            os << label << ":";
            break;
        case Opcode::Nop:
            os << "  nop";
            break;
        case Opcode::Alloca:
            os << "  " << destStr() << "alloca " << typeStr(type);
            break;
        case Opcode::Load:
            os << "  " << destStr() << "load " << typeStr(type) << ", " << opsStr();
            break;
        case Opcode::Store:
            os << "  store " << typeStr(type) << " " << opsStr();
            break;
        case Opcode::ConstInt:
            os << "  " << destStr() << "const_int " << intData;
            break;
        case Opcode::ConstFloat:
            os << "  " << destStr() << "const_float " << fltData;
            break;
        case Opcode::ConstStr:
            os << "  " << destStr() << "const_str \"" << strData << "\"";
            break;
        case Opcode::ConstBool:
            os << "  " << destStr() << "const_bool " << (intData ? "true" : "false");
            break;
        case Opcode::ConstNull:
            os << "  " << destStr() << "const_null";
            break;
        case Opcode::Add:  os << "  " << destStr() << "add "  << opsStr(); break;
        case Opcode::Sub:  os << "  " << destStr() << "sub "  << opsStr(); break;
        case Opcode::Mul:  os << "  " << destStr() << "mul "  << opsStr(); break;
        case Opcode::Div:  os << "  " << destStr() << "div "  << opsStr(); break;
        case Opcode::Mod:  os << "  " << destStr() << "mod "  << opsStr(); break;
        case Opcode::CmpEq: os << "  " << destStr() << "cmpeq " << opsStr(); break;
        case Opcode::CmpNe: os << "  " << destStr() << "cmpne " << opsStr(); break;
        case Opcode::CmpLt: os << "  " << destStr() << "cmplt " << opsStr(); break;
        case Opcode::CmpLe: os << "  " << destStr() << "cmple " << opsStr(); break;
        case Opcode::CmpGt: os << "  " << destStr() << "cmpgt " << opsStr(); break;
        case Opcode::CmpGe: os << "  " << destStr() << "cmpge " << opsStr(); break;
        case Opcode::LogAnd: os << "  " << destStr() << "and "  << opsStr(); break;
        case Opcode::LogOr:  os << "  " << destStr() << "or "   << opsStr(); break;
        case Opcode::LogNot: os << "  " << destStr() << "not "  << opsStr(); break;
        case Opcode::HeapAlloc:
            os << "  " << destStr() << "heap_alloc " << typeStr(type) << " (" << opsStr() << ")";
            break;
        case Opcode::StackAlloc:
            os << "  " << destStr() << "stack_alloc " << typeStr(type) << " (" << opsStr() << ")";
            break;
        case Opcode::ArenaAlloc:
            os << "  " << destStr() << "arena_alloc @" << label << " " << typeStr(type)
               << " (" << opsStr() << ")";
            break;
        case Opcode::ZoneAlloc:
            os << "  " << destStr() << "zone_alloc @" << label << " " << typeStr(type)
               << " (" << opsStr() << ")";
            break;
        case Opcode::Free:
            os << "  free " << opsStr();
            break;
        case Opcode::Move_:
            os << "  " << destStr() << "move " << opsStr();
            break;
        case Opcode::Copy_:
            os << "  " << destStr() << "copy " << opsStr();
            break;
        case Opcode::MakeRef:
            os << "  " << destStr() << "ref " << opsStr();
            break;
        case Opcode::Call:
            os << "  " << destStr() << "call " << label << " (" << opsStr() << ")";
            break;
        case Opcode::Return:
            os << "  return";
            if (!operands.empty()) os << " " << operands[0].str();
            break;
        case Opcode::Jump:
            os << "  jump @" << label;
            break;
        case Opcode::CondJump:
            os << "  condjump " << operands[0].str()
               << ", @" << label << ", @" << strData;
            break;
        case Opcode::Cast:
            os << "  " << destStr() << "cast " << typeStr(type) << " " << opsStr();
            break;
        case Opcode::Throw_:
            os << "  throw " << opsStr();
            break;
        case Opcode::LandingPad:
            os << "  " << destStr() << "landing_pad";
            break;
        case Opcode::GetField:
            os << "  " << destStr() << "getfield " << opsStr() << "." << strData;
            break;
        case Opcode::SetField:
            os << "  setfield " << opsStr() << "." << strData;
            break;
        case Opcode::GetIndex:
            os << "  " << destStr() << "getindex " << opsStr();
            break;
        case Opcode::SetIndex:
            os << "  setindex " << opsStr();
            break;
        default:
            os << "  ???";
    }
    return os.str();
}

// ──────────────────────────────────────────────────────────────────────────
// Function::toString
// ──────────────────────────────────────────────────────────────────────────
std::string Function::toString() const {
    std::ostringstream os;
    os << (isPublic ? "pub " : "") << (isAsync ? "async " : "")
       << "fn @" << name << "(";
    for (std::size_t i = 0; i < params.size(); ++i) {
        if (i) os << ", ";
        os << "%" << params[i].name << ": "
           << (params[i].type ? params[i].type->toString() : "?");
    }
    os << ") -> " << (returnType ? returnType->toString() : "void") << " {\n";
    for (const auto& bb : blocks) {
        if (!bb.name.empty()) os << bb.name << ":\n";
        for (const auto& instr : bb.instrs)
            os << instr.toString() << "\n";
    }
    os << "}\n";
    return os.str();
}

// ──────────────────────────────────────────────────────────────────────────
// Module::toString
// ──────────────────────────────────────────────────────────────────────────
std::string Module::toString() const {
    std::ostringstream os;
    os << "; NSL-IR Module: " << name << "\n\n";
    for (const auto& imp : imports)
        os << "import @" << imp << "\n";
    if (!imports.empty()) os << "\n";
    for (const auto& g : globals) {
        os << (g.isConst ? "const " : "global ")
           << "@" << g.name << ": " << (g.type ? g.type->toString() : "?");
        if (!g.initValue.empty()) os << " = " << g.initValue;
        os << "\n";
    }
    if (!globals.empty()) os << "\n";
    for (const auto& fn : functions)
        os << fn.toString() << "\n";
    return os.str();
}

// ──────────────────────────────────────────────────────────────────────────
// IRBuilder
// ──────────────────────────────────────────────────────────────────────────
IRBuilder::IRBuilder(Module& mod) : m_mod(mod) {}

void IRBuilder::beginFunction(const std::string& name, std::vector<Parameter> params,
                              TypePtr returnType, bool isAsync, bool isPublic) {
    m_mod.functions.push_back(Function{});
    m_curFn               = &m_mod.functions.back();
    m_curFn->name         = name;
    m_curFn->params       = std::move(params);
    m_curFn->returnType   = std::move(returnType);
    m_curFn->isAsync      = isAsync;
    m_curFn->isPublic     = isPublic;
    m_regCnt              = 0;
    m_curFn->newBlock("entry");
}

void IRBuilder::endFunction() {
    m_curFn = nullptr;
}

BasicBlock& IRBuilder::currentBlock() {
    return m_curFn->blocks.back();
}

void IRBuilder::setInsertBlock(const std::string& label) {
    for (auto& bb : m_curFn->blocks) {
        if (bb.name == label) return;
    }
    m_curFn->newBlock(label);
}

Value IRBuilder::freshReg(TypePtr t) {
    Value v;
    v.name = "r" + std::to_string(m_regCnt++);
    v.type = std::move(t);
    return v;
}

Value IRBuilder::freshLabel(const std::string& prefix) {
    Value v;
    v.name = prefix + std::to_string(m_lblCnt++);
    return v;
}

void IRBuilder::emit(Instruction i) {
    currentBlock().emit(std::move(i));
}

Value IRBuilder::emitAlloca(TypePtr t) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::Alloca; i.dest = dst; i.type = t;
    emit(i);
    return dst;
}

Value IRBuilder::emitLoad(TypePtr t, Value ptr) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::Load; i.dest = dst; i.type = t;
    i.operands.push_back(ptr);
    emit(i);
    return dst;
}

void IRBuilder::emitStore(TypePtr t, Value val, Value ptr) {
    Instruction i; i.opcode = Opcode::Store; i.type = t;
    i.operands.push_back(val);
    i.operands.push_back(ptr);
    emit(i);
}

Value IRBuilder::emitConstInt(int64_t v, TypePtr t) {
    Value dst = freshReg(t ? t : Type::makeInt());
    Instruction i; i.opcode = Opcode::ConstInt; i.dest = dst; i.intData = v;
    i.type = dst.type;
    emit(i);
    return dst;
}

Value IRBuilder::emitConstFloat(double v, TypePtr t) {
    Value dst = freshReg(t ? t : Type::makeDouble());
    Instruction i; i.opcode = Opcode::ConstFloat; i.dest = dst; i.fltData = v;
    i.type = dst.type;
    emit(i);
    return dst;
}

Value IRBuilder::emitConstStr(const std::string& s) {
    Value dst = freshReg(Type::makeString());
    Instruction i; i.opcode = Opcode::ConstStr; i.dest = dst; i.strData = s;
    emit(i);
    return dst;
}

Value IRBuilder::emitConstBool(bool v) {
    Value dst = freshReg(Type::makeBool());
    Instruction i; i.opcode = Opcode::ConstBool; i.dest = dst; i.intData = v ? 1 : 0;
    emit(i);
    return dst;
}

Value IRBuilder::emitConstNull() {
    Value dst = freshReg(Type::makeNull());
    Instruction i; i.opcode = Opcode::ConstNull; i.dest = dst;
    emit(i);
    return dst;
}

Value IRBuilder::emitBinOp(Opcode op, Value lhs, Value rhs, TypePtr t) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = op; i.dest = dst;
    i.operands.push_back(lhs); i.operands.push_back(rhs);
    emit(i);
    return dst;
}

Value IRBuilder::emitCmp(Opcode op, Value lhs, Value rhs) {
    Value dst = freshReg(Type::makeBool());
    Instruction i; i.opcode = op; i.dest = dst;
    i.operands.push_back(lhs); i.operands.push_back(rhs);
    emit(i);
    return dst;
}

Value IRBuilder::emitCall(Value callee, std::vector<Value> args, TypePtr retType) {
    Value dst = freshReg(retType);
    Instruction i; i.opcode = Opcode::Call; i.dest = dst;
    i.label = callee.name;
    i.operands = std::move(args);
    i.type = retType;
    emit(i);
    return dst;
}

void IRBuilder::emitReturn(std::optional<Value> val) {
    Instruction i; i.opcode = Opcode::Return;
    if (val) i.operands.push_back(*val);
    emit(i);
}

void IRBuilder::emitJump(const std::string& label) {
    Instruction i; i.opcode = Opcode::Jump; i.label = label;
    emit(i);
}

void IRBuilder::emitCondJump(Value cond, const std::string& trueLabel,
                             const std::string& falseLabel) {
    Instruction i; i.opcode = Opcode::CondJump;
    i.operands.push_back(cond);
    i.label   = trueLabel;
    i.strData = falseLabel;
    emit(i);
}

void IRBuilder::emitLabel(const std::string& label) {
    Instruction i; i.opcode = Opcode::Label; i.label = label;
    emit(i);
}

Value IRBuilder::emitHeapAlloc(TypePtr t, std::vector<Value> args) {
    Value dst = freshReg(Type::makePtr(t));
    Instruction i; i.opcode = Opcode::HeapAlloc; i.dest = dst; i.type = t;
    i.operands = std::move(args);
    emit(i);
    return dst;
}

Value IRBuilder::emitStackAlloc(TypePtr t, std::vector<Value> args) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::StackAlloc; i.dest = dst; i.type = t;
    i.operands = std::move(args);
    emit(i);
    return dst;
}

Value IRBuilder::emitArenaAlloc(TypePtr t, const std::string& arenaName,
                                std::vector<Value> args) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::ArenaAlloc; i.dest = dst; i.type = t;
    i.label    = arenaName;
    i.operands = std::move(args);
    emit(i);
    return dst;
}

Value IRBuilder::emitZoneAlloc(TypePtr t, const std::string& zoneName,
                               std::vector<Value> args) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::ZoneAlloc; i.dest = dst; i.type = t;
    i.label    = zoneName;
    i.operands = std::move(args);
    emit(i);
    return dst;
}

void IRBuilder::emitFree(Value ptr) {
    Instruction i; i.opcode = Opcode::Free; i.operands.push_back(ptr);
    emit(i);
}

Value IRBuilder::emitMove(Value src, TypePtr t) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::Move_; i.dest = dst; i.operands.push_back(src);
    emit(i);
    return dst;
}

Value IRBuilder::emitCopy(Value src, TypePtr t) {
    Value dst = freshReg(t);
    Instruction i; i.opcode = Opcode::Copy_; i.dest = dst; i.operands.push_back(src);
    emit(i);
    return dst;
}

Value IRBuilder::emitRef(Value src, TypePtr t) {
    Value dst = freshReg(Type::makeRef(t));
    Instruction i; i.opcode = Opcode::MakeRef; i.dest = dst; i.operands.push_back(src);
    emit(i);
    return dst;
}

Value IRBuilder::emitGetField(Value obj, const std::string& field, TypePtr fieldType) {
    Value dst = freshReg(fieldType);
    Instruction i; i.opcode = Opcode::GetField; i.dest = dst;
    i.operands.push_back(obj); i.strData = field;
    emit(i);
    return dst;
}

void IRBuilder::emitSetField(Value obj, const std::string& field, Value val) {
    Instruction i; i.opcode = Opcode::SetField;
    i.operands.push_back(obj); i.operands.push_back(val); i.strData = field;
    emit(i);
}

Value IRBuilder::emitGetIndex(Value arr, Value idx, TypePtr elemType) {
    Value dst = freshReg(elemType);
    Instruction i; i.opcode = Opcode::GetIndex; i.dest = dst;
    i.operands.push_back(arr); i.operands.push_back(idx);
    emit(i);
    return dst;
}

void IRBuilder::emitSetIndex(Value arr, Value idx, Value val) {
    Instruction i; i.opcode = Opcode::SetIndex;
    i.operands.push_back(arr); i.operands.push_back(idx); i.operands.push_back(val);
    emit(i);
}

} // namespace ir
} // namespace nsl
