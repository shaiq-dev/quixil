#include "include/debug.h"
#include "include/value.h"
#include "include/scanner.h"

#define SI(i) return simple_instruction((i), offset)

static int 
simple_instruction(const char *name, int offset) 
{
    printf("%s\n", name);
    return offset + 1;
}

static int 
constant_instruction(const char *name, QxlChunk *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    QxlValue_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
}

int 
debug_disassemble_instruction(QxlChunk *chunk, int offset) 
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_NIL:        SI("OP_NIL");
        case OP_TRUE:       SI("OP_TRUE");
        case OP_FALSE:      SI("OP_FALSE");
        case OP_ADD:        SI("OP_ADD");
        case OP_SUBTRACT:   SI("OP_SUBTRACT");
        case OP_MULTIPLY:   SI("OP_MULTIPLY");
        case OP_DIVIDE:     SI("OP_DIVIDE");
        case OP_NOT:        SI("OP_NOT");
        case OP_EQUAL:      SI("OP_EQUAL");
        case OP_GREATER:    SI("OP_GREATER");
        case OP_LESS:       SI("OP_LESS");
        case OP_NEGATE:     SI("OP_NEGATE");
        case OP_PRINT:      SI("OP_PRINT");
        case OP_POP:        SI("OP_POP");
        case OP_RETURN:     SI("OP_RETURN");
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void 
debug_disassemble_chunk(QxlChunk *chunk, const char *name) 
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = debug_disassemble_instruction(chunk, offset);
    }
}

const char* tokens[] = {
    "TOKEN_LEFT_PAREN",
    "TOKEN_RIGHT_PAREN",
    "TOKEN_LEFT_BRACE",
    "TOKEN_RIGHT_BRACE",
    "TOKEN_COMMA",
    "TOKEN_DOT",
    "TOKEN_MINUS",
    "TOKEN_PLUS",
    "TOKEN_SEMICOLON",
    "TOKEN_SLASH",
    "TOKEN_STAR",
    "TOKEN_BANG",
    "TOKEN_BANG_EQUAL",
    "TOKEN_EQUAL",
    "TOKEN_EQUAL_EQUAL",
    "TOKEN_GREATER",
    "TOKEN_GREATER_EQUAL",
    "TOKEN_LESS",
    "TOKEN_LESS_EQUAL",
    "TOKEN_IDENTIFIER",
    "TOKEN_STRING",
    "TOKEN_INTEROP",
    "TOKEN_NUMBER",
    "TOKEN_AND",
    "TOKEN_CLASS",
    "TOKEN_ELSE",
    "TOKEN_FOR",
    "TOKEN_FUNCTION",
    "TOKEN_IF",
    "TOKEN_NIL",
    "TOKEN_OR",
    "TOKEN_PRINT",
    "TOKEN_RETURN",
    "TOKEN_SUPER",
    "TOKEN_THIS",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_VAR",
    "TOKEN_WHILE",
    "TOKEN_ERROR",
    "TOKEN_EOF"
};

void
debug_token(TokenType token)
{
    if (token >= 0 && token <= TOKEN_EOF) {
        printf("%s\n", tokens[token]);
    } else {
        printf("Unknown token\n");
    }
}