#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

int tcp_sock, udp_sock;

void print(char *msg) {
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
    if (signum == SIGINT || signum == 0) exit(0);
    else exit(1);
}

extern void accepting();

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./server <client port> <config port> <registry file>\n");
        exit(1);
    }
    tcp_sock = udp_sock = -1;
    const int client_port = atoi(*(argv+1));
    const int config_port = atoi(*(argv+2));
    // TODO: Enable support for registry file
    // const char *registry = *(argv+3);
    
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
    pause();

    return 0;
}