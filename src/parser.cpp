// NSL Parser Implementation

#include "parser.hpp"
#include <sstream>
#include <cassert>
#include <stdexcept>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────
Parser::Parser(std::vector<Token> tokens, ErrorCallback onError)
    : m_tokens(std::move(tokens))
    , m_onError(std::move(onError))
{}

// ──────────────────────────────────────────────────────────────────────────
// Token navigation
// ──────────────────────────────────────────────────────────────────────────
const Token& Parser::peek(int offset) const {
    std::size_t idx = m_pos + static_cast<std::size_t>(offset);
    if (idx >= m_tokens.size()) return m_tokens.back(); // EOF
    return m_tokens[idx];
}

const Token& Parser::advance() {
    if (!atEnd()) ++m_pos;
    return m_tokens[m_pos - 1];
}

bool Parser::check(TokenKind k, int offset) const {
    return peek(offset).kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { advance(); return true; }
    return false;
}

bool Parser::atEnd() const {
    return m_pos >= m_tokens.size() ||
           m_tokens[m_pos].kind == TokenKind::END_OF_FILE;
}

void Parser::skipNewlines() {
    while (check(TokenKind::NEWLINE)) advance();
}

const Token& Parser::expect(TokenKind k, const std::string& msg) {
    if (!check(k)) {
        std::string err = msg.empty()
            ? "Expected " + Token{k, "", {}}.kindName() + " but got " + current().kindName()
            : msg;
        reportError(err, current().loc);
        // Return current token to continue parsing (error recovery)
        return current();
    }
    return advance();
}

// ──────────────────────────────────────────────────────────────────────────
// Error handling
// ──────────────────────────────────────────────────────────────────────────
void Parser::reportError(const std::string& msg, const SourceLocation& loc) {
    ++m_errorCount;
    if (m_onError) m_onError(msg, loc);
    else throw ParseError(msg, loc);
}

