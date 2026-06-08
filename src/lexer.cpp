// NSL Lexer Implementation

#include "lexer.hpp"
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Token helpers
// ──────────────────────────────────────────────────────────────────────────
std::string SourceLocation::toString() const {
    return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
}

static const std::unordered_map<std::string, TokenKind> kKeywords = {
    {"program",  TokenKind::KW_PROGRAM},
    {"module",   TokenKind::KW_MODULE},
    {"use",      TokenKind::KW_USE},
    {"from",     TokenKind::KW_FROM},
    {"as",       TokenKind::KW_AS},
    {"export",   TokenKind::KW_EXPORT},
    {"let",      TokenKind::KW_LET},
    {"set",      TokenKind::KW_SET},
    {"const",    TokenKind::KW_CONST},
    {"type",     TokenKind::KW_TYPE},
    {"fn",       TokenKind::KW_FN},
    {"return",   TokenKind::KW_RETURN},
    {"if",       TokenKind::KW_IF},
    {"else",     TokenKind::KW_ELSE},
    {"match",    TokenKind::KW_MATCH},
    {"case",     TokenKind::KW_CASE},
    {"default",  TokenKind::KW_DEFAULT},
    {"for",      TokenKind::KW_FOR},
    {"while",    TokenKind::KW_WHILE},
    {"loop",     TokenKind::KW_LOOP},
    {"in",       TokenKind::KW_IN},
    {"break",    TokenKind::KW_BREAK},
    {"continue", TokenKind::KW_CONTINUE},
    {"true",     TokenKind::KW_TRUE},
    {"false",    TokenKind::KW_FALSE},
    {"null",     TokenKind::KW_NULL},
    {"and",      TokenKind::KW_AND},
    {"or",       TokenKind::KW_OR},
    {"not",      TokenKind::KW_NOT},
    {"is",       TokenKind::KW_IS},
    {"new",      TokenKind::KW_NEW},
    {"free",     TokenKind::KW_FREE},
    {"move",     TokenKind::KW_MOVE},
    {"copy",     TokenKind::KW_COPY},
    {"ref",      TokenKind::KW_REF},
    {"ptr",      TokenKind::KW_PTR},
    {"stack",    TokenKind::KW_STACK},
    {"heap",     TokenKind::KW_HEAP},
    {"arena",    TokenKind::KW_ARENA},
    {"zone",     TokenKind::KW_ZONE},
    {"public",   TokenKind::KW_PUBLIC},
    {"private",  TokenKind::KW_PRIVATE},
    {"static",   TokenKind::KW_STATIC},
    {"virtual",  TokenKind::KW_VIRTUAL},
    {"try",      TokenKind::KW_TRY},
    {"catch",    TokenKind::KW_CATCH},
    {"throw",    TokenKind::KW_THROW},
    {"async",    TokenKind::KW_ASYNC},
    {"await",    TokenKind::KW_AWAIT},
    {"end",      TokenKind::KW_END},
    {"context",  TokenKind::KW_CONTEXT},
    {"macro",    TokenKind::KW_MACRO},
};

TokenKind keywordKind(const std::string& word) {
    auto it = kKeywords.find(word);
    return it != kKeywords.end() ? it->second : TokenKind::IDENTIFIER;
}

bool Token::isKeyword() const {
    return kind >= TokenKind::KW_PROGRAM && kind <= TokenKind::KW_MACRO;
}

bool Token::isLiteral() const {
    return kind >= TokenKind::LIT_INT && kind <= TokenKind::LIT_CHAR;
}

bool Token::isOperator() const {
    return kind >= TokenKind::OP_PLUS && kind <= TokenKind::OP_ARROW;
}

bool Token::isBinaryOperator() const {
    switch (kind) {
        case TokenKind::OP_PLUS:    case TokenKind::OP_MINUS:
        case TokenKind::OP_STAR:    case TokenKind::OP_SLASH:
        case TokenKind::OP_PERCENT: case TokenKind::OP_EQEQ:
        case TokenKind::OP_NEQ:     case TokenKind::OP_LT:
        case TokenKind::OP_GT:      case TokenKind::OP_LEQ:
        case TokenKind::OP_GEQ:     case TokenKind::KW_AND:
        case TokenKind::KW_OR:      case TokenKind::KW_IS:
            return true;
        default:
            return false;
    }
}

