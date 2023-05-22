#include "include/compiler.h"
#include "include/debug.h"

#define PARSER_ERROR(m) error_at(p, &p->prev, (m))
#define PARSER_ERROR_AT_CUR(m) error_at(p, &p->cur, (m))
#define EMIT_BYTE(byte) QxlChunk_add(p->compiling_chunk, (byte), p->prev.line)
#define EMIT_RETURN() EMIT_BYTE(OP_RETURN)
#define EMIT_BYTES(byte_1, byte_2) (EMIT_BYTE(byte_1), EMIT_BYTE(byte_2))
#define EMIT_CONST(val) EMIT_BYTES(OP_CONSTANT, make_constant(p, val))
#define CHECK_TYPE(_type) (p->cur.type == (_type))
#define MATCH_TOKEN(_type) (CHECK_TYPE(_type) ? (advance(p), true) : false)
#define CONST_IDENTIFIER(token) (make_constant(p, OBJECT_VAL(QxlObjectString_copy(p->vm_global_strings, token.start, token.length))))
#define PARSER_VARIABLE(e) ({ consume(p, TOKEN_IDENTIFIER, (e)); CONST_IDENTIFIER(p->prev); })
#define NAMED_VARIABLE() (EMIT_BYTES(OP_GET_GLOBAL, (uint8_t)CONST_IDENTIFIER(p->prev)))

static void unary(Parser *p, bool can_assign);
static void grouping(Parser *p, bool can_assign);
static void binary(Parser *p, bool can_assign);
static void number(Parser *p, bool can_assign);
static void template(Parser *p, bool can_assign);
static void string(Parser *p, bool can_assign);
static void literal(Parser *p, bool can_assign);
static void variable(Parser *p, bool can_assign);
static void statement(Parser *p);
static void declaration(Parser *p);

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_INTEROP]       = {template, NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUNCTION]      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule* 
get_rule(TokenType type) 
{
    return &rules[type];
}

static void 
error_at(Parser *p, Token* token, const char* msg) {
    if (p->panic_mode) {
        return;
    }

    p->panic_mode = true;
    Qxl_ERROR("[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        Qxl_ERROR(" at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        Qxl_ERROR(" at '%.*s'", token->length, token->start);
    }

    Qxl_ERROR(": %s\n", msg);
    p->had_error = true;
}

static void
advance(Parser *p) 
{
    p->prev = p->cur;

    for (;;) {
        p->cur = scanner_scan_token(p->s);
        if (p->cur.type != TOKEN_ERROR) break;
        PARSER_ERROR_AT_CUR(p->cur.start);
    }
}

static void 
consume(Parser *p, TokenType type, const char *msg) 
{
    if (p->cur.type == type) {
        advance(p);
        return;
    }

    PARSER_ERROR_AT_CUR(msg);
}

static void 
parse_precedence(Parser *p, Precedence prec) {
    advance(p);
    ParseFn prefix_rule = get_rule(p->prev.type)->prefix;
    if (prefix_rule == NULL) {
        PARSER_ERROR("Expected expression.");
        return;
    }

    bool can_assign = prec <= PREC_CONDITIONAL;
    prefix_rule(p, can_assign);

    while (prec <= get_rule(p->cur.type)->precedence) {
        advance(p);
        ParseFn infix_rule = get_rule(p->prev.type)->infix;
        infix_rule(p, can_assign);
    }

    if (can_assign && MATCH_TOKEN(TOKEN_EQUAL)) {
        PARSER_ERROR("Invalid assignment target.");
    }
}

static void 
expression(Parser *p)
{
    parse_precedence(p, PREC_LOWEST);
}

