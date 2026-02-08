CC = gcc
CFLAGS = -Wall -pthread

SERVER_SRC = server.c game_logic.c scheduler.c
CLIENT_SRC = client.c

SERVER_BIN = server
CLIENT_BIN = client

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o game.log

