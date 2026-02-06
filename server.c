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
#include "game_logic.h"
#include "scheduler.h"
#include "game_struct.h"
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>

#define PORT 9000
#define MAX_PLAYERS 5
#define MIN_PLAYERS 3
#define SHM_KEY 98765
#define SEM_KEY 43210

// have been out into game.struct.h
// // -------- SHARED MEMORY STRUCTURE --------
// typedef struct
// {
//     int player_count;
//     int current_turn;
//     int chests[10];
//     int scores[5];
//     int game_active;
//     int turn_complete;
//     int active_players[5];
//     char log_queue[LOG_QUEUE_SIZE][LOG_MSG_LEN];
//     int log_head, log_tail, log_count;
//     pthread_mutex_t log_mutex;
//     sem_t log_sem;
// } GameState;

// Global pointers (simplifies cleanup)
int shm_id;
int sem_id;
GameState *game;

// -------- SEMAPHORE FUNCTIONS --------
void sem_lock()
{
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock()
{
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}
// -------- CLEANUP HANDLER --------
void cleanup(int sig)
{
    printf("\n[!] Server shutting down. Saving scores...\n");

    save_scores();
    shmdt(game);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    exit(0);
}

void load_scores()
{
    FILE *fp = fopen("score.txt", "r");
    if (fp == NULL)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
            game->scores[i] = 0;
        return;
    }
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (fscanf(fp, "%d", &game->scores[i]) == EOF)
            break;
    }
    fclose(fp);
}

void save_scores()
{
    sem_lock();
    FILE *fp = fopen("score.txt", "w");
    if (fp)
    {
        for (int i = 0; i < game->player_count; i++)
        {
            fprintf(fp, "%d\n", game->scores[i]);
        }
        fflush(fp);
        fclose(fp);
    }
    sem_unlock();
}
// -------- CLIENT HANDLER --------
void handle_client(int client_sock, int player_id)
{
    srand(time(NULL) ^ (getpid() <<16));
    char buffer[1024];
    char game_msg[256];
    int n;

    sprintf(game_msg, "Welcome Player %d! Waiting for game to start...\n", player_id + 1);
    send(client_sock, game_msg, strlen(game_msg), 0);

    int last_turn = -1;

    while (1)
    {
        if(game->logic.winner !=-1){
            char final_msg[50];
            sprintf(final_msg,"\nGame Over!The winner is Player %d\n",game->logic.winner+1);
            send(client_sock, final_msg, strlen(final_msg),0);
            sleep(2);

            close(client_sock);
            exit(0);
            break;
        }
        // prompt once when it's your turn
        if (game->current_turn != player_id)
            last_turn = -1;

        if (game->game_active && game->current_turn == player_id && last_turn != game->current_turn)
        {
            send(client_sock, "Your turn! Type 'roll':\n", 25, 0);
            last_turn = game->current_turn;
        }

        // wait for input, but with timeout so loop keeps running
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(client_sock, &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 0.2 sec

        int ready = select(client_sock + 1, &fds, NULL, NULL, &tv);
        if (ready == 0)
        {
            continue; // no input yet, loop again (so it can notice turn changes)
        }
        if (ready < 0)
        {
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0)
        {
            printf("[-] Player %d disconnected.\n", player_id + 1);
            break;
        }

        // ONLY process roll if it's your turn
        if (strncmp(buffer, "roll", 4) == 0)
        {
            if (game->current_turn == player_id)
            {

                int dice = (rand() % 6) + 1;

                sem_lock();
                play_turn(&game->logic, player_id, dice, game_msg, sizeof(game_msg));

                if (game->logic.winner != -1)
                {
                    game->scores[player_id]++;
                    save_scores();

                send(client_sock, game_msg, strlen(game_msg), 0);

                char win_banner[50];
                sprintf(win_banner,"\n Congratulations to PLAYER %d!YOU WON THIS GAME!\n",player_id+ 1);
                send(client_sock, win_banner,strlen(win_banner),0);
                }
                else{
                    send(client_sock, game_msg,strlen(game_msg),0);
                }

                game->turn_complete = 1;
                sem_unlock();

                enqueue_log(game_msg);
    
            }
        }
    }

    close(client_sock);
    exit(0);
}

// -------- MAIN SERVER --------
int main()
{
    srand(time(NULL) ^ getpid());
    
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGINT, cleanup);  // Ctrl+C cleanup
    signal(SIGCHLD, SIG_IGN); // Prevent zombies

    // -------- SHARED MEMORY --------
    shm_id = shmget(SHM_KEY, sizeof(GameState), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("Shared memory failed");
        exit(1);
    }

    game = (GameState *)shmat(shm_id, NULL, 0);
    memset(game, 0, sizeof(GameState));

    game->logic.winner =-1;

    load_scores();

    // -------- SEMAPHORE --------
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    printf("[+] Shared memory & semaphore initialized.\n");

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->log_mutex, &attr);
    pthread_mutexattr_destroy(&attr); // Clean up attribute

    sem_init(&game->log_sem, 1, 0);

    pthread_t tid_log, tid_sched;
    if (pthread_create(&tid_log, NULL, logger_thread, NULL) != 0)
    {
        perror("Failed to create logger thread");
    }
    if (pthread_create(&tid_sched, NULL, scheduler_thread, NULL) != 0)
    {
        perror("Failed to create scheduler thread");
    }

    printf("[+] Scheduler and Logger threads started.\n");

    // -------- SOCKET SETUP --------
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    listen(server_sock, MAX_PLAYERS);
    printf("[+] Server listening on port %d\n", PORT);

    // -------- MAIN LOOP --------
    while (1)
    {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock,
                             (struct sockaddr *)&client_addr,
                             &addr_size);

        if (client_sock < 0)
            continue;

        sem_lock();

        if (game->player_count >= MAX_PLAYERS)
        {
            sem_unlock();
            close(client_sock);
            continue;
        }

        int my_id = game->player_count;
        game->player_count++;

        printf("[+] Player %d connected from %s\n",
               my_id + 1, inet_ntoa(client_addr.sin_addr));

        if (game->player_count >= MIN_PLAYERS && !game->game_active)
        {
            game->game_active = 1;

            init_game(&game->logic, game->player_count);

            game->logic.winner =-1;

            game->current_turn = 0;  // ✅ FIRST TURN
            game->turn_complete = 0; // ✅ READY FOR SCHEDULER

            printf("[!] Game can start (%d players).\n",
                   game->player_count);
        }

        sem_unlock();

        if (fork() == 0)
        {
            close(server_sock);
            handle_client(client_sock, my_id);
        }

        close(client_sock);
    }

    return 0;
}
