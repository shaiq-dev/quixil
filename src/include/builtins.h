#ifndef Qxl_BUILTINS_H
#define Qxl_BUILTINS_H

#include "quixil.h"
#include "value.h"
#include "vm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void builtins_init(VM *vm);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_BUILTINS_H */