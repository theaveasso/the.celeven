#include "cel.h"

typedef struct CELapp_state CELapp_state;
struct CELapp_state {
    CELgame *game_inst;
};

GlobalVariable bool is_initialized = false;
GlobalVariable CELapp_state state  = {0};

bool application_init(CELgame *game) {
    state.game_inst = game;

    // setup vulkan

    if (!state.game_inst->game_init(state.game_inst)) { return false; }

    is_initialized = true;
    return true;
}

bool application_run() {
    return true;
}
