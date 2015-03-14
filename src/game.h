#pragma once

#include <stdint.h>
#include "map.h"

#define MINUTE (60.0)
#define HOUR (60.0 * 60.0)
#define DAY (24.0 * HOUR)

#define INIT_GOLD         100
#define INIT_WOOD         100
#define INIT_FOOD         10
#define INIT_POPULATION   250
#define INIT_BUILDING_AGE (-60 * 60 * 24) // 1 day
#define STABLE_INC        2
#define HAMLET_INC        100

typedef struct {
    int gold;
    int food;
    int wood;
} yield_t;

#define YIELD_CASTLE     (yield_t){2, 2, 2}
#define YIELD_LUMBERYARD (yield_t){0, 0, 4}
#define YIELD_FARM       (yield_t){0, 4, 0}
#define YIELD_STABLE     (yield_t){0, -2, 0}
#define YIELD_MINE       (yield_t){4, 0, -1}
#define YIELD_ROAD       (yield_t){-1, 0, 0}
#define YIELD_HAMLET     (yield_t){2, -2, 0}

#define COST_CASTLE     (yield_t){1000, 1000, 1000}
#define COST_LUMBERYARD (yield_t){25, 0, 0}
#define COST_FARM       (yield_t){0, 0, 20}
#define COST_STABLE     (yield_t){100, 0, 100}
#define COST_MINE       (yield_t){0, 0, 100}
#define COST_ROAD       (yield_t){1, 0, 0}
#define COST_HAMLET     (yield_t){0, 0, 50}

void yield_string(char *, yield_t, bool rate);
yield_t building_cost(uint16_t);
yield_t building_yield(uint16_t);

#define GAME_WIN_POP 4000

enum game_event {
    EVENT_NONE, EVENT_LOSE, EVENT_WIN, EVENT_PROGRESS_1, EVENT_BATTLE
};

#define INVADER_SPEED 10.0f
#define INVADER_SPAWN_RATE 1
#define INVADER_VISION 10
#define INVADER_RAMPAGE_END DAY

#define SQUAD_SPEED 15
#define MAX_HERO_INIT 4
#define HERO_CANDIDATES 10
#define HERO_INIT 2

enum invader_type {
    I_GOBLIN = 'g'
};

typedef struct invader {
    bool active;
    float x, y;    // position
    float tx, ty;  // target
    uint16_t type;
    long rampage_time;
    bool embarked;
} invader_t;

typedef struct squad {
    float x, y;
    int target;
    unsigned member_count;
} squad_t;

typedef struct hero {
    bool active;
    char name[16];
    int hp, hp_max;
    int ap, ap_max;
    int str, dex, mind;
    int squad;
} hero_t;

typedef struct game {
    uint64_t map_seed;
    long time; // seconds
    int speed;
    double gold;
    double wood;
    double food;
    double population;
    map_t *map;
    float spawn_rate; // per day
    invader_t invaders[16];
    squad_t squads[16];
    int max_hero;
    hero_t heroes[128];
    enum game_event events[8];
} game_t;

game_t *game_create(uint64_t map_seed);
bool    game_save(game_t *game, FILE *out);
game_t *game_load(FILE *out);
void    game_free(game_t *);

bool    game_build(game_t *, uint16_t building, int x, int y);
yield_t game_step(game_t *);
void    game_date(game_t *, char *);
void    game_draw_units(game_t *game, panel_t *p, bool id);

hero_t  game_hero_generate(void);
bool    game_hero_push(game_t *game, hero_t hero);

enum game_event game_event_pop(game_t *game);