static uint8_t 
make_constant(Parser *p, QxlValue value)
{
    int constant = QxlChunk_add_constant(p->compiling_chunk, value);
    if (constant > UINT8_MAX) {
        PARSER_ERROR("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void
number(Parser *p, bool can_assign)
{
    double value = strtod(p->prev.start, NULL);
    EMIT_CONST(NUMBER_VAL(value));
}

static void 
template(Parser *p, bool can_assign)
{
    QxlObjectString *s = QxlObjectString_copy(p->vm_global_strings, "", 0);
    EMIT_CONST(OBJECT_VAL(s));
    do {
        QxlObjectString *s = QxlObjectString_copy(p->vm_global_strings, p->prev.start + 1, p->prev.length - 3);
        EMIT_CONST(OBJECT_VAL(s));
        EMIT_BYTE(OP_ADD);
        expression(p);
        EMIT_BYTE(OP_ADD);

    } while (MATCH_TOKEN(TOKEN_INTEROP));
    consume(p, TOKEN_STRING, "Expected end of template string");
    string(p, false);
    EMIT_BYTE(OP_ADD);
}

static void 
string(Parser *p, bool can_assign)
{
    QxlObjectString *s = QxlObjectString_copy(p->vm_global_strings, p->prev.start + 1, p->prev.length - 2);
    EMIT_CONST(OBJECT_VAL(s));
}

static void 
grouping(Parser *p, bool can_assign) 
{
    expression(p);
    consume(p, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void 
unary(Parser *p, bool can_assign) 
{
    TokenType op_type = p->prev.type;

    // Compile the operand.
    parse_precedence(p, PREC_UNARY);

    // Emit the operator instruction.
    switch (op_type) {
        case TOKEN_BANG:    EMIT_BYTE(OP_NOT); break;
        case TOKEN_MINUS:   EMIT_BYTE(OP_NEGATE); break;
        default: return; // Unreachable
    }
}

static void 
binary(Parser *p, bool can_assign) 
{
    TokenType op_type = p->prev.type;
    ParseRule* rule = get_rule(op_type);
    parse_precedence(p, (Precedence)(rule->precedence + 1));

    switch (op_type) {
        case TOKEN_BANG_EQUAL:      EMIT_BYTES(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:     EMIT_BYTE(OP_EQUAL); break;
        case TOKEN_GREATER:         EMIT_BYTE(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   EMIT_BYTES(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            EMIT_BYTE(OP_LESS); break;
        case TOKEN_LESS_EQUAL:      EMIT_BYTES(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:            EMIT_BYTE(OP_ADD); break;
        case TOKEN_MINUS:           EMIT_BYTE(OP_SUBTRACT); break;
        case TOKEN_STAR:            EMIT_BYTE(OP_MULTIPLY); break;
        case TOKEN_SLASH:           EMIT_BYTE(OP_DIVIDE); break;
        default: return; // Unreachable
    }
}

static void 
literal(Parser *p, bool can_assign) 
{
    switch (p->prev.type) {
        case TOKEN_FALSE:   EMIT_BYTE(OP_FALSE); break;
        case TOKEN_NIL:     EMIT_BYTE(OP_NIL); break;
        case TOKEN_TRUE:    EMIT_BYTE(OP_TRUE); break;
        default: return; // Unreachable
    }
}

static void
named_variable(Parser *p, bool can_assign) {
    uint8_t arg = CONST_IDENTIFIER(p->prev);

    bool does_match = MATCH_TOKEN(TOKEN_EQUAL);

    if (can_assign && does_match ) {
        expression(p);
        EMIT_BYTES(OP_SET_GLOBAL, arg);
    } else {
        EMIT_BYTES(OP_GET_GLOBAL, arg);
    }
}

static void
variable(Parser *p, bool can_assign)
{
    named_variable(p, can_assign);
}

static void 
synchronize(Parser *p) 
{
    p->panic_mode = false;

    while (p->cur.type != TOKEN_EOF) {
        if (p->prev.type == TOKEN_SEMICOLON) return;
        
        switch (p->cur.type) {
            case TOKEN_CLASS:
            case TOKEN_FUNCTION:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
        }

        advance(p);
    }
}

// Declerations
static void
dec_var(Parser *p)
{
    uint8_t global = PARSER_VARIABLE("Expected variable name.");

    if (MATCH_TOKEN(TOKEN_EQUAL)) {
        expression(p);
    } else {
        EMIT_BYTE(OP_NIL);
    }
    consume(p, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    EMIT_BYTES(OP_DEFINE_GLOBAL, global);
}

// Statements
static void 
stmt_print(Parser *p) 
{
    expression(p);
    consume(p, TOKEN_SEMICOLON, "Expect ';' after value.");
    EMIT_BYTE(OP_PRINT);
}

static void 
expression_statement(Parser *p) 
{
    expression(p);
    consume(p, TOKEN_SEMICOLON, "Expect ';' after expression.");
    EMIT_BYTE(OP_POP);
}


static void 
statement(Parser *p) 
{
    if (MATCH_TOKEN(TOKEN_PRINT)) {
        stmt_print(p);
    }
    else {
        expression_statement(p);
    }
}

static void 
declaration(Parser *p) 
{
    if (MATCH_TOKEN(TOKEN_VAR)) {
        dec_var(p);
    }
    else {
        statement(p);
    }

    if (p->panic_mode) {
        synchronize(p);
    }
}

Parser*
parser_init(Scanner *s, QxlChunk *c, QxlHashTable *vm_global_strings) 
{
    Parser *p = calloc(1, sizeof(Parser));
    p->s = s;
    p->had_error = false;
    p->panic_mode = false;
    p->compiling_chunk = c;
    p->vm_global_strings = vm_global_strings;

    return p;
}

bool 
compile(QxlChunk* c, const char *src, QxlHashTable *vm_global_strings) 
{
    Parser *p = parser_init(scanner_init(src), c, vm_global_strings);

    advance(p);
    // expression(p);
    // consume(p, TOKEN_EOF, "Expect end of expression.");

    while (!MATCH_TOKEN(TOKEN_EOF)) {
        declaration(p);
    }

    EMIT_RETURN();
    #ifdef DEBUG_TRACE_COMPILING_CHUNK
        if (!p->had_error) {
            debug_disassemble_chunk(p->compiling_chunk, "COMPILING CHUNK");
        }
#endif

    return !p->had_error;
}