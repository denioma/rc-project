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

void term() {
    if (close(sock))
        perror("Failed to close client socket");
    printf(">> Connection closed\n");
    exit(0);
}

int is_fin(char *buff) {
    return ((strcmp(buff, "FIN") != 0) ? 0 : 1);
}

int recieve(int sock, char *buff, size_t size) {
    int nread = read(sock, buff, size);
    if (nread > 0) buff[nread] = 0;
    if (is_fin(buff)) term();
    return nread;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server.app <host> <port>\n");
        exit(1);
    }

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
        fprintf(stderr, "Failed to connect to server\n");
        exit(-1);
    }

    char buff[BUFFSIZE];
    int nread;
    while ((nread = recieve(sock, buff, sizeof(buff))) > 0) {
        if (nread > 0) {
            if (is_fin(buff)) break;
            printf("Received >> %s (%d bytes)\n", buff, nread);
            write(sock, "ACK", 4);
        } else break;
        memset(buff, 0, sizeof(buff));
    }

    close(sock);

    return 0;
}