#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "map.h"
#include "game.h"

static bool
is_exit_key(int key)
{
    return key == 'q' || key == 27 || key == 3;
}

static void
popup_error(char *format, ...)
{
    font_t font = {COLOR_YELLOW, COLOR_BLACK, true};
    va_list ap;
    va_start(ap, format);
    int length = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    va_start(ap, format);
    char buffer[length + 1];
    vsprintf(buffer, format, ap);
    va_end(ap);
    int width = length + 2;
    int height = 3;
    panel_t popup;
    panel_center_init(&popup, width, height);
    panel_puts(&popup, 1, 1, font, buffer);
    display_push(&popup);
    while (!is_exit_key(display_getch()));
    display_pop();
    panel_free(&popup);
    display_refresh();
}

static void
popup_unknown_key(int key)
{
    popup_error("Unknown input: %d", key);
}

#define SIDEMENU_WIDTH (DISPLAY_WIDTH - MAP_WIDTH)

static void
sidemenu_draw(panel_t *p, game_t *game)
{
    font_t font_title = {COLOR_WHITE, COLOR_BLACK, false};
    panel_fill(p, font_title, ' ');
    panel_border(p, font_title);
    panel_puts(p, 5, 1, font_title, "Goblin-COM");

    font_t font_totals = {COLOR_WHITE, COLOR_BLACK, true};
    panel_printf(p, 2, 3, font_totals, "Gold: %ld", game->gold);
    panel_printf(p, 2, 4, font_totals, "Pop.: %ld", game->population);
    panel_printf(p, 2, 5, font_totals, "Wood: %ld", game->wood);

    font_t base = {COLOR_WHITE, COLOR_BLACK, false};
    font_t highlight = {COLOR_RED, COLOR_BLACK, true};
    int x = 2;
    int y = 7;
    panel_puts(p, x,   y+0, base, "Create Building");
    panel_attr(p, x+7, y+0, highlight);
    panel_puts(p, x,   y+1, base, "View Heroes");
    panel_attr(p, x+5, y+1, highlight);
    panel_puts(p, x,   y+2, base, "Show Visibility");
    panel_attr(p, x+5, y+2, highlight);

    panel_puts(p, 2, 21, base, "Speed: >>>>>");
}

static char
popup_build_select(void)
{
    int width = 30;
    int height = 16;
    panel_t build;
    panel_center_init(&build, width, height);
    display_push(&build);
    int input;
    char result = 0;
    panel_t *p = &build;
    font_t font = FONT_DEFAULT;
    panel_puts(p, 1, 1, font, "(w) Lumber Yard");
    panel_puts(p, 1, 3, font, "(f) Farm");
    panel_puts(p, 1, 5, font, "(c) Cottage");
    panel_puts(p, 1, 7, font, "(h) Hamlet");
    panel_puts(p, 1, 9, font, "(m) Mine");
    font_t highlight = {COLOR_RED, COLOR_BLACK, true};
    for (int i = 1; i <= 9; i +=2)
        panel_attr(p, 2, i, highlight);
    while (result == 0 && !is_exit_key(input = display_getch()))
        if (strchr("wfchm", input))
            result = input;
    display_pop();
    panel_free(&build);
    return result;
}

int
main(void)
{
    int w, h;
    device_size(&w, &h);
    if (w < DISPLAY_WIDTH || h < DISPLAY_HEIGHT) {
        printf("Goblin-COM requires a terminal of at least %dx%d characters!\n"
               "Press enter to exit ...", DISPLAY_WIDTH, DISPLAY_HEIGHT);
        getchar();
        exit(EXIT_FAILURE);
    }
    display_init();
    device_title("Goblin-COM");

    game_t game;
    game_init(&game, device_uepoch());

    panel_t world;
    panel_init(&world, 0, 0, MAP_WIDTH, MAP_HEIGHT);
    display_push(&world);

    panel_t sidemenu;
    panel_init(&sidemenu, DISPLAY_WIDTH - SIDEMENU_WIDTH, 0,
               SIDEMENU_WIDTH, DISPLAY_HEIGHT);
    display_push(&sidemenu);

    /* Main Loop */
    bool running = true;
    int fps = 5;
    while (running) {
        sidemenu_draw(&sidemenu, &game);
        map_draw(game.map, &world);
        display_refresh();
        if (device_kbhit(1000000 / fps)) {
            int key = device_getch();
            switch (key) {
            case 'b': {
                char build = popup_build_select();
            } break;
            case 'q':
            case 3:
                running = false;
                break;
            default:
                popup_unknown_key(key);
                break;
            }
        }
    };

    game_free(&game);

    display_pop(); // info
    display_pop(); // world
    panel_free(&sidemenu);
    panel_free(&world);
    display_free();
    return 0;
}
