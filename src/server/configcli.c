// Redes de Comunicação 2020/2021 - Projeto Final
// Rodrigo Alexandre da Mota Machado - 2019218299
// Rui Bernardo Lopes Rodrigues - 2019217573

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
    // msg[strcspn(msg, "\n")] = 0;
    snprintf(buffer, sizeof(buffer), "<< %s", msg);
    cli_print(buffer, prefix);
}

void parsing_err() {
    sndmsg("Parsing error\n");
}

void new_user(char* opt) {
    strtok(opt, " ");
    char username[ENTRYSIZE], pass[ENTRYSIZE], ip[INET_ADDRSTRLEN];
    bool cs, p2p, multicast;
    char* token = strtok(NULL, " ");
    // Parse username
    if (!token) {
        parsing_err();
        return;
    }
    strncpy(username, token, sizeof(username));
    // Parse IP address
    token = strtok(NULL, " ");
    if (!token) {
        parsing_err();
        return;
    }
    strncpy(ip, token, sizeof(ip));
    // Parse password
    token = strtok(NULL, " ");
    if (!token) {
        parsing_err();
        return;
    }
    strncpy(pass, token, sizeof(pass));
    // Parse permissions
    token = strtok(NULL, " ");
    if (token) {
        if (strcmp(token, "yes") == 0) cs = true;
        else if (strcmp(token, "no") == 0) cs = false;
    } else {
        parsing_err();
        return;
    }
    token = strtok(NULL, " ");
    if (token) {
        if (strcmp(token, "yes") == 0) p2p = true;
        else if (strcmp(token, "no") == 0) p2p = false;    
    } else {
        parsing_err();
        return;
    }
    token = strtok(NULL, "\r\n\0");
    if (token) {
        if (strcmp(token, "yes") == 0) multicast = true;
        else if (strcmp(token, "no") == 0) multicast = false;
    } else {
        parsing_err();
        return;
    }
    
    if (add_user(username, pass, ip, cs, p2p, multicast)) sndmsg("[ADD] Failed to add user\n");
    else sndmsg("[ADD] User added\n");
}

void menu(char *opt) {
    printf("%s >> %s\n", prefix, opt);
    char buff[BUFFSIZE];
    if (strcmp(opt, "LIST") == 0) list_users();
    else if (strncmp(opt, "ADD", 3) == 0) new_user(opt);
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
        client_fd = -1;
    }
}

void* cli_setup() {
    slen = sizeof(client);
    pid_t mypid = getpid();
    pid_t parent = getppid();
    printf("[Admin CLI] PID: %u | PPID: %u\n", mypid, parent);
    cli_print("TCP socket accepting connections...", NULL);
    accepting();
    return NULL;
}
