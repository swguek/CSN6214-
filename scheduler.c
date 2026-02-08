#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scheduler.h"
#include "game_struct.h"

extern GameState *game;

// add to log queue
void enqueue_log(char *msg) {
    if (!game) return;

    pthread_mutex_lock(&game->log_mutex);

    if (game->log_count < LOG_QUEUE_SIZE) {
        snprintf(game->log_queue[game->log_tail], LOG_MSG_LEN, "%s", msg);
        game->log_tail = (game->log_tail + 1) % LOG_QUEUE_SIZE;
        game->log_count++;
        sem_post(&game->log_sem);
    }

    pthread_mutex_unlock(&game->log_mutex);
}

// Logger Thread
void *logger_thread(void *arg) {
    printf("[Logger] Thread started.\n");
    FILE *fp;
    char buffer[LOG_MSG_LEN];

    while (1) {
        sem_wait(&game->log_sem);

        pthread_mutex_lock(&game->log_mutex);
        if (game->log_count > 0) {
            strncpy(buffer, game->log_queue[game->log_head], LOG_MSG_LEN);
            game->log_head = (game->log_head + 1) % LOG_QUEUE_SIZE;
            game->log_count--;
        }
        pthread_mutex_unlock(&game->log_mutex);

        fp = fopen("game.log", "a");
        if (fp) {
            fprintf(fp, "%s\n", buffer);
            fclose(fp);
        }
    }
    return NULL;
}

// Round Robin Scheduler
void *scheduler_thread(void *arg) {
    printf("[Scheduler] Thread started.\n");
    char msg[100];

    while (1) {
        if (!game || !game->game_active) {
            usleep(100000);
            continue;
        }

        /* -------- GAME ENDED → RESET FOR NEW GAME -------- */
        if (game->logic.winner != -1) {
            enqueue_log("Game ended. Resetting for next round.");

            sleep(1); // small pause so clients see winner

            game->logic.winner = -1;
            game->turn_complete = 0;
            game->current_turn = 0;

            enqueue_log("New game started.");
            usleep(200000);
            continue;
        }

        if (game->turn_complete) {
            snprintf(msg, 100, "Turn Finished: Player %d", game->current_turn + 1);
            enqueue_log(msg);

            /* -------- FIND NEXT ACTIVE PLAYER -------- */
            int next = game->current_turn;
            int checked = 0;

            do {
                next = (next + 1) % game->player_count;
                checked++;
            } while (!game->active_players[next] && checked <= game->player_count);

            game->current_turn = next;
            game->turn_complete = 0;

            snprintf(msg, 100, "Turn Start: Player %d", game->current_turn + 1);
            enqueue_log(msg);
        }

        usleep(100000);
    }
    return NULL;
}
