#ifndef Qxl_VM_H
#define Qxl_VM_H

#include "chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VM_STACK_MAX 256

typedef struct {
    QxlChunk *chunk;
    uint8_t *ip;
    QxlValue stack[VM_STACK_MAX];
    QxlValue *stack_top;
    QxlObject *objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

VM *vm_init();
void vm_free();
InterpretResult vm_interpret(VM *vm, const char *src);
void vm_stack_push(VM *vm, QxlValue value);
QxlValue vm_stack_pop(VM *vm);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_VM_H */