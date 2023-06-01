#include "include/compiler.h"
#include "include/debug.h"

#define STATEMENT(type) static void stmt##type(Compiler *c)
#define DEFINITION(type) static void def##type(Compiler *c)
#define BIND_STATEMENT(type) stmt##type(c)
#define BIND_DEFINITION(type) def##type(c)
#define PARSER_ERROR(m) error_at(c->p, &c->p->prev, (m))
#define PARSER_ERROR_AT_CUR(m) error_at(c->p, &c->p->cur, (m))
#define EMIT_BYTE(byte) QxlChunk_add(&c->fn->chunk, (byte), c->p->prev.line)
#define EMIT_BYTES(byte_1, byte_2) (EMIT_BYTE(byte_1), EMIT_BYTE(byte_2))
#define EMIT_CONST(val) EMIT_BYTES(OP_CONSTANT, make_constant(c, val))
#define EMIT_RETURN() EMIT_BYTES(OP_NIL, OP_RETURN)
#define EMIT_JUMP(inst)                                                        \
    ({                                                                         \
        EMIT_BYTE(inst);                                                       \
        EMIT_BYTE(0xff);                                                       \
        EMIT_BYTE(0xff);                                                       \
        c->fn->chunk.count - 2;                                                \
    })
#define EMIT_LOOP(start)                                                       \
    {                                                                          \
        EMIT_BYTE(OP_LOOP);                                                    \
        int offset = c->fn->chunk.count - (start) + 2;                         \
        if (offset > UINT16_MAX) PARSER_ERROR("loop body too large");          \
        EMIT_BYTE((offset >> 8) & 0xff);                                       \
        EMIT_BYTE(offset & 0xff);                                              \
    }
#define PATCH_JUMP(offset)                                                     \
    {                                                                          \
        int jump = c->fn->chunk.count - (offset)-2;                            \
        if (jump > UINT16_MAX)                                                 \
        {                                                                      \
            PARSER_ERROR("too much code to jump over");                        \
        }                                                                      \
        c->fn->chunk.code[offset]     = (jump >> 8) & 0xff;                    \
        c->fn->chunk.code[offset + 1] = jump & 0xff;                           \
    }
#define CHECK_TYPE(_type) (c->p->cur.type == (_type))
#define MATCH_TOKEN(_type) (CHECK_TYPE(_type) ? (advance(c), true) : false)
#define IS_NEXT_VALUE()                                                        \
    ((CHECK_TYPE(TOKEN_TRUE) || CHECK_TYPE(TOKEN_FALSE) ||                     \
      CHECK_TYPE(TOKEN_NIL) || CHECK_TYPE(TOKEN_NUMBER) ||                     \
      CHECK_TYPE(TOKEN_STRING)))
#define CONST_IDENTIFIER(token)                                                \
    (make_constant(c, OBJECT_VAL(QxlString_copy(c->p->vm_global_strings,       \
                                                token.start, token.length))))
#define MARK_INITIALIZED()                                                     \
    if (c->scope_depth > 0) c->locals[c->local_count - 1].depth = c->scope_depth
#define DEFINE_VAR(v)                                                          \
    {                                                                          \
        if (c->scope_depth > 0)                                                \
        {                                                                      \
            MARK_INITIALIZED();                                                \
        }                                                                      \
        else                                                                   \
            EMIT_BYTES(OP_DEFINE_GLOBAL, v);                                   \
    }

#define NAMED_VARIABLE()                                                       \
    (EMIT_BYTES(OP_GET_GLOBAL, (uint8_t)CONST_IDENTIFIER(c->p->prev)))
#define SCOPE_BEGIN() (c->scope_depth++)
#define SCOPE_END() (c->scope_depth--)
#define IDENTIFIERS_EQUAL(a, b)                                                \
    ((a)->length == (b)->length &&                                             \
     memcmp((a)->start, (b)->start, (a)->length) == 0)

#define MAX_WHEN_CASES 256

static void call(Compiler *c, bool can_assign);
static void unary(Compiler *c, bool can_assign);
static void grouping(Compiler *c, bool can_assign);
static void binary(Compiler *c, bool can_assign);
static void and_(Compiler *c, bool can_assign);
static void or_(Compiler *c, bool can_assign);
static void number(Compiler *c, bool can_assign);
static void template(Compiler *c, bool can_assign);
static void string(Compiler *c, bool can_assign);
static void literal(Compiler *c, bool can_assign);
static void variable(Compiler *c, bool can_assign);
static void statement(Compiler *c);
static void definition(Compiler *c);
static void block(Compiler *c);
static Compiler *Compiler_init(Parser *p, Compiler *parent, FunctionType type);
static QxlFunction *Compiler_end(Compiler *c);

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call, PREC_CALL},
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
    [TOKEN_AND]           = {NULL, and_, PREC_AND},
    [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNCTION]      = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
    [TOKEN_OR]            = {NULL, or_, PREC_OR},
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
get_rule(TokenType type)
{
    return &rules[type];
}

