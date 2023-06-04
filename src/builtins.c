#include "include/builtins.h"
#include "include/object.h"
#include "include/quixil.h"

typedef struct VM VM;

#include <time.h>

#define BUILTIN(name)                                                          \
    static QxlValue builtin_##name(VM *vm, int arg_count, QxlValue *args)

#define ADD_BUILTIN(name) Qxl_add_builtin(vm, #name, builtin_##name)

#define Qxl_CONSOLE_DISABLE_ECHO()                                             \
    do                                                                         \
    {                                                                          \
        struct termios term;                                                   \
        tcgetattr(STDIN_FILENO, &term);                                        \
        term.c_lflag &= ~ECHO;                                                 \
        tcsetattr(STDIN_FILENO, TCSANOW, &term);                               \
    } while (0)

#define Qxl_CONSOLE_ENABLE_ECHO()                                              \
    do                                                                         \
    {                                                                          \
        struct termios term;                                                   \
        tcgetattr(STDIN_FILENO, &term);                                        \
        term.c_lflag |= ECHO;                                                  \
        tcsetattr(STDIN_FILENO, TCSANOW, &term);                               \
    } while (0)

/*
    clock as builtin_clock
    Returns the elapsed time in seconds as a floating-point number since the
    start of the program.
*/
BUILTIN(clock)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/*
    input as builtin_input
        prompt: string? [nil]
        hidden: bool?   [false]

    Read a string from standard input. The trailing newline is stripped.
    The prompt string, if given, is printed to standard output without a
    trailing newline before reading input.
*/
BUILTIN(input)
{
    const char *prompt = "Enter a string : ";
    bool hidden        = true;

    if (!isatty(STDIN_FILENO))
    {
        // TODO: handle later
    }

    if (!isatty(STDOUT_FILENO))
    {
        // TODO: handle later
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
        Qxl_CONSOLE_DISABLE_ECHO();
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
        Qxl_CONSOLE_ENABLE_ECHO();
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

    input = realloc(input, sizeof(char) * (length + 1));
    return OBJECT_VAL(QxlString_copy(vm, input, length));
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
builitins_init(VM *vm)
{
    ADD_BUILTIN(clock);
    ADD_BUILTIN(input);
}