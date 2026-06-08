#pragma once
// NSL Token Definitions
// Native Systems Language Compiler (nslc)

#include <string>
#include <cstdint>

namespace nsl {

// All token kinds: the 50 core keywords, literals, operators, punctuation.
enum class TokenKind {
    // ── Program Structure Keywords ─────────────────────────────────────────
    KW_PROGRAM,   // program
    KW_MODULE,    // module
    KW_USE,       // use
    KW_FROM,      // from
    KW_AS,        // as
    KW_EXPORT,    // export

    // ── Variable / Function Keywords ──────────────────────────────────────
    KW_LET,       // let
    KW_SET,       // set
    KW_CONST,     // const
    KW_TYPE,      // type
    KW_FN,        // fn
    KW_RETURN,    // return

    // ── Control Flow Keywords ──────────────────────────────────────────────
    KW_IF,        // if
    KW_ELSE,      // else
    KW_MATCH,     // match
    KW_CASE,      // case
    KW_DEFAULT,   // default

    // ── Loop Keywords ─────────────────────────────────────────────────────
    KW_FOR,       // for
    KW_WHILE,     // while
    KW_LOOP,      // loop
    KW_IN,        // in
    KW_BREAK,     // break
    KW_CONTINUE,  // continue

    // ── Boolean / Null Literals ────────────────────────────────────────────
    KW_TRUE,      // true
    KW_FALSE,     // false
    KW_NULL,      // null

    // ── Logical Operators (keyword form) ──────────────────────────────────
    KW_AND,       // and
    KW_OR,        // or
    KW_NOT,       // not
    KW_IS,        // is

    // ── Memory Ownership Keywords ──────────────────────────────────────────
    KW_NEW,       // new
    KW_FREE,      // free
    KW_MOVE,      // move
    KW_COPY,      // copy
    KW_REF,       // ref
    KW_PTR,       // ptr

    // ── Memory Region Keywords ─────────────────────────────────────────────
    KW_STACK,     // stack
    KW_HEAP,      // heap
    KW_ARENA,     // arena
    KW_ZONE,      // zone

    // ── Access Modifier Keywords ───────────────────────────────────────────
    KW_PUBLIC,    // public
    KW_PRIVATE,   // private
    KW_STATIC,    // static
    KW_VIRTUAL,   // virtual

    // ── Error Handling Keywords ────────────────────────────────────────────
    KW_TRY,       // try
    KW_CATCH,     // catch
    KW_THROW,     // throw

    // ── Async Keywords ────────────────────────────────────────────────────
    KW_ASYNC,     // async
    KW_AWAIT,     // await

    // ── Block Terminator ─────────────────────────────────────────────────
    KW_END,       // end

    // ── Compiler-Hint Keywords (extensions beyond the 50 core) ───────────
    KW_CONTEXT,   // context
    KW_MACRO,     // macro

    // ── Literals ─────────────────────────────────────────────────────────
    LIT_INT,      // 42, 0xFF, 0b1010
    LIT_FLOAT,    // 3.14, 2.0
    LIT_STRING,   // "hello"
    LIT_CHAR,     // 'a'

    // ── Identifier ───────────────────────────────────────────────────────
    IDENTIFIER,

    // ── Arithmetic Operators ──────────────────────────────────────────────
    OP_PLUS,      // +
    OP_MINUS,     // -
    OP_STAR,      // *
    OP_SLASH,     // /
    OP_PERCENT,   // %

    // ── Comparison Operators ──────────────────────────────────────────────
    OP_EQEQ,      // ==
    OP_NEQ,       // !=
    OP_LT,        // <
    OP_GT,        // >
    OP_LEQ,       // <=
    OP_GEQ,       // >=

    // ── Assignment ────────────────────────────────────────────────────────
    OP_ASSIGN,    // =

    // ── Return Type Arrow ─────────────────────────────────────────────────
    OP_ARROW,     // ->

    // ── Punctuation ───────────────────────────────────────────────────────
    PUNCT_DOT,        // .
    PUNCT_COLON,      // :
    PUNCT_COMMA,      // ,
    PUNCT_LPAREN,     // (
    PUNCT_RPAREN,     // )
    PUNCT_LBRACKET,   // [
    PUNCT_RBRACKET,   // ]
    PUNCT_LANGLE,     // < (generic open – resolved contextually)
    PUNCT_RANGLE,     // > (generic close – resolved contextually)
    PUNCT_AT,         // @  (optional memory alias)
    PUNCT_HASH,       // #  (optional stack alias)
    PUNCT_TILDE,      // ~

    // ── Structural ────────────────────────────────────────────────────────
    NEWLINE,          // logical statement separator
    END_OF_FILE,

    UNKNOWN,
};

// ──────────────────────────────────────────────────────────────────────────
// Source Location
// ──────────────────────────────────────────────────────────────────────────
struct SourceLocation {
    std::string filename;
    uint32_t    line   = 1;
    uint32_t    column = 1;

    std::string toString() const;
};

// ──────────────────────────────────────────────────────────────────────────
// Token
// ──────────────────────────────────────────────────────────────────────────
struct Token {
    TokenKind      kind  = TokenKind::UNKNOWN;
    std::string    text;          // raw source text
    SourceLocation loc;

    bool is(TokenKind k)     const { return kind == k; }
    bool isKeyword()         const;
    bool isLiteral()         const;
    bool isOperator()        const;
    bool isBinaryOperator()  const;

    // Human-readable name for diagnostics.
    std::string kindName() const;
};

// Map a keyword string to its TokenKind; returns IDENTIFIER if not a keyword.
TokenKind keywordKind(const std::string& word);

} // namespace nsl
