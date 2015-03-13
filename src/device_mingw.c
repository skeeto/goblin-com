#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include "display.h"
#include "rand.h"
#include "device.h"

static CHAR_INFO buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
static HANDLE console_out;
static HANDLE console_in;
static bool cursor_visible = true;
static int cursor_x, cursor_y;

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
    cursor_x = x;
    cursor_y = y;
}

void
device_cursor_show(bool show)
{
    if (cursor_visible != show) {
        cursor_visible = show;
        CONSOLE_CURSOR_INFO info = {100, show};
        SetConsoleCursorInfo(console_out, &info);
    }
}

bool
device_cursor_get(int *x, int *y)
{
    if (x)
        *x = cursor_x;
    if (y)
        *y = cursor_y;
    return cursor_visible;
}

void
device_putc(font_t font, uint16_t c)
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
    if (font.fore_bright)
        color |= FOREGROUND_INTENSITY;
    if (font.back_bright)
        color |= BACKGROUND_INTENSITY;
    buffer[cursor_y][cursor_x].Char.UnicodeChar = c;
    buffer[cursor_y][cursor_x].Attributes = color;
    cursor_x++;
}

void
device_flush(void)
{
    COORD size = {
        .X = DISPLAY_WIDTH,
        .Y = DISPLAY_HEIGHT
    };
    COORD origin = {0, 0};
    SMALL_RECT area = {
        .Left = 0,
        .Top = 0,
        .Right = DISPLAY_WIDTH,
        .Bottom = DISPLAY_HEIGHT,
    };
    WriteConsoleOutputW(console_out, buffer[0], size, origin, &area);
}

int
device_getch(void)
{
    int result = getch();
    if (result != 0xE0 && result != 0x00) {
        return result;
    } else {
        result = getch();
        switch (result) {
        case 72:
            return ARROW_U;
        case 80:
            return ARROW_D;
        case 75:
            return ARROW_L;
        case 77:
            return ARROW_R;
        case 71:
            return ARROW_UL;
        case 73:
            return ARROW_UR;
        case 79:
            return ARROW_DL;
        case 81:
            return ARROW_DR;
        default:
            return result + 256;
        }
    }
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

/* http://stackoverflow.com/a/4568846 */
uint64_t
device_uepoch(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t tt = ft.dwHighDateTime;
    tt <<= 32;
    tt |= ft.dwLowDateTime;
    tt /=10;
    tt -= UINT64_C(11644473600000000);
    return tt;
}

void
device_title(const char *title)
{
    SetConsoleTitle(title);
}

void
device_terminal_size(int *width, int *height)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void
device_entropy(void *buffer, size_t size)
{
    HCRYPTPROV h = 0;
    DWORD type = PROV_RSA_FULL;
    DWORD flags = CRYPT_VERIFYCONTEXT | CRYPT_SILENT;
    if (!CryptAcquireContext(&h, 0, 0, type, flags) ||
        !CryptGenRandom(h, size, buffer)) {
        /* Fallback */
        uint32_t pid = GetProcessId(GetCurrentProcess());
        uint64_t state = device_uepoch() ^ (pid < 32);
        xorshift_fill(&state, buffer, size);
    }
    if (h)
        CryptReleaseContext(h, 0);
}
