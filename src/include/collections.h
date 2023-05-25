#ifndef Qxl_COLLECTIONS_H
#define Qxl_COLLECTIONS_H

#include "quixil.h"
#include "value.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define Qxl_COLLECTION_MAX_LOAD 0.75

    typedef struct
    {
        QxlObjectString *key;
        QxlValue value;
    } HashTableEntry;

    typedef struct
    {
        int count;
        int cap;
        HashTableEntry *entries;
    } QxlHashTable;

    void QxlHashTable_init(QxlHashTable *t);
    void QxlHashTable_free(QxlHashTable *t);
    bool QxlHashTable_put(QxlHashTable *t, QxlObjectString *k, QxlValue v);
    void QxlHashTable_merge(QxlHashTable *from, QxlHashTable *to);
    bool QxlHashTable_get(QxlHashTable *t, QxlObjectString *k, QxlValue *v);
    bool QxlHashTable_remove(QxlHashTable *t, QxlObjectString *k);
    QxlObjectString *QxlHashTable_find_string(QxlHashTable *t,
                                              const char *chars, int length,
                                              uint32_t hash);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_COLLECTIONS_H */