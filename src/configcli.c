#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFSIZE 512

char prefix[64] = "[Config CLI]";
extern int tcp_sock;
int client_fd;

/*
// TODO: Thinking about adding a process collector as a thread, seems a good idea
pthread_t cli_collector;
void* collector() {
    char buffer[BUFFSIZE];
    while (1) {
        sleep(60);
        pid_t collecting;
        while((collecting = waitpid(0, NULL, WNOHANG)) > 0)
            printf("[CLI collector] Collected worker %u\n", collecting);
    }
}
*/

int is_ack(char* buff) {
    return ((strcmp(buff, "ACK") != 0) ? 0 : 1);
}

void cli_print(char* msg, char* prefix) {
    if (!prefix) printf("[Config CLI] %s\n", msg);
    else printf("%s %s\n", prefix, msg);
}
void sndmsg(char* msg) {
    write(client_fd, msg, strlen(msg)+1);
    cli_print(msg, prefix);
}

void cli_term() {
    write(client_fd, "FIN", 4);
    if (close(client_fd) != 0)
        cli_print("Failed to close client socket (TCP)", prefix);
    else cli_print("Closed client socket (TCP)", prefix);
    exit(0);
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


void menu(char *opt) {
    char buff[BUFFSIZE];
    if (strcmp(opt, "LIST") == 0) sndmsg("Not yet implemented!");
    else if (strncmp(opt, "ADD", 3) == 0) sndmsg("Not yet implemented!");
    else if (strncmp(opt, "DEL", 3) == 0) sndmsg("Not yet implemented!");
    else if (strcmp(opt, "QUIT") == 0) cli_term();
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

    snprintf(prefix, sizeof(prefix), "[CLI worker %u]", getpid());
    
    char buff[BUFFSIZE];
    int nread;
    cli_print("Received TCP connection", prefix);
    while(1) {
        nread = read(client_fd, buff, sizeof(buff));
        if (nread > 0) menu(buff);
        else break;
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