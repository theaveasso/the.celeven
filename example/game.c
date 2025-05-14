#include "game.h"
#include <cel.h>

bool game_init(CELgame *game) {
    GameState *state    = cel_arena_alloc(&game->state.persistent_arena, sizeof(GameState));
    state->format       = VK_FORMAT_R16G16B16A16_SFLOAT;
    state->draw_texture = celvk_image_create(
        &(CELvk_image_create_info){
            .usages            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .format            = state->format,
            .base_array_layers = 1,
            .extent            = (VkExtent3D){.width = game->config.render_width, .height = game->config.render_height, 1}},
        &(VmaAllocationCreateInfo){
            .usage         = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT});

    game->user_data = state;
    return true;
}

bool game_update(CELgame *game) {
    (void) game;
    return true;
}

bool game_draw(CELgame *game) {
    GameState *state = (GameState*) game->user_data;

    VkCommandBuffer cmd = celvk_begin_draw();

    celvk_draw();

    celvk_end_draw(cmd, state->draw_texture);

    return true;
}

bool game_destroy(CELgame *game) {
    (void) game;
    return true;
}
