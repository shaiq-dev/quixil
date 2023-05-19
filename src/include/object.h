#ifndef Qxl_OBJECT_H
#define Qxl_OBJECT_H

#include "quixil.h"
#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJECT_TYPE(value)              (AS_OBJECT(value)->type)
#define OBJECT_IS_STRING(value)         is_object_type(value, OBJ_STRING)
#define OBJECT_AS_STRING(value)         ((QxlObjectString*)AS_OBJECT(value))
#define OBJECT_AS_CSTRING(value)        (((QxlObjectString*)AS_OBJECT(value))->chars)

typedef enum {
    OBJ_STRING,
} QxlObjectType;

struct QxlObject {
    QxlObjectType type;
    struct QxlObject *next;
    char *obj_name;
};

struct QxlObjectString {
    QxlObject obj;
    int length;
    char* chars;
};

static inline bool 
is_object_type(QxlValue value, QxlObjectType type) 
{
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

void QxlObject_print(QxlValue value);
QxlObjectString* QxlObjectString_copy(const char *chars, int length);
QxlObjectString* QxlObjectString_take(char *chars, int length);

// String Methods
QxlObjectString *QxlString_concatenate(QxlObjectString *a, QxlObjectString *b);
QxlObjectString *QxlString_repeat(QxlObjectString *s,  int count);


#ifdef __cplusplus
}
#endif

#endif /* Qxl_OBJECT_H */

