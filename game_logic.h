#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#define BOARD_SIZE 50
#define MAX_PLAYERS 5

// Tile types
#define TILE_NONE   0
#define TILE_BOOST  1   // go forward extra
#define TILE_TRAP   2   // skip next turn
#define TILE_BACK   3   // go backward

typedef struct {
    int type;
    int value;   // used for BOOST/BACK, ignored for TRAP/NONE
} Tile;

typedef struct {
    int pos[MAX_PLAYERS];
    int skip[MAX_PLAYERS];
    int winner;                 // -1 if none, else player index
    Tile board[BOARD_SIZE + 1]; // 0..50
} GameLogic;

void init_game(GameLogic *g, int player_count);

void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size);

#endif
