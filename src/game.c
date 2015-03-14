#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <math.h>
#include "game.h"
#include "rand.h"

static bool
game_event_push(game_t *game, enum game_event event)
{
    for (int i = 0; i < (int)countof(game->events); i++) {
        if (game->events[i] == EVENT_NONE) {
            game->events[i] = event;
            return true;
        }
    }
    return false;
}

hero_t
game_hero_generate(void)
{
    hero_t hero;
    memset(&hero, 0, sizeof(hero));
    hero.active = true;
    rand_name(hero.name, sizeof(hero.name));
    hero.hp = hero.hp_max = rand_range(10, 20);
    hero.ap = hero.ap_max = rand_range(20, 40);
    hero.str = rand_range(10, 18);
    hero.dex = rand_range(10, 18);
    hero.mind = rand_range(10, 18);
    hero.squad = -1;
    return hero;
}

game_t *
game_create(uint64_t map_seed)
{
    game_t *game = calloc(sizeof(*game), 1);
    game->map_seed = map_seed;
    game->time = 0;
    game->speed = 1;
    game->gold = INIT_GOLD;
    game->wood = INIT_WOOD;
    game->food = INIT_FOOD;
    game->population = INIT_POPULATION;
    game->spawn_rate = INVADER_SPAWN_RATE;
    game->map = map_generate(map_seed);
    game->map->high[CASTLE_X][CASTLE_Y].building = C_CASTLE;
    game->max_hero = MAX_HERO_INIT;
    for (int i = 0; i < (int)countof(game->squads); i++) {
        game->squads[i].x = CASTLE_X;
        game->squads[i].y = CASTLE_Y;
        game->squads[i].target = -1;
    }
    for (int i = 0; i < HERO_INIT; i++) {
        game->heroes[i] = game_hero_generate();
        game->heroes[i].squad = 0;
    }
    game->squads[0].member_count = HERO_INIT;
    return game;
}

bool
game_save(game_t *game, FILE *out)
{
    if (fwrite(game, sizeof(*game), 1, out) != 1)
        return false;
    if (fwrite(game->map->high, sizeof(game->map->high), 1, out) != 1)
        return false;
    return true;
}

game_t *
game_load(FILE *out)
{
    game_t *game = malloc(sizeof(*game));
    if (fread(game, sizeof(*game), 1, out) == 1) {
        game->map = map_generate(game->map_seed);
        if (fread(game->map->high, sizeof(game->map->high), 1, out) == 1)
            return game;
    }
    return NULL;
}

void
game_free(game_t *game)
{
    map_free(game->map);
    free(game);
}

static void
add_population(game_t *game, long amount)
{
    long start_population = game->population;
    game->population += amount;
    if (start_population < GAME_WIN_POP / 2 &&
        game->population >= GAME_WIN_POP / 2) {
        game_event_push(game, EVENT_PROGRESS_1);
    }
    if (game->population <= 0)
        game_event_push(game, EVENT_LOSE);
}

bool
game_build(game_t *game, uint16_t building, int x, int y)
{
    if (building == C_NONE) {
        /* Erase */
        if (game->map->high[x][y].building != C_NONE) {
            game->map->high[x][y].building = C_NONE;
            return true;
        }
    }
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT ||
        game->map->high[x][y].building != C_NONE) {
        return false;
    }

    bool valid = false;
    if (x > 0 && game->map->high[x - 1][y].building != C_NONE)
        valid = true;
    else if (y > 0 && game->map->high[x][y - 1].building != C_NONE)
        valid = true;
    else if (x + 1 < MAP_WIDTH && game->map->high[x + 1][y].building != C_NONE)
        valid = true;
    else if (y + 1 < MAP_HEIGHT && game->map->high[x][y + 1].building != C_NONE)
        valid = true;
    if (valid) {
        valid = false;
        enum map_base base = game->map->high[x][y].base;
        switch (building) {
        case C_NONE:
        case C_CASTLE:
        case C_ROAD:
            valid = base != BASE_OCEAN && base != BASE_COAST;
            break;
        case C_LUMBERYARD:
            valid = base == BASE_FOREST;
            break;
        case C_STABLE:
            valid = base == BASE_GRASSLAND;
            game->max_hero += STABLE_INC;
            break;
        case C_HAMLET:
            valid =
                base == BASE_GRASSLAND ||
                base == BASE_FOREST ||
                base == BASE_HILL;
            if (valid)
                add_population(game, HAMLET_INC);
            break;
        case C_MINE:
            valid = base == BASE_HILL;
            break;
            break;
        case C_FARM:
            valid = base == BASE_GRASSLAND || base == BASE_FOREST;
            break;
        }
    }
    if (valid) {
        yield_t cost = building_cost(building);
        game->food -= cost.food;
        game->wood -= cost.wood;
        game->gold -= cost.gold;
        game->map->high[x][y].building = building;
        if (building == C_ROAD)
            game->map->high[x][y].building_age = 0;
        else
            game->map->high[x][y].building_age = INIT_BUILDING_AGE;
    }
    return valid;
}

