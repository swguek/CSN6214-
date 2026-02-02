#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#define BOARD_SIZE 50
#define MAX_PLAYERS 5

// Tile types
#define TILE_NONE   0
#define TILE_BOOST  1
#define TILE_TRAP   2
#define TILE_BACK   3

typedef struct {
    int type;
    int value;
} Tile;

typedef struct {
    int pos[MAX_PLAYERS];
    int skip[MAX_PLAYERS];
    int winner;
    Tile board[BOARD_SIZE + 1];
} GameLogic;

// initialize board and players
void init_game(GameLogic *g, int player_count);

// apply one player turn
void play_turn(GameLogic *g, int player_id, int dice,
               char *msg, int msg_size);

#endif
