#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "display.h"

static struct {
    struct {
        char c;
        font_t font;
    } current[DISPLAY_WIDTH][DISPLAY_HEIGHT];
    panel_t base;
    panel_t *panels;
} display;

void
display_init(void)
{
    device_init();
    panel_init(&display.base, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
        for (int x = 0; x < DISPLAY_WIDTH; x++)
            panel_putc(&display.base, x, y, FONT_DEFAULT, ' ');
    display.panels = &display.base;
    device_cursor(false);
    display_refresh();
}

void
display_free()
{
    device_cursor(true);
    device_move(0, DISPLAY_HEIGHT);
    panel_free(&display.base);
    device_free();
    assert(display.panels == &display.base);
}

void
display_push(panel_t *p)
{
    p->next = display.panels;
    display.panels = p;
}

void
display_pop(void)
{
    panel_t *discard = display.panels;
    display.panels = display.panels->next;
    discard->next = NULL;
}

void
display_refresh(void)
{
    int cx = 0;
    int cy = 0;
    device_move(cx, cy);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            panel_t *p = display.panels;
            while (p->tiles[x][y].transparent)
                p = p->next;
            char oldc = display.current[x][y].c;
            char newc = p->tiles[x][y].c;
            font_t oldf = display.current[x][y].font;
            font_t newf = p->tiles[x][y].font;
            if (oldc != newc || !font_equal(oldf, newf)) {
                if (cx != x || cy != y)
                    device_move(cx = x, cy = y);
                device_putc(newf, newc);
                display.current[x][y].font = newf;
                display.current[x][y].c = newc;
                cx++;
            }
        }
    }
    device_flush();
}

int
display_getch(void)
{
    display_refresh();
    return device_getch();
}

/* Panels */

void
panel_init(panel_t *p, int x, int y, int w, int h)
{
    p->x = x;
    p->y = y;
    p->w = w;
    p->h = h;
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
        for (int x = 0; x < DISPLAY_WIDTH; x++)
            p->tiles[x][y].transparent = true;
    p->next = NULL;
}

void
panel_free(panel_t *p)
{
    assert(p->next == NULL);
}

void
panel_putc(panel_t *p, int x, int y, font_t font, char c)
{
    x += p->x;
    y += p->y;
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        p->tiles[x][y].transparent = false;
        p->tiles[x][y].c = c;
        p->tiles[x][y].font = font;
    }
}

void
panel_puts(panel_t *p, int x, int y, font_t font, char *s)
{
    for (; *s; s++, x++)
        panel_putc(p, x, y, font, *s);
}

void
panel_printf(panel_t *p, int x, int y, font_t font, char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int length = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    char buffer[length + 1];
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
    panel_puts(p, x, y, font, buffer);
}

void
panel_fill(panel_t *p, font_t font, char c)
{
    for (int y = 0; y < p->h; y++)
        for (int x = 0; x < p->w; x++)
            panel_putc(p, x, y, font, c);
}

void
panel_border(panel_t *p, font_t font)
{
    for (int x = 1; x < p->w - 1; x++) {
        panel_putc(p, x, 0,        font, '-');
        panel_putc(p, x, p->h - 1, font, '-');
    }
    for (int y = 1; y < p->h - 1; y++) {
        panel_putc(p, 0,        y, font, '|');
        panel_putc(p, p->w - 1, y, font, '|');
    }
    panel_putc(p, 0,        0,        font, '/');
    panel_putc(p, p->w - 1, 0,        font, '\\');
    panel_putc(p, 0,        p->h - 1, font, '\\');
    panel_putc(p, p->w - 1, p->h - 1, font, '/');
}
