#pragma once
// NSL Abstract Syntax Tree
// All AST nodes are owned via unique_ptr and arranged in a Visitor hierarchy.

#include "token.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <variant>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────────────────
struct ASTNode;
struct Expr;
struct Stmt;
struct Decl;

// ── Memory Region ─────────────────────────────────────────────────────────
enum class MemRegion { None, Stack, Heap, Arena, Zone };

struct MemorySpec {
    MemRegion   region = MemRegion::None;
    std::string label;  // arena name or zone name
};

// ──────────────────────────────────────────────────────────────────────────
// Type Syntax (as written in source, before resolution)
// ──────────────────────────────────────────────────────────────────────────
struct TypeExpr {
    std::string              name;       // "int", "string", "array", "ptr", "ref", etc.
    std::vector<TypeExpr>    params;     // generic params: array<T> → params = [T]
    bool                     isPtr = false;
    bool                     isRef = false;
    SourceLocation           loc;
};

// ──────────────────────────────────────────────────────────────────────────
// Visitor interface (forward-declared; implemented by each pass)
// ──────────────────────────────────────────────────────────────────────────
struct ASTVisitor;

// ──────────────────────────────────────────────────────────────────────────
// Base node
// ──────────────────────────────────────────────────────────────────────────
struct ASTNode {
    SourceLocation loc;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& v) = 0;
};

using NodePtr  = std::unique_ptr<ASTNode>;
using ExprPtr  = std::unique_ptr<Expr>;
using StmtPtr  = std::unique_ptr<Stmt>;
using DeclPtr  = std::unique_ptr<Decl>;

// ──────────────────────────────────────────────────────────────────────────
// Expression base
// ──────────────────────────────────────────────────────────────────────────
struct Expr : ASTNode {
    // Filled in by the semantic analyser.
    std::string resolvedType;
};

// ──────────────────────────────────────────────────────────────────────────
// Statement base
// ──────────────────────────────────────────────────────────────────────────
struct Stmt : ASTNode {};

// ──────────────────────────────────────────────────────────────────────────
// Declaration base
// ──────────────────────────────────────────────────────────────────────────
struct Decl : ASTNode {
    std::string name;
    bool        isPublic  = false;
    bool        isPrivate = false;
    bool        isStatic  = false;
    bool        isVirtual = false;
    bool        isExport  = false;
};

// ══════════════════════════════════════════════════════════════════════════
// Expressions
// ══════════════════════════════════════════════════════════════════════════

// ── Identifier Expression ─────────────────────────────────────────────────
struct IdentExpr : Expr {
    std::string id;
    void accept(ASTVisitor& v) override;
};

// ── Integer Literal ───────────────────────────────────────────────────────
struct IntLitExpr : Expr {
    int64_t value = 0;
    void accept(ASTVisitor& v) override;
};

// ── Float Literal ─────────────────────────────────────────────────────────
struct FloatLitExpr : Expr {
    double value = 0.0;
    void accept(ASTVisitor& v) override;
};

// ── String Literal ────────────────────────────────────────────────────────
struct StringLitExpr : Expr {
    std::string value;
    void accept(ASTVisitor& v) override;
};

// ── Char Literal ─────────────────────────────────────────────────────────
struct CharLitExpr : Expr {
    char value = '\0';
    void accept(ASTVisitor& v) override;
};

// ── Bool Literal ─────────────────────────────────────────────────────────
struct BoolLitExpr : Expr {
    bool value = false;
    void accept(ASTVisitor& v) override;
};

// ── Null Literal ─────────────────────────────────────────────────────────
struct NullLitExpr : Expr {
    void accept(ASTVisitor& v) override;
};

// ── Binary Expression ─────────────────────────────────────────────────────
struct BinaryExpr : Expr {
    std::string op; // "+", "-", "==", "and", etc.
    ExprPtr     lhs;
    ExprPtr     rhs;
    void accept(ASTVisitor& v) override;
};

// ── Unary Expression ─────────────────────────────────────────────────────
struct UnaryExpr : Expr {
    std::string op;  // "-", "not"
    ExprPtr     operand;
    void accept(ASTVisitor& v) override;
};

// ── Call Expression ───────────────────────────────────────────────────────
struct CallExpr : Expr {
    ExprPtr              callee;
    std::vector<ExprPtr> args;
    void accept(ASTVisitor& v) override;
};

// ── Member Access Expression ──────────────────────────────────────────────
struct MemberExpr : Expr {
    ExprPtr     object;
    std::string member;
    void accept(ASTVisitor& v) override;
};

// ── Index Expression ─────────────────────────────────────────────────────
struct IndexExpr : Expr {
    ExprPtr index;
    ExprPtr object;
    void accept(ASTVisitor& v) override;
};

