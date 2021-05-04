#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFSIZE 512

extern int tcp_sock;
int client_fd;

int is_ack(char* buff) {
    return ((strcmp(buff, "ACK") != 0) ? 0 : 1);
}

void cli_print(char* msg, char* prefix) {
    if (!prefix) printf("[Config CLI] %s\n", msg);
    else printf("%s %s\n", prefix, msg);
}

void cli_term() {
    write(client_fd, "FIN", 4);
    if (close(client_fd) != 0)
        cli_print("Failed to close client socket (TCP)", NULL);
    else cli_print("Closed client socket (TCP)", NULL);
    exit(0);
}

void serve_cli() {
    // Define SIGINT action
    struct sigaction new;
    new.sa_handler = cli_term;
    sigemptyset(&new.sa_mask);
    new.sa_flags = 0;
    sigaction(SIGINT, &new, NULL);

    char* prefix = "[CLI worker]";
    pid_t mypid = getpid();
    pid_t parent = getppid();
    printf("%s PID: %u | PPID: %u\n", prefix, mypid, parent);
    
    char buff[BUFFSIZE];
    int nread;
    cli_print("Received TCP connection", prefix);
    write(client_fd, "Not yet implemented!", 21);
    nread = read(client_fd, buff, sizeof(buff));
    if (nread > 0) buff[nread] = 0;
    if (is_ack(buff)) cli_print("ACK received", prefix);
    cli_term();
}

void term(int signum) {
    pid_t collected;
    signal(SIGINT, SIG_IGN);
    if (signum == SIGINT) {
        puts("");
        cli_print("SIGINT received, closing workers...", NULL); 
        while ((collected = wait(NULL)) != -1)
            printf("[Config CLI] collected pid %u\n", collected);
        exit(0);
    }
}

void accepting(struct sockaddr_in addr, socklen_t slen) {
    pid_t mypid = getpid();
    pid_t parent = getppid();
    printf("[Config CLI] PID: %u | PPID: %u\n", mypid, parent);
    // Define SIGINT action
    struct sigaction new;
    new.sa_handler = term;
    sigemptyset(&new.sa_mask);
    new.sa_flags = 0;
    sigaction(SIGINT, &new, NULL);

    cli_print("TCP socket accepting connections...", NULL);
    while (1) {
        // Collecting any zombie processes
        while (waitpid(-1, NULL, WNOHANG) > 0);

        client_fd = accept(tcp_sock, (struct sockaddr*)&addr, &slen);
        if (client_fd > 0) {
            if (fork() == 0) {
                close(tcp_sock);
                serve_cli();
            }
        }
    }
}