std::string Token::kindName() const {
    switch (kind) {
        case TokenKind::KW_PROGRAM:   return "'program'";
        case TokenKind::KW_MODULE:    return "'module'";
        case TokenKind::KW_USE:       return "'use'";
        case TokenKind::KW_FROM:      return "'from'";
        case TokenKind::KW_AS:        return "'as'";
        case TokenKind::KW_EXPORT:    return "'export'";
        case TokenKind::KW_LET:       return "'let'";
        case TokenKind::KW_SET:       return "'set'";
        case TokenKind::KW_CONST:     return "'const'";
        case TokenKind::KW_TYPE:      return "'type'";
        case TokenKind::KW_FN:        return "'fn'";
        case TokenKind::KW_RETURN:    return "'return'";
        case TokenKind::KW_IF:        return "'if'";
        case TokenKind::KW_ELSE:      return "'else'";
        case TokenKind::KW_MATCH:     return "'match'";
        case TokenKind::KW_CASE:      return "'case'";
        case TokenKind::KW_DEFAULT:   return "'default'";
        case TokenKind::KW_FOR:       return "'for'";
        case TokenKind::KW_WHILE:     return "'while'";
        case TokenKind::KW_LOOP:      return "'loop'";
        case TokenKind::KW_IN:        return "'in'";
        case TokenKind::KW_BREAK:     return "'break'";
        case TokenKind::KW_CONTINUE:  return "'continue'";
        case TokenKind::KW_TRUE:      return "'true'";
        case TokenKind::KW_FALSE:     return "'false'";
        case TokenKind::KW_NULL:      return "'null'";
        case TokenKind::KW_AND:       return "'and'";
        case TokenKind::KW_OR:        return "'or'";
        case TokenKind::KW_NOT:       return "'not'";
        case TokenKind::KW_IS:        return "'is'";
        case TokenKind::KW_NEW:       return "'new'";
        case TokenKind::KW_FREE:      return "'free'";
        case TokenKind::KW_MOVE:      return "'move'";
        case TokenKind::KW_COPY:      return "'copy'";
        case TokenKind::KW_REF:       return "'ref'";
        case TokenKind::KW_PTR:       return "'ptr'";
        case TokenKind::KW_STACK:     return "'stack'";
        case TokenKind::KW_HEAP:      return "'heap'";
        case TokenKind::KW_ARENA:     return "'arena'";
        case TokenKind::KW_ZONE:      return "'zone'";
        case TokenKind::KW_PUBLIC:    return "'public'";
        case TokenKind::KW_PRIVATE:   return "'private'";
        case TokenKind::KW_STATIC:    return "'static'";
        case TokenKind::KW_VIRTUAL:   return "'virtual'";
        case TokenKind::KW_TRY:       return "'try'";
        case TokenKind::KW_CATCH:     return "'catch'";
        case TokenKind::KW_THROW:     return "'throw'";
        case TokenKind::KW_ASYNC:     return "'async'";
        case TokenKind::KW_AWAIT:     return "'await'";
        case TokenKind::KW_END:       return "'end'";
        case TokenKind::KW_CONTEXT:   return "'context'";
        case TokenKind::KW_MACRO:     return "'macro'";
        case TokenKind::LIT_INT:      return "integer literal";
        case TokenKind::LIT_FLOAT:    return "float literal";
        case TokenKind::LIT_STRING:   return "string literal";
        case TokenKind::LIT_CHAR:     return "char literal";
        case TokenKind::IDENTIFIER:   return "identifier";
        case TokenKind::OP_PLUS:      return "'+'";
        case TokenKind::OP_MINUS:     return "'-'";
        case TokenKind::OP_STAR:      return "'*'";
        case TokenKind::OP_SLASH:     return "'/'";
        case TokenKind::OP_PERCENT:   return "'%'";
        case TokenKind::OP_EQEQ:      return "'=='";
        case TokenKind::OP_NEQ:       return "'!='";
        case TokenKind::OP_LT:        return "'<'";
        case TokenKind::OP_GT:        return "'>'";
        case TokenKind::OP_LEQ:       return "'<='";
        case TokenKind::OP_GEQ:       return "'>='";
        case TokenKind::OP_ASSIGN:    return "'='";
        case TokenKind::OP_ARROW:     return "'->'";
        case TokenKind::PUNCT_DOT:    return "'.'";
        case TokenKind::PUNCT_COLON:  return "':'";
        case TokenKind::PUNCT_COMMA:  return "','";
        case TokenKind::PUNCT_LPAREN: return "'('";
        case TokenKind::PUNCT_RPAREN: return "')'";
        case TokenKind::PUNCT_LBRACKET: return "'['";
        case TokenKind::PUNCT_RBRACKET: return "']'";
        case TokenKind::PUNCT_LANGLE: return "'<'";
        case TokenKind::PUNCT_RANGLE: return "'>'";
        case TokenKind::PUNCT_AT:     return "'@'";
        case TokenKind::PUNCT_HASH:   return "'#'";
        case TokenKind::PUNCT_TILDE:  return "'~'";
        case TokenKind::NEWLINE:      return "newline";
        case TokenKind::END_OF_FILE:  return "end-of-file";
        default:                      return "unknown";
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Lexer Implementation
// ──────────────────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source, std::string filename, ErrorCallback onError)
    : m_source(std::move(source))
    , m_filename(std::move(filename))
    , m_onError(std::move(onError))
{}

SourceLocation Lexer::currentLoc() const {
    return {m_filename, m_line, m_col};
}

Token Lexer::makeToken(TokenKind kind, const std::string& text) const {
    return Token{kind, text, currentLoc()};
}

char Lexer::peek(int offset) const {
    std::size_t idx = m_pos + static_cast<std::size_t>(offset);
    if (idx >= m_source.size()) return '\0';
    return m_source[idx];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') { ++m_line; m_col = 1; }
    else            { ++m_col; }
    return c;
}

bool Lexer::match(char c) {
    if (atEnd() || m_source[m_pos] != c) return false;
    advance();
    return true;
}

bool Lexer::atEnd() const {
    return m_pos >= m_source.size();
}

void Lexer::reportError(const std::string& msg) {
    if (m_onError) m_onError(msg, currentLoc());
}

void Lexer::skipLineComment() {
    while (!atEnd() && peek() != '\n') advance();
}

void Lexer::skipBlockComment() {
    // Already consumed '/*'
    while (!atEnd()) {
        if (peek() == '*' && peek(1) == '/') {
            advance(); advance();
            return;
        }
        advance();
    }
    reportError("Unterminated block comment");
}

Token Lexer::readString() {
    SourceLocation loc = currentLoc();
    // Already consumed opening '"'
    std::string value;
    while (!atEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\') {
            if (atEnd()) { reportError("Unterminated string escape"); break; }
            char esc = advance();
            switch (esc) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                case '0':  value += '\0'; break;
                default:
                    value += '\\';
                    value += esc;
                    break;
            }
        } else if (c == '\n') {
            reportError("Unterminated string literal");
            break;
        } else {
            value += c;
        }
    }
    if (atEnd()) {
        reportError("Unterminated string literal");
    } else {
        advance(); // consume closing '"'
    }
    return Token{TokenKind::LIT_STRING, value, loc};
}

