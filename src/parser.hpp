#pragma once
// NSL Recursive-Descent Parser
// Turns a token stream into a typed AST.

#include "ast.hpp"
#include "token.hpp"
#include <vector>
#include <functional>
#include <stdexcept>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Parse Error (thrown on unrecoverable syntax errors)
// ──────────────────────────────────────────────────────────────────────────
struct ParseError : std::runtime_error {
    SourceLocation loc;
    ParseError(const std::string& msg, SourceLocation l)
        : std::runtime_error(msg), loc(std::move(l)) {}
};

// ──────────────────────────────────────────────────────────────────────────
// Parser
// ──────────────────────────────────────────────────────────────────────────
class Parser {
public:
    using ErrorCallback = std::function<void(const std::string&, const SourceLocation&)>;

    explicit Parser(std::vector<Token> tokens,
                    ErrorCallback onError = nullptr);

    // Parse the entire token stream as a top-level Program.
    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> m_tokens;
    std::size_t        m_pos = 0;
    ErrorCallback      m_onError;
    int                m_errorCount = 0;

    // ── Token navigation ─────────────────────────────────────────────────
    const Token& peek(int offset = 0) const;
    const Token& current() const { return peek(0); }
    const Token& advance();
    bool         check(TokenKind k, int offset = 0) const;
    bool         match(TokenKind k);
    const Token& expect(TokenKind k, const std::string& msg = "");
    void         skipNewlines();
    bool         atEnd() const;

    // ── Error recovery ───────────────────────────────────────────────────
    void   reportError(const std::string& msg, const SourceLocation& loc);
    void   synchronize();   // skip to next statement boundary

    // ── Top-level parsing ────────────────────────────────────────────────
    DeclPtr parseTopDecl();

    // ── Declarations ─────────────────────────────────────────────────────
    std::unique_ptr<UseDecl>      parseUseDecl();
    std::unique_ptr<ExportDecl>   parseExportDecl();
    std::unique_ptr<ContextDecl>  parseContextDecl();
    std::unique_ptr<MacroDecl>    parseMacroDecl();
    std::unique_ptr<FunctionDecl> parseFunctionDecl(bool isAsync = false);
    std::unique_ptr<TypeDecl>     parseTypeDecl();
    std::unique_ptr<ConstDecl>    parseConstDecl();

    // ── Statements ───────────────────────────────────────────────────────
    StmtPtr                  parseStmt();
    std::unique_ptr<LetDecl> parseLetDecl();
    std::unique_ptr<SetStmt> parseSetStmt();
    std::unique_ptr<ReturnStmt>   parseReturnStmt();
    std::unique_ptr<IfStmt>       parseIfStmt();
    std::unique_ptr<MatchStmt>    parseMatchStmt();
    std::unique_ptr<ForStmt>      parseForStmt();
    std::unique_ptr<WhileStmt>    parseWhileStmt();
    std::unique_ptr<LoopStmt>     parseLoopStmt();
    std::unique_ptr<TryCatchStmt> parseTryCatchStmt();
    std::unique_ptr<FreeStmt>     parseFreeStmt();
    std::unique_ptr<ThrowStmt>    parseThrowStmt();

    // Parse a block of statements until a terminator token (end / else / case / catch / default)
    std::vector<StmtPtr> parseBlock(std::initializer_list<TokenKind> terminators);

    // ── Type expressions ─────────────────────────────────────────────────
    TypeExpr parseTypeExpr();

    // ── Memory spec ──────────────────────────────────────────────────────
    MemorySpec parseMemorySpec(); // consumes 'in stack/heap/arena X/zone X'

    // ── Expressions (Pratt parser) ────────────────────────────────────────
    ExprPtr parseExpr(int minPrec = 0);
    ExprPtr parsePrimary();
    ExprPtr parsePostfix(ExprPtr base);
    int     getBinaryPrec(TokenKind k) const;
    bool    isBinaryOp(TokenKind k) const;
};

} // namespace nsl
