#include "include/object.h"
#include "include/collections.h"
#include "include/common.h"
#include "include/memory.h"
#include "include/quixil.h"
#include "include/value.h"

#define ALLOCATE_OBJECT(vm, qxl_type, obj_type, obj_name)                      \
    (qxl_type *)QxlObject_allocate(vm, obj_type, sizeof(qxl_type), obj_name)

// QxlObject

static QxlObject *
QxlObject_allocate(VM *vm, QxlObjectType type, size_t size, char *obj_name)
{
    QxlObject *object = (QxlObject *)QxlMem_reallocate(NULL, 0, size);
    object->type      = type;
    object->obj_name  = obj_name;
    object->next      = vm->objects;
    vm->objects       = object;

    return object;
}

void
QxlObject_print(QxlValue value)
{
    switch (OBJECT_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    case OBJ_FUNCTION:
        if (AS_FUNCTION(value)->name == NULL)
        {
            printf("<script-main>");
            return;
        }
        printf("<function %s at %p>", AS_FUNCTION(value)->name->chars,
               (void *)&value);
        break;
    case OBJ_BUILTIN:
        printf("<built-in function %s>", AS_BUILTIN(value)->name->chars);
        break;
    }
}

// QxlString

static QxlString *
QxlString_allocate(VM *vm, char *chars, int length, uint32_t hash)
{
    QxlString *string = ALLOCATE_OBJECT(vm, QxlString, OBJ_STRING, "str");
    string->length    = length;
    string->chars     = chars;
    string->hash      = hash;
    QxlHashTable_put(&vm->strings, string, NIL_VAL);
    return string;
}

QxlString *
QxlString_copy(VM *vm, const char *chars, int length)
{
    uint32_t hash = Qxl_hash_str(chars, length);

    QxlString *interned =
        QxlHashTable_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL) return interned;

    char *heap_chars = QxlMem_Allocate(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return QxlString_allocate(vm, heap_chars, length, hash);
}

QxlString *
QxlString_take(VM *vm, char *chars, int length)
{
    uint32_t hash = Qxl_hash_str(chars, length);
    QxlString *interned =
        QxlHashTable_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL)
    {
        QxlMem_Free_Array(char, chars, length + 1);
        return interned;
    }

    return QxlString_allocate(vm, chars, length, hash);
}

QxlString *
QxlString_concatenate(VM *vm, QxlString *a, QxlString *b)
{
    int length  = a->length + b->length;
    char *chars = QxlMem_Allocate(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    return QxlString_take(vm, chars, length);
}

QxlString *
QxlString_repeat(VM *vm, QxlString *s, int count)
{
    size_t length = s->length * count;
    char *chars   = QxlMem_Allocate(char, length + 1);
    char *dest    = chars;

    if (count <= 0 || s->length == 0)
    {
        return QxlString_take(vm, "", 0);
    }

    for (size_t i = 0; i < count; i++)
    {
        memcpy(dest, s->chars, s->length);
        dest += s->length;
    }
    chars[length] = '\0';

    return QxlString_take(vm, chars, length);
}

// QxlFunction

QxlFunction *
QxlFunction_new(VM *vm)
{
    QxlFunction *fn =
        ALLOCATE_OBJECT(vm, QxlFunction, OBJ_FUNCTION, "function");
    fn->arity = 0;
    fn->name  = NULL;
    QxlChunk_init(&fn->chunk);
    return fn;
}

// QxlBuiltin

QxlBuiltin *
QxlBuiltin_new(VM *vm, QxlString *name, BuiltinFn fn)
{
    QxlBuiltin *bltin = ALLOCATE_OBJECT(vm, QxlBuiltin, OBJ_BUILTIN, "builtin");
    bltin->fn         = fn;
    bltin->name       = name;
    return bltin;
}