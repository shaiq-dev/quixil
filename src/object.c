#include "include/object.h"
#include "include/collections.h"
#include "include/common.h"
#include "include/memory.h"
#include "include/quixil.h"
#include "include/value.h"

#define ALLOCATE_OBJECT(type, obj_type, obj_name)                              \
    (type *)allocate_object(sizeof(type), obj_type, obj_name)

static QxlObject *
allocate_object(size_t size, QxlObjectType type, char *obj_name)
{
    QxlObject *object = (QxlObject *)QxlMem_reallocate(NULL, 0, size);
    object->type      = type;
    object->obj_name  = obj_name;
    return object;
}

static QxlObjectString *
allocate_string(QxlHashTable *vm_global_strings, char *chars, int length,
                uint32_t hash)
{
    QxlObjectString *string =
        ALLOCATE_OBJECT(QxlObjectString, OBJ_STRING, "str");
    string->length = length;
    string->chars  = chars;
    string->hash   = hash;
    QxlHashTable_put(vm_global_strings, string, NIL_VAL);
    return string;
}

QxlObjectString *
QxlObjectString_copy(QxlHashTable *vm_global_strings, const char *chars,
                     int length)
{
    uint32_t hash = Qxl_hash_str(chars, length);

    QxlObjectString *interned =
        QxlHashTable_find_string(vm_global_strings, chars, length, hash);
    if (interned != NULL) return interned;

    char *heap_chars = QxlMem_Allocate(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(vm_global_strings, heap_chars, length, hash);
}

QxlObjectString *
QxlObjectString_take(QxlHashTable *vm_global_strings, char *chars, int length)
{
    uint32_t hash = Qxl_hash_str(chars, length);
    QxlObjectString *interned =
        QxlHashTable_find_string(vm_global_strings, chars, length, hash);
    if (interned != NULL)
    {
        QxlMem_Free(char, chars, length + 1);
        return interned;
    }

    return allocate_string(vm_global_strings, chars, length, hash);
}

void
QxlObject_print(QxlValue value)
{
    switch (OBJECT_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", OBJECT_AS_CSTRING(value));
        break;
    }
}

// String Methods
QxlObjectString *
QxlString_concatenate(QxlHashTable *vm_global_strings, QxlObjectString *a,
                      QxlObjectString *b)
{
    int length  = a->length + b->length;
    char *chars = QxlMem_Allocate(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    return QxlObjectString_take(vm_global_strings, chars, length);
}

QxlObjectString *
QxlString_repeat(QxlHashTable *vm_global_strings, QxlObjectString *s, int count)
{
    size_t length = s->length * count;
    char *chars   = QxlMem_Allocate(char, length + 1);
    char *dest    = chars;

    if (count <= 0 || s->length == 0)
    {
        return QxlObjectString_take(vm_global_strings, "", 0);
    }

    for (size_t i = 0; i < count; i++)
    {
        memcpy(dest, s->chars, s->length);
        dest += s->length;
    }
    chars[length] = '\0';

    return QxlObjectString_take(vm_global_strings, chars, length);
}
