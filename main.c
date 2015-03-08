#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "display.h"
#include "map.h"
#include "game.h"

#define FPS 6
#define PERIOD (1000000 / 6)

static bool
is_exit_key(int key)
{
    return key == 'q' || key == 27 || key == 3;
}

static int
game_getch(game_t *game, panel_t *world)
{
    for (;;) {
        map_draw(game->map, world);
        display_refresh();
        uint64_t wait = device_uepoch() % PERIOD;
        if (device_kbhit(wait))
            return device_getch();
    }
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
    for (;;) {
        int key = display_getch();
        if (is_exit_key(key) || key == 13 || key == ' ')
            break;
    }
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
popup_build_select(game_t *game, panel_t *world)
{
    int width = 56;
    int height = 13;
    panel_t build;
    panel_center_init(&build, width, height);
    display_push(&build);
    font_t border = {COLOR_WHITE, COLOR_BLACK, false};
    panel_border(&build, border);

    int input;
    char result = 0;
    panel_t *p = &build;
    font_t item = {COLOR_WHITE, COLOR_BLACK, true};
    font_t desc = {COLOR_WHITE, COLOR_BLACK, false};
    panel_puts(p, 1, 1, item, "(w) Lumberyard [-2 gold]");
    panel_puts(p, 5, 2, desc, "Place on forest (#) [+4 wood/t]");
    panel_puts(p, 1, 3, item, "(f) Farm       [-10 wood]");
    panel_puts(p, 5, 4, desc, "Place on grassland (.), forest (#) [+4 food/t]");
    panel_puts(p, 1, 5, item, "(c) Stable     [-25 wood]");
    panel_puts(p, 5, 6, desc,
               "Place on grassland (.) [-1 food/t, +2 hero slots]");
    panel_puts(p, 1, 7, item, "(m) Mine       [-100 wood]");
    panel_puts(p, 5, 8, desc, "Place on hill (=)");
    panel_puts(p, 1, 9, item, "(h) Hamlet     [-20 wood]");
    panel_puts(p, 5, 10, desc, "Place on forest (#), grass (.), or hill (=)");
    panel_puts(p, 7, 11, desc, "[-2 food/t, +2 gold/t, +10 workers]");
    font_t highlight = {COLOR_RED, COLOR_BLACK, true};
    for (int i = 1; i <= 9; i +=2)
        panel_attr(p, 2, i, highlight);
    while (result == 0 && !is_exit_key(input = game_getch(game, world)))
        if (strchr("wfchm", input))
            result = toupper(input);
    display_pop();
    panel_free(&build);
    return result;
}

static bool
arrow_adjust(int input, int *x, int *y)
{
    switch (input) {
    case ARROW_U:
        (*y)--;
        return true;
    case ARROW_D:
        (*y)++;
        return true;
    case ARROW_L:
        (*x)--;
        return true;
    case ARROW_R:
        (*x)++;
        return true;
    case ARROW_UL:
        (*x)--;
        (*y)--;
        return true;
    case ARROW_UR:
        (*x)++;
        (*y)--;
        return true;
    case ARROW_DL:
        (*x)--;
        (*y)++;
        return true;
    case ARROW_DR:
        (*x)++;
        (*y)++;
        return true;
    }
    return false;
}

static bool
select_position(game_t *game, panel_t *world, int *x, int *y)
{
    font_t highlight = {COLOR_WHITE, COLOR_RED, true};
    *x = MAP_WIDTH / 2;
    *y = MAP_HEIGHT / 2;
    bool selected = false;
    panel_t overlay;
    panel_init(&overlay, 0, 0, MAP_WIDTH, MAP_HEIGHT);
    panel_putc(&overlay, *x, *y, highlight, panel_getc(world, *x, *y));
    display_push(&overlay);
    int input;
    while (!selected && !is_exit_key(input = game_getch(game, world))) {
        panel_erase(&overlay, *x, *y);
        arrow_adjust(input, x, y);
        panel_putc(&overlay, *x, *y, highlight, panel_getc(world, *x, *y));
        if (input == 13)
            selected = true;
    }
    display_pop();
    panel_free(&overlay);
    return selected;
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
    while (running) {
        sidemenu_draw(&sidemenu, &game);
        map_draw(game.map, &world);
        display_refresh();
        if (device_kbhit(PERIOD)) {
            int key = device_getch();
            switch (key) {
            case 'b': {
                char building = popup_build_select(&game, &world);
                if (building) {
                    int x, y;
                    if (select_position(&game, &world, &x, &y))
                        if (!game_build(&game, building, x, y))
                            popup_error("Invalid building location!");
                }
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