// ── New Expression ───────────────────────────────────────────────────────
struct NewExpr : Expr {
    TypeExpr             typeExpr;
    std::vector<ExprPtr> args;
    void accept(ASTVisitor& v) override;
};

// ── Move Expression ──────────────────────────────────────────────────────
struct MoveExpr : Expr {
    ExprPtr source;
    void accept(ASTVisitor& v) override;
};

// ── Copy Expression ──────────────────────────────────────────────────────
struct CopyExpr : Expr {
    ExprPtr source;
    void accept(ASTVisitor& v) override;
};

// ── Ref Expression ───────────────────────────────────────────────────────
struct RefExpr : Expr {
    ExprPtr source;
    void accept(ASTVisitor& v) override;
};

// ── Await Expression ─────────────────────────────────────────────────────
struct AwaitExpr : Expr {
    ExprPtr expr;
    void accept(ASTVisitor& v) override;
};

// ══════════════════════════════════════════════════════════════════════════
// Statements
// ══════════════════════════════════════════════════════════════════════════

// ── Expression Statement ─────────────────────────────────────────────────
struct ExprStmt : Stmt {
    ExprPtr expr;
    void accept(ASTVisitor& v) override;
};

// ── Let Declaration / Statement ──────────────────────────────────────────
struct LetDecl : Stmt {
    std::string              name;
    std::optional<TypeExpr>  typeAnnotation;
    MemorySpec               memSpec;
    ExprPtr                  init;    // may be null (no initializer)
    void accept(ASTVisitor& v) override;
};

// ── Set Statement (mutation) ─────────────────────────────────────────────
struct SetStmt : Stmt {
    ExprPtr target;   // left-hand side (may be a MemberExpr or IndexExpr)
    ExprPtr value;
    void accept(ASTVisitor& v) override;
};

// ── Const Declaration ────────────────────────────────────────────────────
struct ConstDecl : Decl {
    TypeExpr typeAnnotation;
    ExprPtr  init;
    void accept(ASTVisitor& v) override;
};

// ── Return Statement ─────────────────────────────────────────────────────
struct ReturnStmt : Stmt {
    ExprPtr value; // may be null
    void accept(ASTVisitor& v) override;
};

// ── Break Statement ──────────────────────────────────────────────────────
struct BreakStmt : Stmt {
    void accept(ASTVisitor& v) override;
};

// ── Continue Statement ───────────────────────────────────────────────────
struct ContinueStmt : Stmt {
    void accept(ASTVisitor& v) override;
};

// ── Free Statement ───────────────────────────────────────────────────────
struct FreeStmt : Stmt {
    ExprPtr ptr;
    void accept(ASTVisitor& v) override;
};

// ── Throw Statement ──────────────────────────────────────────────────────
struct ThrowStmt : Stmt {
    ExprPtr value; // may be null (re-throw)
    void accept(ASTVisitor& v) override;
};

// ── If Statement ─────────────────────────────────────────────────────────
struct IfStmt : Stmt {
    ExprPtr              condition;
    std::vector<StmtPtr> thenBlock;
    // else can be a single IfStmt (else-if chain) or a plain block
    std::unique_ptr<IfStmt>  elseIf;
    std::vector<StmtPtr>     elseBlock;
    void accept(ASTVisitor& v) override;
};

// ── Match Case ───────────────────────────────────────────────────────────
struct MatchCase {
    ExprPtr              pattern;    // null → default
    std::vector<StmtPtr> body;
};

// ── Match Statement ──────────────────────────────────────────────────────
struct MatchStmt : Stmt {
    ExprPtr                  subject;
    std::vector<MatchCase>   cases;
    void accept(ASTVisitor& v) override;
};

// ── For Statement ────────────────────────────────────────────────────────
struct ForStmt : Stmt {
    std::string          iterVar;
    ExprPtr              iterable;
    std::vector<StmtPtr> body;
    void accept(ASTVisitor& v) override;
};

// ── While Statement ──────────────────────────────────────────────────────
struct WhileStmt : Stmt {
    ExprPtr              condition;
    std::vector<StmtPtr> body;
    void accept(ASTVisitor& v) override;
};

// ── Loop Statement (infinite loop) ──────────────────────────────────────
struct LoopStmt : Stmt {
    std::vector<StmtPtr> body;
    void accept(ASTVisitor& v) override;
};

// ── Try-Catch Statement ──────────────────────────────────────────────────
struct TryCatchStmt : Stmt {
    std::vector<StmtPtr> tryBlock;
    std::optional<std::string> catchVar;    // optional bound variable
    std::optional<TypeExpr>    catchType;
    std::vector<StmtPtr>       catchBlock;
    void accept(ASTVisitor& v) override;
};

// ══════════════════════════════════════════════════════════════════════════
// Declarations
// ══════════════════════════════════════════════════════════════════════════

