CC = gcc
FLAGS = -Wall -Wextra

all: client server

client: src/client.c
	${CC} ${FLAGS} $< -o $@

server: src/server.c
	${CC} ${FLAGS} $< -o $@
