#ifndef Qxl_OBJECT_H
#define Qxl_OBJECT_H

#include "chunk.h"
#include "collections.h"
#include "quixil.h"
#include "value.h"

// Forward decleration of VM to break the cycle between vm.h and object.h
typedef struct VM VM;

#ifdef __cplusplus
extern "C"
{
#endif

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) is_object_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_object_type(value, OBJ_FUNCTION)
#define IS_BUILTIN(value) is_object_type(value, OBJ_BUILTIN)
#define AS_STRING(value) ((QxlString *)AS_OBJECT(value))
#define AS_CSTRING(value) (((QxlString *)AS_OBJECT(value))->chars)
#define AS_FUNCTION(value) ((QxlFunction *)AS_OBJECT(value))
#define AS_BUILTIN(value) ((QxlBuiltin *)AS_OBJECT(value))
#define AS_BUILTIN_FUNCTION(value) (((QxlBuiltin *)AS_OBJECT(value))->fn)

    typedef enum
    {
        OBJ_STRING,
        OBJ_FUNCTION,
        OBJ_BUILTIN
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

    typedef bool (*BuiltinFn)(VM *vm, int arg_count, QxlValue *args);

    typedef struct
    {
        QxlObject obj;
        BuiltinFn fn;
        QxlString *name;
    } QxlBuiltin;

    static inline bool
    is_object_type(QxlValue value, QxlObjectType type)
    {
        return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
    }

    void QxlObject_print(QxlValue value);
    QxlString *QxlString_copy(VM *vm, const char *chars, int length);
    QxlString *QxlString_take(VM *vm, char *chars, int length);
    QxlString *QxlString_concatenate(VM *vm, QxlString *a, QxlString *b);
    QxlString *QxlString_repeat(VM *vm, QxlString *s, int count);
    QxlFunction *QxlFunction_new(VM *vm);
    QxlBuiltin *QxlBuiltin_new(VM *vm, QxlString *name, BuiltinFn fn);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_OBJECT_H */
