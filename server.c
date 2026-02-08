#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>

#include "game_logic.h"
#include "scheduler.h"
#include "game_struct.h"

#define PORT 9000
#define MAX_PLAYERS 5
#define MIN_PLAYERS 3
#define SHM_KEY 98765
#define SEM_KEY 43210

int shm_id;
int sem_id;
GameState *game;

/* ✅ parent PID so only parent writes scores + frees IPC */
static pid_t parent_pid;

void sem_lock() {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock() {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

static void load_scores();

/* ✅ ONLY parent writes the file */
static void save_scores_parent_only() {
    sem_lock();
    FILE *fp = fopen("scores.txt", "w");
    if (fp) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            fprintf(fp, "%d\n", game->scores[i]);
        }
        fclose(fp);
    }
    sem_unlock();
}

/* ✅ cleanup: children exit silently, parent saves + removes shm/sem */
static void cleanup(int sig) {
    (void)sig;

    if (getpid() != parent_pid) {
        exit(0);
    }

    printf("\n[!] Server shutting down. Saving scores...\n");
    save_scores_parent_only();

    shmdt(game);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    exit(0);
}

static void load_scores() {
    FILE *fp = fopen("scores.txt", "r");
    if (!fp) {
        /* if no file yet, start from 0 */
        for (int i = 0; i < MAX_PLAYERS; i++) game->scores[i] = 0;
        fp = fopen("scores.txt", "w");
        if (fp) {
            for (int i = 0; i < MAX_PLAYERS; i++) fprintf(fp, "0\n");
            fclose(fp);
        }
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (fscanf(fp, "%d", &game->scores[i]) == EOF) {
            game->scores[i] = 0;
        }
    }
    fclose(fp);
}

static void handle_client(int client_sock, int player_id) {
    srand((unsigned)(time(NULL) ^ getpid() ^ (player_id << 8)));

    char buffer[1024];
    char msg[256];

    snprintf(msg, sizeof(msg),
             "Welcome Player %d!\nRULES: Reach tile %d to WIN. BOOST goes up, BACK goes down, TRAP skips.\n",
             player_id + 1, BOARD_SIZE);
    send(client_sock, msg, strlen(msg), 0);

    int last_turn = -1;

    while (1) {
        if (game->logic.winner != -1) {
            char final_msg[120];
            snprintf(final_msg, sizeof(final_msg),
                     "\nGAME OVER! Winner is Player %d (Reached %d)\n",
                     game->logic.winner + 1, BOARD_SIZE);
            send(client_sock, final_msg, strlen(final_msg), 0);
            usleep(300000);
            break;
        }

        if (game->current_turn != player_id) last_turn = -1;

        if (game->game_active && game->current_turn == player_id && last_turn != game->current_turn) {
            send(client_sock, "Your turn! Press Enter (client auto) or type roll:\n", 54, 0);
            last_turn = game->current_turn;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(client_sock, &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;

        int ready = select(client_sock + 1, &fds, NULL, NULL, &tv);
        if (ready <= 0) continue;

        memset(buffer, 0, sizeof(buffer));
        int n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        if (strncmp(buffer, "roll", 4) == 0 || buffer[0] == '\n' || buffer[0] == '\r') {
            if (game->current_turn == player_id) {
                int dice = (rand() % 6) + 1;

                sem_lock();
                play_turn(&game->logic, player_id, dice, msg, sizeof(msg));

                /* ✅ ONLY increment in shared memory; parent will write to file */
                if (game->logic.winner == player_id) {
                    game->scores[player_id]++;
                }

                game->turn_complete = 1;
                sem_unlock();

                enqueue_log(msg);
                send(client_sock, msg, strlen(msg), 0);
            } else {
                send(client_sock, "Wait your turn.\n", 16, 0);
            }
        }
    }

    close(client_sock);
    exit(0);
}

int main() {
    parent_pid = getpid();

    signal(SIGINT, cleanup);
    signal(SIGCHLD, SIG_IGN);

    shm_id = shmget(SHM_KEY, sizeof(GameState), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    game = (GameState *)shmat(shm_id, NULL, 0);
    memset(game, 0, sizeof(GameState));
    game->logic.winner = -1;

    /* ✅ make sure scores are always valid */
    load_scores();

    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    printf("[+] Shared memory & semaphore initialized.\n");

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->log_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    sem_init(&game->log_sem, 1, 0);

    pthread_t tid_log, tid_sched;
    pthread_create(&tid_log, NULL, logger_thread, NULL);
    pthread_create(&tid_sched, NULL, scheduler_thread, NULL);

    printf("[+] Scheduler and Logger threads started.\n");

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(server_sock, MAX_PLAYERS);
    printf("[+] Server listening on port %d\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        sem_lock();
        if (game->player_count >= MAX_PLAYERS) {
            sem_unlock();
            close(client_sock);
            continue;
        }

        int my_id = game->player_count;
        game->player_count++;

        printf("[+] Player %d connected from %s\n", my_id + 1, inet_ntoa(client_addr.sin_addr));

        if (game->player_count >= MIN_PLAYERS && !game->game_active) {
            game->game_active = 1;

            init_game(&game->logic, game->player_count);
            game->logic.winner = -1;

            game->current_turn = 0;
            game->turn_complete = 0;

            printf("[!] Game can start (%d players).\n", game->player_count);
            enqueue_log("Game started (need reach 50 to win)");
        }
        sem_unlock();

        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock, my_id);
        }

        close(client_sock);
    }

    return 0;
}
