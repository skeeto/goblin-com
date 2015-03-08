#pragma once

#include <stdint.h>
#include "map.h"

#define INIT_GOLD         100
#define INIT_WOOD         100
#define INIT_FOOD         10
#define INIT_POPULATION   250
#define INIT_BUILDING_AGE (-60 * 24) // 1 day

typedef struct {
    int gold;
    int food;
    int wood;
} yield_t;

#define YIELD_CASTLE     (yield_t){1, 1, 1}
#define YIELD_LUMBERYARD (yield_t){0, 0, 4}
#define YIELD_FARM       (yield_t){0, 4, 0}
#define YIELD_STABLE     (yield_t){0, -1, 4}
#define YIELD_MINE       (yield_t){4, 0, -1}
#define YIELD_ROAD       (yield_t){-1, 0, 0}
#define YIELD_HAMLET     (yield_t){2, -2, 0}

#define COST_CASTLE     (yield_t){1000, 1000, 1000}
#define COST_LUMBERYARD (yield_t){25, 0, 0}
#define COST_FARM       (yield_t){0, 0, 20}
#define COST_STABLE     (yield_t){25, 0, 25}
#define COST_MINE       (yield_t){0, 0, 100}
#define COST_ROAD       (yield_t){1, 0, 0}
#define COST_HAMLET     (yield_t){0, 0, 50}

void yield_string(char *, yield_t, bool rate);
yield_t building_cost(enum building);
yield_t building_yield(enum building);

typedef struct game {
    uint64_t seed;
    long time; // minutes
    int speed;
    double gold;
    double wood;
    double food;
    double population;
    map_t *map;
} game_t;

void game_init(game_t *, uint64_t seed);
bool game_save(game_t *game, FILE *out);
bool game_load(game_t *game, FILE *out);
void game_free(game_t *);

bool    game_build(game_t *, enum building, int x, int y);
yield_t game_step(game_t *);
void    game_date(game_t *, char *);
