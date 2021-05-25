// Redes de Comunicação 2020/2021 - Projeto Final
// Rodrigo Alexandre da Mota Machado - 2019218299
// Rui Bernardo Lopes Rodrigues - 2019217573

#include <pthread.h>
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

int tcp_sock, udp_sock;
extern int client_fd;
pthread_t admincli;

void print(char* msg) {
    printf("[Server] %s\n", msg);
}

void server_close(int signum) {
    pthread_cancel(admincli);
    pthread_join(admincli, NULL);
    if (signum == SIGINT) print("Interrupt received. Closing server...");
    if (client_fd != -1 && close(client_fd))
        perror("[Server] Failed to close TCP client socket");
    else puts("[Server] TCP client socket closed");
    if (tcp_sock != -1 && close(tcp_sock))
        perror("[Server] Failed to close TCP greeter socket");
    else puts("[Server] TCP greeter socket closed");
    if (udp_sock != -1 && close(udp_sock))
        perror("Failed to close UDP socket");
    else puts("[Server] UDP socket closed");
    if (modified == true) {
        puts("[Server] Changes made to users");
        save_users(registry_file);
    }
    free_users(uroot);
    puts("\n[Server] Server closed");
    if (signum == SIGINT || signum == 0) exit(0);
    else exit(1);
}

// Accept TCP connections to manager registry file
extern void* cli_setup();
extern int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast);

void udp_listen();
void* proc_collector();

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./server <client port> <config port> <registry file>\n");
        exit(1);
    }

    tcp_sock = udp_sock = -1;
    const int client_port = atoi(*(argv + 1));
    const int config_port = atoi(*(argv + 2));
    registry_file = *(argv + 3);

    uroot = NULL;
    load_users(registry_file);
    modified = false;

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
    // if (fork() == 0) {
    //     cli_setup();
    // }
    pthread_create(&admincli, NULL, cli_setup, NULL);


    // pthread_create(&client_collector, NULL, proc_collector, NULL);
    udp_listen();

    return 0;
}

/* ----- UDP listener ----- */

struct sockaddr_in addr;
socklen_t slen;
void* serve_threaded(void *v_args);

typedef struct {
    char msg[BUFFSIZE];
    struct sockaddr_in addr;
} thread_arg;

void udp_listen() {
    int recvsize;
    pthread_t new_thread;
    while (1) {
        thread_arg* args = malloc(sizeof(*args));
        slen = sizeof(args->addr);
        if ((recvsize = recvfrom(udp_sock, args->msg, sizeof(args->msg), 0, (struct sockaddr*) &args->addr, &slen))) {
            args->msg[recvsize] = 0;
            pthread_create(&new_thread, NULL, serve_threaded, args);
        }
    }
}

/* ----- Serving clientes (Threaded) ----- */

void auth(char* msg, struct sockaddr_in* addr) {
    char username[ENTRYSIZE], pass[ENTRYSIZE];
    strtok(msg, " ");
    char *token = strtok(NULL, " ");
    strncpy(username, token, sizeof(username));
    token = strtok(NULL, " ");
    strncpy(pass, token, sizeof(pass));
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    
    user* find;
    char buff[BUFFSIZE];
    printf("[AUTH] Trying to authenticate %s: user: %s, pass: %s\n", ip, username, pass);
    if ((find = matchUser(username, pass, ip))) {
        puts("[AUTH] User authorized");
        int perms = find->server * 100 + find->p2p * 10 + find->multicast;
        snprintf(buff, sizeof(buff),"%d", perms);
        // TODO Mutex this (probably a named semaphore bc it's easier to open)
        sendto(udp_sock, "SUCCESS", strlen("SUCCESS")+1, 0, (struct sockaddr*) addr, sizeof(*addr));
        sendto(udp_sock, &perms, sizeof(perms), 0,(struct sockaddr*) addr, sizeof(*addr));
    } else {
        puts("User not authorized");
        sendto(udp_sock, "FAIL", strlen("FAIL")+1, 0, (struct sockaddr*) addr, sizeof(*addr));
    }
}

void user_bind(char *msg, struct sockaddr_in* addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    strtok(msg, " ");
    char* username = strtok(NULL, " ");
    user* client = findUser(username, ip); 
    if (client) {
        client->msg_addr = *addr; // IDEA consider deprecating this
        client->msgport = addr->sin_port;
        printf("[DEBUG] %d %d\n", addr->sin_port, client->msgport);
        printf("[User] %s online\n", client->username);
    }
}

