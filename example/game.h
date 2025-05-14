#pragma once

#include <cel.h>
#include <cel_vulkan.h>

typedef struct GameState GameState;
struct GameState {
    VkFormat format;
    CELimage_handle draw_texture;
};

bool game_init(CELgame *game);

bool game_update(CELgame *game);

bool game_draw(CELgame *game);

bool game_destroy(CELgame *game);
