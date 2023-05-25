#ifndef Qxl_COMPILER_H
#define Qxl_COMPILER_H

#include "chunk.h"
#include "object.h"
#include "scanner.h"
#include "vm.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

    typedef struct
    {
        Scanner *s;
        Token cur;
        Token prev;
        bool had_error;
        bool panic_mode;
        QxlChunk *compiling_chunk;

        /*
        Parsers needs access to the vm's strings table to
        perform string interning. Thus attaching the vm's
        global string table here.
        */
        QxlHashTable *vm_global_strings;
    } Parser;

    typedef enum
    {
        PREC_NONE,
        PREC_LOWEST,
        PREC_ASSIGNMENT,  // =
        PREC_CONDITIONAL, // ?:
        PREC_OR,          // or
        PREC_AND,         // and
        PREC_EQUALITY,    // == !=
        PREC_COMPARISON,  // < > <= >=
        PREC_TERM,        // + -
        PREC_FACTOR,      // * /
        PREC_UNARY,       // ! -
        PREC_CALL,        // . ()
        PREC_PRIMARY
    } Precedence;

    typedef struct
    {
        Token name;
        int depth;
    } Local;

    typedef struct
    {
        Local locals[UINT8_COUNT];
        int local_count;
        int scope_depth;
        Parser *p;
    } Compiler;

    typedef void (*ParseFn)(Compiler *c, bool can_assign);

    typedef struct
    {
        ParseFn prefix;
        ParseFn infix;
        Precedence precedence;
    } ParseRule;

    Parser *parser_init(Scanner *s, QxlChunk *c,
                        QxlHashTable *vm_global_strings);
    bool compile(QxlChunk *chunk, const char *src,
                 QxlHashTable *vm_global_strings);
    Compiler *compiler_init(Parser *p);

#ifdef __cplusplus
}
#endif

#endif /* Qxl_COMPILER_H */
