#ifndef Qxl_CHUNK_H
#define Qxl_CHUNK_H

#include "quixil.h"
#include "value.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Operation code for each instruction in the bytecode
    typedef enum
    {
        OP_CONSTANT,
        OP_NEGATE,
        OP_NOT,
        OP_ADD,
        OP_SUBTRACT,
        OP_MULTIPLY,
        OP_DIVIDE,
        OP_RETURN,
        OP_NIL,
        OP_TRUE,
        OP_FALSE,
        OP_EQUAL,
        OP_GREATER,
        OP_LESS,
        OP_PRINT,
        OP_POP,
        OP_DEFINE_GLOBAL,
        OP_GET_GLOBAL,
        OP_SET_GLOBAL,
        OP_GET_LOCAL,
        OP_SET_LOCAL
    } OpCode;

    // Chunk represents the sequences of byte code
    typedef struct
    {
        size_t count;           // in use
        size_t cap;             // allocated elements
        QxlValueList constants; // runtime constants
        uint8_t *code;          // instructions
        int *lines;
    } QxlChunk;

    void QxlChunk_init (QxlChunk *chunk);
    void QxlChunk_add (QxlChunk *chunk, uint8_t byte, int line);
    int QxlChunk_add_constant (QxlChunk *chunk, QxlValue value);
    void QxlChunk_free (QxlChunk *chunk);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_CHUNK_H */
