#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <sys/time.h>
#include "device.h"

static HANDLE console_out;
static HANDLE console_in;

void
device_init(void)
{
    console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    console_in = GetStdHandle(STD_INPUT_HANDLE);
}

void
device_free(void)
{
    /* Nothing */
}

void
device_move(int x, int y)
{
    SetConsoleCursorPosition(console_out, (COORD){x, y});
}

void
device_cursor(bool show)
{
    CONSOLE_CURSOR_INFO info = {100, show};
    SetConsoleCursorInfo(console_out, &info);
}

void
device_putc(font_t font, char c)
{
    WORD color = 0;
    switch (font.fore) {
    case COLOR_BLUE:
        color |= FOREGROUND_BLUE;
        break;
    case COLOR_GREEN:
        color |= FOREGROUND_GREEN;
        break;
    case COLOR_RED:
        color |= FOREGROUND_RED;
        break;
    case COLOR_WHITE:
        color |= FOREGROUND_RED;
        color |= FOREGROUND_GREEN;
        color |= FOREGROUND_BLUE;
        break;
    case COLOR_YELLOW:
        color |= FOREGROUND_RED;
        color |= FOREGROUND_GREEN;
        break;
    case COLOR_MAGENTA:
        color |= FOREGROUND_RED;
        color |= FOREGROUND_BLUE;
        break;
    case COLOR_CYAN:
        color |= FOREGROUND_GREEN;
        color |= FOREGROUND_BLUE;
        break;
    }
    switch (font.back) {
    case COLOR_BLUE:
        color |= BACKGROUND_BLUE;
        break;
    case COLOR_GREEN:
        color |= BACKGROUND_GREEN;
        break;
    case COLOR_RED:
        color |= BACKGROUND_RED;
        break;
    case COLOR_WHITE:
        color |= BACKGROUND_RED;
        color |= BACKGROUND_GREEN;
        color |= BACKGROUND_BLUE;
        break;
    case COLOR_YELLOW:
        color |= BACKGROUND_RED;
        color |= BACKGROUND_GREEN;
        break;
    case COLOR_MAGENTA:
        color |= BACKGROUND_RED;
        color |= BACKGROUND_BLUE;
        break;
    case COLOR_CYAN:
        color |= BACKGROUND_GREEN;
        color |= BACKGROUND_BLUE;
        break;
    }
    if (font.bold)
        color |= FOREGROUND_INTENSITY;
    SetConsoleTextAttribute(console_out, color);
    WriteConsole(console_out, &c, 1, NULL, NULL);
}

void
device_flush(void)
{
    /* Nothing */
}

int
device_getch(void)
{
    return getch();
}

/* http://stackoverflow.com/a/21749034 */
bool
device_kbhit(uint64_t useconds)
{
    DWORD mseconds = useconds / 1000ULL;
    for (;;) {
        DWORD result = WaitForSingleObject(console_in, mseconds);
        if (result == WAIT_TIMEOUT) {
            return false;
        } else if (_kbhit()) {
            return true;
        } else {
            /* Throw away non-input event. */
            INPUT_RECORD r;
            DWORD read;
            ReadConsoleInput(console_in, &r, 1, &read);
            continue;
        }
    }
    return false;
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
    SetConsoleTitle(title);
}