// ── Field Declaration (inside a type) ────────────────────────────────────
struct FieldDecl {
    std::string  name;
    TypeExpr     typeAnnotation;
    bool         isPublic  = false;
    bool         isPrivate = false;
    SourceLocation loc;
};

// ── Function Parameter ────────────────────────────────────────────────────
struct Param {
    std::string  name;
    TypeExpr     typeAnnotation;
    SourceLocation loc;
};

// ── Function Declaration ─────────────────────────────────────────────────
struct FunctionDecl : Decl {
    std::vector<Param>   params;
    TypeExpr             returnType;
    std::vector<StmtPtr> body;
    bool                 isAsync = false;
    void accept(ASTVisitor& v) override;
};

// ── Type Declaration ─────────────────────────────────────────────────────
struct TypeDecl : Decl {
    std::vector<FieldDecl>                      fields;
    std::vector<std::unique_ptr<FunctionDecl>>  methods;
    void accept(ASTVisitor& v) override;
};

// ── Use Declaration ──────────────────────────────────────────────────────
struct UseDecl : Decl {
    std::vector<std::string>  modulePath; // ["net", "Socket"]
    std::string               alias;      // optional 'as X'
    bool                      isFromImport = false; // from ... use ...
    std::string               fromSymbol;           // symbol imported by 'from'
    void accept(ASTVisitor& v) override;
};

// ── Export Declaration ───────────────────────────────────────────────────
struct ExportDecl : Decl {
    std::string symbol;
    void accept(ASTVisitor& v) override;
};

// ── Context Declaration ──────────────────────────────────────────────────
struct ContextDecl : Decl {
    std::string contextName; // "safe", "fast", "realtime", etc.
    void accept(ASTVisitor& v) override;
};

// ── Macro Declaration ────────────────────────────────────────────────────
struct MacroDecl : Decl {
    std::vector<std::string>  args;
    std::vector<StmtPtr>      body;
    void accept(ASTVisitor& v) override;
};

// ──────────────────────────────────────────────────────────────────────────
// Top-Level Program
// ──────────────────────────────────────────────────────────────────────────
struct Program : ASTNode {
    std::string              programName;
    std::vector<DeclPtr>     decls;     // use, fn, type, const, export, context, macro
    void accept(ASTVisitor& v) override;
};

// ──────────────────────────────────────────────────────────────────────────
// ASTVisitor interface – implement to traverse the entire tree
// ──────────────────────────────────────────────────────────────────────────
struct ASTVisitor {
    virtual ~ASTVisitor() = default;

    // Program
    virtual void visitProgram(Program& n)          {}

    // Declarations
    virtual void visitUseDecl(UseDecl& n)          {}
    virtual void visitExportDecl(ExportDecl& n)    {}
    virtual void visitContextDecl(ContextDecl& n)  {}
    virtual void visitMacroDecl(MacroDecl& n)      {}
    virtual void visitFunctionDecl(FunctionDecl& n){}
    virtual void visitTypeDecl(TypeDecl& n)        {}
    virtual void visitConstDecl(ConstDecl& n)      {}

    // Statements
    virtual void visitLetDecl(LetDecl& n)          {}
    virtual void visitSetStmt(SetStmt& n)          {}
    virtual void visitReturnStmt(ReturnStmt& n)    {}
    virtual void visitBreakStmt(BreakStmt& n)      {}
    virtual void visitContinueStmt(ContinueStmt& n){}
    virtual void visitFreeStmt(FreeStmt& n)        {}
    virtual void visitThrowStmt(ThrowStmt& n)      {}
    virtual void visitIfStmt(IfStmt& n)            {}
    virtual void visitMatchStmt(MatchStmt& n)      {}
    virtual void visitForStmt(ForStmt& n)          {}
    virtual void visitWhileStmt(WhileStmt& n)      {}
    virtual void visitLoopStmt(LoopStmt& n)        {}
    virtual void visitTryCatchStmt(TryCatchStmt& n){}
    virtual void visitExprStmt(ExprStmt& n)        {}

    // Expressions
    virtual void visitIdentExpr(IdentExpr& n)      {}
    virtual void visitIntLitExpr(IntLitExpr& n)    {}
    virtual void visitFloatLitExpr(FloatLitExpr& n){}
    virtual void visitStringLitExpr(StringLitExpr& n){}
    virtual void visitCharLitExpr(CharLitExpr& n)  {}
    virtual void visitBoolLitExpr(BoolLitExpr& n)  {}
    virtual void visitNullLitExpr(NullLitExpr& n)  {}
    virtual void visitBinaryExpr(BinaryExpr& n)    {}
    virtual void visitUnaryExpr(UnaryExpr& n)      {}
    virtual void visitCallExpr(CallExpr& n)        {}
    virtual void visitMemberExpr(MemberExpr& n)    {}
    virtual void visitIndexExpr(IndexExpr& n)      {}
    virtual void visitNewExpr(NewExpr& n)          {}
    virtual void visitMoveExpr(MoveExpr& n)        {}
    virtual void visitCopyExpr(CopyExpr& n)        {}
    virtual void visitRefExpr(RefExpr& n)          {}
    virtual void visitAwaitExpr(AwaitExpr& n)      {}
};

