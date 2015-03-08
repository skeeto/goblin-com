#include <stddef.h>
#include "game.h"

void
game_init(game_t *game, uint64_t seed)
{
    game->seed = seed;
    game->gold = INIT_GOLD;
    game->wood = INIT_WOOD;
    game->population = INIT_POPULATION;
    game->map = map_generate(seed);
}

void
game_free(game_t *game)
{
    map_free(game->map);
    game->map = NULL;
}
