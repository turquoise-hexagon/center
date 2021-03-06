#include <sys/ioctl.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

static size_t x            = 0;
static size_t y            = 0;
static size_t content_size = 0;
static char **content      = NULL;

#define ERROR(code, ...) {        \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(code);                   \
}

static inline void *
allocate(size_t size)
{
    void *ptr;

    if (! (ptr = malloc(size)))
        ERROR(1, "error : failed to allocate '%lu' bytes of memory\n", size);

    return ptr;
}

static void *
reallocate(void *old, size_t size)
{
    void *new;

    if (! (new = realloc(old, size)))
        ERROR(1, "error : failed to reallocate '%lu' bytes of memory\n", size);

    return new;
}

static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file;

    if (! (file = fopen(path, mode)))
        ERROR(1, "error : failed to open '%s'\n", path);

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) != 0)
        ERROR(1, "error : failed to close '%s'\n", path);
}

static void
get_terminal_size(void)
{
    struct winsize window;

    if (ioctl(fileno(stdout), TIOCGWINSZ, &window) == -1)
        ERROR(1, "error : failed to get terminal size\n");

    x = window.ws_row;
    y = window.ws_col;
}

static char *
copy_input(const char *str)
{
    char *cpy;

    {
        size_t len;

        len = strnlen(str, LINE_MAX);

        if (! (cpy = strndup(str, len)))
            ERROR(1, "error : failed to duplicated string\n");

        /* fix string */
        cpy[len - 1] = 0;
    }

    return cpy;
}

static void
load_content(const char *path)
{
    FILE *file;

    file = open_file(path, "r");

    {
        size_t allocated = 2;
        size_t assigned  = 0;

        content = allocate(allocated * sizeof(*content));

        char input[LINE_MAX] = {0};

        while (fgets(input, LINE_MAX, file)) {
            content[assigned] = copy_input(input);

            if (++assigned == allocated)
                content = reallocate(content,
                    (allocated = allocated * 3 / 2) * sizeof(*content));
        }

        content_size = assigned;
    }

    close_file(path, file);
}

static void
cleanup_content(void)
{
    for (size_t i = 0; i < content_size; ++i)
        free(content[i]);

    free(content);
}

static void
install_signal_handler(void (*function)(int), int signal)
{
    struct sigaction sig;

    sigemptyset(&sig.sa_mask);

    sig.sa_flags = SA_RESTART;
    sig.sa_handler = function;

    if (sigaction(signal, &sig, NULL) == -1)
        ERROR(1, "error : failed to install signal handler\n");
}

static void
draw_content(int signal)
{
    /* throw signal away */
    (void)signal;

    /*
     * ?25l : hide cursor
     * 2J   : clear terminal
     * x;yH : move cursor to x;y
     */
    get_terminal_size();
    printf("\033[?25l\033[2J\033[H");
    printf("\033[%ldH", (x - content_size) / 2 + 1);

    for (size_t i = 0; i < content_size; ++i) {
        size_t len;

        len = strnlen(content[i], LINE_MAX);
        printf("%*s\n", (int)(y + len) / 2, content[i]);
    }
}

static noreturn void
cleanup(int signal)
{
    /* throw signal away */
    (void)signal;

    /* ?25h : show cursor */
    printf("\033[?25h");
    cleanup_content();
    exit(1);
}

int
main(int argc, char **argv)
{
    load_content(argc != 1 ? argv[1] : "/dev/stdin");

    draw_content(0);

    install_signal_handler(cleanup, SIGINT);
    install_signal_handler(draw_content, SIGWINCH);

    for (;;)
        sleep(10);
}
