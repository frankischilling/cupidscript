#include "cs_lexer.h"
#include <ctype.h>
#include <string.h>

static char peek(lexer* L) { return L->src[L->pos]; }
static char peek2(lexer* L) { return L->src[L->pos + 1]; }

static char advance(lexer* L) {
    char c = L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; }
    else L->col++;
    return c;
}

static void skip_ws_and_comments(lexer* L) {
    for (;;) {
        char c = peek(L);
        // whitespace
        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(L);
            c = peek(L);
        }
        // // comment
        if (c == '/' && peek2(L) == '/') {
            while (peek(L) && peek(L) != '\n') advance(L);
            continue;
        }
        // /* comment */
        if (c == '/' && peek2(L) == '*') {
            advance(L); advance(L);
            while (peek(L)) {
                if (peek(L) == '*' && peek2(L) == '/') { advance(L); advance(L); break; }
                advance(L);
            }
            continue;
        }
        break;
    }
}

static token make_tok(lexer* L, token_type t, const char* start, size_t len, long long iv, int line, int col) {
    (void)L;
    token tok;
    tok.type = t;
    tok.start = start;
    tok.len = len;
    tok.int_val = iv;
    tok.line = line;
    tok.col = col;
    return tok;
}

static int is_ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int is_ident_char(char c)  { return isalnum((unsigned char)c) || c == '_'; }

static token_type keyword_type(const char* s, size_t n) {
    if (n == 3 && memcmp(s, "let", 3) == 0) return TK_LET;
    if (n == 2 && memcmp(s, "fn", 2) == 0) return TK_FN;
    if (n == 2 && memcmp(s, "if", 2) == 0) return TK_IF;
    if (n == 4 && memcmp(s, "else", 4) == 0) return TK_ELSE;
    if (n == 5 && memcmp(s, "while", 5) == 0) return TK_WHILE;
    if (n == 3 && memcmp(s, "for", 3) == 0) return TK_FOR;
    if (n == 2 && memcmp(s, "in", 2) == 0) return TK_IN;
    if (n == 6 && memcmp(s, "return", 6) == 0) return TK_RETURN;
    if (n == 5 && memcmp(s, "break", 5) == 0) return TK_BREAK;
    if (n == 8 && memcmp(s, "continue", 8) == 0) return TK_CONTINUE;
    if (n == 5 && memcmp(s, "throw", 5) == 0) return TK_THROW;
    if (n == 3 && memcmp(s, "try", 3) == 0) return TK_TRY;
    if (n == 5 && memcmp(s, "catch", 5) == 0) return TK_CATCH;
    if (n == 4 && memcmp(s, "true", 4) == 0) return TK_TRUE;
    if (n == 5 && memcmp(s, "false", 5) == 0) return TK_FALSE;
    if (n == 3 && memcmp(s, "nil", 3) == 0) return TK_NIL;
    return TK_IDENT;
}

void lex_init(lexer* L, const char* src) {
    L->src = src ? src : "";
    L->pos = 0;
    L->line = 1;
    L->col = 1;
    L->current.type = TK_EOF;
}

