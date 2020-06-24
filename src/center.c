#include <sys/ioctl.h>

#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

/*
 * handling parameters with signal handles is complicated
 * we're gonna use global variables here, since the program is short
 */
static size_t x_size = 0;
static size_t y_size = 0;
static size_t file_length = 0;
static char **file_content = NULL;

/*
 * helper functions
 */
static inline void *
allocate(size_t size)
{
    void *ptr;

    if ((ptr = malloc(size)) == NULL)
        errx(1, "failed to allocate memory");

    return ptr;
}

static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file;

    if ((file = fopen(path, mode)) == NULL)
        errx(1, "failed to open '%s'", path);

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) == EOF)
        errx(1, "failed to close '%s'", path);
}

static void
get_terminal_size(void)
{
    struct winsize window;

    if (ioctl(fileno(stdout), TIOCGWINSZ, &window) == -1)
        errx(1, "failed to get terminal size");

    x_size = window.ws_row;
    y_size = window.ws_col;
}

static void
install_signal_handler(void (*function)(int), int signal)
{
    struct sigaction sig;

    sigemptyset(&sig.sa_mask);

    sig.sa_flags = SA_RESTART;
    sig.sa_handler = function;

    if (sigaction(signal, &sig, NULL) == -1)
        errx(1, "failed to install signal handler");
}

/*
 * main functions
 */
static void
load_file(const char *path)
{
    FILE *file;

    file = open_file(path, "r");

    char line[LINE_MAX] = {0};
    size_t allocated_size = 1;

    file_content = allocate(allocated_size * sizeof(*file_content));

    for (;;) {
        if (fgets(line, LINE_MAX, file) == NULL)
            break;

        file_content[file_length] = allocate(LINE_MAX *
            sizeof(*file_content[file_length]));

        strncpy(file_content[file_length], line, LINE_MAX);

        if (++file_length == allocated_size)
            if ((file_content = realloc(file_content,
                    (allocated_size *= 2) * sizeof(*file_content))) == NULL)
                errx(1, "failed to allocate memory");
    }

    close_file(path, file);
}

static void
draw_content(int signal)
{
    (void)signal; /* throw away signal */

    /*
     * 2J : clear terminal
     *  H : start printing in (0, 0)
     */
    printf("\033[2J\033[H");

    get_terminal_size();

    /* align vertically */
    for (size_t i = 0; i < (x_size - file_length) / 2; ++i)
        putchar('\n');

    /* align horizontally */
    size_t length;

    for (size_t i = 0; i < file_length; ++i) {
        length = strnlen(file_content[i], LINE_MAX);

        printf("%*s", (int)(y_size + length) / 2, file_content[i]);
    }
}

static noreturn void
cleanup(int signal)
{
    (void)signal; /* throw away signal */

    /* ?25h : show cursor */
    printf("\033[?25h");

    for (size_t i = 0; i < file_length; ++i)
        free(file_content[i]);

    free(file_content);

    exit(1);
}

int
main(int argc, char **argv)
{
    load_file(argc != 1 ? argv[1] : "/dev/stdin");

    /* ?25l : hide cursor */
    printf("\033[?25l");

    draw_content(0);

    install_signal_handler(cleanup, SIGINT);
    install_signal_handler(draw_content, SIGWINCH);

    for (;;) /* keep the program busy */
        sleep(10);
}
