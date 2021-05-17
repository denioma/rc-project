#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "libs/users.h"

void load_users(char* file);
int save_users(char* file);
void free_users(user* node);

int tcp_sock, udp_sock;
pthread_t client_collector;

void print(char* msg) {
    printf("[Server] %s\n", msg);
}

void server_close(int signum) {
    while (wait(NULL) != -1);
    if (signum == SIGINT) print("Interrupt received. Closing server...");
    if (tcp_sock != -1 && close(tcp_sock))
        perror("[Server] Failed to close TCP socket");
    else print("TCP socket closed");
    if (udp_sock != -1 && close(udp_sock))
        perror("Failed to close UDP socket");
    else puts("[Server] UDP socket closed");
    puts("\n[Server] Server closed");
    if (modified) save_users(registry_file);
    free_users(root);
    if (signum == SIGINT || signum == 0) exit(0);
    else exit(1);
}

// Accept TCP connections to manager registry file
extern void accepting();
extern int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast);

void udp_listen();

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./server <client port> <config port> <registry file>\n");
        exit(1);
    }

    tcp_sock = udp_sock = -1;
    const int client_port = atoi(*(argv + 1));
    const int config_port = atoi(*(argv + 2));
    registry_file = *(argv + 3);

    root = NULL;
    modified = false;
    load_users(registry_file);

    // Change to sigaction
    signal(SIGINT, server_close);

    struct sockaddr_in server_tcp, server_udp;
    memset(&server_tcp, 0, sizeof(server_tcp));
    memset(&server_udp, 0, sizeof(server_udp));

    server_tcp.sin_family = AF_INET;
    server_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
    server_tcp.sin_port = htons(config_port);

    server_udp.sin_family = AF_INET;
    server_udp.sin_addr.s_addr = htonl(INADDR_ANY);
    server_udp.sin_port = htons(client_port);

    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[Server] Failed to create TCP socket");
        server_close(-1);
    } else puts("[Server] TCP socket created");

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("[Server] Failed to create UDP socket");
        server_close(-1);
    } else puts("[Server] UDP socket created");

    if (bind(tcp_sock, (struct sockaddr*)&server_tcp, sizeof(server_tcp)) == -1) {
        perror("[Server] Failed to listen on TCP socket");
        server_close(-1);
    } else puts("[Server] Bound TCP socket");

    if (bind(udp_sock, (struct sockaddr*)&server_udp, sizeof(server_udp)) == -1) {
        perror("Failed to listen on UDP socket");
        server_close(-1);
    } else puts("[Server] Bound UDP socket");

    // TODO: Check if useful to increase backlog
    if (listen(tcp_sock, 5) == -1) {
        perror("[Server] Failed to listen on socket");
        server_close(-1);
    } else puts("[Server] Listening on TCP socket");

    puts("[Server] Send ^C to close server");
    if (fork() == 0) {
        accepting();
    }

    udp_listen();
    pause();

    return 0;
}

/* ----- UDP listener ----- */

struct sockaddr_in addr;
socklen_t slen;
void serve(char *msg);

void udp_listen() {
    slen = sizeof(addr);
    char buff[BUFFSIZE];
    while (1) {
        if (recvfrom(udp_sock, buff, sizeof(buff), 0, (struct sockaddr*)&addr, &slen) && fork() == 0) {
            serve(buff);
            exit(0);
        }
    }
}

/* ----- Serving clients ----- */

void auth(char *msg) {
    char username[ENTRYSIZE], pass[ENTRYSIZE];
    strtok(msg, " ");
    char *token = strtok(NULL, " ");
    strncpy(username, token, sizeof(username));
    token = strtok(NULL, " ");
    strncpy(pass, token, sizeof(pass));
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    
    user* find;
    char buff[BUFFSIZE];
    if ((find = findUser(username, pass, ip))) {
        int perms = find->server * 100 + find->p2p * 10 + find->multicast;
        snprintf(buff, sizeof(buff),"%d", perms);
        // TODO Mutex this
        sendto(udp_sock, "SUCCESS", strlen("SUCCESS")+1, 0, (struct sockaddr*) &addr, slen);
        sendto(udp_sock, &perms, sizeof(perms), 0,(struct sockaddr*) &addr, slen);
    } else {
        sendto(udp_sock, "FAIL", strlen("FAIL")+1, 0, (struct sockaddr*) &addr, slen);
    }
}

void serve(char *msg) {
    if (strncmp(msg, "AUTH", 4) == 0) auth(msg);
    else puts("What?");
}   

void* proc_collector() {
    printf("[Client collector] Started\n");
    pid_t collecting;
    while (1) {
        sleep(60);
        puts("[CLI collector] Heartbeat");
        while((collecting = waitpid(0, NULL, WNOHANG)) > 0)
            printf("[Client collector] Collected worker %u\n", collecting);
    }
}

/* ----- Registry File Loading and Unloading ----- */

void load_users(char* file) {
    FILE* registry = fopen(file, "r");
    if (!registry) {
        perror("Failed to open registry file");
        exit(1);
    }

    char* token;
    char entry[BUFFSIZE];
    char user[ENTRYSIZE], pass[ENTRYSIZE], ip[INET_ADDRSTRLEN];
    bool cs, p2p, multicast;
    while (1) {
        if (!fgets(entry, sizeof(entry), registry)) break;
        // Parse username
        token = strtok(entry, ";");
        if (token) strncpy(user, token, sizeof(user));
        else continue;
        // Parse password
        token = strtok(NULL, ";");
        if (token) strncpy(pass, token, sizeof(pass));
        else continue;
        // Parse ip
        token = strtok(NULL, ";");
        if (token) strncpy(ip, token, sizeof(ip)); 
        else continue;
        // Parse cs
        token = strtok(NULL, ";");
        if (token) {
            if (strcmp(token, "yes") == 0) cs = true;
            else if (strcmp(token, "no") == 0) cs = false;
            else continue;
        } else continue;
        // Parse p2p
        token = strtok(NULL, ";");
        if (token) {
            if (strcmp(token, "yes") == 0) p2p = true;
            else if (strcmp(token, "no") == 0) p2p = false;
            else continue;
        } else continue;
        // Parse multicast
        token = strtok(NULL, "\n");
        if (token) {
            if (strcmp(token, "yes") == 0) multicast = true;
            else if (strcmp(token, "no") == 0) multicast = false;
            else continue;
        } else continue;

        if (!add_user(user, pass, ip, cs, p2p, multicast))
            puts("[Server] USER");
    }

    fclose(registry);
}

void dump(FILE* stream) {
    if (!root) return;
    user* curr = root;
    char yes[] = "yes", no[] = "no";
    char* cs, * p2p, * multicast;
    while (1) {
        if (curr->server) cs = yes;
        else cs = no;
        if (curr->p2p) p2p = yes;
        else p2p = no;
        if (curr->multicast) multicast = yes;
        else multicast = no;
        fprintf(stream, "%s;%s;%s;%s;%s;%s\n",
            curr->username, curr->ip, curr->pass, cs, p2p, multicast);
        if (curr->next) curr = curr->next;
        else break;
    }
}

int save_users(char* file) {
    FILE* registry = fopen(file, "w");
    if (!registry) {
        perror("Failed to open registry file");
        dump(stdout);
        return 1;
    }
    dump(registry);
    fclose(registry);
    return 0;
}

void free_users(user* node) {
    if (!node) return;
    if (node->next) free_users(node->next);
    free(node);
}