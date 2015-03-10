#pragma once

#include "display.h"

#define MAP_WIDTH  60
#define MAP_HEIGHT 24

enum map_base {
    BASE_OCEAN = ' ', BASE_COAST = 0x2248, BASE_GRASSLAND = '.',
    BASE_FOREST = 0x2663, BASE_HILL = 0x2229, BASE_MOUNTAIN = 0x25B2,
    BASE_SAND = ':'
};

enum building {
    C_NONE = 0,
    C_CASTLE = 'C',
    C_LUMBERYARD = 'W',
    C_STABLE = 'S',
    C_HAMLET = 'H',
    C_MINE = 'M',
    C_ROAD = '+',
    C_FARM = 'F'
};

typedef struct map {
    struct {
        uint16_t base;
        uint16_t building;
        long building_age;
    } high[MAP_WIDTH][MAP_HEIGHT];
    struct {
        float height;
    } low[MAP_WIDTH * MAP_WIDTH][MAP_HEIGHT * MAP_HEIGHT];
} map_t;

map_t *map_generate(uint64_t seed);
void   map_free(map_t *map);
void   map_draw_terrain(map_t *, panel_t *);
void   map_draw_buildings(map_t *, panel_t *);
