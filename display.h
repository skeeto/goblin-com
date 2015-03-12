#pragma once

#include <stdbool.h>
#include "device.h"

#define DISPLAY_WIDTH  80
#define DISPLAY_HEIGHT 24

typedef struct panel {
    int x, y, w, h;
    struct {
        uint16_t c;
        bool transparent;
        font_t font;
    } tiles[DISPLAY_WIDTH][DISPLAY_HEIGHT];
    struct panel *next;
} panel_t;

void display_init(void);
void display_free(void);
void display_push(panel_t *);
void display_pop(void);
void display_refresh(void);
void display_invalidate(void);
int  display_getch(void);

void     panel_init(panel_t *, int x, int y, int w, int h);
void     panel_center_init(panel_t *, int w, int h);
void     panel_free(panel_t *);
void     panel_putc(panel_t *, int x, int y, font_t, uint16_t);
void     panel_puts(panel_t *, int x, int y, font_t, const char *);
void     panel_printf(panel_t *, int x, int y, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)));
void     panel_attr(panel_t *, int x, int y, font_t);
void     panel_erase(panel_t *, int x, int y);
void     panel_clear(panel_t *);
uint16_t panel_getc(panel_t *, int x, int y);
void     panel_fill(panel_t *, font_t, uint16_t);
void     panel_border(panel_t *, font_t);
