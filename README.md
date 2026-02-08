1. How to Compile

This project uses a Makefile to compile the server and client programs.

To compile the project, run:

make clean
make


This will generate two executables:

server

client

2. How to Run
Step 1: Start the server
./server


The server will start listening on port 9000 and wait for clients to connect.

Step 2: Start the clients (minimum 3 players)

Open separate terminals and run:

./client


Run the command at least three times to start the game.

3. Example Commands
Compilation
make clean
make

Running
./server
./client
./client
./client

Viewing logs
cat game.log

Viewing scores
cat scores.txt

4. Game Rules Summary

This is a text-based, turn-based multiplayer game.

A minimum of 3 players is required to start the game.

Each player takes turns rolling a dice.

All gameplay updates are displayed in the terminal.

Special Tile Effects:

BOOST: Moves the player forward.

BACK: Moves the player backward.

TRAP: Player skips the next turn.

Win Condition:

The first player to reach the final position wins the game.

After the game ends, the winner is announced to all players.

The winning player’s score is updated in scores.txt.

5. Mode Supported

Multiplayer mode (minimum 3 players, maximum 5 players)

Text-based interface (terminal)

Client-server architecture using TCP

Concurrent processing using:

Multiple processes (fork)

Threads (pthread)

Shared memory and semaphores

Persistent scoring using file storage

6. Additional Notes

The server uses a round-robin scheduler to manage player turns.

A dedicated logger thread records game events in real time to game.log.

Shared memory is used to maintain a consistent game state across processes.