Token Lexer::readChar() {
    SourceLocation loc = currentLoc();
    // Already consumed opening '\''
    std::string value;
    if (!atEnd() && peek() != '\'') {
        char c = advance();
        if (c == '\\' && !atEnd()) {
            char esc = advance();
            switch (esc) {
                case 'n':  value = "\n"; break;
                case 't':  value = "\t"; break;
                case 'r':  value = "\r"; break;
                case '\'': value = "'";  break;
                case '\\': value = "\\"; break;
                case '0':  value = "\0"; break;
                default:   value += '\\'; value += esc; break;
            }
        } else {
            value += c;
        }
    }
    if (!match('\'')) reportError("Unterminated char literal");
    return Token{TokenKind::LIT_CHAR, value, loc};
}

Token Lexer::readNumber() {
    SourceLocation loc = currentLoc();
    std::string text;
    bool isFloat = false;

    // Hex, binary, or octal prefix
    if (peek() == '0') {
        text += advance();
        if (!atEnd() && (peek() == 'x' || peek() == 'X')) {
            text += advance();
            while (!atEnd() && std::isxdigit(static_cast<unsigned char>(peek())))
                text += advance();
            return Token{TokenKind::LIT_INT, text, loc};
        }
        if (!atEnd() && (peek() == 'b' || peek() == 'B')) {
            text += advance();
            while (!atEnd() && (peek() == '0' || peek() == '1'))
                text += advance();
            return Token{TokenKind::LIT_INT, text, loc};
        }
    }

    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
        text += advance();

    if (!atEnd() && peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        text += advance(); // '.'
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
            text += advance();
    }

    if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
        isFloat = true;
        text += advance();
        if (!atEnd() && (peek() == '+' || peek() == '-')) text += advance();
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
            text += advance();
    }

    return Token{isFloat ? TokenKind::LIT_FLOAT : TokenKind::LIT_INT, text, loc};
}

