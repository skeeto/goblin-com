#include <stdlib.h>
#include <string.h>
#include "game.h"

void
game_init(game_t *game, uint64_t seed)
{
    game->seed = seed;
    game->gold = INIT_GOLD;
    game->wood = INIT_WOOD;
    game->population = INIT_POPULATION;
    game->map = map_generate(seed);
    game->map->high[MAP_WIDTH / 2][MAP_HEIGHT / 2].building = C_CASTLE;
}

void
game_free(game_t *game)
{
    map_free(game->map);
    game->map = NULL;
}

bool
game_build(game_t *game, enum building building, int x, int y)
{
    if (building == C_NONE) {
        /* Erase */
        if (game->map->high[x][y].building != C_NONE) {
            game->map->high[x][y].building = C_NONE;
            return true;
        }
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
            break;
        case C_LUMBERYARD:
            valid = strchr("#", base) != NULL;
            break;
        case C_STABLE:
            valid = strchr(".", base) != NULL;
            break;
        case C_HAMLET:
            valid = strchr(".#=", base) != NULL;
            break;
        case C_MINE:
            valid = strchr("=", base) != NULL;
            break;
            break;
        case C_FARM:
            valid = strchr(".#", base) != NULL;
            break;
        }
    }
    if (valid) {
        game->map->high[x][y].building = building;
        game->map->high[x][y].building_age = -1000;
    }
    return valid;
}