void
game_unbuild(game_t *game, int x, int y)
{
    uint16_t building = map_building(game->map, x, y);
    switch (building) {
    case C_HAMLET:
        game->population -= HAMLET_INC;
        break;
    case C_STABLE:
        game->population -= STABLE_INC;
        break;
    case C_MINE:
    case C_LUMBERYARD:
    case C_ROAD:
    case C_FARM:
        /* Nothing special */
        break;
    case C_CASTLE:
        game->population -= 50;
        if (game->population < 0)
            game->population = 0; // game over
        return; // don't destroy
    }
    game->map->high[x][y].building = C_NONE;
}

void
game_date(game_t *game, char *buffer)
{
    long time = game->time;
    long day = game->time / DAY;
    time -= day * DAY;
    long hour = time / HOUR;
    time -= hour * HOUR;
    long minute = time / MINUTE;
    char *ampm = hour < 12 ? "am" : "pm";
    long hour12 = hour == 0 ? 12 : hour;
    if (hour12 > 12)
        hour12 -= 12;
    sprintf(buffer, "Day %ld, %ld:%02ld%s", day, hour12, minute, ampm);
}

static void
yield_apply(game_t *game, yield_t yield)
{
    game->wood += yield.wood / DAY;
    game->food += yield.food / DAY;
    game->gold += yield.gold / DAY;
}

static void
building_process(game_t *game, uint16_t building)
{
    yield_t yield = {0, 0, 0};
    switch (building) {
    case C_NONE:
        return;
    case C_CASTLE:
        yield = YIELD_CASTLE;
        break;
    case C_LUMBERYARD:
        yield = YIELD_LUMBERYARD;
        break;
    case C_STABLE:
        yield = YIELD_STABLE;
        break;
    case C_HAMLET:
        yield = YIELD_HAMLET;
        break;
    case C_MINE:
        yield = YIELD_MINE;
        break;
    case C_ROAD:
        yield = YIELD_ROAD;
        break;
    case C_FARM:
        yield = YIELD_FARM;
        break;
    }
    yield_apply(game, yield);
}

void
yield_string(char *b, yield_t yield, bool rate)
{
    char *end = b;
    int count = 0;
    if (yield.gold)
        end += sprintf(end, "%s%d gold%s",
                       count++ > 0 ? ", " : "",
                       yield.gold,
                       rate ? "/day" : "");
    if (yield.wood)
        end += sprintf(end, "%s%d wood%s",
                       count++ > 0 ? ", " : "",
                       yield.wood,
                       rate ? "/day" : "");
    if (yield.food)
        end += sprintf(end, "%s%d food%s",
                       count++ > 0 ? ", " : "",
                       yield.food,
                       rate ? "/day" : "");
}

bool
game_hero_push(game_t *game, hero_t hero)
{
    for (int i = 0; i < game->max_hero; i++) {
        if (!game->heroes[i].active) {
            game->heroes[i] = hero;
            return true;
        }
    }
    return false;
}

