#include "include/quixil.h"
#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"

#define ALLOCATE_OBJECT(type, obj_type, obj_name) \
    (type*)allocate_object(sizeof(type), obj_type, obj_name)

static QxlObject* 
allocate_object(size_t size, QxlObjectType type, char *obj_name) 
{
    QxlObject* object = (QxlObject*)QxlMem_reallocate(NULL, 0, size);
    object->type = type;
    object->obj_name = obj_name;
    return object;
}

static QxlObjectString* 
allocate_string(char* chars, int length) 
{
    QxlObjectString *string = ALLOCATE_OBJECT(QxlObjectString, OBJ_STRING, "str");
    string->length = length;
    string->chars = chars;
    return string;
}

QxlObjectString* 
QxlObjectString_copy(const char *chars, int length)
{
    char* heap_chars = QxlMem_Allocate(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length);
}

QxlObjectString* 
QxlObjectString_take(char *chars, int length)
{
    return allocate_string(chars, length);
}

void 
QxlObject_print(QxlValue value)
{
    switch (OBJECT_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", OBJECT_AS_CSTRING(value));
            break;
    }
}


// String Methods
QxlObjectString*
QxlString_concatenate(QxlObjectString *a, QxlObjectString *b)
{
    int length = a->length + b->length;
    char* chars = QxlMem_Allocate(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    return QxlObjectString_take(chars, length);
}

QxlObjectString*
QxlString_repeat(QxlObjectString *s,  int count)
{
    size_t length = s->length * count;
    char *chars = QxlMem_Allocate(char, length + 1);
    char *dest = chars;

    if (count <= 0 || s->length == 0) {
        return QxlObjectString_take("", 0);
    }

    for (size_t i = 0; i < count; i++) {
        memcpy(dest, s->chars, s->length);
        dest += s->length;
    }
    chars[length] = '\0';

    return QxlObjectString_take(chars, length);
}