static void
error_at(Parser *p, Token *token, const char *msg)
{
    if (p->panic_mode)
    {
        return;
    }

    p->panic_mode = true;
    Qxl_ERROR("[Line %d]: error: %s\n  ", token->line, msg);

    if (token->type == TOKEN_EOF)
    {
        Qxl_ERROR(" at end\n");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing
    }
    else
    {
        Qxl_ERROR(" at \"%.*s\"\n", token->length, token->start);
    }
    p->had_error = true;
}

static void
advance(Compiler *c)
{
    c->p->prev = c->p->cur;

    for (;;)
    {
        c->p->cur = scanner_scan_token(c->p->s);
        if (c->p->cur.type != TOKEN_ERROR) break;
        PARSER_ERROR_AT_CUR(c->p->cur.start);
    }
}

static void
consume(Compiler *c, TokenType type)
{
    if (c->p->cur.type == type)
    {
        advance(c);
        return;
    }

    PARSER_ERROR_AT_CUR("invalid syntax");
}

static void
parse_precedence(Compiler *c, Precedence prec)
{
    advance(c);
    ParseFn prefix_rule = get_rule(c->p->prev.type)->prefix;
    if (prefix_rule == NULL)
    {
        PARSER_ERROR("expected expression");
        return;
    }

    bool can_assign = prec <= PREC_CONDITIONAL;
    prefix_rule(c, can_assign);

    while (prec <= get_rule(c->p->cur.type)->precedence)
    {
        advance(c);
        ParseFn infix_rule = get_rule(c->p->prev.type)->infix;
        infix_rule(c, can_assign);
    }

    if (can_assign && MATCH_TOKEN(TOKEN_EQUAL))
    {
        PARSER_ERROR("invalid assignment target");
    }
}

static void
expression(Compiler *c)
{
    parse_precedence(c, PREC_LOWEST);
}

static uint8_t
make_constant(Compiler *c, QxlValue value)
{
    int constant = QxlChunk_add_constant(&c->fn->chunk, value);
    if (constant > UINT8_MAX)
    {
        PARSER_ERROR("too many constants in one chunk");
        return 0;
    }

    return (uint8_t)constant;
}

static void
scope_end(Compiler *c)
{
    c->scope_depth--;

    while (c->local_count > 0 &&
           c->locals[c->local_count - 1].depth > c->scope_depth)
    {
        EMIT_BYTE(OP_POP);
        c->local_count--;
    }
}

static uint8_t
argument_list(Compiler *c)
{
    uint8_t arg_count = 0;
    if (!CHECK_TYPE(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression(c);
            if (arg_count == 255)
            {
                PARSER_ERROR("can't have more than 255 arguments");
            }
            arg_count++;
        } while (MATCH_TOKEN(TOKEN_COMMA));
    }
    consume(c, TOKEN_RIGHT_PAREN);
    return arg_count;
}

static void
call(Compiler *c, bool can_assign)
{
    uint8_t arg_count = argument_list(c);
    EMIT_BYTES(OP_CALL, arg_count);
}

static void
and_(Compiler *c, bool can_assign)
{
    int end_jump = EMIT_JUMP(OP_JUMP_IF_FALSE);
    EMIT_BYTE(OP_POP);
    parse_precedence(c, PREC_AND);
    PATCH_JUMP(end_jump);
}

static void
or_(Compiler *c, bool can_assign)
{
    int else_jump = EMIT_JUMP(OP_JUMP_IF_FALSE);
    int end_jump  = EMIT_JUMP(OP_JUMP);
    PATCH_JUMP(else_jump);
    EMIT_BYTE(OP_POP);
    parse_precedence(c, PREC_OR);
    PATCH_JUMP(end_jump);
}

static void
number(Compiler *c, bool can_assign)
{
    double value = strtod(c->p->prev.start, NULL);
    EMIT_CONST(NUMBER_VAL(value));
}