void Parser::synchronize() {
    // Skip tokens until we find a statement boundary.
    while (!atEnd()) {
        if (check(TokenKind::NEWLINE)) { advance(); return; }
        switch (current().kind) {
            case TokenKind::KW_FN:
            case TokenKind::KW_TYPE:
            case TokenKind::KW_LET:
            case TokenKind::KW_SET:
            case TokenKind::KW_RETURN:
            case TokenKind::KW_IF:
            case TokenKind::KW_FOR:
            case TokenKind::KW_WHILE:
            case TokenKind::KW_LOOP:
            case TokenKind::KW_END:
                return;
            default:
                advance();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Top-level: program
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    prog->loc = current().loc;

    skipNewlines();
    expect(TokenKind::KW_PROGRAM, "Expected 'program' at start of file");
    skipNewlines();
    prog->programName = expect(TokenKind::IDENTIFIER, "Expected program name").text;

    while (!atEnd()) {
        skipNewlines();
        if (atEnd()) break;
        try {
            auto decl = parseTopDecl();
            if (decl) prog->decls.push_back(std::move(decl));
        } catch (const ParseError& e) {
            reportError(e.what(), e.loc);
            synchronize();
        }
    }
    return prog;
}

// ──────────────────────────────────────────────────────────────────────────
// Top-level declarations
// ──────────────────────────────────────────────────────────────────────────
DeclPtr Parser::parseTopDecl() {
    skipNewlines();
    const Token& tok = current();

    switch (tok.kind) {
        case TokenKind::KW_USE:     return parseUseDecl();
        case TokenKind::KW_FROM:    return parseUseDecl();   // from ... use ...
        case TokenKind::KW_EXPORT:  return parseExportDecl();
        case TokenKind::KW_CONTEXT: return parseContextDecl();
        case TokenKind::KW_MACRO:   return parseMacroDecl();
        case TokenKind::KW_FN:      return parseFunctionDecl();
        case TokenKind::KW_ASYNC:
            advance(); // consume 'async'
            expect(TokenKind::KW_FN, "Expected 'fn' after 'async'");
            { auto fn = parseFunctionDecl(true);
              fn->isAsync = true;
              return fn; }
        case TokenKind::KW_TYPE:    return parseTypeDecl();
        case TokenKind::KW_CONST:   return parseConstDecl();
        case TokenKind::KW_PUBLIC:
        case TokenKind::KW_PRIVATE:
        case TokenKind::KW_STATIC:
        case TokenKind::KW_VIRTUAL: {
            // Access modifier prefix – remember it, then re-dispatch
            bool isPublic  = match(TokenKind::KW_PUBLIC);
            bool isPrivate = !isPublic && match(TokenKind::KW_PRIVATE);
            bool isStatic  = match(TokenKind::KW_STATIC);
            bool isVirtual = match(TokenKind::KW_VIRTUAL);
            skipNewlines();
            DeclPtr d = parseTopDecl();
            if (d) {
                d->isPublic  = isPublic;
                d->isPrivate = isPrivate;
                d->isStatic  = isStatic;
                d->isVirtual = isVirtual;
            }
            return d;
        }
        default:
            reportError("Unexpected token '" + tok.text + "' at top level", tok.loc);
            advance();
            return nullptr;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// use / from … use
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<UseDecl> Parser::parseUseDecl() {
    auto node   = std::make_unique<UseDecl>();
    node->loc   = current().loc;

    if (match(TokenKind::KW_FROM)) {
        // from <path> use <symbol> as <alias>?
        node->isFromImport = true;
        do {
            node->modulePath.push_back(
                expect(TokenKind::IDENTIFIER, "Expected module name").text);
        } while (match(TokenKind::PUNCT_DOT));
        expect(TokenKind::KW_USE, "Expected 'use' after module path");
        node->fromSymbol = expect(TokenKind::IDENTIFIER, "Expected symbol name").text;
        if (match(TokenKind::KW_AS))
            node->alias = expect(TokenKind::IDENTIFIER, "Expected alias").text;
    } else {
        // use <path> (as <alias>)?
        expect(TokenKind::KW_USE);
        do {
            node->modulePath.push_back(
                expect(TokenKind::IDENTIFIER, "Expected module name").text);
        } while (match(TokenKind::PUNCT_DOT));
        if (match(TokenKind::KW_AS))
            node->alias = expect(TokenKind::IDENTIFIER, "Expected alias").text;
    }
    node->name = node->modulePath.empty() ? "" : node->modulePath.back();
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// export
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ExportDecl> Parser::parseExportDecl() {
    auto node = std::make_unique<ExportDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_EXPORT);
    node->symbol = expect(TokenKind::IDENTIFIER, "Expected symbol name after 'export'").text;
    node->name   = node->symbol;
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// context
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ContextDecl> Parser::parseContextDecl() {
    auto node = std::make_unique<ContextDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_CONTEXT);
    node->contextName = expect(TokenKind::IDENTIFIER, "Expected context name").text;
    node->name        = node->contextName;
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// macro
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<MacroDecl> Parser::parseMacroDecl() {
    auto node = std::make_unique<MacroDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_MACRO);
    node->name = expect(TokenKind::IDENTIFIER, "Expected macro name").text;
    if (match(TokenKind::PUNCT_LPAREN)) {
        while (!check(TokenKind::PUNCT_RPAREN) && !atEnd()) {
            node->args.push_back(
                expect(TokenKind::IDENTIFIER, "Expected macro argument").text);
            if (!match(TokenKind::PUNCT_COMMA)) break;
        }
        expect(TokenKind::PUNCT_RPAREN);
    }
    skipNewlines();
    node->body = parseBlock({TokenKind::KW_END});
    expect(TokenKind::KW_END);
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// fn
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl(bool isAsync) {
    auto node    = std::make_unique<FunctionDecl>();
    node->loc    = current().loc;
    node->isAsync = isAsync;

    if (!isAsync) expect(TokenKind::KW_FN, "Expected 'fn'");

    node->name = expect(TokenKind::IDENTIFIER, "Expected function name").text;

    // Parameter list
    expect(TokenKind::PUNCT_LPAREN, "Expected '(' after function name");
    while (!check(TokenKind::PUNCT_RPAREN) && !atEnd()) {
        Param p;
        p.loc  = current().loc;
        p.name = expect(TokenKind::IDENTIFIER, "Expected parameter name").text;
        expect(TokenKind::PUNCT_COLON, "Expected ':' after parameter name");
        p.typeAnnotation = parseTypeExpr();
        // Skip optional default value (= expr) - stored as expression statement
        if (match(TokenKind::OP_ASSIGN)) parseExpr(); // consume but ignore for now
        node->params.push_back(std::move(p));
        if (!match(TokenKind::PUNCT_COMMA)) break;
    }
    expect(TokenKind::PUNCT_RPAREN, "Expected ')' after parameters");

    // Return type
    if (match(TokenKind::OP_ARROW)) {
        node->returnType = parseTypeExpr();
    } else {
        node->returnType = TypeExpr{"void", {}, false, false, current().loc};
    }

    skipNewlines();
    node->body = parseBlock({TokenKind::KW_END});
    expect(TokenKind::KW_END, "Expected 'end' to close function body");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// type
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<TypeDecl> Parser::parseTypeDecl() {
    auto node = std::make_unique<TypeDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_TYPE);
    node->name = expect(TokenKind::IDENTIFIER, "Expected type name").text;
    skipNewlines();

    while (!check(TokenKind::KW_END) && !atEnd()) {
        skipNewlines();
        if (check(TokenKind::KW_END)) break;

        // Method
        if (check(TokenKind::KW_FN) || check(TokenKind::KW_ASYNC)) {
            bool isAsync = match(TokenKind::KW_ASYNC);
            if (isAsync) expect(TokenKind::KW_FN);
            auto fn = parseFunctionDecl(isAsync);
            node->methods.push_back(std::move(fn));
            skipNewlines();
            continue;
        }

        // Field: 'let' name ':' type
        if (match(TokenKind::KW_LET)) {
            FieldDecl f;
            f.loc  = current().loc;
            f.name = expect(TokenKind::IDENTIFIER, "Expected field name").text;
            expect(TokenKind::PUNCT_COLON, "Expected ':' after field name");
            f.typeAnnotation = parseTypeExpr();
            node->fields.push_back(std::move(f));
            skipNewlines();
            continue;
        }

        // Access modifiers on fields/methods
        bool isPublic  = match(TokenKind::KW_PUBLIC);
        bool isPrivate = !isPublic && match(TokenKind::KW_PRIVATE);
        if (check(TokenKind::KW_LET)) {
            advance();
            FieldDecl f;
            f.loc       = current().loc;
            f.isPublic  = isPublic;
            f.isPrivate = isPrivate;
            f.name      = expect(TokenKind::IDENTIFIER, "Expected field name").text;
            expect(TokenKind::PUNCT_COLON, "Expected ':' after field name");
            f.typeAnnotation = parseTypeExpr();
            node->fields.push_back(std::move(f));
            skipNewlines();
            continue;
        }

        // Unexpected token inside type
        reportError("Unexpected token inside type: '" + current().text + "'", current().loc);
        advance();
    }
    expect(TokenKind::KW_END, "Expected 'end' to close type body");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// const
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ConstDecl> Parser::parseConstDecl() {
    auto node = std::make_unique<ConstDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_CONST);
    node->name = expect(TokenKind::IDENTIFIER, "Expected constant name").text;
    expect(TokenKind::PUNCT_COLON, "Expected ':' after constant name");
    node->typeAnnotation = parseTypeExpr();
    expect(TokenKind::OP_ASSIGN, "Expected '=' after constant type");
    node->init = parseExpr();
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// Block parsing (read statements until a terminator keyword)
// ──────────────────────────────────────────────────────────────────────────
std::vector<StmtPtr> Parser::parseBlock(std::initializer_list<TokenKind> terminators) {
    std::vector<StmtPtr> stmts;
    while (!atEnd()) {
        skipNewlines();
        for (auto tk : terminators)
            if (check(tk)) return stmts;
        if (atEnd()) break;
        try {
            auto s = parseStmt();
            if (s) stmts.push_back(std::move(s));
        } catch (const ParseError& e) {
            reportError(e.what(), e.loc);
            synchronize();
        }
    }
    return stmts;
}

// ──────────────────────────────────────────────────────────────────────────
// Statements
// ──────────────────────────────────────────────────────────────────────────
StmtPtr Parser::parseStmt() {
    skipNewlines();
    const Token& tok = current();

    switch (tok.kind) {
        case TokenKind::KW_LET:      return parseLetDecl();
        case TokenKind::KW_SET:      return parseSetStmt();
        case TokenKind::KW_RETURN:   return parseReturnStmt();
        case TokenKind::KW_IF:       return parseIfStmt();
        case TokenKind::KW_MATCH:    return parseMatchStmt();
        case TokenKind::KW_FOR:      return parseForStmt();
        case TokenKind::KW_WHILE:    return parseWhileStmt();
        case TokenKind::KW_LOOP:     return parseLoopStmt();
        case TokenKind::KW_TRY:      return parseTryCatchStmt();
        case TokenKind::KW_FREE:     return parseFreeStmt();
        case TokenKind::KW_THROW:    return parseThrowStmt();
        case TokenKind::KW_BREAK: {
            auto n = std::make_unique<BreakStmt>(); n->loc = tok.loc; advance(); return n;
        }
        case TokenKind::KW_CONTINUE: {
            auto n = std::make_unique<ContinueStmt>(); n->loc = tok.loc; advance(); return n;
        }
        // Nested const/type declarations inside functions
        case TokenKind::KW_CONST: {
            auto cd = parseConstDecl();
            // Wrap ConstDecl in an ExprStmt placeholder – carry it as a stmt
            // by treating it as a LetDecl with a const flag.
            auto ld = std::make_unique<LetDecl>();
            ld->loc  = cd->loc;
            ld->name = cd->name;
            ld->typeAnnotation = cd->typeAnnotation;
            ld->init = std::move(cd->init);
            return ld;
        }
        default: {
            // Expression statement (function call, await, etc.)
            auto e = parseExpr();
            if (!e) {
                reportError("Expected statement", tok.loc);
                advance();
                return nullptr;
            }
            auto es  = std::make_unique<ExprStmt>();
            es->loc  = e->loc;
            es->expr = std::move(e);
            return es;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// let
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<LetDecl> Parser::parseLetDecl() {
    auto node = std::make_unique<LetDecl>();
    node->loc = current().loc;
    expect(TokenKind::KW_LET);
    node->name = expect(TokenKind::IDENTIFIER, "Expected variable name").text;

    // Optional type annotation: ': type'
    if (match(TokenKind::PUNCT_COLON)) {
        node->typeAnnotation = parseTypeExpr();
    }

    // Optional memory region: 'in stack/heap/arena X/zone X'
    if (check(TokenKind::KW_IN)) {
        node->memSpec = parseMemorySpec();
    }

    // Optional initializer: '= expr'
    if (match(TokenKind::OP_ASSIGN)) {
        node->init = parseExpr();
    }
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// set
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<SetStmt> Parser::parseSetStmt() {
    auto node = std::make_unique<SetStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_SET);
    node->target = parseExpr(); // lhs – may be member/index
    expect(TokenKind::OP_ASSIGN, "Expected '=' in set statement");
    node->value = parseExpr();
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// return
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    auto node = std::make_unique<ReturnStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_RETURN);
    // Return value is optional; if the next token starts an expression, parse it
    if (!check(TokenKind::NEWLINE) && !check(TokenKind::KW_END) &&
        !check(TokenKind::KW_ELSE) && !atEnd()) {
        node->value = parseExpr();
    }
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// if
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    auto node = std::make_unique<IfStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_IF);
    node->condition = parseExpr();
    skipNewlines();
    node->thenBlock = parseBlock({TokenKind::KW_ELSE, TokenKind::KW_END});

    if (match(TokenKind::KW_ELSE)) {
        skipNewlines();
        if (check(TokenKind::KW_IF)) {
            node->elseIf = std::unique_ptr<IfStmt>(
                static_cast<IfStmt*>(parseIfStmt().release()));
        } else {
            node->elseBlock = parseBlock({TokenKind::KW_END});
            expect(TokenKind::KW_END, "Expected 'end' to close if block");
        }
    } else {
        expect(TokenKind::KW_END, "Expected 'end' to close if block");
    }
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// match
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<MatchStmt> Parser::parseMatchStmt() {
    auto node = std::make_unique<MatchStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_MATCH);
    node->subject = parseExpr();
    skipNewlines();

    while (!check(TokenKind::KW_END) && !atEnd()) {
        skipNewlines();
        if (check(TokenKind::KW_END)) break;

        MatchCase mc;
        if (match(TokenKind::KW_CASE)) {
            mc.pattern = parseExpr();
        } else if (match(TokenKind::KW_DEFAULT)) {
            mc.pattern = nullptr;
        } else {
            reportError("Expected 'case' or 'default' in match", current().loc);
            advance();
            continue;
        }
        skipNewlines();
        mc.body = parseBlock({TokenKind::KW_CASE, TokenKind::KW_DEFAULT, TokenKind::KW_END});
        node->cases.push_back(std::move(mc));
    }
    expect(TokenKind::KW_END, "Expected 'end' to close match block");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// for
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ForStmt> Parser::parseForStmt() {
    auto node = std::make_unique<ForStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_FOR);
    node->iterVar = expect(TokenKind::IDENTIFIER, "Expected loop variable").text;
    expect(TokenKind::KW_IN, "Expected 'in' after loop variable");
    node->iterable = parseExpr();
    skipNewlines();
    node->body = parseBlock({TokenKind::KW_END});
    expect(TokenKind::KW_END, "Expected 'end' to close for loop");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// while
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    auto node = std::make_unique<WhileStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_WHILE);
    node->condition = parseExpr();
    skipNewlines();
    node->body = parseBlock({TokenKind::KW_END});
    expect(TokenKind::KW_END, "Expected 'end' to close while loop");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// loop
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<LoopStmt> Parser::parseLoopStmt() {
    auto node = std::make_unique<LoopStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_LOOP);
    skipNewlines();
    node->body = parseBlock({TokenKind::KW_END});
    expect(TokenKind::KW_END, "Expected 'end' to close loop");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// try / catch
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<TryCatchStmt> Parser::parseTryCatchStmt() {
    auto node = std::make_unique<TryCatchStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_TRY);
    skipNewlines();
    node->tryBlock = parseBlock({TokenKind::KW_CATCH, TokenKind::KW_END});

    if (match(TokenKind::KW_CATCH)) {
        // Optional: catch (e: ErrorType)
        if (match(TokenKind::PUNCT_LPAREN)) {
            node->catchVar = expect(TokenKind::IDENTIFIER, "Expected catch variable").text;
            if (match(TokenKind::PUNCT_COLON))
                node->catchType = parseTypeExpr();
            expect(TokenKind::PUNCT_RPAREN);
        }
        skipNewlines();
        node->catchBlock = parseBlock({TokenKind::KW_END});
    }
    expect(TokenKind::KW_END, "Expected 'end' to close try block");
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// free
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<FreeStmt> Parser::parseFreeStmt() {
    auto node = std::make_unique<FreeStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_FREE);
    node->ptr = parseExpr();
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// throw
// ──────────────────────────────────────────────────────────────────────────
std::unique_ptr<ThrowStmt> Parser::parseThrowStmt() {
    auto node = std::make_unique<ThrowStmt>();
    node->loc = current().loc;
    expect(TokenKind::KW_THROW);
    if (!check(TokenKind::NEWLINE) && !check(TokenKind::KW_END) && !atEnd()) {
        node->value = parseExpr();
    }
    return node;
}

// ──────────────────────────────────────────────────────────────────────────
// Memory spec: in stack / in heap / in arena <name> / in zone <name>
// ──────────────────────────────────────────────────────────────────────────
MemorySpec Parser::parseMemorySpec() {
    MemorySpec spec;
    expect(TokenKind::KW_IN, "Expected 'in' for memory region");
    switch (current().kind) {
        case TokenKind::KW_STACK:
            advance(); spec.region = MemRegion::Stack; break;
        case TokenKind::KW_HEAP:
            advance(); spec.region = MemRegion::Heap;  break;
        case TokenKind::KW_ARENA:
            advance(); spec.region = MemRegion::Arena;
            spec.label = expect(TokenKind::IDENTIFIER, "Expected arena name").text;
            break;
        case TokenKind::KW_ZONE:
            advance(); spec.region = MemRegion::Zone;
            spec.label = expect(TokenKind::IDENTIFIER, "Expected zone name").text;
            break;
        default:
            reportError("Expected memory region (stack/heap/arena/zone)", current().loc);
    }
    return spec;
}

// ──────────────────────────────────────────────────────────────────────────
// Type expression
// ──────────────────────────────────────────────────────────────────────────
TypeExpr Parser::parseTypeExpr() {
    TypeExpr t;
    t.loc = current().loc;

    // Built-in type keywords that are also keyword tokens
    switch (current().kind) {
        case TokenKind::KW_REF:
            t.isRef = true;
            advance();
            // Optional: ref<T>
            if (match(TokenKind::OP_LT) || match(TokenKind::PUNCT_LANGLE)) {
                t.params.push_back(parseTypeExpr());
                if (!match(TokenKind::OP_GT) && !match(TokenKind::PUNCT_RANGLE))
                    reportError("Expected '>' after generic type parameter", current().loc);
            }
            t.name = "ref";
            return t;
        case TokenKind::KW_PTR:
            t.isPtr = true;
            advance();
            if (match(TokenKind::OP_LT) || match(TokenKind::PUNCT_LANGLE)) {
                t.params.push_back(parseTypeExpr());
                if (!match(TokenKind::OP_GT) && !match(TokenKind::PUNCT_RANGLE))
                    reportError("Expected '>' after generic type parameter", current().loc);
            }
            t.name = "ptr";
            return t;
        default:
            break;
    }

    // Identifier or built-in type name
    if (!check(TokenKind::IDENTIFIER) && !current().isKeyword()) {
        reportError("Expected type name but got " + current().kindName(), current().loc);
        t.name = "unknown";
        return t;
    }
    t.name = current().text;
    advance();

    // Generic parameters: Name<T, U>
    if (check(TokenKind::OP_LT) || check(TokenKind::PUNCT_LANGLE)) {
        // Only consume if this looks like a generic (next token is an identifier or keyword type)
        advance(); // consume '<'
        do {
            t.params.push_back(parseTypeExpr());
        } while (match(TokenKind::PUNCT_COMMA));
        if (!match(TokenKind::OP_GT) && !match(TokenKind::PUNCT_RANGLE))
            reportError("Expected '>' to close generic type", current().loc);
    }
    return t;
}

// ──────────────────────────────────────────────────────────────────────────
// Expressions (Pratt / precedence climbing)
// ──────────────────────────────────────────────────────────────────────────
int Parser::getBinaryPrec(TokenKind k) const {
    switch (k) {
        case TokenKind::KW_OR:       return 1;
        case TokenKind::KW_AND:      return 2;
        case TokenKind::KW_IS:       return 3;
        case TokenKind::OP_EQEQ:
        case TokenKind::OP_NEQ:      return 4;
        case TokenKind::OP_LT:
        case TokenKind::OP_GT:
        case TokenKind::OP_LEQ:
        case TokenKind::OP_GEQ:      return 5;
        case TokenKind::OP_PLUS:
        case TokenKind::OP_MINUS:    return 6;
        case TokenKind::OP_STAR:
        case TokenKind::OP_SLASH:
        case TokenKind::OP_PERCENT:  return 7;
        default:                     return -1;
    }
}

bool Parser::isBinaryOp(TokenKind k) const {
    return getBinaryPrec(k) >= 0;
}

ExprPtr Parser::parseExpr(int minPrec) {
    ExprPtr lhs = parsePrimary();
    if (!lhs) return nullptr;

    while (true) {
        int prec = getBinaryPrec(current().kind);
        if (prec < minPrec) break;

        std::string op  = current().text;
        SourceLocation l = current().loc;
        advance();

        ExprPtr rhs = parseExpr(prec + 1);
        auto bin = std::make_unique<BinaryExpr>();
        bin->loc = l;
        bin->op  = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

ExprPtr Parser::parsePrimary() {
    const Token& tok = current();

    // ── Prefix unary ─────────────────────────────────────────────────────
    if (tok.kind == TokenKind::OP_MINUS || tok.kind == TokenKind::KW_NOT) {
        auto u  = std::make_unique<UnaryExpr>();
        u->loc  = tok.loc;
        u->op   = tok.text;
        advance();
        u->operand = parsePrimary();
        return parsePostfix(std::move(u));
    }

    // ── new ───────────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::KW_NEW) {
        auto n  = std::make_unique<NewExpr>();
        n->loc  = tok.loc;
        advance();
        n->typeExpr = parseTypeExpr();
        if (match(TokenKind::PUNCT_LPAREN)) {
            while (!check(TokenKind::PUNCT_RPAREN) && !atEnd()) {
                n->args.push_back(parseExpr());
                if (!match(TokenKind::PUNCT_COMMA)) break;
            }
            expect(TokenKind::PUNCT_RPAREN);
        }
        return parsePostfix(std::move(n));
    }

    // ── move ─────────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::KW_MOVE) {
        auto m = std::make_unique<MoveExpr>(); m->loc = tok.loc; advance();
        m->source = parsePrimary();
        return parsePostfix(std::move(m));
    }

    // ── copy ─────────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::KW_COPY) {
        auto c = std::make_unique<CopyExpr>(); c->loc = tok.loc; advance();
        c->source = parsePrimary();
        return parsePostfix(std::move(c));
    }

    // ── ref ───────────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::KW_REF) {
        // ref used as an expression (reference creation), not a type annotation
        // Disambiguate: if next is '<', this is a type; otherwise it's a ref expr.
        if (peek(1).kind != TokenKind::OP_LT && peek(1).kind != TokenKind::PUNCT_LANGLE) {
            auto r = std::make_unique<RefExpr>(); r->loc = tok.loc; advance();
            r->source = parsePrimary();
            return parsePostfix(std::move(r));
        }
    }

    // ── await ─────────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::KW_AWAIT) {
        auto a = std::make_unique<AwaitExpr>(); a->loc = tok.loc; advance();
        a->expr = parseExpr();
        return parsePostfix(std::move(a));
    }

    // ── Grouping ─────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::PUNCT_LPAREN) {
        advance();
        auto e = parseExpr();
        expect(TokenKind::PUNCT_RPAREN, "Expected ')' to close parenthesized expression");
        return parsePostfix(std::move(e));
    }

    // ── Literals ─────────────────────────────────────────────────────────
    if (tok.kind == TokenKind::LIT_INT) {
        auto l = std::make_unique<IntLitExpr>(); l->loc = tok.loc;
        l->value = static_cast<int64_t>(std::stoll(tok.text, nullptr, 0));
        advance();
        return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::LIT_FLOAT) {
        auto l = std::make_unique<FloatLitExpr>(); l->loc = tok.loc;
        l->value = std::stod(tok.text);
        advance();
        return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::LIT_STRING) {
        auto l = std::make_unique<StringLitExpr>(); l->loc = tok.loc;
        l->value = tok.text;
        advance();
        return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::LIT_CHAR) {
        auto l = std::make_unique<CharLitExpr>(); l->loc = tok.loc;
        l->value = tok.text.empty() ? '\0' : tok.text[0];
        advance();
        return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::KW_TRUE) {
        auto l = std::make_unique<BoolLitExpr>(); l->loc = tok.loc; l->value = true;
        advance(); return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::KW_FALSE) {
        auto l = std::make_unique<BoolLitExpr>(); l->loc = tok.loc; l->value = false;
        advance(); return parsePostfix(std::move(l));
    }
    if (tok.kind == TokenKind::KW_NULL) {
        auto l = std::make_unique<NullLitExpr>(); l->loc = tok.loc;
        advance(); return parsePostfix(std::move(l));
    }

    // ── Identifier ───────────────────────────────────────────────────────
    if (tok.kind == TokenKind::IDENTIFIER) {
        auto id  = std::make_unique<IdentExpr>(); id->loc = tok.loc;
        id->id   = tok.text;
        advance();
        return parsePostfix(std::move(id));
    }

    // ── Built-in type names used as identifiers (e.g., io.print) ─────────
    if (tok.isKeyword()) {
        // Allow keywords that can also be identifiers in call contexts
        // (e.g., module-level names like 'stack', 'heap', etc. used in expressions)
        auto id  = std::make_unique<IdentExpr>(); id->loc = tok.loc;
        id->id   = tok.text;
        advance();
        return parsePostfix(std::move(id));
    }

    // Unexpected
    reportError("Unexpected token in expression: " + tok.kindName(), tok.loc);
    advance();
    auto err = std::make_unique<NullLitExpr>(); err->loc = tok.loc;
    return err;
}

ExprPtr Parser::parsePostfix(ExprPtr base) {
    while (true) {
        if (check(TokenKind::PUNCT_DOT)) {
            SourceLocation loc = current().loc;
            advance();
            std::string member = current().text;
            if (!check(TokenKind::IDENTIFIER) && !current().isKeyword()) {
                reportError("Expected member name after '.'", current().loc);
            } else {
                advance();
            }
            auto mem = std::make_unique<MemberExpr>();
            mem->loc    = loc;
            mem->object = std::move(base);
            mem->member = member;
            base = std::move(mem);
            continue;
        }

        if (check(TokenKind::PUNCT_LPAREN)) {
            SourceLocation loc = current().loc;
            advance(); // '('
            auto call = std::make_unique<CallExpr>();
            call->loc    = loc;
            call->callee = std::move(base);
            while (!check(TokenKind::PUNCT_RPAREN) && !atEnd()) {
                call->args.push_back(parseExpr());
                if (!match(TokenKind::PUNCT_COMMA)) break;
            }
            expect(TokenKind::PUNCT_RPAREN, "Expected ')' after call arguments");
            base = std::move(call);
            continue;
        }

        if (check(TokenKind::PUNCT_LBRACKET)) {
            SourceLocation loc = current().loc;
            advance(); // '['
            auto idx = std::make_unique<IndexExpr>();
            idx->loc    = loc;
            idx->object = std::move(base);
            idx->index  = parseExpr();
            expect(TokenKind::PUNCT_RBRACKET, "Expected ']' after index");
            base = std::move(idx);
            continue;
        }

        break;
    }
    return base;
}

} // namespace nsl
