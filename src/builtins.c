#include "include/builtins.h"
#include "include/object.h"
#include "include/vm.h"

#include <time.h>

#define BUILTIN(name)                                                          \
    static QxlValue builtin_##name(int arg_count, QxlValue *args)

/*
    clock as builtin_clock
    Returns the current timestamp in uix format
*/
BUILTIN(clock)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void
Qxl_add_builtin(VM *vm, const char *name, BuiltinFn fn)
{
    QxlString *fn_name = QxlString_copy(vm, name, (int)strlen(name));
    vm_stack_push(vm, OBJECT_VAL(fn_name));
    vm_stack_push(vm, OBJECT_VAL(QxlBuiltin_new(vm, fn_name, fn)));
    QxlHashTable_put(&vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_stack_pop(vm);
    vm_stack_pop(vm);
}

void
builitins_init(VM *vm)
{
    Qxl_add_builtin(vm, "clock", builtin_clock);
}