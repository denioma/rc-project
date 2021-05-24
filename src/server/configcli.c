#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "libs/users.h"

char prefix[64] = "[Admin CLI]";
extern int tcp_sock;
int client_fd;
struct sockaddr_in client;
socklen_t slen;
char ip[INET_ADDRSTRLEN];
unsigned short int port;

void accepting();

void cli_print(char* msg, char* prefix) {
    if (!prefix) printf("[Admin CLI] %s\n", msg);
    else printf("%s %s\n", prefix, msg);
}
void sndmsg(char* msg) {
    char buffer[BUFFSIZE];
    write(client_fd, msg, strlen(msg)+1);
    msg[strcspn(msg, "\n")] = 0;
    snprintf(buffer, sizeof(buffer), "<< %s", msg);
    cli_print(buffer, prefix);
}

void menu(char *opt) {
    printf("%s >> %s\n", prefix, opt);
    char buff[BUFFSIZE];
    if (strcmp(opt, "LIST") == 0) list_users();
    else if (strncmp(opt, "ADD", 3) == 0) sndmsg("Not yet implemented!\n");
    else if (strncmp(opt, "DEL", 3) == 0) del_user(opt);
    else if (strcmp(opt, "QUIT") == 0) {
        close(client_fd);
        accepting();
    }
    else if (strcmp(opt, "SHUTDOWN") == 0) kill(0, SIGINT); // FIX: Just for fun, delete later
    else {
        snprintf(buff, sizeof(buff), "%s: command not found\n", opt);
        sndmsg(buff);
    }
}

void serve_cli() {
    inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
    port = ntohs(client.sin_port);
    snprintf(prefix, sizeof(prefix), "[Admin CLI] %s:%d", ip, port);
    
    char buff[BUFFSIZE];
    int nread;
    cli_print(">> Connected", prefix);

    while (1) {
        nread = read(client_fd, buff, sizeof(buff));
        if (nread > 0) {
            buff[nread-1] = 0;
            menu(buff);
        } else break;
    }
}

void accepting() {
    memset(&client, 0, slen);
    client_fd = accept(tcp_sock, (struct sockaddr*)&client, &slen);
    if (client_fd > 0) {
        serve_cli();
        close(client_fd);
    }
}

void cli_setup() {
    slen = sizeof(client);
    pid_t mypid = getpid();
    pid_t parent = getppid();
    printf("[Admin CLI] PID: %u | PPID: %u\n", mypid, parent);
    cli_print("TCP socket accepting connections...", NULL);
    accepting();
}
