#include "include/compiler.h"
#include "include/debug.h"

#define PARSER_ERROR(m) error_at (c->p, &c->p->prev, (m))
#define PARSER_ERROR_AT_CUR(m) error_at (c->p, &c->p->cur, (m))
#define EMIT_BYTE(byte)                                                        \
    QxlChunk_add (c->p->compiling_chunk, (byte), c->p->prev.line)
#define EMIT_RETURN() EMIT_BYTE (OP_RETURN)
#define EMIT_BYTES(byte_1, byte_2) (EMIT_BYTE (byte_1), EMIT_BYTE (byte_2))
#define EMIT_CONST(val) EMIT_BYTES (OP_CONSTANT, make_constant (c, val))
#define CHECK_TYPE(_type) (c->p->cur.type == (_type))
#define MATCH_TOKEN(_type) (CHECK_TYPE (_type) ? (advance (c), true) : false)
#define CONST_IDENTIFIER(token)                                                \
    (make_constant (                                                           \
        c, OBJECT_VAL (QxlObjectString_copy (c->p->vm_global_strings,          \
                                             token.start, token.length))))
#define DECLARE_VAR()                                                          \
    ({                                                                         \
        if (c->scope_depth == 0)                                               \
            return;                                                            \
        add_local (c, c->p->prev);                                             \
    })
#define NAMED_VARIABLE()                                                       \
    (EMIT_BYTES (OP_GET_GLOBAL, (uint8_t)CONST_IDENTIFIER (c->p->prev)))
#define SCOPE_BEGIN() (c->scope_depth++)
#define SCOPE_END() (c->scope_depth--)
#define IDENTIFIERS_EQUAL(a, b)                                                \
    ((a)->length == (b)->length &&                                             \
     memcmp ((a)->start, (b)->start, (a)->length) == 0)
#define MARK_INITIALIZED() c->locals[c->local_count - 1].depth = c->scope_depth

