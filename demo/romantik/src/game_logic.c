#include "game_logic.h"

void romantik_game_init(Romantik_Game *game)
{
    hex_map_init(&game->map, 2, 10, 10);

    game->score            = 0;
    game->num_placed_grids = 0;
    game->num_avail_grids  = 0;
}

static void set_neighbor_avails(Romantik_Game *game, i32 q, i32 r, vec2 *new_avails, u8 *num_avails)
{
    vec2 const dirs[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    *num_avails       = 0;
    for (u8 i = 0; i < 6; ++i) {
        i32 n_q = q + dirs[i][0];
        i32 n_r = r + dirs[i][1];
        if (romantik_set_avail(game, n_q, n_r)) {
            new_avails[*num_avails][0] = n_q;
            new_avails[*num_avails][1] = n_r;
            ++(*num_avails);
        }
    }
}

bool romantik_place_grid(Romantik_Game *game, i32 q, i32 r)
{
    vec2 new_avails[6];
    u8   num_avails = 0;
    if (hex_map_test_flags(&game->map, q, r, HEX_FLAG_AVAIL)) {
        hex_map_set_flags(&game->map, q, r, HEX_FLAG_PLACED);
        ++game->num_placed_grids;
        --game->num_avail_grids;
        set_neighbor_avails(game, q, r, new_avails, &num_avails);
        return true;
    }
    return false;
}

bool romantik_set_avail(Romantik_Game *game, i32 q, i32 r)
{
    if (hex_map_check_in_bound(&game->map, q, r) && !hex_map_test_flags(&game->map, q, r, HEX_FLAG_PLACED) &&
        !hex_map_test_flags(&game->map, q, r, HEX_FLAG_AVAIL)) {
        hex_map_set_flags(&game->map, q, r, HEX_FLAG_AVAIL);
        ++game->num_avail_grids;
        return true;
    }
    return false;
}
