#include "game_logic.h"
#include <stdio.h>
#include <string.h>

static void clamp(int *p) {
    if (*p < 0) *p = 0;
    if (*p > BOARD_SIZE) *p = BOARD_SIZE;
}

void init_game(GameLogic *g, int player_count) {
    int i;

    for (i = 0; i <= BOARD_SIZE; i++) {
        g->board[i].type = TILE_NONE;
        g->board[i].value = 0;
    }

    for (i = 0; i < player_count; i++) {
        g->pos[i] = 0;
        g->skip[i] = 0;
    }

    g->winner = -1;

    // Special tiles
    g->board[3].type  = TILE_BOOST; g->board[3].value  = 4;
    g->board[10].type = TILE_BOOST; g->board[10].value = 6;

    g->board[12].type = TILE_BACK;  g->board[12].value = 3;
    g->board[28].type = TILE_BACK;  g->board[28].value = 5;

    g->board[16].type = TILE_TRAP;
    g->board[30].type = TILE_TRAP;
}

void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size) {
    int old, pos;
    Tile t;

    if (g->winner != -1) {
        snprintf(msg, msg_size, "Game over. Winner: Player %d\n",
                 g->winner + 1);
        return;
    }

    if (g->skip[player_id]) {
        g->skip[player_id] = 0;
        snprintf(msg, msg_size,
                 "Player %d skips this turn (TRAP)\n",
                 player_id + 1);
        return;
    }

    old = g->pos[player_id];
    pos = old + dice;
    clamp(&pos);
    g->pos[player_id] = pos;

    if (pos == BOARD_SIZE) {
        g->winner = player_id;
        snprintf(msg, msg_size,
                 "Player %d rolled %d and WON the game!\n",
                 player_id + 1, dice);
        return;
    }

    t = g->board[pos];
    if (t.type == TILE_BOOST) {
        g->pos[player_id] += t.value;
        clamp(&g->pos[player_id]);
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BOOST)\n",
                 player_id + 1, dice, old, g->pos[player_id]);
    } else if (t.type == TILE_BACK) {
        g->pos[player_id] -= t.value;
        clamp(&g->pos[player_id]);
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BACK)\n",
                 player_id + 1, dice, old, g->pos[player_id]);
    } else if (t.type == TILE_TRAP) {
        g->skip[player_id] = 1;
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (TRAP)\n",
                 player_id + 1, dice, old, pos);
    } else {
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d\n",
                 player_id + 1, dice, old, pos);
    }

    if (g->pos[player_id] == BOARD_SIZE) {
        g->winner = player_id;
    }
}
