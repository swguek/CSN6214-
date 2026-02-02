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

#define PORT 9000
#define MAX_PLAYERS 5
#define MIN_PLAYERS 3
#define SHM_KEY 98765
#define SEM_KEY 43210

// -------- SHARED MEMORY STRUCTURE --------
typedef struct
{
    int player_count;
    int current_turn;
    int chests[10];
    int scores[5];
    int game_active;
} GameState;

// Global pointers (simplifies cleanup)
int shm_id;
int sem_id;
GameState *game;
GameLogic game_logic;

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
    printf("\n[!] Server shutting down. Cleaning resources...\n");
    shmdt(game);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    exit(0);
}

// -------- CLIENT HANDLER --------
void handle_client(int client_sock, int player_id)
{
    char buffer[1024];
    int n;

    char welcome[200];
    sprintf(welcome,
            "Welcome Player %d! Waiting for game to start...\n",
            player_id + 1);
    send(client_sock, welcome, strlen(welcome), 0);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        n = recv(client_sock, buffer, sizeof(buffer), 0);

        if (n <= 0)
        {
            printf("[-] Player %d disconnected.\n", player_id + 1);
            break;
        }

        printf("Player %d says: %s", player_id + 1, buffer);
    }

    close(client_sock);
    exit(0);
}

// -------- MAIN SERVER --------
int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGINT, cleanup);      // Ctrl+C cleanup
    signal(SIGCHLD, SIG_IGN);     // Prevent zombies

    // -------- SHARED MEMORY --------
    shm_id = shmget(SHM_KEY, sizeof(GameState), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("Shared memory failed");
        exit(1);
    }

    game = (GameState *)shmat(shm_id, NULL, 0);
    memset(game, 0, sizeof(GameState));

    // -------- SEMAPHORE --------
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    printf("[+] Shared memory & semaphore initialized.\n");

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
	    init_game(&game_logic, game->player_count);
            printf("[!] Game can start (%d players).\n",
                   game->player_count);
        }

        sem_unlock();

        if (fork() == 0)
        {
            close(server_sock);
            handle_client(client_sock, my_id);
        }

        close(client_if (game->player_count >= MIN_PLAYERS && !game->game_active)
{
    game->game_active = 1;

    init_game(&game_logic, game->player_count);

    printf("[!] Game can start (%d players).\n",
           game->player_count);
}
sock);
    }

    return 0;
}
