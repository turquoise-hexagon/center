#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*
 * handling parameters with signal handlers is complex
 * we will use global variables here for simplicity
 */
static size_t x_terminal_size = 0;
static size_t y_terminal_size = 0;
static size_t number_lines = 0;
static char **file_content = NULL;

static inline void *
allocate(size_t size)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        errx(EXIT_FAILURE, "failed to allocate memory");

    return ptr;
}

static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file = fopen(path, mode);

    if (file == NULL)
        errx(EXIT_FAILURE, "failed to open '%s'", path);

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) == EOF)
        errx(EXIT_FAILURE, "failed to close '%s'", path);
}

static void
get_terminal_size(void)
{
    struct winsize window;

    if (ioctl(fileno(stdout), TIOCGWINSZ, &window) == -1)
        errx(EXIT_FAILURE, "failed to get terminal size");

    x_terminal_size = window.ws_row;
    y_terminal_size = window.ws_col;
}

static void
install_signal_handler(void (*function)(int), int signal)
{
    struct sigaction sig;

    sigemptyset(&sig.sa_mask);

    sig.sa_flags = SA_RESTART;
    sig.sa_handler = function;

    if (sigaction(signal, &sig, NULL) == -1)
        errx(EXIT_FAILURE, "failed to install signal handler");
}

static void
load_file(const char *path)
{
    FILE *file = open_file(path, "r");

    char line[LINE_MAX] = {0};
    size_t allocated_size = 1;

    file_content = allocate(allocated_size * sizeof(*file_content));

    /* load file in memory */
    for (;;) {
        if (fgets(line, LINE_MAX, file) == NULL)
            break;

        file_content[number_lines] = allocate(LINE_MAX * sizeof(*file_content[number_lines]));

        strncpy(file_content[number_lines], line, LINE_MAX);

        if (++number_lines == allocated_size)
            if ((file_content = realloc(file_content,
                (allocated_size *= 2) * sizeof(*file_content))) == NULL)
                errx(EXIT_FAILURE, "failed to allocate memory");
    }

    close_file(path, file);
}

static void
draw_content(int signal)
{
    (void) signal; /* throw away signal */

    /*
     * 2J : clear terminal
     * H  : start printing in 0;0
     */
    printf("\033[2J\033[H");

    get_terminal_size();

    /* align vertically */
    for (size_t i = 0; i < (x_terminal_size - number_lines) / 2; ++i)
        putchar('\n');

    size_t line_length;

    for (size_t i = 0; i < number_lines; ++i) {
        line_length = strnlen(file_content[i], LINE_MAX);

        printf("%*s", (int)(y_terminal_size + line_length) / 2, file_content[i]);
    }
}

static void
cleanup_program(int signal)
{
    (void)signal; /* throw away signal */

    /* ?25h : show cursor */
    printf("\033[?25h");

    for (size_t i = 0; i < number_lines; ++i)
        free(file_content[i]);

    free(file_content);

    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
    load_file(argc != 1 ? argv[1] : "/dev/stdin");

    /* ?25l : hide cursor */
    printf("\033[?25l");

    draw_content(0);

    install_signal_handler(cleanup_program, SIGINT);
    install_signal_handler(draw_content, SIGWINCH);

    for (;;) /* keep the program busy */
        sleep(10);

    return EXIT_SUCCESS;
}
