#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

static unsigned x = 0;
static unsigned y = 0;
static unsigned l = 0;
static char **content = NULL;

static void
load_file(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
        errx(EXIT_FAILURE, "failed to open '%s'", path);

    unsigned tmp = 1;
    char input[LINE_MAX];

    if ((content = malloc(tmp * sizeof *content)) == NULL)
        errx(EXIT_FAILURE, "failed to allocate memory");

    for (;;) {
        if (fgets(input, LINE_MAX, file) == NULL)
            break;

        if ((content[l] = malloc(LINE_MAX * sizeof *content[l])) == NULL)
            errx(EXIT_FAILURE, "failed to allocate memory");

        strncpy(content[l], input, LINE_MAX);

        if (++l == tmp)
            if ((content = realloc(content, (tmp *= 2) * sizeof *content)) == NULL)
                errx(EXIT_FAILURE, "failed to allocate memory");
    }

    if (fclose(file) == EOF)
        errx(EXIT_FAILURE, "failed to close '%s'", path);
}

static void
get_size(void)
{
    struct winsize win;

    if (ioctl(fileno(stdout), TIOCGWINSZ, &win) == -1)
        errx(EXIT_FAILURE, "failed to get terminal size");

    x = win.ws_row;
    y = win.ws_col;
}

static void
draw(int signal)
{
    (void)signal;

    printf("\033[2J\033[H");

    get_size();

    for (unsigned i = 0; i < (x - l) / 2; ++i)
        putchar('\n');

    for (unsigned i = 0; i < l; ++i)
        printf("%*s", (y + (unsigned)strnlen(content[i], LINE_MAX)) / 2, content[i]);
}

static void
cleanup(int signal)
{
    (void)signal;

    printf("\033[?25h");

    for (unsigned i = 0; i < l; ++i)
        free(content[i]);

    free(content);

    exit(EXIT_FAILURE);
}

static void
install_handler(void (*function)(int), int signal)
{
    struct sigaction sig;

    sigemptyset(&sig.sa_mask);

    sig.sa_flags = SA_RESTART;
    sig.sa_handler = function;

    if (sigaction(signal, &sig, NULL) == -1)
        errx(EXIT_FAILURE, "failed to install signal handler");
}

int
main(int argc, char **argv)
{
    load_file(argc != 1 ? argv[1] : "/dev/stdin");

    printf("\033[?25l");

    draw(0);

    install_handler(cleanup, SIGINT);
    install_handler(draw, SIGWINCH);

    for (;;)
        sleep(10);

    return EXIT_SUCCESS;
}
