/**
 * Platform layer. All of the platform specific calls, especially
 * those for communicating with the terminal (the "device"), are
 * captured behind this interface.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ARROW_U  321
#define ARROW_D  322
#define ARROW_L  324
#define ARROW_R  323
#define ARROW_UL 305
#define ARROW_DL 308
#define ARROW_UR 309
#define ARROW_DR 310

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define FONT_DEFAULT ((font_t){COLOR_WHITE, COLOR_BLACK, true, false})

typedef struct {
    uint8_t fore, back;
    bool fore_bright, back_bright;
} font_t;

static inline bool
font_equal(font_t a, font_t b)
{
    return a.fore == b.fore && a.back == b.back &&
        a.fore_bright == b.fore_bright && a.back_bright && b.back_bright;
}

void     device_init(void);
void     device_free(void);
void     device_move(int x, int y);
void     device_cursor_show(bool show);
bool     device_cursor_get(int *x, int *y);
void     device_putc(font_t font, uint16_t c);
void     device_flush(void);
int      device_getch(void);
bool     device_kbhit(uint64_t);
uint64_t device_uepoch(void);
void     device_title(const char *);
void     device_terminal_size(int *, int *);
void     device_entropy(void *, size_t);

/* Shorthand Font Literals */

#define COLOR__FONT_R (0x10 | COLOR_RED)
#define COLOR__FONT_G (0x10 | COLOR_GREEN)
#define COLOR__FONT_B (0x10 | COLOR_BLUE)
#define COLOR__FONT_C (0x10 | COLOR_CYAN)
#define COLOR__FONT_M (0x10 | COLOR_MAGENTA)
#define COLOR__FONT_Y (0x10 | COLOR_YELLOW)
#define COLOR__FONT_K (0x10 | COLOR_BLACK)
#define COLOR__FONT_W (0x10 | COLOR_WHITE)

#define COLOR__FONT_r COLOR_RED
#define COLOR__FONT_g COLOR_GREEN
#define COLOR__FONT_b COLOR_BLUE
#define COLOR__FONT_c COLOR_CYAN
#define COLOR__FONT_m COLOR_MAGENTA
#define COLOR__FONT_y COLOR_YELLOW
#define COLOR__FONT_k COLOR_BLACK
#define COLOR__FONT_w COLOR_WHITE

#define FONT_STATIC(f, b)                               \
    {COLOR__FONT_##f & 0x0F, COLOR__FONT_##b & 0xF,     \
     COLOR__FONT_##f & 0x10, COLOR__FONT_##b & 0x10}

#define FONT(f, b) ((font_t) FONT_STATIC(f, b))
