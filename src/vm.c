
#include "include/vm.h"
#include "include/builtins.h"
#include "include/collections.h"
#include "include/common.h"
#include "include/compiler.h"
#include "include/debug.h"
#include "include/memory.h"
#include "include/object.h"
#include "include/quixil.h"
#include "include/scanner.h"

#define STACK_PEEK(d) vm->stack_top[-1 - (d)]

static bool
is_falsey(QxlValue value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void
vm_stack_reset(VM *vm)
{
    vm->stack_top   = vm->stack;
    vm->frame_count = 0;
}

static void
runtime_error(VM *vm, const char *format, ...)
{
    CallFrame *frame = &vm->frames[vm->frame_count - 1];

    size_t instruction = frame->ip - frame->fn->chunk.code - 1;
    int line           = frame->fn->chunk.lines[instruction];
    Qxl_ERROR("[Line %d] ", line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm->frame_count - 1; i >= 0; i--)
    {
        CallFrame *frame   = &vm->frames[i];
        QxlFunction *fn    = frame->fn;
        size_t instruction = frame->ip - fn->chunk.code - 1;
        fprintf(stderr, "[Line %d] in ", fn->chunk.lines[instruction]);
        if (fn->name == NULL)
        {
            fprintf(stderr, "<script-main>\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", fn->name->chars);
        }
    }

    vm_stack_reset(vm);
}

VM *
vm_init()
{
    VM *vm = calloc(1, sizeof(VM));
    vm_stack_reset(vm);
    vm->objects = NULL;
    QxlHashTable_init(&vm->strings);
    QxlHashTable_init(&vm->globals);

    // Load builtins
    builitins_init(vm);

    return vm;
}

void
vm_free(VM *vm)
{
    QxlHashTable_free(&vm->strings);
    QxlHashTable_free(&vm->globals);
    QxlMem_free_objects(vm);
}

static bool
call(VM *vm, QxlFunction *fn, int arg_count)
{
    if (arg_count != fn->arity)
    {
        runtime_error(vm, "%s() takes %d arguments but got %d", fn->name->chars,
                      fn->arity, arg_count);
        return false;
    }

    if (vm->frame_count == VM_FRAMES_MAX)
    {
        runtime_error(vm, "Stack overflow");
        return false;
    }

    CallFrame *frame = &vm->frames[vm->frame_count++];
    frame->fn        = fn;
    frame->ip        = fn->chunk.code;
    frame->slots     = vm->stack_top - arg_count - 1;
    return true;
}

static bool
call_value(VM *vm, QxlValue callee, int arg_count)
{
    if (IS_OBJECT(callee))
    {
        switch (OBJECT_TYPE(callee))
        {
        case OBJ_FUNCTION:
            return call(vm, AS_FUNCTION(callee), arg_count);
        case OBJ_BUILTIN:
        {
            BuiltinFn bltin = AS_BUILTIN_FUNCTION(callee);
            QxlValue result = bltin(vm, arg_count, vm->stack_top - arg_count);
            vm->stack_top -= arg_count + 1;
            vm_stack_push(vm, result);
            return true;
        }
        default:
            break; // Non-callable object type
        }
    }
    runtime_error(vm, "can only call functions and classes");
    return false;
}

static InterpretResult
run(VM *vm)
{
    CallFrame *frame     = &vm->frames[vm->frame_count - 1];
    register uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (frame->fn->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define BINARY_OP(value_type, op)                                              \
    do                                                                         \
    {                                                                          \
        if (!IS_NUMBER(STACK_PEEK(0)) || !IS_NUMBER(STACK_PEEK(1)))            \
        {                                                                      \
            frame->ip = ip;                                                    \
            runtime_error(vm, "operands must be numbers");                     \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(vm_stack_pop(vm));                                \
        double a = AS_NUMBER(vm_stack_pop(vm));                                \
        vm_stack_push(vm, value_type(a op b));                                 \
    } while (false)

#define BINARY_OP_(value_type, op, error)                                      \
    do                                                                         \
    {                                                                          \
        if (!IS_NUMBER(STACK_PEEK(0)) || !IS_NUMBER(STACK_PEEK(1)))            \
        {                                                                      \
            frame->ip = ip;                                                    \
            runtime_error(vm, (error));                                        \
            free(error);                                                       \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(vm_stack_pop(vm));                                \
        double a = AS_NUMBER(vm_stack_pop(vm));                                \
        vm_stack_push(vm, value_type(a op b));                                 \
    } while (false)

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (QxlValue *slot = vm->stack; slot < vm->stack_top; slot++)
        {
            printf("[ ");
            QxlValue_print(*slot);
            printf(" ]");
        }
        printf("\n");
        debug_disassemble_instruction(&frame->fn->chunk,
                                      (int)(frame->ip - frame->fn->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
        {
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
        case OP_EQUAL:
        {
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
        {
            // String concatination
            if (IS_STRING(STACK_PEEK(0)) && IS_STRING(STACK_PEEK(1)))
            {
                QxlValue b = vm_stack_pop(vm);
                QxlValue a = vm_stack_pop(vm);
                QxlString *str =
                    QxlString_concatenate(vm, AS_STRING(a), AS_STRING(b));
                vm_stack_push(vm, OBJECT_VAL(str));
            }
            else if ((IS_STRING(STACK_PEEK(0)) && IS_NUMBER(STACK_PEEK(1))) ||
                     (IS_NUMBER(STACK_PEEK(0)) && IS_STRING(STACK_PEEK(1))))
            {
                QxlValue str_op;
                QxlString *num_op, *str;
                if (IS_OBJECT(STACK_PEEK(0)))
                {
                    str_op   = vm_stack_pop(vm);
                    char *ds = Qxl_num_as_str(AS_NUMBER(vm_stack_pop(vm)));
                    num_op   = QxlString_copy(vm, ds, strlen(ds));
                    str = QxlString_concatenate(vm, num_op, AS_STRING(str_op));
                }
                else
                {
                    char *ds = Qxl_num_as_str(AS_NUMBER(vm_stack_pop(vm)));
                    num_op   = QxlString_copy(vm, ds, strlen(ds));
                    str_op   = vm_stack_pop(vm);
                    str = QxlString_concatenate(vm, AS_STRING(str_op), num_op);
                }
                vm_stack_push(vm, OBJECT_VAL(str));
            }
            else if (IS_STRING(STACK_PEEK(0)) || IS_STRING(STACK_PEEK(1)))
            {
                frame->ip = ip;
                runtime_error(
                    vm,
                    "RuntimeError: Can only concatenate str (not '%s') to str",
                    Qxl_TYPE_NAME(
                        STACK_PEEK(IS_STRING(STACK_PEEK(0)) ? 1 : 0)));
            }
            else
            {
                char *error = Qxl_fstr("RuntimeError: Unsupported operand "
                                       "types(s) for + : '%s' and '%s'",
                                       Qxl_TYPE_NAME(STACK_PEEK(1)),
                                       Qxl_TYPE_NAME(STACK_PEEK(0)));
                BINARY_OP_(NUMBER_VAL, +, error);
            }
            break;
        }
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
        {
            if (IS_STRING(STACK_PEEK(0)) && IS_NUMBER(STACK_PEEK(1)) ||
                IS_NUMBER(STACK_PEEK(0)) && IS_STRING(STACK_PEEK(1)))
            {
                QxlValue l = vm_stack_pop(vm);
                QxlValue r = vm_stack_pop(vm);
                QxlString *str =
                    QxlString_repeat(vm, AS_STRING((IS_OBJECT(l) ? l : r)),
                                     AS_NUMBER(IS_OBJECT(l) ? r : l));
                vm_stack_push(vm, OBJECT_VAL(str));
            }
            else
            {
                char *error = Qxl_fstr("RuntimeError: Unsupported operand "
                                       "types(s) for * : '%s' and '%s'",
                                       Qxl_TYPE_NAME(STACK_PEEK(1)),
                                       Qxl_TYPE_NAME(STACK_PEEK(0)));
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
            if (!IS_NUMBER(STACK_PEEK(0)))
            {
                frame->ip = ip;
                runtime_error(vm, "operand must be a number");
                return INTERPRET_RUNTIME_ERROR;
            }
            vm_stack_push(vm, NUMBER_VAL(-AS_NUMBER(vm_stack_pop(vm))));
            break;
        case OP_PRINT:
        {
            QxlValue_print(vm_stack_pop(vm));
            printf("\n");
            break;
        }
        case OP_POP:
            vm_stack_pop(vm);
            break;
        case OP_DUP:
            vm_stack_push(vm, STACK_PEEK(0));
            break;
        case OP_DEFINE_GLOBAL:
        {
            QxlString *name = READ_STRING();
            QxlHashTable_put(&vm->globals, name, STACK_PEEK(0));
            vm_stack_pop(vm);
            break;
        }
        case OP_GET_GLOBAL:
        {
            QxlString *name = READ_STRING();
            QxlValue value;
            if (!QxlHashTable_get(&vm->globals, name, &value))
            {
                frame->ip = ip;
                runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            vm_stack_push(vm, value);
            break;
        }
        case OP_SET_GLOBAL:
        {
            QxlString *name = READ_STRING();
            if (QxlHashTable_put(&vm->globals, name, STACK_PEEK(0)))
            {
                QxlHashTable_remove(&vm->globals, name);
                frame->ip = ip;
                runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            vm_stack_push(vm, frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot       = READ_BYTE();
            frame->slots[slot] = STACK_PEEK(0);
            break;
        }
        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (is_falsey(STACK_PEEK(0))) ip += offset;
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            ip -= offset;
            break;
        }
        case OP_CALL:
        {
            int arg_count = READ_BYTE();
            frame->ip     = ip;
            if (!call_value(vm, STACK_PEEK(arg_count), arg_count))
            {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm->frames[vm->frame_count - 1];
            ip    = frame->ip;
            break;
        }
        case OP_RETURN:
        {
            QxlValue result = vm_stack_pop(vm);
            vm->frame_count--;
            if (vm->frame_count == 0)
            {
                vm_stack_pop(vm);
                return INTERPRET_OK;
            }

            vm->stack_top = frame->slots;
            vm_stack_push(vm, result);
            frame = &vm->frames[vm->frame_count - 1];
            ip    = frame->ip;
            break;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
#undef READ_STRING
}

InterpretResult
vm_interpret(VM *vm, const char *src)
{

    QxlFunction *fn = compile(src, vm);
    if (fn == NULL)
    {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_stack_push(vm, OBJECT_VAL(fn));
    call(vm, fn, 0);
    return run(vm);
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