static void template(Compiler *c, bool can_assign)
{
    QxlString *s = QxlString_copy(c->p->vm_global_strings, "", 0);
    EMIT_CONST(OBJECT_VAL(s));
    do
    {
        QxlString *s =
            QxlString_copy(c->p->vm_global_strings, c->p->prev.start + 1,
                           c->p->prev.length - 3);
        EMIT_CONST(OBJECT_VAL(s));
        EMIT_BYTE(OP_ADD);
        expression(c);
        EMIT_BYTE(OP_ADD);

    } while (MATCH_TOKEN(TOKEN_INTEROP));
    consume(c, TOKEN_STRING);
    string(c, false);
    EMIT_BYTE(OP_ADD);
}

static void
string(Compiler *c, bool can_assign)
{
    QxlString *s = QxlString_copy(c->p->vm_global_strings, c->p->prev.start + 1,
                                  c->p->prev.length - 2);
    EMIT_CONST(OBJECT_VAL(s));
}

static void
grouping(Compiler *c, bool can_assign)
{
    expression(c);
    consume(c, TOKEN_RIGHT_PAREN);
}

static void
unary(Compiler *c, bool can_assign)
{
    TokenType op_type = c->p->prev.type;

    // Compile the operand
    parse_precedence(c, PREC_UNARY);

    // Emit the operator instruction
    switch (op_type)
    {
    case TOKEN_BANG:
        EMIT_BYTE(OP_NOT);
        break;
    case TOKEN_MINUS:
        EMIT_BYTE(OP_NEGATE);
        break;
    default:
        return; // Unreachable
    }
}