void forward(char* msg, struct sockaddr_in* addr) {
    char username[ENTRYSIZE], payload[BUFFSIZE], * token;
    strtok(msg, " ");
    token = strtok(NULL, " ");
    if (!token) {
        puts("[MSG] Incomplete");
        sendto(udp_sock, "0", 2, 0, (struct sockaddr*) addr, sizeof(addr));
        return;   
    } puts(token);
    strncpy(username, token, sizeof(username));
    token = strtok(NULL, "\0");
    if (!token) {
        puts("[MSG] Incomplete");
        sendto(udp_sock, "0", 2, 0, (struct sockaddr*) addr, sizeof(addr));
        return;   
    } puts(token);
    strncpy(payload, token, sizeof(payload));
    snprintf(payload, sizeof(payload), "%s %s", username, token);
    printf("[DEBUG] MSG %s %s\n", username, payload);
    user* dest = findUser(username, NULL);
    if (dest && dest->msgport) {
        char response[] = "Message sent";
        puts(response);
        sendto(udp_sock, payload, strlen(payload)+1, 0, (struct sockaddr*) &dest->msg_addr, sizeof(dest->msg_addr));
        sendto(udp_sock, response, strlen(response)+1, 0, (struct sockaddr*) addr, sizeof(*addr));
    } else {
        char buff[] = "User not found";
        puts(buff);
        sendto(udp_sock, buff, strlen(buff)+1, 0, (struct sockaddr*) addr, sizeof(*addr));
    }
}

void peer(char* msg, struct sockaddr_in* addr) {
    strtok(msg, " ");
    char* username = strtok(NULL, "\0");
    // if (!username) {
        // TODO check if needed
    // }
    user* dest = findUser(username, NULL);
    char buff[BUFFSIZE];
    if (dest) {
        sendto(udp_sock, dest->ip, INET_ADDRSTRLEN, 0, (struct sockaddr*) addr, sizeof(*addr));
        sendto(udp_sock, dest->msgport, sizeof(dest->msgport), 0, (struct sockaddr*) addr, sizeof(*addr));
    }
}

void groupchat(char* msg, struct sockaddr_in* addr) {
    strtok(msg, " ");
    char* name = strtok(NULL, "\0");
    group* dest = new_group(name);
    if (!dest) {
        // TODO COMPLAIN
    }
    sendto(udp_sock, &dest->ip, sizeof(dest->ip), 0, (struct sockaddr*) addr, sizeof(*addr));
}

void disconnect(char* msg) {
    strtok(msg, " ");
    char *username = strtok(NULL, "\0");
    user* client = findUser(username, NULL);
    if (client) {
        client->msgport = 0;
        memset(&client->msg_addr, 0, sizeof(client->msg_addr));
        printf("[User] %s offline\n", client->username);
    }
}

void* serve_threaded(void *v_args) {
    thread_arg* args = (thread_arg*) v_args;
    char *msg = args->msg;
    struct sockaddr_in* addr = &(args->addr);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));
    unsigned short int port = ntohs(addr->sin_port);
    printf("[%s:%d] %s\n", ip, port, msg);
    if (strncmp(msg, "AUTH", 4) == 0) auth(msg, addr);
    else if (strncmp(msg, "BIND", 4) == 0) user_bind(msg, addr);
    else if (strncmp(msg, "MSG", 3) == 0) forward(msg, addr);
    else if (strncmp(msg, "PEER", 4) == 0) peer(msg, addr);
    else if (strncmp(msg, "GROUP", 5) == 0) groupchat(msg, addr);
    else if (strncmp(msg, "BYE", 3) == 0) disconnect(msg);
    else puts("What?");
    free(v_args);
    pthread_exit(NULL);
}

/* ----- Serving clients (Forks) ----- */

void* proc_collector() {
    printf("[Client collector] Started\n");
    pid_t collecting;
    while (1) {
        sleep(60);
        // puts("[CLI collector] Heartbeat");
        while((collecting = waitpid(0, NULL, WNOHANG)) > 0);
            // printf("[Client collector] Collected worker %u\n", collecting);
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
    while (fgets(entry, sizeof(entry), registry)) {
        // Parse username
        token = strtok(entry, ";");
        if (token) strncpy(user, token, sizeof(user));
        else continue;
        // Parse ip
        token = strtok(NULL, ";");
        if (token) strncpy(ip, token, sizeof(ip)); 
        else continue;
        // Parse password
        token = strtok(NULL, ";");
        if (token) strncpy(pass, token, sizeof(pass));
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
        token = strtok(NULL, "\r\n");
        if (token) {
            if (strcmp(token, "yes") == 0) multicast = true;
            else if (strcmp(token, "no") == 0) multicast = false;
            else continue;
        } else continue;
        add_user(user, pass, ip, cs, p2p, multicast);
    }
    fclose(registry);
}

void dump(FILE* stream) {
    if (!uroot) return;
    user* curr = uroot;
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