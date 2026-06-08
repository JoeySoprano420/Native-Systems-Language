#pragma once
// NSL Semantic Analyzer
// Type-checks and annotates the AST; resolves symbols; validates memory regions.

#include "ast.hpp"
#include "types.hpp"
#include "diagnostics.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Symbol Entry
// ──────────────────────────────────────────────────────────────────────────
struct Symbol {
    std::string  name;
    TypePtr      type;
    MemorySpec   memSpec;
    bool         isMutable  = true;
    bool         isConst    = false;
    bool         isFunction = false;
    SourceLocation declLoc;
};

// ──────────────────────────────────────────────────────────────────────────
// Scope
// ──────────────────────────────────────────────────────────────────────────
class Scope {
public:
    void define(const std::string& name, Symbol sym);
    Symbol* lookup(const std::string& name);          // current scope only
    Symbol* lookupAll(const std::string& name);       // walk parent chain
    void setParent(Scope* p) { m_parent = p; }
    Scope* parent() const    { return m_parent; }

private:
    std::unordered_map<std::string, Symbol> m_symbols;
    Scope* m_parent = nullptr;
};

// ──────────────────────────────────────────────────────────────────────────
// Semantic Analyzer
// ──────────────────────────────────────────────────────────────────────────
class SemanticAnalyzer : public ASTVisitor {
public:
    explicit SemanticAnalyzer(DiagnosticEngine& diag);

    // Entry point – analyse a full program, returns false on error
    bool analyse(Program& prog);

    // Public for testing individual nodes
    TypePtr inferExprType(Expr& e);

    TypeRegistry& typeRegistry() { return m_types; }

private:
    DiagnosticEngine& m_diag;
    TypeRegistry      m_types;

    // Scope stack
    std::vector<Scope> m_scopeStack;
    Scope* currentScope();
    void pushScope();
    void popScope();

    // Current function return type (for 'return' checking)
    TypePtr m_currentReturnType;
    bool    m_inLoop   = false;
    bool    m_inAsync  = false;

    // Register all top-level names first (so forward references work)
    void firstPass(Program& prog);

    // ── Statement / Declaration visitors ─────────────────────────────────
    void visitProgram(Program& n)          override;
    void visitUseDecl(UseDecl& n)          override;
    void visitExportDecl(ExportDecl& n)    override;
    void visitContextDecl(ContextDecl& n)  override;
    void visitMacroDecl(MacroDecl& n)      override;
    void visitFunctionDecl(FunctionDecl& n)override;
    void visitTypeDecl(TypeDecl& n)        override;
    void visitConstDecl(ConstDecl& n)      override;
    void visitLetDecl(LetDecl& n)          override;
    void visitSetStmt(SetStmt& n)          override;
    void visitReturnStmt(ReturnStmt& n)    override;
    void visitBreakStmt(BreakStmt& n)      override;
    void visitContinueStmt(ContinueStmt& n)override;
    void visitFreeStmt(FreeStmt& n)        override;
    void visitThrowStmt(ThrowStmt& n)      override;
    void visitIfStmt(IfStmt& n)            override;
    void visitMatchStmt(MatchStmt& n)      override;
    void visitForStmt(ForStmt& n)          override;
    void visitWhileStmt(WhileStmt& n)      override;
    void visitLoopStmt(LoopStmt& n)        override;
    void visitTryCatchStmt(TryCatchStmt& n)override;
    void visitExprStmt(ExprStmt& n)        override;

    // ── Expression visitors (return type via inferExprType) ───────────────
    void visitIdentExpr(IdentExpr& n)        override;
    void visitIntLitExpr(IntLitExpr& n)      override;
    void visitFloatLitExpr(FloatLitExpr& n)  override;
    void visitStringLitExpr(StringLitExpr& n)override;
    void visitCharLitExpr(CharLitExpr& n)    override;
    void visitBoolLitExpr(BoolLitExpr& n)    override;
    void visitNullLitExpr(NullLitExpr& n)    override;
    void visitBinaryExpr(BinaryExpr& n)      override;
    void visitUnaryExpr(UnaryExpr& n)        override;
    void visitCallExpr(CallExpr& n)          override;
    void visitMemberExpr(MemberExpr& n)      override;
    void visitIndexExpr(IndexExpr& n)        override;
    void visitNewExpr(NewExpr& n)            override;
    void visitMoveExpr(MoveExpr& n)          override;
    void visitCopyExpr(CopyExpr& n)          override;
    void visitRefExpr(RefExpr& n)            override;
    void visitAwaitExpr(AwaitExpr& n)        override;

    // ── Helpers ───────────────────────────────────────────────────────────
    void analyseStmts(std::vector<StmtPtr>& stmts);
    TypePtr resolveTypeAnnotation(const TypeExpr& te);
    void checkType(const TypePtr& actual, const TypePtr& expected,
                   const SourceLocation& loc, const std::string& context);
    Symbol* requireSymbol(const std::string& name, const SourceLocation& loc);
};

} // namespace nsl
