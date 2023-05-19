#include "include/common.h"

char* 
Qxl_fstr(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t len = vsnprintf(NULL, 0, format, args);

    char* result = (char*)malloc((len + 1) * sizeof(char));
    if (result == NULL) {
        va_end(args);
        return NULL;
    }

    vsnprintf(result, len + 1, format, args);
    va_end(args);
    return result;
}