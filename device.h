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

#define FONT_DEFAULT ((font_t){COLOR_WHITE, COLOR_BLACK, true})

typedef struct {
    uint8_t fore, back;
    bool bold;
} font_t;

static inline bool
font_equal(font_t a, font_t b)
{
    return a.fore == b.fore && a.back == b.back && a.bold == b.bold;
}

void     device_init(void);
void     device_free(void);
void     device_move(int x, int y);
void     device_cursor(bool show);
void     device_putc(font_t font, char c);
void     device_flush(void);
int      device_getch(void);
bool     device_kbhit(uint64_t);
uint64_t device_uepoch(void);
void     device_title(const char *);
void     device_size(int *, int *);