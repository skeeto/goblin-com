#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include "display.h"
#include "rand.h"
#include "map.h"
#include "game.h"
#include "utf.h"

#define FPS 15
#define PERIOD (1000000 / FPS)
#define SPEED_MAX 7776
#define SPEED_FACTOR 6
#define PERSIST_FILE "persist.gcom"

static const font_t font_error = FONT_STATIC(Y, k);

static bool
is_exit_key(int key)
{
    return key == 'Q' || key == 'q' || key == 27;
}

static int
game_getch(game_t *game, panel_t *terrain)
{
    for (;;) {
        map_draw_terrain(game->map, terrain);
        display_refresh();
        uint64_t wait = device_uepoch() % PERIOD;
        if (device_kbhit(wait))
            return device_getch();
    }
}

static void
popup_message(font_t font, char *format, ...)
{
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
    display_pop_free();
    display_refresh();
}

static bool
popup_quit(bool saving)
{
    panel_t popup;
    char *message;
    if (saving)
        message = "Really save and quit? (Rk{y}/Rk{n})";
    else
        message = "Really quit Yk{without saving}? (Rk{y}/Rk{n})";
    size_t length = strlen(message) - 8;
    panel_center_init(&popup, length + 2, 3);
    panel_printf(&popup, 1, 1, message);
    display_push(&popup);
    display_refresh();
    int input = device_getch();
    display_pop_free();
    display_refresh();
    if (input == 'y' || input == 'Y')
        return true;
    return false;
}

#define SIDEMENU_WIDTH (DISPLAY_WIDTH - MAP_WIDTH)

static int
sideinfo(panel_t *p, char *message)
{
    panel_init(p, DISPLAY_WIDTH - SIDEMENU_WIDTH, 0,
               SIDEMENU_WIDTH, DISPLAY_HEIGHT);
    display_push(p);
    panel_fill(p, FONT(K, k), 0x2591);
    panel_border(p, FONT(K, k));
    int y = DISPLAY_HEIGHT / 2 - 1;
    int x = p->w / 2 - panel_strlen(message) / 2 - 1;
    panel_printf(p, x, y, message);
    return y;
}

static void
sidemenu_draw(panel_t *p, game_t *game, yield_t diff)
{
    font_t font_title = FONT(w, k);
    panel_fill(p, font_title, ' ');
    panel_border(p, font_title);
    panel_puts(p, 5, 1, font_title, "Goblin-COM");

    font_t font_totals = FONT(W, k);
    int ty = 3;
    panel_printf(p, 2, ty++, "Gold: Yk{%ld}wk{%+d}",
                 (long)game->gold, (int)diff.gold);
    panel_printf(p, 2, ty++, "Food: Yk{%ld}wk{%+d}",
                 (long)game->food, (int)diff.food);
    panel_printf(p, 2, ty++, "Wood: Yk{%ld}wk{%+d}",
                 (long)game->wood, (int)diff.wood);
    panel_printf(p, 2, ty++, "Pop.: %ld", (long)game->population);

    int x = 2;
    int y = 8;
    panel_printf(p, x, y++, "Kk{♦}    wk{Rk{B}uild}     Kk{♦}");
    panel_printf(p, x, y++, "Kk{♦}    wk{Rk{H}eroes}    Kk{♦}");
    panel_printf(p, x, y++, "Kk{♦}    wk{Rk{S}quads}    Kk{♦}");

    y = 17;
    panel_printf(p, x, y++, "Kk{♦}    wk{SRk{t}ory}     Kk{♦}");
    panel_printf(p, x, y++, "Kk{♦}     wk{HelRk{p}}     Kk{♦}");

    char date[128];
    game_date(game, date);
    panel_puts(p, 2, 20, font_totals, date);

    font_t base = FONT(w, k);
    panel_puts(p, 2, 21, base, "Speed: ");
    for (int x = 0, i = 1; i <= game->speed; i *= SPEED_FACTOR, x++)
        panel_puts(p, 9 + x, 21, font_totals, ">");
}

/* Sloppy, but it works! */
static inline char *
u8encode(uint16_t c)
{
    static int i = 0;
    static char buf[8][7];
    buf[i][utf32_to_8(c, (uint8_t *)buf[i % 8])] = '\0';
    return buf[i++ % 8];
}

