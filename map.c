#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "map.h"

static uint64_t
xorshift(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    *state = x;
    return x * UINT64_C(2685821657736338717);
}

static double
randf(uint64_t *state)
{
    return (xorshift(state) * 2.0) / UINT64_MAX - 1.0;
}

#define WORK_SIZE 4097
#define NOISE_SCALE 4.0f

static size_t
grow(const float *map, size_t size, float *out, uint64_t *seed)
{
    size_t osize = (size - 1) * 2 + 1;
    /* Copy */
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            out[y * 2 * osize + x * 2] = map[y * size + x];
        }
    }
    /* Diamond */
    for (size_t y = 1; y < osize; y += 2) {
        for (size_t x = 1; x < osize; x += 2) {
            int count = 0;
            float sum = 0;
            for (int dy = -1; dy <= 1; dy += 2) {
                for (int dx = -1; dx <= 1; dx += 2) {
                    long i = (y + dy) * osize + (x + dx);
                    if (i >= 0 && (size_t)i < osize * osize) {
                        sum += out[i];
                        count++;
                    }
                }
            }
            out[y * osize + x] =
                sum / count + randf(seed) / osize * NOISE_SCALE;
        }
    }
    /* Square */
    struct {
        int x, y;
    } pos[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (size_t y = 1; y < osize; y++) {
        for (size_t x = (y + 1) % 2; x < osize; x += 2) {
            int count = 0;
            float sum = 0;
            for (int p = 0; p < 4; p++) {
                long i = (y + pos[p].y) * osize + (x + pos[p].x);
                if (i >= 0 && (size_t)i < osize * osize) {
                    sum += out[i];
                    count++;
                }
            }
            out[y * osize + x] = sum / count +
                randf(seed) / osize  * NOISE_SCALE;
        }
    }
    return osize;
}

static void
summarize(map_t *map, uint64_t *seed)
{
    for (size_t y = 0; y < MAP_HEIGHT; y++) {
        for (size_t x = 0; x < MAP_WIDTH; x++) {
            float mean = 0;
            for (size_t sy = 0; sy < MAP_HEIGHT; sy++) {
                for (size_t sx = 0; sx < MAP_WIDTH; sx++) {
                    size_t ix = x * MAP_WIDTH + sx;
                    size_t iy = y * MAP_HEIGHT + sy;
                    mean += map->tiles[ix][iy].height;
                }
            }
            mean /= (MAP_WIDTH * MAP_HEIGHT);
            float std = 0;
            for (size_t sy = 0; sy < MAP_HEIGHT; sy++) {
                for (size_t sx = 0; sx < MAP_WIDTH; sx++) {
                    size_t ix = x * MAP_WIDTH + sx;
                    size_t iy = y * MAP_HEIGHT + sy;
                    float diff = mean - map->tiles[ix][iy].height;
                    std += diff * diff;
                }
            }
            std = sqrt(std / (MAP_HEIGHT * MAP_WIDTH));
            enum map_base base;
            if (mean < -0.8)
                base = OCEAN;
            else if (mean < -0.6)
                base = COAST;
            else if (std > 0.05)
                base = MOUNTAIN;
            else if (std > 0.04)
                base = HILL;
            else if (randf(seed) > 0.3)
                base = GRASS;
            else
                base = FOREST;
            map->summary[x][y].base = base;
        }
    }
}

map_t *
map_generate(uint64_t seed)
{
    map_t *map = malloc(sizeof(*map));
    size_t alloc_size = WORK_SIZE * WORK_SIZE * sizeof(float);
    float *buf_a = malloc(alloc_size);
    float *buf_b = malloc(alloc_size);
    float *heightmap = buf_a;
    for (int i = 0; i < 4; i++)
        heightmap[i] = randf(&seed);
    size_t size = 3;
    while (size < WORK_SIZE) {
        size = grow(buf_a, size, buf_b, &seed);
        heightmap = buf_b;
        buf_b = buf_a;
        buf_a = heightmap;
    }
    for (size_t y = 0; y < MAP_HEIGHT * MAP_HEIGHT; y++) {
        for (size_t x = 0; x < MAP_WIDTH * MAP_WIDTH; x++) {
            float height = heightmap[y * WORK_SIZE + x];
            float sx = x / (float)(MAP_WIDTH * MAP_WIDTH) - 0.5;
            float sy = y / (float)(MAP_HEIGHT * MAP_HEIGHT) - 0.5;
            float s = sqrt(sx * sx + sy * sy) * 3 - 0.45f;
            map->tiles[x][y].height = height - s;
        }
    }
    free(buf_a);
    free(buf_b);
    summarize(map, &seed);
    return map;
}

void
map_free(map_t *map)
{
    free(map);
}


void
map_draw(map_t *map, panel_t *p)
{
    for (size_t y = 0; y < MAP_HEIGHT; y++) {
        for (size_t x = 0; x < MAP_WIDTH; x++) {
            enum map_base base = map->summary[x][y].base;
            font_t font = {0, 0, false};
            char c = 'X';
            switch (base) {
            case OCEAN:
                font.fore = COLOR_WHITE;
                font.back = COLOR_BLUE;
                c = ' ';
                break;
            case COAST: {
                font.fore = COLOR_WHITE;
                font.back = COLOR_BLUE;
                float dx = (x / (float)MAP_WIDTH) - 0.5;
                float dy = (y / (float)MAP_HEIGHT) - 0.5;
                float dist = sqrt(dx * dx + dy * dy) * 100;
                float offset = (device_uepoch() / 100000) % 31;
                font.bold = sinf(dist + offset) < 0 ? true : false;
                c = '~';
            } break;
            case GRASS:
                font.fore = COLOR_GREEN;
                font.back = COLOR_GREEN;
                font.bold = true;
                c = '.';
                break;
            case FOREST:
                font.fore = COLOR_GREEN;
                font.back = COLOR_GREEN;
                font.bold = true;
                c = '#';
                break;
            case HILL:
                font.fore = COLOR_BLACK;
                font.back = COLOR_GREEN;
                font.bold = true;
                c = '=';
                break;
            case MOUNTAIN:
                font.fore = COLOR_BLACK;
                font.back = COLOR_WHITE;
                c = 'M';
                break;
            }
            panel_putc(p, x, y, font, c);
        }
    }
}
