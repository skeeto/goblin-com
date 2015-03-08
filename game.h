#pragma once

#include <stdint.h>
#include "map.h"

#define INIT_GOLD        100
#define INIT_WOOD        10
#define INIT_POPULATION  100

typedef struct game {
    uint64_t seed;
    long gold;
    long wood;
    long population;
    map_t *map;
} game_t;

void game_init(game_t *, uint64_t seed);
void game_free(game_t *);

bool game_build(game_t *, enum building, int x, int y);
