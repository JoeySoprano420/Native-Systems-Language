// NSL Lexer Tests
// Validates tokenization of all 50 keywords and various literal types.

#include "lexer.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace nsl;

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT(cond, msg)                                               \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cerr << "FAIL [" << __func__ << "]: " << (msg) << " (" #cond ")\n"; \
            ++g_fail;                                                   \
        } else {                                                        \
            ++g_pass;                                                   \
        }                                                               \
    } while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b), "Expected " #a " == " #b)

// ──────────────────────────────────────────────────────────────────────────
static void test_keywords() {
    const char* src = "program module use from as export "
                      "let set const type fn return "
                      "if else match case default "
                      "for while loop in break continue "
                      "true false null "
                      "and or not is "
                      "new free move copy ref ptr "
                      "stack heap arena zone "
                      "public private static virtual "
                      "try catch throw "
                      "async await "
                      "end";
    Lexer lex(src, "<test>");
    auto tokens = lex.tokenize();

    // Remove NEWLINE and EOF
    std::vector<Token> kw;
    for (auto& t : tokens)
        if (t.kind != TokenKind::NEWLINE && t.kind != TokenKind::END_OF_FILE)
            kw.push_back(t);

    ASSERT_EQ(kw.size(), 50u);

    ASSERT_EQ(kw[0].kind,  TokenKind::KW_PROGRAM);
    ASSERT_EQ(kw[1].kind,  TokenKind::KW_MODULE);
    ASSERT_EQ(kw[2].kind,  TokenKind::KW_USE);
    ASSERT_EQ(kw[3].kind,  TokenKind::KW_FROM);
    ASSERT_EQ(kw[4].kind,  TokenKind::KW_AS);
    ASSERT_EQ(kw[5].kind,  TokenKind::KW_EXPORT);
    ASSERT_EQ(kw[6].kind,  TokenKind::KW_LET);
    ASSERT_EQ(kw[7].kind,  TokenKind::KW_SET);
    ASSERT_EQ(kw[8].kind,  TokenKind::KW_CONST);
    ASSERT_EQ(kw[9].kind,  TokenKind::KW_TYPE);
    ASSERT_EQ(kw[10].kind, TokenKind::KW_FN);
    ASSERT_EQ(kw[11].kind, TokenKind::KW_RETURN);
    ASSERT_EQ(kw[12].kind, TokenKind::KW_IF);
    ASSERT_EQ(kw[13].kind, TokenKind::KW_ELSE);
    ASSERT_EQ(kw[14].kind, TokenKind::KW_MATCH);
    ASSERT_EQ(kw[15].kind, TokenKind::KW_CASE);
    ASSERT_EQ(kw[16].kind, TokenKind::KW_DEFAULT);
    ASSERT_EQ(kw[17].kind, TokenKind::KW_FOR);
    ASSERT_EQ(kw[18].kind, TokenKind::KW_WHILE);
    ASSERT_EQ(kw[19].kind, TokenKind::KW_LOOP);
    ASSERT_EQ(kw[20].kind, TokenKind::KW_IN);
    ASSERT_EQ(kw[21].kind, TokenKind::KW_BREAK);
    ASSERT_EQ(kw[22].kind, TokenKind::KW_CONTINUE);
    ASSERT_EQ(kw[23].kind, TokenKind::KW_TRUE);
    ASSERT_EQ(kw[24].kind, TokenKind::KW_FALSE);
    ASSERT_EQ(kw[25].kind, TokenKind::KW_NULL);
    ASSERT_EQ(kw[26].kind, TokenKind::KW_AND);
    ASSERT_EQ(kw[27].kind, TokenKind::KW_OR);
    ASSERT_EQ(kw[28].kind, TokenKind::KW_NOT);
    ASSERT_EQ(kw[29].kind, TokenKind::KW_IS);
    ASSERT_EQ(kw[30].kind, TokenKind::KW_NEW);
    ASSERT_EQ(kw[31].kind, TokenKind::KW_FREE);
    ASSERT_EQ(kw[32].kind, TokenKind::KW_MOVE);
    ASSERT_EQ(kw[33].kind, TokenKind::KW_COPY);
    ASSERT_EQ(kw[34].kind, TokenKind::KW_REF);
    ASSERT_EQ(kw[35].kind, TokenKind::KW_PTR);
    ASSERT_EQ(kw[36].kind, TokenKind::KW_STACK);
    ASSERT_EQ(kw[37].kind, TokenKind::KW_HEAP);
    ASSERT_EQ(kw[38].kind, TokenKind::KW_ARENA);
    ASSERT_EQ(kw[39].kind, TokenKind::KW_ZONE);
    ASSERT_EQ(kw[40].kind, TokenKind::KW_PUBLIC);
    ASSERT_EQ(kw[41].kind, TokenKind::KW_PRIVATE);
    ASSERT_EQ(kw[42].kind, TokenKind::KW_STATIC);
    ASSERT_EQ(kw[43].kind, TokenKind::KW_VIRTUAL);
    ASSERT_EQ(kw[44].kind, TokenKind::KW_TRY);
    ASSERT_EQ(kw[45].kind, TokenKind::KW_CATCH);
    ASSERT_EQ(kw[46].kind, TokenKind::KW_THROW);
    ASSERT_EQ(kw[47].kind, TokenKind::KW_ASYNC);
    ASSERT_EQ(kw[48].kind, TokenKind::KW_AWAIT);
    ASSERT_EQ(kw[49].kind, TokenKind::KW_END);
}