token lex_next(lexer* L) {
    skip_ws_and_comments(L);
    const char* start = &L->src[L->pos];
    int line = L->line;
    int col  = L->col;

    char c = peek(L);
    if (!c) return make_tok(L, TK_EOF, start, 0, 0, line, col);

    // numbers (decimal with underscores, hex 0xFF, or floats 3.14)
    if (isdigit((unsigned char)c)) {
        long long v = 0;
        double fv = 0.0;
        int is_hex = 0;
        int is_float = 0;

        if (c == '0' && (peek2(L) == 'x' || peek2(L) == 'X')) {
            is_hex = 1;
            advance(L); // 0
            advance(L); // x
            while (1) {
                char h = peek(L);
                if (h == '_') { advance(L); continue; }
                if (isdigit((unsigned char)h)) { v = (v * 16) + (advance(L) - '0'); continue; }
                if (h >= 'a' && h <= 'f') { v = (v * 16) + (advance(L) - 'a' + 10); continue; }
                if (h >= 'A' && h <= 'F') { v = (v * 16) + (advance(L) - 'A' + 10); continue; }
                break;
            }
        } else {
            // Parse integer part
            while (1) {
                char d = peek(L);
                if (d == '_') { advance(L); continue; }
                if (!isdigit((unsigned char)d)) break;
                v = (v * 10) + (advance(L) - '0');
            }
            
            // Check for decimal point
            if (peek(L) == '.' && isdigit((unsigned char)peek2(L))) {
                is_float = 1;
                fv = (double)v;
                advance(L); // consume '.'
                
                double frac = 0.0;
                double divisor = 10.0;
                while (1) {
                    char d = peek(L);
                    if (d == '_') { advance(L); continue; }
                    if (!isdigit((unsigned char)d)) break;
                    frac += (advance(L) - '0') / divisor;
                    divisor *= 10.0;
                }
                fv += frac;
            }
            
            // Check for exponent (works with or without decimal point)
            if (peek(L) == 'e' || peek(L) == 'E') {
                if (!is_float) {
                    is_float = 1;
                    fv = (double)v;
                }
                advance(L);
                int exp_sign = 1;
                if (peek(L) == '+') advance(L);
                else if (peek(L) == '-') { exp_sign = -1; advance(L); }
                
                int exp = 0;
                while (isdigit((unsigned char)peek(L))) {
                    exp = exp * 10 + (advance(L) - '0');
                }
                
                double multiplier = 1.0;
                for (int i = 0; i < exp; i++) {
                    multiplier *= 10.0;
                }
                if (exp_sign < 0) fv /= multiplier;
                else fv *= multiplier;
            }
        }

        (void)is_hex;
        size_t len = (size_t)(&L->src[L->pos] - start);
        if (is_float) {
            token tok = make_tok(L, TK_FLOAT, start, len, 0, line, col);
            tok.float_val = fv;
            return tok;
        }
        return make_tok(L, TK_INT, start, len, v, line, col);
    }

    // identifiers / keywords
    if (is_ident_start(c)) {
        advance(L);
        while (is_ident_char(peek(L))) advance(L);
        size_t len = (size_t)(&L->src[L->pos] - start);
        token_type kt = keyword_type(start, len);
        return make_tok(L, kt, start, len, 0, line, col);
    }

    // strings
    if (c == '"') {
        advance(L); // opening
        while (peek(L) && peek(L) != '"') {
            if (peek(L) == '\\' && peek2(L)) { advance(L); advance(L); continue; }
            advance(L);
        }
        if (peek(L) != '"') {
            return make_tok(L, TK_ERR, start, (size_t)(&L->src[L->pos] - start), 0, line, col);
        }
        advance(L); // closing
        size_t len = (size_t)(&L->src[L->pos] - start);
        return make_tok(L, TK_STR, start, len, 0, line, col);
    }

    // operators / punctuation
    advance(L);
    token_type t = TK_ERR;
    switch (c) {
        case '(': t = TK_LPAREN; break;
        case ')': t = TK_RPAREN; break;
        case '[': t = TK_LBRACKET; break;
        case ']': t = TK_RBRACKET; break;
        case '{': t = TK_LBRACE; break;
        case '}': t = TK_RBRACE; break;
        case ',': t = TK_COMMA; break;
        case ';': t = TK_SEMI; break;
        case '.':
            // Check for .. or ..=
            if (peek(L) == '.') {
                advance(L);
                if (peek(L) == '=') {
                    advance(L);
                    t = TK_RANGE_INC;
                } else {
                    t = TK_RANGE;
                }
            } else {
                t = TK_DOT;
            }
            break;
        case ':': t = TK_COLON; break;
        case '?': t = TK_QMARK; break;

        case '+':
            if (peek(L) == '=') { advance(L); t = TK_PLUSEQ; }
            else t = TK_PLUS;
            break;
        case '-':
            if (peek(L) == '=') { advance(L); t = TK_MINUSEQ; }
            else t = TK_MINUS;
            break;
        case '*':
            if (peek(L) == '=') { advance(L); t = TK_STAREQ; }
            else t = TK_STAR;
            break;
        case '/':
            if (peek(L) == '=') { advance(L); t = TK_SLASHEQ; }
            else t = TK_SLASH;
            break;
        case '%': t = TK_PERCENT; break;

        case '!':
            if (peek(L) == '=') { advance(L); t = TK_NE; }
            else t = TK_BANG;
            break;

        case '=':
            if (peek(L) == '=') { advance(L); t = TK_EQ; }
            else t = TK_ASSIGN;
            break;

        case '<':
            if (peek(L) == '=') { advance(L); t = TK_LE; }
            else t = TK_LT;
            break;

        case '>':
            if (peek(L) == '=') { advance(L); t = TK_GE; }
            else t = TK_GT;
            break;

        case '&':
            if (peek(L) == '&') { advance(L); t = TK_ANDAND; }
            break;

        case '|':
            if (peek(L) == '|') { advance(L); t = TK_OROR; }
            break;
        default:
            t = TK_ERR;
            break;
    }

    return make_tok(L, t, start, (size_t)(&L->src[L->pos] - start), 0, line, col);
}
