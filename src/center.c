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
static char **input = NULL;

static void
load_file(char *name)
{
    FILE *file = fopen(name, "r");

    if (file == NULL)
        errx(EXIT_FAILURE, "failed to open '%s'", name);

    unsigned tmp = 1;
    char cur[LINE_MAX];
    
    if ((input = malloc(tmp * sizeof *input)) == NULL)
        errx(EXIT_FAILURE, "failed to allocate memory");

    for (;;) {
        if (fgets(cur, LINE_MAX, file) == NULL)
            break;

        if ((input[l] = malloc(LINE_MAX * sizeof *input[l])) == NULL)
            errx(EXIT_FAILURE, "failed to allocate memory");

        strncpy(input[l], cur, LINE_MAX);

        if (++l == tmp)
            if ((input = realloc(input, (tmp *= 2) * sizeof *input)) == NULL)
                errx(EXIT_FAILURE, "failed to allocate memory");
    }

    if (fclose(file) == EOF)
        errx(EXIT_FAILURE, "failed to close input");
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
        printf("%*s", (y + (unsigned)strnlen(input[i], LINE_MAX)) / 2, input[i]);
}

static void
cleanup(int signal)
{
    (void)signal;

    printf("\033[?25h");

    for (unsigned i = 0; i < l; ++i)
        free(input[i]);

    free(input);

    exit(EXIT_FAILURE);
}
    
int
main(int argc, char **argv)
{
    load_file(argc != 1 ? argv[1] : "/dev/stdin");

    printf("\033[?25l");

    draw(0);

    struct sigaction sig_int;
    sig_int.sa_handler = cleanup;

    if (sigaction(SIGINT, &sig_int, NULL) == -1)
        errx(EXIT_FAILURE, "failed to install signal handler");

    struct sigaction sig_winch;
    sig_winch.sa_handler = draw;

    if (sigaction(SIGWINCH, &sig_winch, NULL) == -1)
        errx(EXIT_FAILURE, "failed to install signal handler");

    for (;;)
        sleep(10);

    return EXIT_SUCCESS;
}
