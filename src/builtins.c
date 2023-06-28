#include "include/builtins.h"
#include "include/common.h"
#include "include/object.h"
#include "include/quixil.h"
#include "include/value.h"

typedef struct VM VM;

#define BUILTIN(name)                                                          \
    static bool builtin_##name(VM *vm, int arg_count, QxlValue *args)
#define ADD_BUILTIN(name) Qxl_add_builtin(vm, #name, builtin_##name)
#define VALIDATE_ARGS(arg_count, req_count, has_default)                       \
    ((arg_count == 0 && has_default) || (arg_count == req_count))
#define ARGS_ERROR(name, req_count)                                            \
    char *err = Qxl_fstr("%s() takes exactly %d arguments (%d given)", name,   \
                         req_count, arg_count);                                \
    args[-1]  = OBJECT_VAL(QxlString_copy(vm, err, strlen(err)));

/*
    clock as builtin_clock
    Returns the elapsed time in seconds as a floating-point number since the
    start of the program.
*/
BUILTIN(clock)
{
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

/*
    input as builtin_input
        prompt: string? [nil]
        hidden: bool?   [false]

    Read a string from standard input. The trailing newline is stripped.
    The prompt string, if given, is printed to standard output without a
    trailing newline before reading input. Echoing to console is disabled
    if hidden is true.
*/
BUILTIN(input)
{
    if (!VALIDATE_ARGS(arg_count, 2, true))
    {
        ARGS_ERROR("input", 2);
        return false;
    }

    char *prompt = NULL;
    bool hidden  = false;

    if (arg_count == 2)
    {
        prompt = AS_STRING(args[0])->chars;
        hidden = AS_BOOL(args[1]);
    }

    if (!isatty(STDIN_FILENO))
    {
        // TODO: stdin not available
    }

    if (!isatty(STDOUT_FILENO))
    {
        // TODO: stdout not available
    }

    if (prompt != NULL)
    {
        printf("%s", prompt);
        fflush(stdout);
    }

    char *input   = (char *)malloc(sizeof(char) * 100);
    size_t size   = 100;
    size_t length = 0;

    if (hidden)
    {
        // Windows platform or DOS specfic implementation
#ifdef _WIN32
        int i   = 0;
        char ch = 0;
        while ((ch = _getch()) != '\r')
        {
            if (ch == '\b' && i > 0)
            {
                printf("\b \b");
                i--;
                length--;
            }
            else if (ch != '\b')
            {
                input[i++] = ch;
                length++;

                // Resize the input buffer if necessary
                if (length >= size)
                {
                    size *= 2;
                    input = (char *)realloc(input, sizeof(char) * size);
                }
            }
        }
        input[i] = '\0';
        printf("\n");
#else
        // UNIX platform specific implementation
        struct termios term;
        tcgetattr(STDIN_FILENO, &term);

        // Disable echo
        term.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
        while (1)
        {
            int c = getchar();
            if (c == '\r' || c == '\n')
                break;
            else if (c == 127 && length > 0)
            {
                printf("\b \b");
                length--;
            }
            else
            {
                input[length++] = (char)c;

                // Resize the input buffer if necessary
                if (length >= size)
                {
                    size *= 2;
                    input = (char *)realloc(input, sizeof(char) * size);
                }
            }
        }
        // Enable echo
        term.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
        printf("\n");
#endif
    }
    else
    {
        // Regular input without hiding
        getline(&input, &size, stdin);
        length = strlen(input);

        if (length > 0 && input[length - 1] == '\n')
        {
            input[length - 1] = '\0';
            length--;
        }
    }

    input    = realloc(input, sizeof(char) * (length + 1));
    args[-1] = OBJECT_VAL(QxlString_copy(vm, input, length));
    return true;
}

static void
Qxl_add_builtin(VM *vm, const char *name, BuiltinFn fn)
{
    QxlString *fn_name = QxlString_copy(vm, name, (int)strlen(name));
    vm_stack_push(vm, OBJECT_VAL(fn_name));
    vm_stack_push(vm, OBJECT_VAL(QxlBuiltin_new(vm, fn_name, fn)));
    QxlHashTable_put(&vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_stack_pop(vm);
    vm_stack_pop(vm);
}

void
builtins_init(VM *vm)
{
    ADD_BUILTIN(clock);
    ADD_BUILTIN(input);
}