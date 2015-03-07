#pragma once

#include "display.h"

#define MAP_WIDTH  60
#define MAP_HEIGHT 24

enum map_base {
    OCEAN, COAST, GRASS, FOREST, HILL, MOUNTAIN
};

typedef struct map {
    struct {
        enum map_base base;
    } summary[MAP_WIDTH][MAP_HEIGHT];
    struct {
        float height;
    } tiles[MAP_WIDTH * MAP_WIDTH][MAP_HEIGHT * MAP_HEIGHT];
} map_t;

map_t *map_generate(uint64_t seed);
void   map_free(map_t *map);
void   map_draw(map_t *, panel_t *);