static void unary (Compiler *c, bool can_assign);
static void grouping (Compiler *c, bool can_assign);
static void binary (Compiler *c, bool can_assign);
static void number (Compiler *c, bool can_assign);
static void template (Compiler *c, bool can_assign);
static void string (Compiler *c, bool can_assign);
static void literal (Compiler *c, bool can_assign);
static void variable (Compiler *c, bool can_assign);
static void statement (Compiler *c);
static void declaration (Compiler *c);

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
    [TOKEN_STRING]        = {string, NULL, PREC_NONE},
    [TOKEN_INTEROP]       = {template, NULL, PREC_NONE},
    [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
    [TOKEN_AND]           = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNCTION]      = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
    [TOKEN_OR]            = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
    [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static ParseRule *
get_rule (TokenType type)
{
    return &rules[type];
}

static void
error_at (Parser *p, Token *token, const char *msg)
{
    if (p->panic_mode)
    {
        return;
    }

    p->panic_mode = true;
    Qxl_ERROR ("[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        Qxl_ERROR (" at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing.
    }
    else
    {
        Qxl_ERROR (" at '%.*s'", token->length, token->start);
    }

    Qxl_ERROR (": %s\n", msg);
    p->had_error = true;
}

static void
advance (Compiler *c)
{
    c->p->prev = c->p->cur;

    for (;;)
    {
        c->p->cur = scanner_scan_token (c->p->s);
        if (c->p->cur.type != TOKEN_ERROR)
            break;
        PARSER_ERROR_AT_CUR (c->p->cur.start);
    }
}

static void
consume (Compiler *c, TokenType type, const char *msg)
{
    if (c->p->cur.type == type)
    {
        advance (c);
        return;
    }

    PARSER_ERROR_AT_CUR (msg);
}

static void
parse_precedence (Compiler *c, Precedence prec)
{
    advance (c);
    ParseFn prefix_rule = get_rule (c->p->prev.type)->prefix;
    if (prefix_rule == NULL)
    {
        PARSER_ERROR ("Expected expression.");
        return;
    }

    bool can_assign = prec <= PREC_CONDITIONAL;
    prefix_rule (c, can_assign);

    while (prec <= get_rule (c->p->cur.type)->precedence)
    {
        advance (c);
        ParseFn infix_rule = get_rule (c->p->prev.type)->infix;
        infix_rule (c, can_assign);
    }

    if (can_assign && MATCH_TOKEN (TOKEN_EQUAL))
    {
        PARSER_ERROR ("Invalid assignment target.");
    }
}

static void
expression (Compiler *c)
{
    parse_precedence (c, PREC_LOWEST);
}

static uint8_t
make_constant (Compiler *c, QxlValue value)
{
    int constant = QxlChunk_add_constant (c->p->compiling_chunk, value);
    if (constant > UINT8_MAX)
    {
        PARSER_ERROR ("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void
scope_end (Compiler *c)
{
    c->scope_depth--;

    while (c->local_count > 0 &&
           c->locals[c->local_count - 1].depth > c->scope_depth)
    {
        EMIT_BYTE (OP_POP);
        c->local_count--;
    }
}

static void
number (Compiler *c, bool can_assign)
{
    double value = strtod (c->p->prev.start, NULL);
    EMIT_CONST (NUMBER_VAL (value));
}

static void template (Compiler *c, bool can_assign)
{
    QxlObjectString *s = QxlObjectString_copy (c->p->vm_global_strings, "", 0);
    EMIT_CONST (OBJECT_VAL (s));
    do
    {
        QxlObjectString *s =
            QxlObjectString_copy (c->p->vm_global_strings, c->p->prev.start + 1,
                                  c->p->prev.length - 3);
        EMIT_CONST (OBJECT_VAL (s));
        EMIT_BYTE (OP_ADD);
        expression (c);
        EMIT_BYTE (OP_ADD);

    } while (MATCH_TOKEN (TOKEN_INTEROP));
    consume (c, TOKEN_STRING, "Expected end of template string");
    string (c, false);
    EMIT_BYTE (OP_ADD);
}

static void
string (Compiler *c, bool can_assign)
{
    QxlObjectString *s = QxlObjectString_copy (
        c->p->vm_global_strings, c->p->prev.start + 1, c->p->prev.length - 2);
    EMIT_CONST (OBJECT_VAL (s));
}

static void
grouping (Compiler *c, bool can_assign)
{
    expression (c);
    consume (c, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void
unary (Compiler *c, bool can_assign)
{
    TokenType op_type = c->p->prev.type;

    // Compile the operand.
    parse_precedence (c, PREC_UNARY);

    // Emit the operator instruction.
    switch (op_type)
    {
    case TOKEN_BANG:
        EMIT_BYTE (OP_NOT);
        break;
    case TOKEN_MINUS:
        EMIT_BYTE (OP_NEGATE);
        break;
    default:
        return; // Unreachable
    }
}

static void
binary (Compiler *c, bool can_assign)
{
    TokenType op_type = c->p->prev.type;
    ParseRule *rule   = get_rule (op_type);
    parse_precedence (c, (Precedence)(rule->precedence + 1));

    switch (op_type)
    {
    case TOKEN_BANG_EQUAL:
        EMIT_BYTES (OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        EMIT_BYTE (OP_EQUAL);
        break;
    case TOKEN_GREATER:
        EMIT_BYTE (OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        EMIT_BYTES (OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        EMIT_BYTE (OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        EMIT_BYTES (OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        EMIT_BYTE (OP_ADD);
        break;
    case TOKEN_MINUS:
        EMIT_BYTE (OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        EMIT_BYTE (OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        EMIT_BYTE (OP_DIVIDE);
        break;
    default:
        return; // Unreachable
    }
}

static void
literal (Compiler *c, bool can_assign)
{
    switch (c->p->prev.type)
    {
    case TOKEN_FALSE:
        EMIT_BYTE (OP_FALSE);
        break;
    case TOKEN_NIL:
        EMIT_BYTE (OP_NIL);
        break;
    case TOKEN_TRUE:
        EMIT_BYTE (OP_TRUE);
        break;
    default:
        return; // Unreachable
    }
}

static int
resolve_local_variable (Compiler *c, Token *name)
{
    for (int i = c->local_count - 1; i >= 0; i--)
    {
        Local *local = &c->locals[i];
        if (IDENTIFIERS_EQUAL (name, &local->name))
        {
            if (local->depth == -1)
            {
                PARSER_ERROR (
                    "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

static void
named_variable (Compiler *c, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = resolve_local_variable (c, &c->p->prev);

    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else
    {
        arg    = CONST_IDENTIFIER (c->p->prev);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && MATCH_TOKEN (TOKEN_EQUAL))
    {
        expression (c);
        EMIT_BYTES (set_op, (uint8_t)arg);
    }
    else
    {
        EMIT_BYTES (get_op, (uint8_t)arg);
    }
}

static void
variable (Compiler *c, bool can_assign)
{
    named_variable (c, can_assign);
}

static void
synchronize (Compiler *c)
{
    c->p->panic_mode = false;

    while (c->p->cur.type != TOKEN_EOF)
    {
        if (c->p->prev.type == TOKEN_SEMICOLON)
            return;

        switch (c->p->cur.type)
        {
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

        advance (c);
    }
}

// Declerations
static void
add_local (Compiler *c, Token name)
{

    if (c->local_count == UINT8_COUNT)
    {
        PARSER_ERROR ("Too many local variables in function.");
        return;
    }

    Local *local = &c->locals[c->local_count++];
    local->name  = name;
    local->depth = -1;
}

static void
declare_variable (Compiler *c)
{
    if (c->scope_depth == 0)
        return;
    Token *name = &c->p->prev;

    for (int i = c->local_count - 1; i >= 0; i--)
    {
        Local *local = &c->locals[i];
        if (local->depth != -1 && local->depth < c->scope_depth)
        {
            break;
        }

        if (IDENTIFIERS_EQUAL (name, &local->name))
        {
            PARSER_ERROR ("Already a variable with this name in this scope");
        }
    }

    add_local (c, *name);
}

static uint8_t
parse_variable (Compiler *c)
{
    consume (c, TOKEN_IDENTIFIER, "Expected variable name");
    declare_variable (c);
    return c->scope_depth > 0 ? 0 : CONST_IDENTIFIER (c->p->prev);
}

static void
define_variable (Compiler *c, uint8_t global)
{
    if (c->scope_depth > 0)
    {
        MARK_INITIALIZED ();
        return;
    }
    EMIT_BYTES (OP_DEFINE_GLOBAL, global);
}

static void
dec_var (Compiler *c)
{
    uint8_t global = parse_variable (c);

    if (MATCH_TOKEN (TOKEN_EQUAL))
    {
        expression (c);
    }
    else
    {
        EMIT_BYTE (OP_NIL);
    }
    consume (c, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    define_variable (c, global);
}

// Statements
static void
stmt_print (Compiler *c)
{
    expression (c);
    consume (c, TOKEN_SEMICOLON, "Expect ';' after value.");
    EMIT_BYTE (OP_PRINT);
}

static void
expression_statement (Compiler *c)
{
    expression (c);
    consume (c, TOKEN_SEMICOLON, "Expect ';' after expression.");
    EMIT_BYTE (OP_POP);
}

static void
block (Compiler *c)
{
    while (!CHECK_TYPE (TOKEN_RIGHT_BRACE) && !CHECK_TYPE (TOKEN_EOF))
    {
        declaration (c);
    }

    consume (c, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void
statement (Compiler *c)
{
    if (MATCH_TOKEN (TOKEN_PRINT))
    {
        stmt_print (c);
    }
    else if (MATCH_TOKEN (TOKEN_LEFT_BRACE))
    {
        SCOPE_BEGIN ();
        block (c);
        scope_end (c);
    }
    else
    {
        expression_statement (c);
    }
}

static void
declaration (Compiler *c)
{
    if (MATCH_TOKEN (TOKEN_VAR))
    {
        dec_var (c);
    }
    else
    {
        statement (c);
    }

    if (c->p->panic_mode)
    {
        synchronize (c);
    }
}

Compiler *
compiler_init (Parser *p)
{
    Compiler *c    = calloc (1, sizeof (Compiler));
    c->local_count = 0;
    c->scope_depth = 0;
    c->p           = p;
    return c;
}

Parser *
parser_init (Scanner *s, QxlChunk *c, QxlHashTable *vm_global_strings)
{
    Parser *p            = calloc (1, sizeof (Parser));
    p->s                 = s;
    p->had_error         = false;
    p->panic_mode        = false;
    p->compiling_chunk   = c;
    p->vm_global_strings = vm_global_strings;

    return p;
}

bool
compile (QxlChunk *chunk, const char *src, QxlHashTable *vm_global_strings)
{
    Parser *p   = parser_init (scanner_init (src), chunk, vm_global_strings);
    Compiler *c = compiler_init (p);

    advance (c);
    // expression(p);
    // consume(p, TOKEN_EOF, "Expect end of expression.");

    while (!MATCH_TOKEN (TOKEN_EOF))
    {
        declaration (c);
    }

    EMIT_RETURN ();
#ifdef DEBUG_TRACE_COMPILING_CHUNK
    if (!c->p->had_error)
    {
        debug_disassemble_chunk (c->p->compiling_chunk, "COMPILING CHUNK");
    }
#endif

    return !c->p->had_error;
}