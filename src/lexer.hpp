#pragma once
// NSL Lexer
// Converts a source string into a flat stream of Tokens.

#include "token.hpp"
#include <string>
#include <vector>
#include <functional>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Lexer
// ──────────────────────────────────────────────────────────────────────────
class Lexer {
public:
    // Callback invoked on lex errors: (message, location).
    using ErrorCallback = std::function<void(const std::string&, const SourceLocation&)>;

    explicit Lexer(std::string source,
                   std::string filename  = "<input>",
                   ErrorCallback onError = nullptr);

    // Tokenize the entire source and return all tokens
    // (including a final END_OF_FILE token).
    std::vector<Token> tokenize();

private:
    std::string    m_source;
    std::string    m_filename;
    ErrorCallback  m_onError;

    std::size_t m_pos    = 0;
    uint32_t    m_line   = 1;
    uint32_t    m_col    = 1;

    // ── Lookahead helpers ─────────────────────────────────────────────────
    char peek(int offset = 0) const;
    char advance();
    bool match(char c);
    bool atEnd() const;

    // ── Token factories ───────────────────────────────────────────────────
    Token makeToken(TokenKind kind, const std::string& text) const;
    SourceLocation currentLoc() const;

    // ── Scanning routines ─────────────────────────────────────────────────
    void        skipLineComment();
    void        skipBlockComment();
    Token       readString();
    Token       readChar();
    Token       readNumber();
    Token       readIdentOrKeyword();
    Token       readOperatorOrPunct();

    void        reportError(const std::string& msg);
};

} // namespace nsl
