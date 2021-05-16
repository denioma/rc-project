#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFSIZE 512

int sock;

void term(int signum) {
    if (signum == SIGINT) {
        write(sock, "QUIT", 5);
        puts("");
    }
    if (close(sock))
        perror("Failed to close client socket");
    puts("Connection closed");
    exit(0);
}

int is_fin(char *buff) {
    return ((strcmp(buff, "FIN") != 0) ? 0 : 1);
}

int recieve(int sock, char *buff, size_t size) {
    int nread = read(sock, buff, size);
    if (nread > 0) buff[nread] = 0;
    if (is_fin(buff)) term(-1);
    return nread;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server.app <host> <port>\n");
        exit(1);
    }

    struct sigaction sigint;
    sigset_t blocking;
    sigemptyset(&blocking);
    sigaddset(&blocking, SIGINT);
    sigint.sa_handler = term;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);

    const char *hostname = *(argv+1);
    const short int port = atoi(*(argv+2));

    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (!(hostPtr = gethostbyname(hostname))) {
        perror("Failed to get host");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr*)hostPtr->h_addr_list[0])->s_addr;
    addr.sin_port = htons(port);
    socklen_t slen = sizeof(addr);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        exit(-1);
    }
  
    if (connect(sock, (struct sockaddr*)&addr, slen) < 0) {
        perror("Failed to connect to server");
        exit(-1);
    }

/*
    while ((nread = recieve(sock, buff, sizeof(buff))) > 0) {
        if (nread > 0) {
            if (is_fin(buff)) break;
            printf("Received >> %s (%d bytes)\n", buff, nread);
            write(sock, "ACK", 4);
        } else break;
        memset(buff, 0, sizeof(buff));
    }
*/

    char buff[BUFFSIZE];
    int nread;
    do {
        memset(buff, 0, sizeof(buff));
        printf(">> ");
        fgets(buff, sizeof(buff) - 1, stdin);
        buff[strcspn(buff, "\n")] = 0;
        write(sock, buff, strlen(buff) + 1);
        sigprocmask(SIG_BLOCK, &blocking, NULL);
        memset(buff, 0, sizeof(buff));
        nread = recieve(sock, buff, sizeof(buff));
        if (nread > 0)
            printf("%s\n\n", buff);
        sigprocmask(SIG_UNBLOCK, &blocking, NULL);
    } while(strcmp(buff, "QUIT") != 0 && nread > 0);

    close(sock);

    return 0;
}