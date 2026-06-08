#pragma once
// NSL IR Generator
// Walks the AST and emits NSL-IR via IRBuilder.

#include "ast.hpp"
#include "ir.hpp"
#include "sema.hpp"
#include "diagnostics.hpp"
#include <optional>

namespace nsl {

class IRGen : public ASTVisitor {
public:
    IRGen(DiagnosticEngine& diag, SemanticAnalyzer& sema);

    // Generate IR for a full program.  Returns the populated Module.
    ir::Module generate(Program& prog);

private:
    DiagnosticEngine& m_diag;
    SemanticAnalyzer& m_sema;

    ir::Module    m_mod;
    ir::IRBuilder m_builder;

    // Current expression result value (thread through visitor calls)
    std::optional<ir::Value> m_exprValue;

    // Scope: variable name → alloca pointer value
    struct VarEntry { ir::Value ptr; TypePtr type; };
    std::vector<std::unordered_map<std::string, VarEntry>> m_varScopes;

    void pushVarScope() { m_varScopes.emplace_back(); }
    void popVarScope()  { if (!m_varScopes.empty()) m_varScopes.pop_back(); }
    void defineVar(const std::string& name, ir::Value ptr, TypePtr t);
    VarEntry* lookupVar(const std::string& name);

    // Loop labels for break/continue
    struct LoopContext { std::string continueLabel; std::string breakLabel; };
    std::vector<LoopContext> m_loopStack;

    // ── AST Visitor overrides ─────────────────────────────────────────────
    void visitProgram(Program& n)           override;
    void visitUseDecl(UseDecl& n)           override;
    void visitFunctionDecl(FunctionDecl& n) override;
    void visitTypeDecl(TypeDecl& n)         override;
    void visitConstDecl(ConstDecl& n)       override;
    void visitLetDecl(LetDecl& n)           override;
    void visitSetStmt(SetStmt& n)           override;
    void visitReturnStmt(ReturnStmt& n)     override;
    void visitBreakStmt(BreakStmt& n)       override;
    void visitContinueStmt(ContinueStmt& n) override;
    void visitFreeStmt(FreeStmt& n)         override;
    void visitThrowStmt(ThrowStmt& n)       override;
    void visitIfStmt(IfStmt& n)             override;
    void visitMatchStmt(MatchStmt& n)       override;
    void visitForStmt(ForStmt& n)           override;
    void visitWhileStmt(WhileStmt& n)       override;
    void visitLoopStmt(LoopStmt& n)         override;
    void visitTryCatchStmt(TryCatchStmt& n) override;
    void visitExprStmt(ExprStmt& n)         override;

    void visitIdentExpr(IdentExpr& n)         override;
    void visitIntLitExpr(IntLitExpr& n)       override;
    void visitFloatLitExpr(FloatLitExpr& n)   override;
    void visitStringLitExpr(StringLitExpr& n) override;
    void visitCharLitExpr(CharLitExpr& n)     override;
    void visitBoolLitExpr(BoolLitExpr& n)     override;
    void visitNullLitExpr(NullLitExpr& n)     override;
    void visitBinaryExpr(BinaryExpr& n)       override;
    void visitUnaryExpr(UnaryExpr& n)         override;
    void visitCallExpr(CallExpr& n)           override;
    void visitMemberExpr(MemberExpr& n)       override;
    void visitIndexExpr(IndexExpr& n)         override;
    void visitNewExpr(NewExpr& n)             override;
    void visitMoveExpr(MoveExpr& n)           override;
    void visitCopyExpr(CopyExpr& n)           override;
    void visitRefExpr(RefExpr& n)             override;
    void visitAwaitExpr(AwaitExpr& n)         override;

    // ── Helpers ───────────────────────────────────────────────────────────
    ir::Value emitExpr(Expr& e);
    void      emitStmts(std::vector<StmtPtr>& stmts);
    TypePtr   resolveType(const std::string& name);
    TypePtr   resolveType(const TypeExpr& te);

    static ir::Opcode binOpcode(const std::string& op);
};

} // namespace nsl
