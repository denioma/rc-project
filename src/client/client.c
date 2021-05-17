#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUFFSIZE 512
#define AUTHSIZE 32

typedef enum { false, true } bool;

int sock;
bool cs, p2p, multicast;

int auth(struct sockaddr_in* addr, socklen_t* slen);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server.app <host> <port>\n");
        exit(1);
    }

    char* hostname = *(argv + 1);
    int port = atoi(*(argv + 2));
    printf("%s:%d\n", hostname, port);

    struct sockaddr_in addr;
    struct hostent* hostPtr;

    if (!(hostPtr = gethostbyname(hostname))) {
        perror("Failed to get host");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr*)hostPtr->h_addr_list[0])->s_addr;
    addr.sin_port = htons(port);
    socklen_t slen = sizeof(addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr*) &addr, slen) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(1);
    }

    // Request user authentication
    int attempts;
    for (attempts = 0; attempts < 3; attempts++) {
        if (auth(&addr, &slen)) {
            puts("Authentication successful\n");
            break;
        } else puts("Authentication failed");
    }
    if (attempts == 3) {
        puts("Failed authentication too many times");
        close(sock);
        exit(1);
    }

    printf("C-S: %d | P2P: %d | Multicast: %d\n", cs, p2p, multicast);

    puts("Exiting");
    close(sock);

    return 0;
}

int auth(struct sockaddr_in* addr, socklen_t* slen) {
    printf("Username: ");
    char user[AUTHSIZE];
    fgets(user, sizeof(user), stdin);
    user[strcspn(user, "\n")] = 0;
    printf("Password: ");
    char pass[AUTHSIZE];
    fgets(pass, sizeof(pass), stdin);
    pass[strcspn(pass, "\n")] = 0;

    char buffer[BUFFSIZE];
    int recvsize;
    do {
        snprintf(buffer, sizeof(buffer), "AUTH %s %s", user, pass);
        sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)addr, *slen);
        recvsize = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    } while (recvsize == -1);
    buffer[recvsize] = 0;
    if (strcmp(buffer, "SUCCESS") == 0) {
        int perms;
        recvfrom(sock, &perms, sizeof(perms), 0, NULL, NULL);
        cs = perms / 100 % 10;
        p2p = perms / 10 % 10;
        multicast = perms % 10;
        return 1;
    }
    else return 0;
}