static uint16_t
popup_build_select(game_t *game, panel_t *terrain)
{
    int width = 56;
    int height = 20;
    panel_t build;
    panel_center_init(&build, width, height);
    display_push(&build);
    panel_border(&build, FONT(w, k));

    int input;
    uint16_t result = 0;
    panel_t *p = &build;
    char cost[128];
    char yield[128];

    int y = 1;
    yield_string(cost, COST_LUMBERYARD, false);
    yield_string(yield, YIELD_LUMBERYARD, true);
    panel_printf(p,   1, y++, "(Rk{w}) Yk{Lumberyard} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}", yield);
    panel_printf(p, 5, y++, "wk{Target: forest (%s)}",
                 u8encode(BASE_FOREST));

    yield_string(cost, COST_FARM, false);
    yield_string(yield, YIELD_FARM, true);
    panel_printf(p, 1, y++, "(Rk{f}) Yk{Farm} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}", yield);
    panel_printf(p, 5, y++, "wk{Target: grassland (%s), forest (%s)}",
                 u8encode(BASE_GRASSLAND), u8encode(BASE_FOREST));

    yield_string(cost, COST_STABLE, false);
    yield_string(yield, YIELD_STABLE, true);
    panel_printf(p, 1, y++, "(Rk{s}) Yk{Stable} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}, gk{adds %d hero slots}",
                 yield, STABLE_INC);
    panel_printf(p, 5, y++, "wk{Target: grassland (%s)}",
                 u8encode(BASE_GRASSLAND));

    yield_string(cost, COST_MINE, false);
    yield_string(yield, YIELD_MINE, true);
    panel_printf(p, 1, y++, "(Rk{m}) Yk{Mine} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}", yield);
    panel_printf(p, 5, y++, "wk{Target: hill (%s)}",
                 u8encode(BASE_HILL));

    yield_string(cost, COST_HAMLET, false);
    yield_string(yield, YIELD_HAMLET, true);
    panel_printf(p, 1, y++, "(Rk{h}) Yk{Hamlet} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}, gk{adds %d pop.}", yield,
                 HAMLET_INC);
    panel_printf(p, 5, y++,
                 "wk{Target: grassland (%s), forest (%s), hill (%s)}",
                 u8encode(BASE_GRASSLAND),
                 u8encode(BASE_FOREST),
                 u8encode(BASE_HILL));

    yield_string(cost, COST_ROAD, false);
    yield_string(yield, YIELD_ROAD, true);
    panel_printf(p, 1, y++, "(Rk{r}) Yk{Road} [%s]", cost);
    panel_printf(p, 5, y++, "wk{Yield: %s}, "
                 "gk{removes movement penalties}", yield);
    panel_printf(p, 5, y++, "wk{Target: (any land)}");

    while (result == 0 && !is_exit_key(input = game_getch(game, terrain)))
        if (strchr("wfshmr", input))
            result = toupper(input);
    if (result == 'R')
        result = '+';
    display_pop_free();
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
    panel_t info;
    int sidey = sideinfo(&info, "Yk{Select Location}");
    panel_printf(&info, 6, sidey + 1, "Use Rk{←↑→↓}");

    font_t highlight = FONT(W, r);
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

    display_pop_free(); // overlay
    display_pop_free(); // info
    return selected;
}

static bool
can_afford(game_t *game, yield_t yield)
{
    return
        (yield.food == 0 || game->food >= yield.food) &&
        (yield.wood == 0 || game->wood >= yield.wood) &&
        (yield.gold == 0 || game->gold >= yield.gold);
}

static bool
persist(game_t *game)
{
    FILE *save = fopen(PERSIST_FILE, "wb");
    if (save) {
        game_save(game, save);
        return fclose(save) == 0;
    }
    return false;
}

static game_t *atexit_save_game;
static void
atexit_save(void)
{
    if (atexit_save_game)
        persist(atexit_save_game);
}

static void
ui_build(game_t *game, panel_t *terrain)
{
    uint16_t building;
    while ((building = popup_build_select(game, terrain))) {
        yield_t cost = building_cost(building);
        if (!can_afford(game, cost)) {
            popup_message(font_error, "Not enough funding/materials!");
        } else {
            int x = MAP_WIDTH / 2;
            int y = MAP_HEIGHT / 2;
            while (select_position(game, terrain, &x, &y)) {
                if (!game_build(game, building, x, y))
                    popup_message(font_error, "Invalid building location!");
                else
                    break;
            }
            break;
        }
    }

}