enum game_event
game_event_pop(game_t *game)
{
    for (int i = (int)countof(game->events) - 1; i >= 0; i--) {
        if (game->events[i] != EVENT_NONE) {
            enum game_event event = game->events[i];
            game->events[i] = EVENT_NONE;
            return event;
        }
    }
    return EVENT_NONE;
}

/* Invaders */

static bool
invader_push(game_t *game, invader_t invader)
{
    for (unsigned i = 0; i < countof(game->invaders); i++) {
        if (!game->invaders[i].active) {
            game->invaders[i] = invader;
            return true;
        }
    }
    return false;
}

static void
invader_delete(game_t *game, invader_t *i)
{
    (void) game;
    i->active = false;
}

static invader_t
invader_generate(void)
{
    float az = rand_uniform(0, 2 * PI);
    invader_t invader = {
        .active = true,
        .type = I_GOBLIN,
        .x = cosf(az) * MAP_WIDTH + CASTLE_X,
        .y = sinf(az) * MAP_HEIGHT + CASTLE_Y,
        .embarked = true
    };
    return invader;
}

static inline uint16_t
invader_base(game_t *game, invader_t *i)
{
    return map_base(game->map, i->x, i->y);
}

static inline uint16_t
invader_building(game_t *game, invader_t *i)
{
    return map_building(game->map, i->x, i->y);
}

static void
invader_find_target(game_t *game, invader_t *i)
{
    int ix = i->x;
    int iy = i->y;
    int best_x = ix + rand_uniform(-INVADER_VISION, INVADER_VISION);
    int best_y = iy + rand_uniform(-INVADER_VISION, INVADER_VISION);
    float best_d = INFINITY;
    for (int y = -INVADER_VISION; y <= INVADER_VISION; y++) {
        for (int x = -INVADER_VISION; x <= INVADER_VISION; x++) {
            float xx = ix + x;
            float yy = iy + y;
            uint16_t building = map_building(game->map, xx, yy);
            if (building != C_NONE) {
                float d = xx * xx + yy * yy;
                if (d < best_d) {
                    best_x = xx;
                    best_y = yy;
                    best_d = d;
                }
            }
        }
    }
    i->tx = best_x;
    i->ty = best_y;
}

static void
invader_step(game_t *game, invader_t *i)
{
    uint16_t base = invader_base(game, i);
    uint16_t building = invader_building(game, i);
    uint16_t target_base = map_base(game->map, i->tx, i->ty);
    uint16_t target_building = map_building(game->map, i->tx, i->ty);
    if (i->embarked || game->population >= GAME_WIN_POP) {
        if (IS_WATER(base)) {
            i->tx = CASTLE_X;
            i->ty = CASTLE_Y;
        } else {
            i->embarked = false;
            invader_find_target(game, i);
        }
    } else if (IS_WATER(target_base)) {
        invader_find_target(game, i);
    }
    if (building != C_NONE) {
        if (++i->rampage_time > INVADER_RAMPAGE_END) {
            i->rampage_time = 0;
            game_unbuild(game, i->x, i->y); // destroy
        }
    } else {
        /* Pursue */
        i->rampage_time = 0;
        float dx = i->tx - i->x;
        float dy = i->ty - i->y;
        float d = sqrt(dx * dx + dy * dy);
        if (d < 0.1) {
            if (target_building != C_NONE) {
                i->x = i->tx;
                i->y = i->ty;
            } else {
                invader_find_target(game, i);
            }
        } else {
            float speed = INVADER_SPEED;
            if (base == BASE_MOUNTAIN)
                speed *= 0.5;
            else if (IS_WATER(base))
                speed *= 1.5;
            if (game->population >= GAME_WIN_POP)
                speed *= -1; // run away
            i->x += (speed / (float)DAY) * dx / d;
            i->y += (speed / (float)DAY) * dy / d;
        }
    }
    i->embarked = IS_WATER(base);
}

