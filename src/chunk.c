#include "include/chunk.h"
#include "include/memory.h"
#include "include/quixil.h"

void
QxlChunk_init(QxlChunk *chunk)
{
    chunk->count = 0;
    chunk->cap   = 0;
    QxlValueList_init(&chunk->constants);
    chunk->code  = NULL;
    chunk->lines = NULL;
}

void
QxlChunk_add(QxlChunk *chunk, uint8_t byte, int line)
{
    if (chunk->cap < chunk->count + 1)
    {
        int old_cap  = chunk->cap;
        chunk->cap   = QxlMem_Resize(old_cap);
        chunk->lines = QxlMem_Realloc(int, chunk->lines, old_cap, chunk->cap);
        chunk->code = QxlMem_Realloc(uint8_t, chunk->code, old_cap, chunk->cap);
    }

    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int
QxlChunk_add_constant(QxlChunk *chunk, QxlValue value)
{
    QxlValueList_push(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void
QxlChunk_free(QxlChunk *chunk)
{
    QxlMem_Free(uint8_t, chunk->code, chunk->cap);
    QxlMem_Free(int, chunk->lines, chunk->cap);
    QxlValueList_free(&chunk->constants);
    QxlChunk_init(chunk);
}
