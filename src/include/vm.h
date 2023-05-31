#ifndef Qxl_VM_H
#define Qxl_VM_H

#include "chunk.h"
#include "collections.h"
#include "object.h"
#include "quixil.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define VM_FRAMES_MAX 64
#define VM_STACK_MAX (VM_FRAMES_MAX * UINT8_COUNT)

    typedef struct
    {
        QxlFunction *fn;
        uint8_t *ip;
        QxlValue *slots;
    } CallFrame;

    typedef struct
    {
        CallFrame frames[VM_FRAMES_MAX];
        int frame_count;
        QxlValue stack[VM_STACK_MAX];
        QxlValue *stack_top;
        QxlObject *objects;
        QxlHashTable strings;
        QxlHashTable globals;
    } VM;

    typedef enum
    {
        INTERPRET_OK,
        INTERPRET_COMPILE_ERROR,
        INTERPRET_RUNTIME_ERROR
    } InterpretResult;

    VM *vm_init();
    void vm_free(VM *vm);
    InterpretResult vm_interpret(VM *vm, const char *src);
    void vm_stack_push(VM *vm, QxlValue value);
    QxlValue vm_stack_pop(VM *vm);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_VM_H */