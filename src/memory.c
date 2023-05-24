#include "include/memory.h"

void *
QxlMem_reallocate (void *ptr, size_t size, size_t new_size)
{
    if (new_size == 0)
    {
        free (ptr);
        return NULL;
    }

    void *result = realloc (ptr, new_size);
    if (result == NULL)
    {
        exit (1);
    }
    return result;
}