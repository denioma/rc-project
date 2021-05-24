CC = gcc
FLAGS = -Wall -Wextra -g -pthread

all: client.app server.app admin.app

client.app: src/client/client.c
	${CC} ${FLAGS} $< -o $@

server.app: src/server/server.c src/server/configcli.c src/server/users.c
	${CC} ${FLAGS} $^ -o $@

admin.app: src/client/admin.c
	${CC} ${FLAGS} $< -o $@

clean:
	@rm -f server.app client.app admin.app