static void
binary(Compiler *c, bool can_assign)
{
    TokenType op_type = c->p->prev.type;
    ParseRule *rule   = get_rule(op_type);
    parse_precedence(c, (Precedence)(rule->precedence + 1));

    switch (op_type)
    {
    case TOKEN_BANG_EQUAL:
        EMIT_BYTES(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        EMIT_BYTE(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        EMIT_BYTE(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        EMIT_BYTES(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        EMIT_BYTE(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        EMIT_BYTES(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        EMIT_BYTE(OP_ADD);
        break;
    case TOKEN_MINUS:
        EMIT_BYTE(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        EMIT_BYTE(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        EMIT_BYTE(OP_DIVIDE);
        break;
    default:
        return; // Unreachable
    }
}

static void
literal(Compiler *c, bool can_assign)
{
    switch (c->p->prev.type)
    {
    case TOKEN_FALSE:
        EMIT_BYTE(OP_FALSE);
        break;
    case TOKEN_NIL:
        EMIT_BYTE(OP_NIL);
        break;
    case TOKEN_TRUE:
        EMIT_BYTE(OP_TRUE);
        break;
    default:
        return; // Unreachable
    }
}

static int
resolve_local_variable(Compiler *c, Token *name)
{
    for (int i = c->local_count - 1; i >= 0; i--)
    {
        Local *local = &c->locals[i];
        if (IDENTIFIERS_EQUAL(name, &local->name))
        {
            if (local->depth == -1)
            {
                PARSER_ERROR(
                    "can't read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static void
named_variable(Compiler *c, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = resolve_local_variable(c, &c->p->prev);

    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else
    {
        arg    = CONST_IDENTIFIER(c->p->prev);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && MATCH_TOKEN(TOKEN_EQUAL))
    {
        expression(c);
        EMIT_BYTES(set_op, (uint8_t)arg);
    }
    else
    {
        EMIT_BYTES(get_op, (uint8_t)arg);
    }
}

static void
variable(Compiler *c, bool can_assign)
{
    named_variable(c, can_assign);
}

static void
synchronize(Compiler *c)
{
    c->p->panic_mode = false;

    while (c->p->cur.type != TOKEN_EOF)
    {
        if (c->p->prev.type == TOKEN_SEMICOLON) return;

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

        advance(c);
    }
}

// Defintions
static void
add_local(Compiler *c, Token name)
{

    if (c->local_count == UINT8_COUNT)
    {
        PARSER_ERROR("too many local variables in function");
        return;
    }

    Local *local = &c->locals[c->local_count++];
    local->name  = name;
    local->depth = -1;
}

static uint8_t
parse_variable(Compiler *c)
{
    consume(c, TOKEN_IDENTIFIER);
    if (c->scope_depth != 0)
    {
        Token *name = &c->p->prev;

        for (int i = c->local_count - 1; i >= 0; i--)
        {
            Local *local = &c->locals[i];
            if (local->depth != -1 && local->depth < c->scope_depth)
            {
                break;
            }

            if (IDENTIFIERS_EQUAL(name, &local->name))
            {
                PARSER_ERROR(
                    "variable with same name already exists in this scope");
            }
        }

        add_local(c, *name);
    }

    return c->scope_depth > 0 ? 0 : CONST_IDENTIFIER(c->p->prev);
}

static void
define_function(Compiler *parent, FunctionType type)
{
    Compiler *c = Compiler_init(parent->p, parent, type);
    SCOPE_BEGIN();

    consume(c, TOKEN_LEFT_PAREN);
    if (!CHECK_TYPE(TOKEN_RIGHT_PAREN))
    {
        do
        {
            c->fn->arity++;
            if (c->fn->arity > 255)
            {
                PARSER_ERROR_AT_CUR("can't have more than 255 parameters");
            }
            uint8_t constant = parse_variable(c);
            DEFINE_VAR(constant);
        } while (MATCH_TOKEN(TOKEN_COMMA));
    }

    consume(c, TOKEN_RIGHT_PAREN);
    consume(c, TOKEN_LEFT_BRACE);
    block(c);

    QxlFunction *fn = Compiler_end(c);

    c = parent;
    EMIT_BYTES(OP_CONSTANT, make_constant(c, OBJECT_VAL(fn)));
}

DEFINITION(_function)
{
    uint8_t global = parse_variable(c);
    MARK_INITIALIZED();
    define_function(c, TYPE_GENERIC);
    DEFINE_VAR(global);
}

// Compiles a "var" variable definition statement
DEFINITION(_variable)
{
    uint8_t global = parse_variable(c);
    if (MATCH_TOKEN(TOKEN_EQUAL))
    {
        expression(c);
    }
    else
    {
        EMIT_BYTE(OP_NIL);
    }
    consume(c, TOKEN_SEMICOLON);
    DEFINE_VAR(global);
}

// Statements
STATEMENT(_print)
{
    expression(c);
    consume(c, TOKEN_SEMICOLON);
    EMIT_BYTE(OP_PRINT);
}

STATEMENT(_if)
{
    consume(c, TOKEN_LEFT_PAREN);
    expression(c);
    consume(c, TOKEN_RIGHT_PAREN);

    int then_jump = EMIT_JUMP(OP_JUMP_IF_FALSE);
    EMIT_BYTE(OP_POP);
    statement(c);
    int else_jump = EMIT_JUMP(OP_JUMP);

    PATCH_JUMP(then_jump);
    EMIT_BYTE(OP_POP);

    if (MATCH_TOKEN(TOKEN_ELSE)) statement(c);
    PATCH_JUMP(else_jump);
}

STATEMENT(_when)
{
    consume(c, TOKEN_LEFT_PAREN);
    expression(c);
    consume(c, TOKEN_RIGHT_PAREN);
    consume(c, TOKEN_LEFT_BRACE);

    int state = 0; /* 0: before all cases, 1: before else, 2: after else */
    int case_ends[MAX_WHEN_CASES];
    int case_count     = 0;
    int prev_case_skip = -1;

    while (!MATCH_TOKEN(TOKEN_RIGHT_BRACE) && !CHECK_TYPE(TOKEN_EOF))
    {
        bool is_next_val = IS_NEXT_VALUE();
        if (is_next_val || MATCH_TOKEN(TOKEN_ELSE))
        {
            if (state == 2)
            {
                PARSER_ERROR("can't have another case or else after the else "
                             "case");
            }

            if (state == 1)
            {
                case_ends[case_count++] = EMIT_JUMP(OP_JUMP);
                PATCH_JUMP(prev_case_skip);
                EMIT_BYTE(OP_POP);
            }

            if (is_next_val)
            {
                state = 1;
                EMIT_BYTE(OP_DUP);
                expression(c);
                consume(c, TOKEN_ARROW);
                EMIT_BYTE(OP_EQUAL);
                prev_case_skip = EMIT_JUMP(OP_JUMP_IF_FALSE);
                EMIT_BYTE(OP_POP);
            }
            else
            {
                state = 2;
                consume(c, TOKEN_ARROW);
                prev_case_skip = -1;
            }
        }
        else
        {
            if (state == 0)
            {
                PARSER_ERROR("can't have statements before any case");
            }
            statement(c);
        }
    }

    if (state == 1)
    {
        PATCH_JUMP(prev_case_skip);
        EMIT_BYTE(OP_POP);
    }

    // Patch all the case jumps to the end
    for (int i = 0; i < case_count; i++)
    {
        PATCH_JUMP(case_ends[i]);
    }

    EMIT_BYTE(OP_POP);
}

STATEMENT(_while)
{
    int loop_start = c->fn->chunk.count;
    consume(c, TOKEN_LEFT_PAREN);
    expression(c);
    consume(c, TOKEN_RIGHT_PAREN);

    int exit_jump = EMIT_JUMP(OP_JUMP_IF_FALSE);
    EMIT_BYTE(OP_POP);
    statement(c);
    EMIT_LOOP(loop_start);

    PATCH_JUMP(exit_jump);
    EMIT_BYTE(OP_POP);
}

STATEMENT(_return)
{
    if (c->type == TYPE_MAIN)
    {
        PARSER_ERROR("can't \"return\" from top level code");
    }

    if (MATCH_TOKEN(TOKEN_SEMICOLON))
    {
        EMIT_RETURN();
    }
    else
    {
        expression(c);
        consume(c, TOKEN_SEMICOLON);
        EMIT_BYTE(OP_RETURN);
    }
}

static void
expression_statement(Compiler *c)
{
    expression(c);
    consume(c, TOKEN_SEMICOLON);
    EMIT_BYTE(OP_POP);
}

static void
block(Compiler *c)
{
    while (!CHECK_TYPE(TOKEN_RIGHT_BRACE) && !CHECK_TYPE(TOKEN_EOF))
    {
        definition(c);
    }

    consume(c, TOKEN_RIGHT_BRACE);
}

// Compiles a simple statement. These can only appear at the top-level or
// within curly blocks. Simple statements exclude variable binding statements
// like "var" and "class" which are not allowed directly in places like the
// branches of an "if" statement. Statements, unlike expressions, do not leave a
// value on the vm stack.
static void
statement(Compiler *c)
{
    if (MATCH_TOKEN(TOKEN_PRINT))
    {
        BIND_STATEMENT(_print);
    }
    else if (MATCH_TOKEN(TOKEN_IF))
    {
        BIND_STATEMENT(_if);
    }
    else if (MATCH_TOKEN(TOKEN_WHEN))
    {
        BIND_STATEMENT(_when);
    }
    else if (MATCH_TOKEN(TOKEN_WHILE))
    {
        BIND_STATEMENT(_while);
    }
    else if (MATCH_TOKEN(TOKEN_RETURN))
    {
        BIND_STATEMENT(_return);
    }
    else if (MATCH_TOKEN(TOKEN_LEFT_BRACE))
    {
        SCOPE_BEGIN();
        block(c);
        scope_end(c);
    }
    else
    {
        expression_statement(c);
    }
}

// Compiles a "definition". These are the statements that bind new variables.
// They can only appear at the top level of a block and are prohibited in places
// like the non-curly body of an if or while.
static void
definition(Compiler *c)
{
    if (MATCH_TOKEN(TOKEN_FUNCTION))
    {
        BIND_DEFINITION(_function);
    }
    else if (MATCH_TOKEN(TOKEN_VAR))
    {
        BIND_DEFINITION(_variable);
    }
    else
    {
        statement(c);
    }

    if (c->p->panic_mode)
    {
        synchronize(c);
    }
}

static Compiler *
Compiler_init(Parser *p, Compiler *parent, FunctionType type)
{
    Compiler *c    = calloc(1, sizeof(struct compiler_t));
    c->parent      = parent;
    c->p           = p;
    c->fn          = NULL;
    c->type        = type;
    c->scope_depth = 0;
    c->local_count = 0;

    c->fn = QxlFunction_new();

    if (type != TYPE_MAIN)
    {
        c->fn->name =
            QxlString_copy(p->vm_global_strings, p->prev.start, p->prev.length);
    }

    Local *local       = &c->locals[c->local_count++];
    local->depth       = 0;
    local->name.start  = "";
    local->name.length = 0;

    return c;
}

static QxlFunction *
Compiler_end(Compiler *c)
{
    EMIT_RETURN();
    QxlFunction *fn = c->fn;

#ifdef DEBUG_TRACE_COMPILING_CHUNK
    if (!c->p->had_error)
    {
        debug_disassemble_chunk(&fn->chunk, fn->name != NULL ? fn->name->chars
                                                             : "<script-main>");
    }
#endif

    // [TODO] : Rest compiler to parent
    // This is not working for some reason
    // c = c->parent;
    return fn;
}

QxlFunction *
compile(const char *src, QxlHashTable *vm_global_strings)
{
    Parser *p = &(Parser){.s                 = scanner_init(src),
                          .had_error         = false,
                          .panic_mode        = false,
                          .vm_global_strings = vm_global_strings};

    Compiler *c = Compiler_init(p, NULL, TYPE_MAIN);

    advance(c);

    while (!MATCH_TOKEN(TOKEN_EOF))
    {
        definition(c);
    }

    QxlFunction *fn = Compiler_end(c);
    return c->p->had_error ? NULL : fn;
}