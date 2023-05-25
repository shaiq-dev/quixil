#include "include/value.h"
#include "include/memory.h"
#include "include/object.h"
#include "include/quixil.h"

void
QxlValueList_init(QxlValueList *list)
{
    list->count  = 0;
    list->cap    = 0;
    list->values = NULL;
}

void
QxlValueList_push(QxlValueList *list, QxlValue value)
{
    if (list->cap < list->count + 1)
    {
        size_t old_cap = list->cap;
        list->cap      = QxlMem_Resize(old_cap);
        list->values =
            QxlMem_Realloc(QxlValue, list->values, old_cap, list->cap);
    }

    list->values[list->count] = value;
    list->count++;
}

void
QxlValueList_free(QxlValueList *list)
{
    QxlMem_Free(QxlValue, list->values, list->cap);
    QxlValueList_init(list);
}

void
QxlValue_print(QxlValue value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJECT:
        QxlObject_print(value);
        break;
    }
}

bool
QxlValue_are_equal(QxlValue a, QxlValue b)
{
    if (a.type != b.type) return false;
    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJECT:
        return AS_OBJECT(a) == AS_OBJECT(b);
    default:
        return false; // Unreachable
    }
}