#include "game_logic.h"
#include <stdio.h>
#include <string.h>

static void clamp(int *p) {
    if (*p < 0) *p = 0;
    if (*p > BOARD_SIZE) *p = BOARD_SIZE;
}

void init_game(GameLogic *g, int player_count) {
    // clear board
    for (int i = 0; i <= BOARD_SIZE; i++) {
        g->board[i].type = TILE_NONE;
        g->board[i].value = 0;
    }

    // reset players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g->pos[i] = 0;
        g->skip[i] = 0;
    }
    (void)player_count; // not needed but kept for signature

    g->winner = -1;

    // -----------------------------
    // Special tiles (spread out)
    // BOOST = ladder vibes (go up)
    // BACK  = snake vibes (go down)
    // TRAP  = skip next turn
    // -----------------------------

    // Early game
    g->board[3].type  = TILE_BOOST; g->board[3].value  = 6;  // 3 -> 9
    g->board[5].type  = TILE_BACK;  g->board[5].value  = 3;  // 5 -> 2
    g->board[8].type  = TILE_TRAP;                          // skip next
    g->board[10].type = TILE_BOOST; g->board[10].value = 5;  // 10 -> 15
    g->board[12].type = TILE_BACK;  g->board[12].value = 4;  // 12 -> 8

    // Mid game
    g->board[16].type = TILE_TRAP;
    g->board[18].type = TILE_BOOST; g->board[18].value = 7;  // 18 -> 25
    g->board[21].type = TILE_BACK;  g->board[21].value = 6;  // 21 -> 15
    g->board[24].type = TILE_BOOST; g->board[24].value = 4;  // 24 -> 28
    g->board[28].type = TILE_BACK;  g->board[28].value = 8;  // 28 -> 20
    g->board[30].type = TILE_TRAP;

    // Late game (so it’s not empty after 30)
    g->board[33].type = TILE_BOOST; g->board[33].value = 6;  // 33 -> 39
    g->board[35].type = TILE_TRAP;
    g->board[37].type = TILE_BACK;  g->board[37].value = 7;  // 37 -> 30
    g->board[40].type = TILE_BOOST; g->board[40].value = 5;  // 40 -> 45
    g->board[41].type = TILE_BACK;  g->board[41].value = 9;  // 41 -> 32
    g->board[44].type = TILE_TRAP;
    g->board[47].type = TILE_BOOST; g->board[47].value = 2;  // 47 -> 49
    g->board[48].type = TILE_BACK;  g->board[48].value = 6;  // 48 -> 42
}

void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size) {

    if (g->winner != -1) {
        snprintf(msg, msg_size, "Game over. Winner: Player %d\n", g->winner + 1);
        return;
    }

    // TRAP skip handling
    if (g->skip[player_id]) {
        g->skip[player_id] = 0;
        snprintf(msg, msg_size, "Player %d skips this turn (TRAP)\n", player_id + 1);
        return;
    }

    int old = g->pos[player_id];
    int pos = old + dice;
    clamp(&pos);

    // move to landing tile
    g->pos[player_id] = pos;

    // win if exactly/after clamp hits 50
    if (g->pos[player_id] == BOARD_SIZE) {
        g->winner = player_id;
        snprintf(msg, msg_size, "Player %d rolled %d: %d -> %d and WON!\n",
                 player_id + 1, dice, old, g->pos[player_id]);
        return;
    }

    // apply special tile
    Tile t = g->board[pos];

    if (t.type == TILE_BOOST) {
        int after = g->pos[player_id] + t.value;
        clamp(&after);
        g->pos[player_id] = after;

        if (g->pos[player_id] == BOARD_SIZE) g->winner = player_id;

        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BOOST +%d to %d)\n",
                 player_id + 1, dice, old, pos, t.value, g->pos[player_id]);

    } else if (t.type == TILE_BACK) {
        int after = g->pos[player_id] - t.value;
        clamp(&after);
        g->pos[player_id] = after;

        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (BACK -%d to %d)\n",
                 player_id + 1, dice, old, pos, t.value, g->pos[player_id]);

    } else if (t.type == TILE_TRAP) {
        g->skip[player_id] = 1;
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d (TRAP: skip next turn)\n",
                 player_id + 1, dice, old, pos);

    } else {
        snprintf(msg, msg_size,
                 "P%d rolled %d: %d -> %d\n",
                 player_id + 1, dice, old, g->pos[player_id]);
    }

    // final win check (if BOOST pushed to end)
    if (g->pos[player_id] == BOARD_SIZE) {
        g->winner = player_id;
    }
}
