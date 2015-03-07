#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include "device.h"

#define FONT_INVALID {-1, -1, -1};
static font_t device_font_last = FONT_INVALID;

struct termios termios_orig;

void
device_init(void)
{
    printf("\e[2J");
    tcgetattr(STDIN_FILENO, &termios_orig);
    struct termios raw;
    memcpy(&raw, &termios_orig, sizeof(raw));
    raw.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
    raw.c_oflag &= ~OPOST;
    raw.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    raw.c_cflag &= ~(CSIZE|PARENB);
    raw.c_cflag |= CS8;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void
device_free(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_orig);
    printf("\e[m");
}

void
device_move(int x, int y)
{
    device_font_last = (font_t)FONT_INVALID;
    printf("\e[%d;%dH", y + 1, x + 1);
}

void
device_cursor(bool show)
{
    printf("\e[?25%c", show ? 'h' : 'l');
}

void
device_putc(font_t font, char c)
{
    if (font_equal(device_font_last, font))
        putchar(c);
    else
        printf("\e[%d;%d%sm%c", font.fore + 30, font.back + 40,
               font.bold ? ";1" : "", c);
    device_font_last = font;
}

void
device_flush(void)
{
    fflush(stdout);
}

int
device_getch(void)
{
    int r;
    unsigned char c;
    if ((r = read(STDIN_FILENO, &c, sizeof(c))) < 0) {
        return r;
    } else if (c == '\e') {
        if (!device_kbhit(0))
            return c;
        unsigned char code[2];
        read(STDIN_FILENO, code, sizeof(code));
        return code[1] + 256;
    } else {
        return c;
    }
}

/* http://stackoverflow.com/questions/448944/c-non-blocking-keyboard-input */
bool
device_kbhit(uint64_t useconds)
{
    struct timeval tv = {useconds / 1000000, useconds % 1000000};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

uint64_t
device_uepoch(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

void
device_title(const char *title)
{
    printf("\e]2;%s\a", title);
}

void
device_size(int *width, int *height)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *width = w.ws_col;
    *height = w.ws_row;
}