void
squad_step(game_t *game, squad_t *squad)
{
    float tx, ty;
    if (squad->target < 0) {
        tx = CASTLE_X;
        ty = CASTLE_Y;
    } else {
        invader_t *i = game->invaders + squad->target;
        if (!i->active) {
            squad->target = -1;
            tx = CASTLE_X;
            ty = CASTLE_Y;
        }
        tx = i->x;
        ty = i->y;
    }
    float dx = tx - squad->x;
    float dy = ty - squad->y;
    float d = sqrt(dx * dx + dy * dy);
    if (d < 0.1 && !IS_WATER(map_base(game->map, tx, ty))) {
        squad->x = tx;
        squad->y = ty;
        if (squad->target >= 0) {
            game_event_push(game, EVENT_BATTLE);
            // TODO
            invader_delete(game, game->invaders + squad->target);
        }
    } else {
        float speed = SQUAD_SPEED;
        if (map_base(game->map, squad->x, squad->y) == BASE_MOUNTAIN &&
            map_building(game->map, squad->x, squad->y) == C_NONE)
            speed *= 0.6;
        float newx = squad->x + (speed / (float)DAY) * dx / d;
        float newy = squad->y + (speed / (float)DAY) * dy / d;
        if (squad->target < 0 || !IS_WATER(map_base(game->map, newx, newy))) {
            squad->x = newx;
            squad->y = newy;
        }
    }
}

yield_t
game_step(game_t *game)
{
    struct {
        double gold;
        double food;
        double wood;
    } init = {game->gold, game->food, game->wood};
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            uint16_t building = game->map->high[x][y].building;
            if (building != C_NONE) {
                if (++game->map->high[x][y].building_age >= 0)
                    building_process(game, building);
            }
        }
    }
    yield_t diff = {
        .gold = (game->gold - init.gold) * DAY,
        .food = (game->food - init.food) * DAY,
        .wood = (game->wood - init.wood) * DAY
    };

    for (unsigned i = 0; i < countof(game->squads); i++)
        if (game->squads[i].member_count > 0)
            squad_step(game, game->squads + i);

    if (rand_uniform(0, 1) < game->spawn_rate / DAY)
        invader_push(game, invader_generate());
    for (unsigned i = 0; i < countof(game->invaders); i++)
        if (game->invaders[i].active)
            invader_step(game, game->invaders + i);

    /* Generate events. */
    if (game->population >= GAME_WIN_POP)
        game_event_push(game, EVENT_WIN);

    game->time++;
    return diff;
}

yield_t
building_cost(uint16_t building)
{
    switch (building) {
    case C_CASTLE:
        return COST_CASTLE;
    case C_LUMBERYARD:
        return COST_LUMBERYARD;
    case C_FARM:
        return COST_FARM;
    case C_STABLE:
        return COST_STABLE;
    case C_MINE:
        return COST_MINE;
    case C_ROAD:
        return COST_ROAD;
    case C_HAMLET:
        return COST_HAMLET;
    case C_NONE:
        break;
    }
    return (yield_t){0, 0, 0};
}

yield_t
building_yield(uint16_t building)
{
    switch (building) {
    case C_CASTLE:
        return YIELD_CASTLE;
    case C_LUMBERYARD:
        return YIELD_LUMBERYARD;
    case C_FARM:
        return YIELD_FARM;
    case C_STABLE:
        return YIELD_STABLE;
    case C_MINE:
        return YIELD_MINE;
    case C_ROAD:
        return YIELD_ROAD;
    case C_HAMLET:
        return YIELD_HAMLET;
    case C_NONE:
        break;
    }
    return (yield_t){0, 0, 0};
}

void
game_draw_units(game_t *game, panel_t *p, bool id)
{
    font_t land = FONT(R, k);
    font_t sea  = FONT(r, y);
    for (int i = 0; i < (int)countof(game->invaders); i++) {
        invader_t *inv = game->invaders + i;
        if (inv->active)
            panel_putc(p, inv->x, inv->y, inv->embarked ? sea : land,
                       id ? i + 'A' : inv->type);
    }
    for (int i = 0; i < (int)countof(game->squads); i++) {
        squad_t *s = game->squads + i;
        if (s->member_count > 0 &&
            !((int)s->x == CASTLE_X && (int)s->y == CASTLE_Y))
            panel_putc(p, s->x, s->y, FONT(k, M), i + 'A');
    }
}
