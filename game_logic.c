/*
 * Contributor:
 * Member 2 – Game rules, dice movement, special effects, win conditions
 */


#include "game_logic.h"
#include <stdio.h>

static void clamp(int *p) {
    if (*p < 0) *p = 0;
    if (*p >= BOARD_SIZE) *p = BOARD_SIZE - 1;
}

void init_game(GameLogic *g, int player_count) {
    (void)player_count;

    // clear board
    for (int i = 0; i < BOARD_SIZE; i++) {
        g->board[i].type = TILE_NONE;
        g->board[i].value = 0;
    }

    // reset players (important for multi-game)
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g->pos[i] = 0;
        g->skip[i] = 0;
    }

    g->winner = -1;   // ensure fresh game

    // Special tiles (0-based indexes)

    // early
    g->board[2]  = (Tile){TILE_BOOST, 4};
    g->board[5]  = (Tile){TILE_BACK,  3};
    g->board[8]  = (Tile){TILE_TRAP,  0};
    g->board[10] = (Tile){TILE_BOOST, 5};
    g->board[14] = (Tile){TILE_BACK,  4};

    // mid
    g->board[18] = (Tile){TILE_TRAP,  0};
    g->board[20] = (Tile){TILE_BOOST, 6};
    g->board[24] = (Tile){TILE_BACK,  5};
    g->board[27] = (Tile){TILE_TRAP,  0};

    // late
    g->board[30] = (Tile){TILE_BOOST, 6};
    g->board[33] = (Tile){TILE_BACK,  6};
    g->board[36] = (Tile){TILE_TRAP,  0};
    g->board[40] = (Tile){TILE_BOOST, 5};
    g->board[43] = (Tile){TILE_BACK,  8};
    g->board[46] = (Tile){TILE_TRAP,  0};
}

void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size) {

    if (g->winner != -1) {
        snprintf(msg, msg_size,
                 "Game over. Winner: Player %d\n",
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

    int old = g->pos[player_id];
    int pos = old + dice;
    clamp(&pos);
    g->pos[player_id] = pos;

    if (pos == BOARD_SIZE - 1) {
        g->winner = player_id;
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d and WON!\n",
                 player_id + 1, dice, old + 1, pos + 1);
        return;
    }

    Tile t = g->board[pos];

    if (t.type == TILE_BOOST) {
        int after = pos + t.value;
        clamp(&after);
        g->pos[player_id] = after;

        if (after == BOARD_SIZE - 1) {
            g->winner = player_id;
            snprintf(msg, msg_size,
                     "P%d rolled %d: %d -> %d (BOOST to %d) and WON!\n",
                     player_id + 1, dice, old + 1, pos + 1, after + 1);
            return;
        }

        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BOOST to %d)\n",
                 player_id + 1, dice, old + 1, pos + 1, after + 1);
        return;
    }

    if (t.type == TILE_BACK) {
        int after = pos - t.value;
        clamp(&after);
        g->pos[player_id] = after;

        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BACK to %d)\n",
                 player_id + 1, dice, old + 1, pos + 1, after + 1);
        return;
    }

    if (t.type == TILE_TRAP) {
        g->skip[player_id] = 1;
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (TRAP: skip next)\n",
                 player_id + 1, dice, old + 1, pos + 1);
        return;
    }

    snprintf(msg, msg_size,
             "P%d rolled %d: %d -> %d\n",
             player_id + 1, dice, old + 1, pos + 1);
}
