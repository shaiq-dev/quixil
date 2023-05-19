#include "include/debug.h"
#include "include/value.h"

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