// Accept implementations – defined inline.
inline void Program::accept(ASTVisitor& v)       { v.visitProgram(*this); }
inline void UseDecl::accept(ASTVisitor& v)        { v.visitUseDecl(*this); }
inline void ExportDecl::accept(ASTVisitor& v)     { v.visitExportDecl(*this); }
inline void ContextDecl::accept(ASTVisitor& v)    { v.visitContextDecl(*this); }
inline void MacroDecl::accept(ASTVisitor& v)      { v.visitMacroDecl(*this); }
inline void FunctionDecl::accept(ASTVisitor& v)   { v.visitFunctionDecl(*this); }
inline void TypeDecl::accept(ASTVisitor& v)       { v.visitTypeDecl(*this); }
inline void ConstDecl::accept(ASTVisitor& v)      { v.visitConstDecl(*this); }
inline void LetDecl::accept(ASTVisitor& v)        { v.visitLetDecl(*this); }
inline void SetStmt::accept(ASTVisitor& v)        { v.visitSetStmt(*this); }
inline void ReturnStmt::accept(ASTVisitor& v)     { v.visitReturnStmt(*this); }
inline void BreakStmt::accept(ASTVisitor& v)      { v.visitBreakStmt(*this); }
inline void ContinueStmt::accept(ASTVisitor& v)   { v.visitContinueStmt(*this); }
inline void FreeStmt::accept(ASTVisitor& v)       { v.visitFreeStmt(*this); }
inline void ThrowStmt::accept(ASTVisitor& v)      { v.visitThrowStmt(*this); }
inline void IfStmt::accept(ASTVisitor& v)         { v.visitIfStmt(*this); }
inline void MatchStmt::accept(ASTVisitor& v)      { v.visitMatchStmt(*this); }
inline void ForStmt::accept(ASTVisitor& v)        { v.visitForStmt(*this); }
inline void WhileStmt::accept(ASTVisitor& v)      { v.visitWhileStmt(*this); }
inline void LoopStmt::accept(ASTVisitor& v)       { v.visitLoopStmt(*this); }
inline void TryCatchStmt::accept(ASTVisitor& v)   { v.visitTryCatchStmt(*this); }
inline void ExprStmt::accept(ASTVisitor& v)       { v.visitExprStmt(*this); }
inline void IdentExpr::accept(ASTVisitor& v)      { v.visitIdentExpr(*this); }
inline void IntLitExpr::accept(ASTVisitor& v)     { v.visitIntLitExpr(*this); }
inline void FloatLitExpr::accept(ASTVisitor& v)   { v.visitFloatLitExpr(*this); }
inline void StringLitExpr::accept(ASTVisitor& v)  { v.visitStringLitExpr(*this); }
inline void CharLitExpr::accept(ASTVisitor& v)    { v.visitCharLitExpr(*this); }
inline void BoolLitExpr::accept(ASTVisitor& v)    { v.visitBoolLitExpr(*this); }
inline void NullLitExpr::accept(ASTVisitor& v)    { v.visitNullLitExpr(*this); }
inline void BinaryExpr::accept(ASTVisitor& v)     { v.visitBinaryExpr(*this); }
inline void UnaryExpr::accept(ASTVisitor& v)      { v.visitUnaryExpr(*this); }
inline void CallExpr::accept(ASTVisitor& v)       { v.visitCallExpr(*this); }
inline void MemberExpr::accept(ASTVisitor& v)     { v.visitMemberExpr(*this); }
inline void IndexExpr::accept(ASTVisitor& v)      { v.visitIndexExpr(*this); }
inline void NewExpr::accept(ASTVisitor& v)        { v.visitNewExpr(*this); }
inline void MoveExpr::accept(ASTVisitor& v)       { v.visitMoveExpr(*this); }
inline void CopyExpr::accept(ASTVisitor& v)       { v.visitCopyExpr(*this); }
inline void RefExpr::accept(ASTVisitor& v)        { v.visitRefExpr(*this); }
inline void AwaitExpr::accept(ASTVisitor& v)      { v.visitAwaitExpr(*this); }

// ──────────────────────────────────────────────────────────────────────────
// Helper: Convert a TypeExpr to a human-readable string (for diagnostics).
// ──────────────────────────────────────────────────────────────────────────
std::string typeExprToString(const TypeExpr& t);

} // namespace nsl
