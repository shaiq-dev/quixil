#ifndef Qxl_MEMORY_H
#define Qxl_MEMORY_H

#include "quixil.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QXL_DEFAULT_MEM_SIZE 8

#define QxlMem_Resize(n)                                                       \
    ((n) < QXL_DEFAULT_MEM_SIZE ? QXL_DEFAULT_MEM_SIZE : (n)*2)

#define QxlMem_Allocate(type, count)                                           \
    (type *)QxlMem_reallocate(NULL, 0, sizeof(type) * count)

#define QxlMem_Realloc(type, ptr, size, new_size)                              \
    ((type *)QxlMem_reallocate((ptr), sizeof(type) * (size),                   \
                               sizeof(type) * (new_size)))

#define QxlMem_Free(type, ptr, size)                                           \
    QxlMem_reallocate((ptr), sizeof(type) * (size), 0)

    void *QxlMem_reallocate(void *ptr, size_t size, size_t new_size);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_MEMORY_H */
