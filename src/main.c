#include "include/chunk.h"
#include "include/debug.h"
#include "include/quixil.h"
#include "include/vm.h"

static void Qxl_main (int argc, const char *argv[]);
static char *Qxl_read_source (const char *path);
static void Qxl_run_vm (const char *path);

int
main (int argc, const char *argv[])
{
    Qxl_main (argc, argv);
}

static void
Qxl_main (int argc, const char *argv[])
{
    if (argc == 2)
    {
        return Qxl_run_vm (argv[1]);
    }

    Qxl_ERROR ("A runtime error occured");
}

static char *
Qxl_read_source (const char *path)
{
    FILE *file = fopen (path, "rb");
    if (file == NULL)
    {
        Qxl_ERROR ("Could not open file \"%s\"", path);
        exit (74);
    }

    fseek (file, 0L, SEEK_END);
    size_t file_size = ftell (file);
    rewind (file);

    char *buffer = (char *)malloc (file_size + 1);

    if (buffer == NULL)
    {
        Qxl_ERROR ("Not enough memory to read \"%s\"", path);
        exit (74);
    }

    size_t bytes_read = fread (buffer, sizeof (char), file_size, file);

    if (bytes_read < file_size)
    {
        Qxl_ERROR ("Could not read file \"%s\"", path);
        exit (74);
    }

    buffer[bytes_read] = '\0';
    fclose (file);

    return buffer;
}

static void
Qxl_run_vm (const char *path)
{
    const char *buf     = Qxl_read_source (path);
    InterpretResult res = vm_interpret (vm_init (), buf);
    free (buf);

    Qxl_INTERCEPT_ERROR (res, INTERPRET_COMPILE_ERROR, 65);
    Qxl_INTERCEPT_ERROR (res, INTERPRET_RUNTIME_ERROR, 70);
}