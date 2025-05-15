#include <cel_entry.h>

#include "game.h"

#define PERSISTENT_STORAGE_SIZE (64 * 1024 * 1024)
#define TRANSIENT_STORAGE_SIZE (64 * 1024 * 1024)

GlobalVariable unsigned char buffer[PERSISTENT_STORAGE_SIZE + TRANSIENT_STORAGE_SIZE];

bool game_create(CELgame *game) {
    game->config.title         = "CEL";
    game->config.base_path     = "example\\resources"; // _WIN32 specifics
    game->config.width         = 1920;
    game->config.height        = 1080;
    game->config.render_width  = 640;
    game->config.render_height = 360;

    game->game_init    = game_init;
    game->game_update  = game_update;
    game->game_draw    = game_draw;
    game->game_destroy = game_destroy;

    void *persistent_memory = buffer;
    void *transient_memory  = buffer + PERSISTENT_STORAGE_SIZE;

    cel_arena_init(&game->state.persistent_arena, persistent_memory, PERSISTENT_STORAGE_SIZE);
    cel_arena_init(&game->state.transient_arena, transient_memory, TRANSIENT_STORAGE_SIZE);

    return true;
}