static void test_literals() {
    Lexer lex("42 3.14 \"hello\" 'x' true false null 0xFF 0b1010", "<test>");
    auto tokens = lex.tokenize();
    std::vector<Token> toks;
    for (auto& t : tokens)
        if (t.kind != TokenKind::NEWLINE && t.kind != TokenKind::END_OF_FILE)
            toks.push_back(t);

    ASSERT_EQ(toks[0].kind,  TokenKind::LIT_INT);
    ASSERT_EQ(toks[0].text,  "42");
    ASSERT_EQ(toks[1].kind,  TokenKind::LIT_FLOAT);
    ASSERT_EQ(toks[1].text,  "3.14");
    ASSERT_EQ(toks[2].kind,  TokenKind::LIT_STRING);
    ASSERT_EQ(toks[2].text,  "hello");
    ASSERT_EQ(toks[3].kind,  TokenKind::LIT_CHAR);
    ASSERT_EQ(toks[3].text,  "x");
    ASSERT_EQ(toks[4].kind,  TokenKind::KW_TRUE);
    ASSERT_EQ(toks[5].kind,  TokenKind::KW_FALSE);
    ASSERT_EQ(toks[6].kind,  TokenKind::KW_NULL);
    ASSERT_EQ(toks[7].kind,  TokenKind::LIT_INT); // 0xFF
    ASSERT_EQ(toks[8].kind,  TokenKind::LIT_INT); // 0b1010
}

static void test_operators() {
    Lexer lex("+ - * / % == != < > <= >= = ->", "<test>");
    auto tokens = lex.tokenize();
    std::vector<Token> toks;
    for (auto& t : tokens)
        if (t.kind != TokenKind::NEWLINE && t.kind != TokenKind::END_OF_FILE)
            toks.push_back(t);

    ASSERT_EQ(toks[0].kind,  TokenKind::OP_PLUS);
    ASSERT_EQ(toks[1].kind,  TokenKind::OP_MINUS);
    ASSERT_EQ(toks[2].kind,  TokenKind::OP_STAR);
    ASSERT_EQ(toks[3].kind,  TokenKind::OP_SLASH);
    ASSERT_EQ(toks[4].kind,  TokenKind::OP_PERCENT);
    ASSERT_EQ(toks[5].kind,  TokenKind::OP_EQEQ);
    ASSERT_EQ(toks[6].kind,  TokenKind::OP_NEQ);
    ASSERT_EQ(toks[7].kind,  TokenKind::OP_LT);
    ASSERT_EQ(toks[8].kind,  TokenKind::OP_GT);
    ASSERT_EQ(toks[9].kind,  TokenKind::OP_LEQ);
    ASSERT_EQ(toks[10].kind, TokenKind::OP_GEQ);
    ASSERT_EQ(toks[11].kind, TokenKind::OP_ASSIGN);
    ASSERT_EQ(toks[12].kind, TokenKind::OP_ARROW);
}

static void test_comments() {
    Lexer lex("// this is a comment\n42\n/* block\ncomment */\n99", "<test>");
    auto tokens = lex.tokenize();
    std::vector<Token> toks;
    for (auto& t : tokens)
        if (t.kind != TokenKind::NEWLINE && t.kind != TokenKind::END_OF_FILE)
            toks.push_back(t);
    ASSERT_EQ(toks.size(), 2u);
    ASSERT_EQ(toks[0].text, "42");
    ASSERT_EQ(toks[1].text, "99");
}

static void test_string_escapes() {
    Lexer lex("\"hello\\nworld\\t!\"", "<test>");
    auto tokens = lex.tokenize();
    ASSERT_EQ(tokens[0].kind, TokenKind::LIT_STRING);
    ASSERT_EQ(tokens[0].text, "hello\nworld\t!");
}

static void test_source_location() {
    Lexer lex("foo\nbar\nbaz", "<file.nsl>");
    auto tokens = lex.tokenize();
    ASSERT_EQ(tokens[0].loc.line, 1u);
    ASSERT_EQ(tokens[0].loc.column, 1u);
    // tokens[1] is NEWLINE
    ASSERT_EQ(tokens[2].loc.line, 2u);
}

// ──────────────────────────────────────────────────────────────────────────
// Lexer test runner
// ──────────────────────────────────────────────────────────────────────────
void run_lexer_tests() {
    std::cout << "=== Lexer Tests ===\n";
    test_keywords();
    test_literals();
    test_operators();
    test_comments();
    test_string_escapes();
    test_source_location();
}
