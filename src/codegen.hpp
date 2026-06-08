#pragma once
// NSL C Transpilation Backend
// Converts the NSL AST directly to portable C99 code.
// The generated C code is then compiled with a system C compiler (gcc/clang/cl).

#include "ast.hpp"
#include "types.hpp"
#include "diagnostics.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace nsl {

class CCodeGen : public ASTVisitor {
public:
    explicit CCodeGen(DiagnosticEngine& diag);

    // Generate a full .c file from the AST program.
    std::string generate(Program& prog);

private:
    DiagnosticEngine& m_diag;
    std::ostringstream m_out;   // main output stream
    std::ostringstream m_decls; // forward declarations / struct definitions
    int m_indent = 0;
    int m_tmpCounter = 0;

    // Modules that have been imported
    std::unordered_set<std::string> m_imports;

    // Type registry (for struct layout)
    TypeRegistry m_types;

    // ── Formatting helpers ─────────────────────────────────────────────────
    void indent();
    void line(const std::string& s);
    void line();
    std::string fresh(const std::string& prefix = "tmp");

    // ── Type helpers ──────────────────────────────────────────────────────
    std::string cTypeName(const TypeExpr& te);
    std::string cTypeName(const TypePtr& t);
    std::string cTypeName(const std::string& nslType);

    // ── Expression emission (returns a C expression string) ──────────────
    std::string emitExpr(Expr& e);
    std::string emitExprStr(Expr* e);

    // ── Statement emission ────────────────────────────────────────────────
    void emitStmt(Stmt& s);
    void emitStmts(std::vector<StmtPtr>& stmts);
    void emitBlock(std::vector<StmtPtr>& stmts);

    // ── Visitor overrides ─────────────────────────────────────────────────
    void visitProgram(Program& n)           override;
    void visitUseDecl(UseDecl& n)           override;
    void visitExportDecl(ExportDecl& n)     override;
    void visitContextDecl(ContextDecl& n)   override;
    void visitMacroDecl(MacroDecl& n)       override;
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

    // Expression visitors store result in m_lastExpr
    std::string m_lastExpr;
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

    // ── Runtime header emission ────────────────────────────────────────────
    void emitRuntimeHeader();
    void emitModuleBindings();
};

} // namespace nsl
