#ifndef Qxl_SCANNER_H
#define Qxl_SCANNER_H

#include "quixil.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_TEMPLATE_INTERPOLATION_NESTING 8

    typedef struct
    {
        const char *start;
        const char *current;
        int line;

        // Template String
        int parens[MAX_TEMPLATE_INTERPOLATION_NESTING];
        int num_parens;
    } Scanner;

    typedef enum
    {
        // Single character tokens
        TOKEN_LEFT_PAREN,
        TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE,
        TOKEN_RIGHT_BRACE,
        TOKEN_COMMA,
        TOKEN_DOT,
        TOKEN_MINUS,
        TOKEN_PLUS,
        TOKEN_SEMICOLON,
        TOKEN_SLASH,
        TOKEN_STAR,
        // One or two character tokens
        TOKEN_BANG,
        TOKEN_BANG_EQUAL,
        TOKEN_EQUAL,
        TOKEN_EQUAL_EQUAL,
        TOKEN_GREATER,
        TOKEN_GREATER_EQUAL,
        TOKEN_LESS,
        TOKEN_LESS_EQUAL,
        // Literals
        TOKEN_IDENTIFIER,
        TOKEN_STRING,
        TOKEN_INTEROP,
        TOKEN_NUMBER,
        // Keywords
        TOKEN_AND,
        TOKEN_CLASS,
        TOKEN_ELSE,
        TOKEN_FOR,
        TOKEN_FUNCTION,
        TOKEN_IF,
        TOKEN_NIL,
        TOKEN_OR,
        TOKEN_PRINT,
        TOKEN_RETURN,
        TOKEN_SUPER,
        TOKEN_THIS,
        TOKEN_TRUE,
        TOKEN_FALSE,
        TOKEN_VAR,
        TOKEN_WHILE,
        TOKEN_ERROR,
        TOKEN_EOF
    } TokenType;

    typedef struct
    {
        TokenType type;
        const char *start;
        int length;
        int line;
    } Token;

    Scanner *scanner_init (const char *src);
    Token scanner_scan_token (Scanner *s);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_SCANNER_H */