Token Lexer::readIdentOrKeyword() {
    SourceLocation loc = currentLoc();
    std::string text;
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        text += advance();
    TokenKind kind = keywordKind(text);
    return Token{kind, text, loc};
}

Token Lexer::readOperatorOrPunct() {
    SourceLocation loc = currentLoc();
    char c = advance();
    switch (c) {
        case '+': return Token{TokenKind::OP_PLUS,      "+", loc};
        case '-':
            if (match('>')) return Token{TokenKind::OP_ARROW,   "->", loc};
            return Token{TokenKind::OP_MINUS, "-", loc};
        case '*': return Token{TokenKind::OP_STAR,      "*", loc};
        case '/':
            if (match('/')) { skipLineComment();  return Token{TokenKind::NEWLINE, "\n", loc}; }
            if (match('*')) { skipBlockComment(); return Token{TokenKind::UNKNOWN, "",   loc}; } // will be filtered
            return Token{TokenKind::OP_SLASH,     "/", loc};
        case '%': return Token{TokenKind::OP_PERCENT,   "%", loc};
        case '=':
            if (match('=')) return Token{TokenKind::OP_EQEQ,   "==", loc};
            return Token{TokenKind::OP_ASSIGN,    "=", loc};
        case '!':
            if (match('=')) return Token{TokenKind::OP_NEQ,    "!=", loc};
            reportError(std::string("Unexpected character '!'; did you mean '!='?"));
            return Token{TokenKind::UNKNOWN, "!", loc};
        case '<':
            if (match('=')) return Token{TokenKind::OP_LEQ,    "<=", loc};
            return Token{TokenKind::OP_LT,        "<", loc};
        case '>':
            if (match('=')) return Token{TokenKind::OP_GEQ,    ">=", loc};
            return Token{TokenKind::OP_GT,        ">", loc};
        case '.': return Token{TokenKind::PUNCT_DOT,    ".", loc};
        case ':': return Token{TokenKind::PUNCT_COLON,  ":", loc};
        case ',': return Token{TokenKind::PUNCT_COMMA,  ",", loc};
        case '(': return Token{TokenKind::PUNCT_LPAREN, "(", loc};
        case ')': return Token{TokenKind::PUNCT_RPAREN, ")", loc};
        case '[': return Token{TokenKind::PUNCT_LBRACKET,"[", loc};
        case ']': return Token{TokenKind::PUNCT_RBRACKET,"]", loc};
        case '@': return Token{TokenKind::PUNCT_AT,     "@", loc};
        case '#': return Token{TokenKind::PUNCT_HASH,   "#", loc};
        case '~': return Token{TokenKind::PUNCT_TILDE,  "~", loc};
        default:
            reportError(std::string("Unexpected character '") + c + "'");
            return Token{TokenKind::UNKNOWN, std::string(1, c), loc};
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!atEnd()) {
        // Skip spaces and tabs (but not newlines)
        while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r'))
            advance();

        if (atEnd()) break;

        char c = peek();

        if (c == '\n') {
            // Emit NEWLINE only if the previous meaningful token wasn't already
            // a NEWLINE (collapse blank lines).
            advance();
            if (!tokens.empty() && tokens.back().kind != TokenKind::NEWLINE)
                tokens.push_back(Token{TokenKind::NEWLINE, "\n", currentLoc()});
            continue;
        }

        if (c == '"') {
            advance(); // consume '"'
            tokens.push_back(readString());
            continue;
        }

        if (c == '\'') {
            advance(); // consume '\''
            tokens.push_back(readChar());
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readNumber());
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(readIdentOrKeyword());
            continue;
        }

        // Handle // comment specially so we don't emit an extra NEWLINE
        if (c == '/' && peek(1) == '/') {
            advance(); advance(); // consume '//'
            skipLineComment();
            // The newline that terminates the comment will be consumed in
            // the next iteration; do not emit NEWLINE here.
            continue;
        }

        if (c == '/' && peek(1) == '*') {
            advance(); advance(); // consume '/*'
            skipBlockComment();
            continue;
        }

        Token tok = readOperatorOrPunct();
        if (tok.kind != TokenKind::UNKNOWN)
            tokens.push_back(tok);
    }

    // Remove trailing NEWLINE before EOF
    while (!tokens.empty() && tokens.back().kind == TokenKind::NEWLINE)
        tokens.pop_back();

    tokens.push_back(Token{TokenKind::END_OF_FILE, "", {m_filename, m_line, m_col}});
    return tokens;
}

} // namespace nsl
