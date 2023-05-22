#include "include/collections.h"
#include "include/memory.h"
#include "include/value.h"
#include "include/object.h"

static HashTableEntry* 
find_entry(HashTableEntry *entries, int cap, QxlObjectString *k) 
{
    HashTableEntry *tombstone = NULL;
    uint32_t index = k->hash % cap;

    for (;;) {
        HashTableEntry *entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry
                return tombstone != NULL ? tombstone : entry;
            } else {
                // A tombstone
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == k) {
            return entry;
        }

        index = (index + 1) % cap;
    }
}

static void 
adjust_cap(QxlHashTable* t, int cap) 
{
    HashTableEntry *entries = QxlMem_Allocate(HashTableEntry, cap);
    for (int i = 0; i < cap; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    t->count = 0;
    for (int i = 0; i < t->cap; i++) {
        HashTableEntry* entry = &t->entries[i];
        if (entry->key == NULL) continue;

        HashTableEntry *dest = find_entry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        t->count++;
    }

    QxlMem_Free(HashTableEntry, t->entries, t->cap);


    t->entries = entries;
    t->cap = cap;
}

void 
QxlHashTable_init(QxlHashTable *t)
{
    t->count = 0;
    t->cap = 0;
    t->entries = NULL;
}

void 
QxlHashTable_free(QxlHashTable *t)
{
    QxlMem_Free(HashTableEntry, t->entries, t->cap);
    QxlHashTable_init(t);
}

bool 
QxlHashTable_put(QxlHashTable *t, QxlObjectString *k, QxlValue v)
{
    if (t->count + 1 > t->cap * Qxl_COLLECTION_MAX_LOAD) {
        int cap = QxlMem_Resize(t->cap);
        adjust_cap(t, cap);
    } 

    HashTableEntry* entry = find_entry(t->entries, t->cap, k);
    bool is_new_key = entry->key == NULL;
    if (is_new_key && IS_NIL(entry->value)) t->count++;

    entry->key = k;
    entry->value = v;

    return is_new_key;
}

void 
QxlHashTable_merge(QxlHashTable *from, QxlHashTable *to)
{
    for (int i = 0; i < from->cap; i++) {
        HashTableEntry* entry = &from->entries[i];
        if (entry->key != NULL) {
            QxlHashTable_put(to, entry->key, entry->value);
        }
    }
}

bool 
QxlHashTable_get(QxlHashTable *t, QxlObjectString *k, QxlValue *v)
{
    if (t->count == 0) return false;

    HashTableEntry *entry = find_entry(t->entries, t->cap, k);

    if (entry->key == NULL) return false;

    *v = entry->value;
    return true;
}

bool 
QxlHashTable_remove(QxlHashTable *t, QxlObjectString *k)
{
    if (t->count == 0) return false;

    HashTableEntry* entry = find_entry(t->entries, t->cap, k);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

QxlObjectString* 
QxlHashTable_find_string(QxlHashTable *t, const char* chars, int length, uint32_t hash)
{
    if (t->count == 0) return NULL;

    uint32_t index = hash % t->cap;
    for (;;) {
        HashTableEntry *entry = &t->entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % t->cap;
    }
}
