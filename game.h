#pragma once

#include <stdint.h>
#include "map.h"

#define INIT_GOLD        100
#define INIT_WOOD        10
#define INIT_FOOD        100
#define INIT_POPULATION  100

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

void yield_string(char *, yield_t);

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
void game_free(game_t *);

bool game_build(game_t *, enum building, int x, int y);
void game_step(game_t *);
void game_date(game_t *, char *);
