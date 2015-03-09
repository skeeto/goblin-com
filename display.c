#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "display.h"
#include "utf.h"

static struct {
    struct {
        uint16_t c;
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
            uint16_t oldc = display.current[x][y].c;
            uint16_t newc = p->tiles[x][y].c;
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

void
display_invalidate(void)
{
    memset(display.current, 0, sizeof(display.current));
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
panel_center_init(panel_t *p, int w, int h)
{
    int x = DISPLAY_WIDTH / 2 - w / 2;
    int y = DISPLAY_HEIGHT / 2 - h / 2;
    panel_init(p, x, y, w, h);
    panel_fill(p, FONT_DEFAULT, ' ');
}

void
panel_free(panel_t *p)
{
    (void) p;
    assert(p->next == NULL);
}

void
panel_putc(panel_t *p, int x, int y, font_t font, uint16_t c)
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
    for (uint8_t *s8 = (uint8_t *)s; *s8; s8 += utf8_charlen(*s8), x++) {
        uint32_t c = utf8_to_32(s8);
        assert(c <= UINT16_MAX);
        panel_putc(p, x, y, font, c);
    }

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
panel_attr(panel_t *p, int x, int y, font_t font)
{
    x += p->x;
    y += p->y;
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        p->tiles[x][y].transparent = false;
        p->tiles[x][y].font = font;
    }
}

void
panel_erase(panel_t *p, int x, int y)
{
    p->tiles[x][y].transparent = true;
}

uint16_t
panel_getc(panel_t *p, int x, int y)
{
    return p->tiles[x + p->x][y + p->y].c;
}

void
panel_fill(panel_t *p, font_t font, uint16_t c)
{
    for (int y = 0; y < p->h; y++)
        for (int x = 0; x < p->w; x++)
            panel_putc(p, x, y, font, c);
}

void
panel_border(panel_t *p, font_t font)
{
    for (int x = 1; x < p->w - 1; x++) {
        panel_putc(p, x, 0,        font, 0x2500);
        panel_putc(p, x, p->h - 1, font, 0x2500);
    }
    for (int y = 1; y < p->h - 1; y++) {
        panel_putc(p, 0,        y, font, 0x2502);
        panel_putc(p, p->w - 1, y, font, 0x2502);
    }
    panel_putc(p, 0,        0,        font, 0x250C);
    panel_putc(p, p->w - 1, 0,        font, 0x2510);
    panel_putc(p, 0,        p->h - 1, font, 0x2514);
    panel_putc(p, p->w - 1, p->h - 1, font, 0x2518);
}
