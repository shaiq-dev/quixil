#include "include/memory.h"

void *
QxlMem_reallocate(void *ptr, size_t size, size_t new_size)
{
    if (new_size == 0)
    {
        free(ptr);
        return NULL;
    }

    void *result = realloc(ptr, new_size);
    if (result == NULL)
    {
        exit(1);
    }
    return result;
}

static void
QxlMem_free_object(QxlObject *obj)
{
    switch (obj->type)
    {
    case OBJ_STRING:
    {
        QxlString *string = (QxlString *)obj;
        QxlMem_Free_Array(char, string->chars, string->length + 1);
        QxlMem_Free(QxlString, obj);
        break;
    }
    case OBJ_FUNCTION:
    {
        QxlFunction *fn = (QxlFunction *)obj;
        QxlChunk_free(&fn->chunk);
        QxlMem_Free(QxlFunction, obj);
        break;
    }
    case OBJ_BUILTIN:
        QxlMem_Free(QxlBuiltin, obj);
        break;
    }
}

void *
QxlMem_free_objects(VM *vm)
{
    QxlObject *obj = vm->objects;
    while (obj != NULL)
    {
        QxlObject *next = obj->next;
        QxlMem_free_object(obj);
        obj = next;
    }
}