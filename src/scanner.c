#include "include/scanner.h"

#define is_digit(c) ( c >= '0' &&  c <= '9' )
#define is_alpha(c) ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' )

#define IS_AT_END() *s->current == '\0'
#define PEEK() *s->current
#define ADVANCE() (*s->current++, s->current[-1])
#define PEEK_NEXT() ( (IS_AT_END()) ? '\0' : s->current[1] )
#define MATCH_NEXT_CHAR(e) ((IS_AT_END()) ? false : (*s->current != (e) ? false : (s->current++, true)))


static TokenType 
check_keyword(Scanner *s, int start, int length, const char* rest, TokenType type) 
{
    if (s->current - s->start == start + length &&
        memcmp(s->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType 
get_identifier_type(Scanner *s)
{
    switch (s->start[0]) {
        case 'a':
            return check_keyword(s, 1, 2, "nd", TOKEN_AND);
        case 'c':
            return check_keyword(s, 1, 4, "lass", TOKEN_CLASS);
        case 'e':
            return check_keyword(s, 1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if ((s->current - s->start) > 1) {
                switch (s->start[1]) {
                    case 'a':
                        return check_keyword(s, 2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return check_keyword(s, 2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return check_keyword(s, 2, 6, "nction", TOKEN_FUNCTION);
                }
            }
            break;
        case 'i':
            return check_keyword(s, 1, 1, "f", TOKEN_IF);
        case 'n':
            return check_keyword(s, 1, 2, "il", TOKEN_NIL);
        case 'o':
            return check_keyword(s, 1, 1, "r", TOKEN_OR);
        case 'p':
            return check_keyword(s, 1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return check_keyword(s, 1, 5, "eturn", TOKEN_RETURN);
        case 's':
            return check_keyword(s, 1, 4, "uper", TOKEN_SUPER);
        case 't':
            if ((s->current - s->start) > 1) {
                switch (s->start[1]) {
                    case 'h':
                        return check_keyword(s, 2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return check_keyword(s, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v':
            return check_keyword(s, 1, 2, "ar", TOKEN_VAR);
        case 'w':
            return check_keyword(s, 1, 4, "hile", TOKEN_WHILE);
    }
}

static Token 
make(Scanner* s, TokenType type) 
{
    Token token = {
        .type = type,
        .start = s->start,
        .length = (int)(s->current - s->start),
        .line = s->line,
    };
    return token;
}

static Token 
token_error(Scanner* s, const char* message) 
{
    Token token = {
        .type = TOKEN_ERROR,
        .start = message,
        .length = (int)strlen(message),
        .line = s->line,
    };
    return token;
}

static Token 
token_string(Scanner* s) 
{
    while (PEEK() != '"' && !(IS_AT_END())) {
        if (PEEK() == '\n') {
            s->line++;
        }
        ADVANCE();
    }

    if (IS_AT_END()) {
        return token_error(s, "Unterminated string.");
    }

    // The closing quote
    ADVANCE();
    return make(s, TOKEN_STRING);
} 

static Token 
token_number(Scanner* s) 
{
    while (is_digit(PEEK())) {
        ADVANCE();
    }

    if (PEEK() == '.' && is_digit(PEEK_NEXT())) {
        // Consume the "." for decimal numbers
        ADVANCE();

        while (is_digit(PEEK())) {
            ADVANCE();
        }
    }

    return make(s, TOKEN_NUMBER);
}

static Token 
token_identifier(Scanner* s) 
{
    // Scan the whole token
    while (is_alpha(PEEK()) || is_digit(PEEK())) {
        ADVANCE();
    }

    return make(s, get_identifier_type(s));
}

static void 
skip_whitespace(Scanner* s) {
    for (;;) {
        char c = PEEK();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                ADVANCE();
                break;
            case '\n':
                s->line++;
                ADVANCE();
                break;
            case '/':
                if (PEEK_NEXT() == '/') {
                    // A comment goes until the end of the line
                    while (PEEK() != '\n' && !(IS_AT_END())) {
                        ADVANCE();
                    }
                }
                else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Scanner* 
scanner_init(const char* src) 
{
    Scanner *s = calloc(1, sizeof(Scanner));
    s->current = src;
    s->start = src;
    s->line = 1;

    return s;
}

Token 
scanner_scan_token(Scanner* s) {
    skip_whitespace(s);
    s->start = s->current;

    if (IS_AT_END()) {
        return make(s, TOKEN_EOF);
    }

    char c = ADVANCE();

    if (is_alpha(c)) {
        return token_identifier(s);
    }

    if (is_digit(c)) {
        return token_number(s);
    }

    switch (c) {
        case '(':
            return make(s, TOKEN_LEFT_PAREN);
        case ')':
            return make(s, TOKEN_RIGHT_PAREN);
        case '{':
            return make(s, TOKEN_LEFT_BRACE);
        case '}':
            return make(s, TOKEN_RIGHT_BRACE);
        case ';':
            return make(s, TOKEN_SEMICOLON);
        case ',':
            return make(s, TOKEN_COMMA);
        case '.':
            return make(s, TOKEN_DOT);
        case '-':
            return make(s, TOKEN_MINUS);
        case '+':
            return make(s, TOKEN_PLUS);
        case '/':
            return make(s, TOKEN_SLASH);
        case '*':
            return make(s, TOKEN_STAR);
        case '!':
            return make(s, MATCH_NEXT_CHAR('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make(s, MATCH_NEXT_CHAR('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make(s, MATCH_NEXT_CHAR('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make(s, MATCH_NEXT_CHAR('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return token_string(s);
    }

    return token_error(s, "Unexpected character");
}