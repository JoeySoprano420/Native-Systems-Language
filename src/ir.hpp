#pragma once
// NSL Intermediate Representation (NSL-IR)
// A simple, typed three-address code representation used between
// semantic analysis and final code generation.
//
// The IR is flat: a Module contains Functions, each Function contains
// BasicBlocks, each BasicBlock contains Instructions.

#include "ast.hpp"
#include "types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>

namespace nsl {
namespace ir {

// ──────────────────────────────────────────────────────────────────────────
// Value reference – either a local register (%name) or a global (@name)
// ──────────────────────────────────────────────────────────────────────────
struct Value {
    std::string name;   // e.g. "%tmp0", "@global_x"
    TypePtr     type;
    bool        isGlobal = false;

    std::string str() const {
        return (isGlobal ? "@" : "%") + name;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Instruction opcodes
// ──────────────────────────────────────────────────────────────────────────
enum class Opcode {
    // Data
    Alloca,       // %dst = alloca <type>
    Load,         // %dst = load <type>, %ptr
    Store,        // store <type> %val, %ptr
    GetField,     // %dst = getfield %obj, "field"
    SetField,     // setfield %obj, "field", %val
    GetIndex,     // %dst = getindex %arr, %idx
    SetIndex,     // setindex %arr, %idx, %val

    // Constants
    ConstInt,     // %dst = const_int <val>
    ConstFloat,   // %dst = const_float <val>
    ConstStr,     // %dst = const_str "..."
    ConstBool,    // %dst = const_bool true/false
    ConstNull,    // %dst = const_null

    // Arithmetic
    Add, Sub, Mul, Div, Mod,

    // Comparison
    CmpEq, CmpNe, CmpLt, CmpLe, CmpGt, CmpGe,

    // Logic
    LogAnd, LogOr, LogNot,

    // Memory
    HeapAlloc,    // %dst = heap_alloc <type>
    StackAlloc,   // %dst = stack_alloc <type>
    ArenaAlloc,   // %dst = arena_alloc <type>, @arena
    ZoneAlloc,    // %dst = zone_alloc <type>, @zone
    Free,         // free %ptr

    // Ownership
    Move_,        // %dst = move %src
    Copy_,        // %dst = copy %src
    MakeRef,      // %dst = ref %src

    // Control flow
    Jump,         // jump @label
    CondJump,     // condjump %cond, @true_label, @false_label
    Return,       // return [%val]

    // Calls
    Call,         // %dst = call @fn (%arg0, %arg1, ...)

    // Conversion
    Cast,         // %dst = cast <type> %src

    // Throw / catch
    Throw_,       // throw %val
    LandingPad,   // landing_pad [%exn_var]

    // Phi (SSA)
    Phi,          // %dst = phi [%val0, @bb0] [%val1, @bb1] ...

    // No-op / label
    Label,        // label @name
    Nop,
};

// ──────────────────────────────────────────────────────────────────────────
// IR Instruction
// ──────────────────────────────────────────────────────────────────────────
struct Instruction {
    Opcode               opcode;
    std::optional<Value> dest;    // result register (if any)
    std::vector<Value>   operands;
    std::string          label;   // for Label, Jump, CondJump
    std::string          strData; // for ConstStr, GetField, etc.
    int64_t              intData  = 0;
    double               fltData  = 0.0;
    TypePtr              type;    // for Alloca, Cast, etc.

    // Pretty-print the instruction for --emit-ir
    std::string toString() const;
};

// ──────────────────────────────────────────────────────────────────────────
// Basic Block
// ──────────────────────────────────────────────────────────────────────────
struct BasicBlock {
    std::string              name;
    std::vector<Instruction> instrs;

    void emit(Instruction i) { instrs.push_back(std::move(i)); }
};

// ──────────────────────────────────────────────────────────────────────────
// Function
// ──────────────────────────────────────────────────────────────────────────
struct Parameter {
    std::string name;
    TypePtr     type;
};

struct Function {
    std::string              name;
    std::vector<Parameter>   params;
    TypePtr                  returnType;
    std::vector<BasicBlock>  blocks;
    bool                     isAsync  = false;
    bool                     isPublic = true;

    BasicBlock& entry()   { return blocks.front(); }
    BasicBlock& current() { return blocks.back(); }

    BasicBlock& newBlock(const std::string& label = "") {
        blocks.push_back(BasicBlock{label});
        return blocks.back();
    }

    std::string toString() const;
};

// ──────────────────────────────────────────────────────────────────────────
// Global variable
// ──────────────────────────────────────────────────────────────────────────
struct GlobalVar {
    std::string name;
    TypePtr     type;
    std::string initValue; // literal or empty
    bool        isConst = false;
};

// ──────────────────────────────────────────────────────────────────────────
// IR Module
// ──────────────────────────────────────────────────────────────────────────
struct Module {
    std::string              name;
    std::vector<GlobalVar>   globals;
    std::vector<Function>    functions;
    std::vector<std::string> imports;  // use'd module names

    std::string toString() const;
};

// ──────────────────────────────────────────────────────────────────────────
// IR Builder – wraps module/function/block emission
// ──────────────────────────────────────────────────────────────────────────
class IRBuilder {
public:
    explicit IRBuilder(Module& mod);

    void beginFunction(const std::string& name, std::vector<Parameter> params,
                       TypePtr returnType, bool isAsync = false, bool isPublic = true);
    void endFunction();

    BasicBlock& currentBlock();
    void        setInsertBlock(const std::string& label);

    // Register allocation
    Value freshReg(TypePtr t = nullptr);
    Value freshLabel(const std::string& prefix = "bb");

    // Instruction emitters
    Value emitAlloca(TypePtr t);
    Value emitLoad(TypePtr t, Value ptr);
    void  emitStore(TypePtr t, Value val, Value ptr);
    Value emitConstInt(int64_t v, TypePtr t = nullptr);
    Value emitConstFloat(double v, TypePtr t = nullptr);
    Value emitConstStr(const std::string& s);
    Value emitConstBool(bool v);
    Value emitConstNull();
    Value emitBinOp(Opcode op, Value lhs, Value rhs, TypePtr t);
    Value emitCmp(Opcode op, Value lhs, Value rhs);
    Value emitCall(Value callee, std::vector<Value> args, TypePtr retType);
    void  emitReturn(std::optional<Value> val = std::nullopt);
    void  emitJump(const std::string& label);
    void  emitCondJump(Value cond, const std::string& trueLabel,
                       const std::string& falseLabel);
    void  emitLabel(const std::string& label);
    Value emitHeapAlloc(TypePtr t, std::vector<Value> args);
    Value emitStackAlloc(TypePtr t, std::vector<Value> args);
    Value emitArenaAlloc(TypePtr t, const std::string& arenaName, std::vector<Value> args);
    Value emitZoneAlloc(TypePtr t, const std::string& zoneName, std::vector<Value> args);
    void  emitFree(Value ptr);
    Value emitMove(Value src, TypePtr t);
    Value emitCopy(Value src, TypePtr t);
    Value emitRef(Value src, TypePtr t);
    Value emitGetField(Value obj, const std::string& field, TypePtr fieldType);
    void  emitSetField(Value obj, const std::string& field, Value val);
    Value emitGetIndex(Value arr, Value idx, TypePtr elemType);
    void  emitSetIndex(Value arr, Value idx, Value val);

    Module& module() { return m_mod; }

private:
    Module&   m_mod;
    Function* m_curFn  = nullptr;
    int       m_regCnt = 0;
    int       m_lblCnt = 0;

    void emit(Instruction i);
};

} // namespace ir
} // namespace nsl