static int
select_target(game_t *game, panel_t *terrain, panel_t *units)
{
    panel_t info;
    sideinfo(&info, "Yk{Select Target}");

    int key = 0;
    int result = -1;
    do {
        if (key >= 'a' && key < 'a' + (int)(countof(game->invaders))) {
            result = key - 'a';
            break;
        }
        game_draw_units(game, units, true);
    } while (!is_exit_key(key = game_getch(game, terrain)));

    display_pop_free();
    return result;
}

static void
ui_squads(game_t *game, panel_t *terrain, panel_t *units)
{
    panel_t p;
    panel_center_init(&p, 29, countof(game->squads) + 3);
    panel_border(&p, FONT(w, k));
    panel_printf(&p, 1, 1, "wk{Squad Size Status}");
    display_push(&p);
    int key = 0;
    do {
        if (key >= 'a' && key < 'a' + (int)countof(game->squads)) {
            display_pop();
            int target = select_target(game, terrain, units);;
            game->squads[key - 'a'].target = target;
            display_push(&p);
            break;
        }
        for (unsigned i = 0; i < countof(game->squads); i++) {
            squad_t *s = game->squads + i;
            char status[32];
            if (s->member_count == 0)
                sprintf(status, "Kk{Empty}");
            else if (s->target < 0)
                sprintf(status, "Ck{Idle/Waiting}");
            else
                sprintf(status, "Rk{Intercepting %c}", s->target + 'A');
            panel_printf(&p, 1, i + 2, "Yk{%-5c} %-4u %-16s",
                         i + 'A', s->member_count, status);
        }
    } while (!is_exit_key(key = game_getch(game, terrain)));
    display_pop_free();
}

static void
ui_hire(game_t *game, panel_t *terrain, int slot)
{
    int w = 46;
    int h = 14;
    panel_t listing;
    panel_center_init(&listing, w, h);
    display_push(&listing);
    panel_fill(&listing, FONT_DEFAULT, ' ');
    panel_border(&listing, FONT(Y, k));
    panel_printf(&listing, w / 2 - 7, 1, "yk{Hire New Hero}");
    panel_printf(&listing, 1, 2,
                 "wk{  Name               HP   AP  STR  DEX MIND}");
    hero_t candidates[HERO_CANDIDATES];
    for (unsigned i = 0; i < countof(candidates); i++) {
        hero_t h = candidates[i] = game_hero_generate();
        panel_printf(&listing, 1, i + 3,
                     "Rk{%c} Ck{%-16s} %4d %4d %4d %4d %4d",
                     'A' + i, h.name,
                     h.hp_max, h.ap_max, h.str, h.dex, h.mind);
    }
    int key = 0;
    while (!is_exit_key(key = game_getch(game, terrain))) {
        if (key >= 'a' && key < 'a' + (int)countof(candidates)) {
            game->heroes[slot] = candidates[key - 'a'];
            break;
        }
    }
    display_pop_free();
}

static void
ui_heroes(game_t *game, panel_t *terrain)
{
    panel_t p;
    int w = 50;
    int h = 22;
    panel_center_init(&p, w, h);
    display_push(&p);

    int per_page = h - 3;
    int total = countof(game->heroes);
    int page_max = total / per_page;
    int page = 0;
    int selection = 0;
    int key = 0;
    do {
        panel_fill(&p, FONT_DEFAULT, ' ');
        panel_border(&p, FONT(w, k));
        panel_printf(&p, 1, 1,
                     "wk{Name             Squad   HP   AP  STR  DEX MIND}");
        switch (key) {
        case ARROW_U:
            if (selection > 0)
                selection--;
            break;
        case ARROW_D:
            if (selection < (page + 1) * per_page - 1)
                selection++;
            break;
        case '>':
            if (page < page_max) {
                page++;
                selection = page * per_page;
            }
            break;
        case '<':
            if (page > 0) {
                page--;
                selection = page * per_page;
            }
            break;
        case '+':
        case '=':
        case '-': {
            hero_t *h = game->heroes + selection;
            if (h->active) {
                int new_squad = h->squad + (key == '-' ? -1 : 1);
                if (new_squad >= -1 && new_squad < (int)countof(game->squads)) {
                    if (h->squad >= 0)
                        game->squads[h->squad].member_count--;
                    h->squad = new_squad;
                    if (h->squad >= 0)
                        game->squads[h->squad].member_count++;
                }
            }
        } break;
        case 13: {
            hero_t *h = game->heroes + selection;
            if (!h->active && selection < game->max_hero)
                ui_hire(game, terrain, selection);
        }break;
        }
        panel_printf(&p, 1, h - 1, "Rk{<} wk{Page %d} Rk{>}", page + 1);
        for (int i = 0; i < per_page; i++) {
            int si = page * per_page + i;
            if (si > total)
                break;
            hero_t *h = game->heroes + si;
            char format[] = "Ck{%-16s} Rk{ }Yk{%4c}Rk{ }%4d %4d %4d %4d %4d";
            if (selection == si) {
                format[1] = 'r';
                format[13] = '-';
                format[25] = '+';
            }
            if (!h->active) {
                format[0] = 'K';
                format[9] = '\0';
                if (si < game->max_hero) {
                    strcpy(h->name, "(Rk{enter} to hire)");
                    if (selection == si)
                        h->name[2] = 'r';
                } else {
                    h->name[0] = '\0';
                }
            }
            panel_printf(&p, 1, i + 2, format,
                         h->name, h->squad < 0 ? ' ' : h->squad + 'A',
                         h->hp_max, h->ap_max,
                         h->str, h->dex, h->mind);
        }
    } while (!is_exit_key(key = game_getch(game, terrain)));
    display_pop_free();
}

