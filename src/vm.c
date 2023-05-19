
#include "include/quixil.h"
#include "include/vm.h"
#include "include/compiler.h"
#include "include/memory.h"
#include "include/scanner.h"
#include "include/debug.h"
#include "include/object.h"
#include "include/common.h"

#define STACK_PEEK(d) vm->stack_top[-1 - (d)]

static bool 
is_falsey(QxlValue value) 
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void 
vm_stack_reset(VM *vm) 
{ 
    vm->stack_top = vm->stack; 
}

static void 
runtime_error(VM *vm, const char* format, ...) 
{

    size_t instruction = vm->ip - vm->chunk->code - 1;
    int line = vm->chunk->lines[instruction];
    Qxl_ERROR("[Line %d] ", line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    vm_stack_reset(vm);
}

VM 
*vm_init() 
{
    VM *vm = calloc(1, sizeof(VM));
    vm_stack_reset(vm);
    vm->objects = NULL;
    return vm;
}

void 
vm_free() 
{
    // NOT_IMPLEMENTED
}

static InterpretResult 
run(VM *vm) 
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op) \
    do {                                                                \
        if (!IS_NUMBER(STACK_PEEK(0)) || !IS_NUMBER(STACK_PEEK(1))) {   \
            runtime_error(vm, "Operands must be numbers.");             \
            return INTERPRET_RUNTIME_ERROR;                             \
        }                                                               \
        double b = AS_NUMBER(vm_stack_pop(vm));                         \
        double a = AS_NUMBER(vm_stack_pop(vm));                         \
        vm_stack_push(vm, value_type(a op b));                          \
    } while (false)

#define BINARY_OP_(value_type, op, error)                               \
    do {                                                                \
        if (!IS_NUMBER(STACK_PEEK(0)) || !IS_NUMBER(STACK_PEEK(1))) {   \
            runtime_error(vm, (error));                                 \
            free(error);                                                \
            return INTERPRET_RUNTIME_ERROR;                             \
        }                                                               \
        double b = AS_NUMBER(vm_stack_pop(vm));                         \
        double a = AS_NUMBER(vm_stack_pop(vm));                         \
        vm_stack_push(vm, value_type(a op b));                          \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (QxlValue *slot = vm->stack; slot < vm->stack_top; slot++) {
            printf("[ ");
            QxlValue_print(*slot);
            printf(" ]");
        }
        printf("\n");
        debug_disassemble_instruction(vm->chunk, (int)(vm->ip - vm->chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                QxlValue constant = READ_CONSTANT();
                vm_stack_push(vm, constant);
                break;
            }
            case OP_NIL: 
                vm_stack_push(vm, NIL_VAL); 
                break;
            case OP_TRUE: 
                vm_stack_push(vm, BOOL_VAL(true)); 
                break;
            case OP_FALSE: 
                vm_stack_push(vm, BOOL_VAL(false)); 
                break;
            case OP_EQUAL: {
                QxlValue b = vm_stack_pop(vm);
                QxlValue a = vm_stack_pop(vm);
                vm_stack_push(vm, BOOL_VAL(QxlValue_are_equal(a, b)));
                break;
            }
            case OP_GREATER:  
                BINARY_OP(BOOL_VAL, >); 
                break;
            case OP_LESS:     
                BINARY_OP(BOOL_VAL, <); 
                break;
            case OP_ADD:
                // String concatination
                if (OBJECT_IS_STRING(STACK_PEEK(0)) && OBJECT_IS_STRING(STACK_PEEK(1))) {
                    QxlValue b = vm_stack_pop(vm);
                    QxlValue a = vm_stack_pop(vm);
                    QxlObjectString *str = QxlString_concatenate(OBJECT_AS_STRING(a), OBJECT_AS_STRING(b));

                    // Freeing Objects
                    // str->obj.next = vm->objects;
                    // vm->objects = str->obj;

                    vm_stack_push(vm, OBJECT_VAL(str));
                }
                else if(OBJECT_IS_STRING(STACK_PEEK(0)) || OBJECT_IS_STRING(STACK_PEEK(1))) {
                    runtime_error(vm, 
                        "RuntimeError: Can only concatenate str (not '%s') to str", 
                        Qxl_TYPE_NAME(STACK_PEEK(OBJECT_IS_STRING(STACK_PEEK(0)) ? 1 : 0))
                    );
                }
                else {
                    char *error = Qxl_fstr(
                        "RuntimeError: Unsupported operand types(s) for + : '%s' and '%s'", 
                        Qxl_TYPE_NAME(STACK_PEEK(1)), Qxl_TYPE_NAME(STACK_PEEK(0))
                    );
                    BINARY_OP_(NUMBER_VAL, +, error);
                }
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY: {
                if (OBJECT_IS_STRING(STACK_PEEK(0)) && IS_NUMBER(STACK_PEEK(1)) ||
                    IS_NUMBER(STACK_PEEK(0)) && OBJECT_IS_STRING(STACK_PEEK(1))
                ){
                    QxlValue l = vm_stack_pop(vm);
                    QxlValue r = vm_stack_pop(vm);
                    QxlObjectString *str = QxlString_repeat(
                        OBJECT_AS_STRING((IS_OBJECT(l) ? l : r)) , 
                        AS_NUMBER(IS_OBJECT(l) ? r : l)
                    );
                    vm_stack_push(vm, OBJECT_VAL(str));
                }                
                else {
                    char *error = Qxl_fstr(
                        "RuntimeError: Unsupported operand types(s) for * : '%s' and '%s'", 
                        Qxl_TYPE_NAME(STACK_PEEK(1)), Qxl_TYPE_NAME(STACK_PEEK(0))
                    );
                    BINARY_OP_(NUMBER_VAL, /, error);
                    free(error);
                }
                break;
            }
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT:
                vm_stack_push(vm, BOOL_VAL(is_falsey(vm_stack_pop(vm))));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(STACK_PEEK(0))) {
                    runtime_error(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(vm, NUMBER_VAL(-AS_NUMBER(vm_stack_pop(vm))));
                break;
            case OP_RETURN: {
                QxlValue_print(vm_stack_pop(vm));
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult 
vm_interpret(VM *vm, const char * src) 
{
    QxlChunk compiling_chunk;
    QxlChunk_init(&compiling_chunk);

    if (!compile(&compiling_chunk, src)) {
        QxlChunk_free(&compiling_chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &compiling_chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);
    QxlChunk_free(&compiling_chunk);
    
    return result;
}

void 
vm_stack_push(VM *vm, QxlValue value) 
{
    *(vm->stack_top) = value;
    vm->stack_top++;
}

QxlValue 
vm_stack_pop(VM *vm) 
{
    vm->stack_top--;
    return *(vm->stack_top);
}