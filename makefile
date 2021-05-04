CC = gcc
FLAGS = -Wall -Wextra

all: client.app server.app admin.app

client.app: src/client.c
	${CC} ${FLAGS} $< -o $@

server.app: src/server.c src/configcli.c
	${CC} ${FLAGS} $^ -o $@

admin.app: src/admin.c
	${CC} ${FLAGS} $< -o $@

clean:
	@rm -f server.app client.app admin.app