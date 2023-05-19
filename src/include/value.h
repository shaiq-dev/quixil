#ifndef Qxl_VALUE_H
#define Qxl_VALUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QxlObject QxlObject;
typedef struct QxlObjectString QxlObjectString;

typedef enum {
    VAL_BOOL,
    VAL_NIL, 
    VAL_NUMBER,
    VAL_OBJECT,
} QxlValueType;

typedef struct {
    QxlValueType type;
    union {
        bool boolean;
        double number;
        QxlObject* obj;
    } as;
    char *type_name;
} QxlValue;

typedef struct {
    size_t count;       // in use
    size_t cap;         // allocated elements
    QxlValue *values;   // constant values
} QxlValueList;

#define BOOL_VAL(value)     ((QxlValue){VAL_BOOL, {.boolean = value}, "bool"})
#define NIL_VAL             ((QxlValue){VAL_NIL, {.number = 0}, "nil"})
#define NUMBER_VAL(value)   ((QxlValue){VAL_NUMBER, {.number = value}, "number"})
#define OBJECT_VAL(object)  ((QxlValue){VAL_OBJECT, {.obj = (QxlObject*)object}, "object"})
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
#define AS_OBJECT(value)    ((value).as.obj)
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJECT(value)    ((value).type == VAL_OBJECT)

#define Qxl_TYPE_NAME(value) (  IS_OBJECT(value) ? AS_OBJECT(value)->obj_name : (value).type_name )

void QxlValueList_init(QxlValueList *list);
void QxlValueList_push(QxlValueList *list, QxlValue value);
void QxlValueList_free(QxlValueList *list);

void QxlValue_print(QxlValue value);
bool QxlValue_are_equal(QxlValue a, QxlValue b);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_VALUE_H */

