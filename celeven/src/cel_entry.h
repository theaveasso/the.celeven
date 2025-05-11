#include <stdint.h>
#include <stdio.h>

#include "cel.h"

extern bool game_create(CELgame *game);

int main(void) {
    CELgame game;

    if (!game_create(&game)) { return -1; }

    if (!game.game_init) { return -2; }
    if (!game.game_update) { return -2; }
    if (!game.game_draw) { return -2; }
    if (!game.game_destroy) { return -2; }

    if (!application_init(&game)) { return 1; }
    if (!application_run()) { return 2; }

    return 0;
}