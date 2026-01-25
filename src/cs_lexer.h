#ifndef CS_LEXER_H
#define CS_LEXER_H

#include <stddef.h>

typedef enum {
    TK_EOF = 0,
    TK_ERR,

    TK_IDENT,
    TK_INT,
    TK_FLOAT,
    TK_STR,

    // keywords
    TK_LET,
    TK_FN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_RETURN,
    TK_BREAK,
    TK_CONTINUE,
    TK_FOR,
    TK_IN,
    TK_THROW,
    TK_TRY,
    TK_CATCH,
    TK_TRUE,
    TK_FALSE,
    TK_NIL,

    // punctuation
    TK_LPAREN, TK_RPAREN,
    TK_LBRACKET, TK_RBRACKET,
    TK_LBRACE, TK_RBRACE,
    TK_COMMA,
    TK_SEMI,
    TK_DOT,
    TK_COLON,
    TK_QMARK,

    // operators
    TK_ASSIGN,     // =
    TK_PLUSEQ, TK_MINUSEQ, TK_STAREQ, TK_SLASHEQ,
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
    TK_BANG,
    TK_EQ, TK_NE,
    TK_LT, TK_LE,
    TK_GT, TK_GE,
    TK_ANDAND, TK_OROR,
    TK_RANGE,      // ..
    TK_RANGE_INC   // ..=
} token_type;

typedef struct {
    token_type type;
    const char* start;
    size_t len;
    long long int_val;
    double float_val;
    int line;
    int col;
} token;

typedef struct {
    const char* src;
    size_t pos;
    int line;
    int col;
    token current;
} lexer;

void  lex_init(lexer* L, const char* src);
token lex_next(lexer* L);

#endif
