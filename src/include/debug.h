/*
  Quixil debugging tools (for maintainers). The disassembler includes
  a `QxlChunk` disassembler that can crank throught the bytecode, 
  disassembling each instruction. Tt prints the byte offset of the given 
  instruction — that tells us where in the chunk this instruction is. This
  is a helpful signpost when doing control flow and jumping around in the 
  bytecode.
*/

#ifndef Qxl_DEBUG_H
#define Qxl_DEBUG_H

#include "quixil.h"
#include "chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

int debug_disassemble_instruction(QxlChunk *chunk, int offset);
void debug_disassemble_chunk(QxlChunk *chunk, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_DEBUG_H */