static inline const char *
text_next_line(const char *p)
{
    for (; *p != '\n'; p++);
    return p + 1;
}

static inline const char *
text_prev_line(const char *p)
{
    p -= 2;
    for (; *p != '\n'; p--);
    return p + 1;
}

static inline int
text_numlines(const char *p)
{
    int count = 0;
    for (; *p != '@'; p = text_next_line(p))
        count++;
    return count;
}

static inline int
text_linelen(const char *p)
{
    int count = 0;
    for (; *p != '\n'; p++)
        count++;
    return count;
}

static void
text_page(game_t *game, panel_t *terrain, const char *p, int w, int h)
{
    panel_t page;
    panel_center_init(&page, w + 4, h + 2);
    display_push(&page);
    font_t border = FONT(K, k);
    int numlines = text_numlines(p);
    int topline = 0;
    const char *top = p;

    int key = 0;
    do {
        switch (key) {
        case ARROW_D:
            if (topline < numlines - h) {
                topline++;
                top = text_next_line(top);
            }
            break;
        case ARROW_U:
            if (topline > 1) {
                topline--;
                top = text_prev_line(top);
            } else if (topline == 1) {
                topline = 0;
                top = p;
            }
            break;
        }
        panel_fill(&page, FONT_DEFAULT, ' ');
        panel_border(&page, border);
        if (numlines > h) {
            panel_printf(&page, w + 3, 1, "Rk{↑}");
            int o = (topline / (float)(numlines - h)) * (h - 3);
            panel_printf(&page, w + 3, o + 2, "wk{o}");
            panel_printf(&page, w + 3, h, "Rk{↓}");
        }
        const char *line = top;
        for (int y = 0; y < h && topline + y < numlines; y++) {
            int length = text_linelen(line);
            char copy[length + 1];
            memcpy(copy, line, length);
            copy[length] = '\0';
            panel_printf(&page, 2, y + 1, copy);
            line = text_next_line(line);
        }
    } while (!is_exit_key(key = game_getch(game, terrain)));

    display_pop_free();
}

static void
ui_story(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_story_txt_start[];
    text_page(game, terrain, _binary_doc_story_txt_start, 60, 20);
}

static void
ui_help(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_help_txt_start[];
    text_page(game, terrain, _binary_doc_help_txt_start, 60, 19);
}

static void
ui_gameover(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_game_over_txt_start[];
    text_page(game, terrain, _binary_doc_game_over_txt_start, 60, 16);
}

static void
ui_halfway(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_halfway_txt_start[];
    text_page(game, terrain, _binary_doc_halfway_txt_start, 60, 16);
}

static void
ui_win(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_win_txt_start[];
    text_page(game, terrain, _binary_doc_win_txt_start, 60, 16);
}

static void
ui_apology(game_t *game, panel_t *terrain)
{
    extern const char _binary_doc_apology_txt_start[];
    if (!game->apology_given)
        text_page(game, terrain, _binary_doc_apology_txt_start, 60, 14);
    game->apology_given = true;
}

