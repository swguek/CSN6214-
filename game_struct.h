#ifndef GAME_STRUCT_H
#define GAME_STRUCT_H

#include <pthread.h>
#include <semaphore.h>
#include "game_logic.h"

/* ensure MAX_PLAYERS is known everywhere */
#define MAX_PLAYERS 5

#define LOG_QUEUE_SIZE 20
#define LOG_MSG_LEN 256

typedef struct {
    int player_count;
    int current_turn;
    int game_active;
    int chests[10];
    int scores[5];
    int turn_complete;
    int active_players[MAX_PLAYERS];

    char log_queue[LOG_QUEUE_SIZE][LOG_MSG_LEN];
    int log_head, log_tail, log_count;

    pthread_mutex_t log_mutex;
    sem_t log_sem;

    GameLogic logic;
} GameState;

#endif
