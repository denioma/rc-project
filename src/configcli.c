#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFFSIZE 512

char prefix[64] = "[Config CLI]";
extern int tcp_sock;
int client_fd;
struct sockaddr_in client;

// Defunct process collection
pthread_t cli_collector;
void* collector() {
    printf("[CLI collector] Started\n");
    pid_t collecting;
    while (1) {
        sleep(60);
        puts("[CLI collector] Heartbeat");
        while((collecting = waitpid(0, NULL, WNOHANG)) > 0)
            printf("[CLI collector] Collected worker %u\n", collecting);
    }
}

int is_ack(char* buff) {
    return ((strcmp(buff, "ACK") != 0) ? 0 : 1);
}

void cli_print(char* msg, char* prefix) {
    if (!prefix) printf("[Config CLI] %s\n", msg);
    else printf("%s %s\n", prefix, msg);
}
void sndmsg(char* msg) {
    char buffer[BUFFSIZE];
    write(client_fd, msg, strlen(msg)+1);
    snprintf(buffer, sizeof(buffer), "<< %s", msg);
    cli_print(buffer, prefix);
}

void cli_term(int signum) {
    if (signum == SIGINT) puts("");
    write(client_fd, "FIN", 4);
    if (close(client_fd) != 0)
        cli_print("Failed to close client socket (TCP)", prefix);
    else cli_print(">> Connection closed", prefix);
    exit(0);
}

void term(int signum) {
    signal(SIGINT, SIG_IGN);
    if (signum == SIGINT) {
        puts("");
        cli_print("SIGINT received, closing workers...", NULL); 
        pthread_cancel(cli_collector);
        pthread_join(cli_collector, NULL);
        while (wait(NULL) != -1)
        exit(0);
    }
}


void menu(char *opt) {
    char buff[BUFFSIZE];
    if (strcmp(opt, "LIST") == 0) sndmsg("Not yet implemented!");
    else if (strncmp(opt, "ADD", 3) == 0) sndmsg("Not yet implemented!");
    else if (strncmp(opt, "DEL", 3) == 0) sndmsg("Not yet implemented!");
    else if (strcmp(opt, "QUIT") == 0) cli_term(-1);
    else if (strcmp(opt, "SHUTDOWN") == 0) kill(0, SIGINT); // FIX: Just for fun, delete later
    else {
        snprintf(buff, sizeof(buff), "%s: command not found", opt);
        sndmsg(buff);
    }
}

void serve_cli() {
    // Define SIGINT action
    struct sigaction new;
    new.sa_handler = cli_term;
    sigemptyset(&new.sa_mask);
    new.sa_flags = 0;
    sigaction(SIGINT, &new, NULL);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
    int port = ntohs(client.sin_port);
    snprintf(prefix, sizeof(prefix), "[CLI worker %u] %s:%d", getpid(), ip, port);
    
    char buff[BUFFSIZE];
    int nread;
    cli_print(">> Connected", prefix);

    while(1) {
        nread = read(client_fd, buff, sizeof(buff));
        if (nread > 0) menu(buff);
        else break;
    }
}

void accepting() {
    socklen_t slen = sizeof(client);
    memset(&client, 0, slen);
    pid_t mypid = getpid();
    pid_t parent = getppid();
    printf("[Config CLI] PID: %u | PPID: %u\n", mypid, parent);
    // Override SIGINT
    struct sigaction new;
    new.sa_handler = term;
    sigemptyset(&new.sa_mask);
    new.sa_flags = 0;
    sigaction(SIGINT, &new, NULL);
    pthread_create(&cli_collector, NULL, collector, NULL);
    cli_print("TCP socket accepting connections...", NULL);
    while (1) {
        // Collector thread will pickup any defunct process every 60s
        client_fd = accept(tcp_sock, (struct sockaddr*)&client, &slen);
        if (client_fd > 0) {
            if (fork() == 0) {
                close(tcp_sock);
                serve_cli();
            }
        }
    }
}