int
main(void)
{
    int w, h;
    display_init();
    device_terminal_size(&w, &h);
    if (w < DISPLAY_WIDTH || h < DISPLAY_HEIGHT) {
        display_free();
        printf("Goblin-COM requires a terminal of at least %dx%d characters!\n"
               "I see %dx%d\n"
               "Press enter to exit ...\n",
               DISPLAY_WIDTH, DISPLAY_HEIGHT, w, h);
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    device_entropy(&rand_state, sizeof(rand_state));
    device_title("Goblin-COM");

    panel_t loading;
    uint8_t loading_message[] = "Initializing world ...";
    panel_center_init(&loading, sizeof(loading_message), 1);
    display_push(&loading);
    panel_puts(&loading, 0, 0, FONT_DEFAULT, (char *)loading_message);
    display_refresh();
    game_t *game;
    FILE *save = fopen(PERSIST_FILE, "rb");
    if (save) {
        game = game_load(save);
        fclose(save);
        game->speed = SPEED_FACTOR;
        unlink(PERSIST_FILE);
    } else {
        game = game_create(xorshift(&rand_state));
        game->speed = SPEED_FACTOR;
    }
    atexit_save_game = game;
    atexit(atexit_save);
    display_pop_free();

    panel_t sidemenu;
    panel_init(&sidemenu, DISPLAY_WIDTH - SIDEMENU_WIDTH, 0,
               SIDEMENU_WIDTH, DISPLAY_HEIGHT);
    display_push(&sidemenu);

    panel_t terrain;
    panel_init(&terrain, 0, 0, MAP_WIDTH, MAP_HEIGHT);
    display_push(&terrain);

    panel_t buildings;
    panel_init(&buildings, 0, 0, MAP_WIDTH, MAP_HEIGHT);
    display_push(&buildings);

    panel_t units;
    panel_init(&units, 0, 0, MAP_WIDTH, MAP_HEIGHT);
    display_push(&units);

    /* Main Loop */
    bool running = true;
    while (running) {
        yield_t diff;
        for (int i = 0; running && i < game->speed; i++) {
            diff = game_step(game);
            enum game_event event;
            while ((event = game_event_pop(game)) != EVENT_NONE) {
                sidemenu_draw(&sidemenu, game, diff);
                display_refresh();
                switch (event) {
                case EVENT_LOSE:
                    atexit_save_game = NULL;
                    running = false;
                    ui_gameover(game, &terrain);
                break;
                case EVENT_PROGRESS_1:
                    ui_halfway(game, &terrain);
                    break;
                case EVENT_WIN:
                    atexit_save_game = NULL;
                    running = false;
                    ui_win(game, &terrain);
                    break;
                case EVENT_BATTLE:
                    ui_apology(game, &terrain);
                    break;
                case EVENT_NONE:
                    break;
                }
            }
        }

        sidemenu_draw(&sidemenu, game, diff);
        map_draw_terrain(game->map, &terrain);
        panel_clear(&buildings);
        map_draw_buildings(game->map, &buildings);
        panel_clear(&units);
        game_draw_units(game, &units, false);
        display_refresh();
        uint64_t wait = device_uepoch() % PERIOD;
        if (device_kbhit(wait)) {
            int key = device_getch();
            switch (key) {
            case 'b':
                ui_build(game, &terrain);
                break;
            case 's':
                ui_squads(game, &terrain, &units);
                break;
            case 'h':
                ui_heroes(game, &terrain);
                break;
            case 't':
                ui_story(game, &terrain);
                break;
            case 'p':
                ui_help(game, &terrain);
                break;
            case '>':
            case '.':
                if (game->speed < SPEED_MAX)
                    game->speed *= SPEED_FACTOR;
                break;
            case '<':
            case ',':
                game->speed /= SPEED_FACTOR;
                if (game->speed == 0)
                    game->speed = 1;
                break;
            case 'R':
                display_invalidate();
                break;
            case 'q':
                running = !popup_quit(true);
                break;
            case 'Q':
                running = !popup_quit(false);
                if (!running)
                    atexit_save_game = NULL;
                break;
            default:
                break;
            }
        }
    };

    atexit_save();
    atexit_save_game = NULL;
    game_free(game);

    display_pop(); // units
    display_pop(); // buildings
    display_pop(); // terrain
    display_pop(); // sidemenu
    panel_free(&units);
    panel_free(&buildings);
    panel_free(&terrain);
    panel_free(&sidemenu);
    display_free();
    return 0;
}
