#ifndef Qxl_OBJECT_H
#define Qxl_OBJECT_H

#include "chunk.h"
#include "collections.h"
#include "quixil.h"
#include "value.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) is_object_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_object_type(value, OBJ_FUNCTION)
#define AS_STRING(value) ((QxlString *)AS_OBJECT(value))
#define AS_CSTRING(value) (((QxlString *)AS_OBJECT(value))->chars)
#define AS_FUNCTION(value) ((QxlFunction *)AS_OBJECT(value))

    typedef enum
    {
        OBJ_STRING,
        OBJ_FUNCTION
    } QxlObjectType;

    struct QxlObject
    {
        QxlObjectType type;
        struct QxlObject *next;
        char *obj_name;
    };

    struct QxlString
    {
        QxlObject obj;
        int length;
        char *chars;
        uint32_t hash;
    };

    typedef struct
    {
        QxlObject obj;
        int arity;
        QxlChunk chunk;
        QxlString *name;
    } QxlFunction;

    static inline bool
    is_object_type(QxlValue value, QxlObjectType type)
    {
        return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
    }

    void QxlObject_print(QxlValue value);

    // String Object
    QxlString *QxlString_copy(QxlHashTable *vm_global_strings,
                              const char *chars, int length);
    QxlString *QxlString_take(QxlHashTable *vm_global_strings, char *chars,
                              int length);

    QxlString *QxlString_concatenate(QxlHashTable *vm_global_strings,
                                     QxlString *a, QxlString *b);
    QxlString *QxlString_repeat(QxlHashTable *vm_global_strings, QxlString *s,
                                int count);

    //   Function Object
    QxlFunction *QxlFunction_new();

#ifdef __cplusplus
}
#endif

#endif /* Qxl_OBJECT_H */
