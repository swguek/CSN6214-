#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#define BOARD_SIZE 50
#define MAX_PLAYERS 5

// Tile types
#define TILE_NONE   0
#define TILE_BOOST  1   // go forward
#define TILE_TRAP   2   // skip next turn
#define TILE_BACK   3   // go backward

typedef struct {
    int type;
    int value;
} Tile;

typedef struct {
    int pos[MAX_PLAYERS];      // internal: 0..49 (shown as 1..50)
    int skip[MAX_PLAYERS];
    int winner;                // -1 if none
    Tile board[BOARD_SIZE];    // tiles 1..50 (index 0 unused visually)
} GameLogic;

// initialize board and players
void init_game(GameLogic *g, int player_count);

// apply one turn
void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size);

#endif
