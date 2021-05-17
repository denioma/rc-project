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
struct sockaddr_in addr;
bool cs, p2p, multicast;

int auth(struct sockaddr_in* addr, socklen_t* slen);
void menu();

void close_client(int code) {
    close(sock);
    exit(code);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server.app <host> <port>\n");
        exit(1);
    }

    // Parse arguments
    char* hostname = *(argv + 1);
    int port = atoi(*(argv + 2));
    printf("%s:%d\n", hostname, port);

    // Get IP from hostname
    struct hostent* hostPtr;
    if (!(hostPtr = gethostbyname(hostname))) {
        perror("Failed to get host");
        exit(1);
    }

    // Setup server address struct for socket
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr*)hostPtr->h_addr_list[0])->s_addr;
    addr.sin_port = htons(port);
    socklen_t slen = sizeof(addr);

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }

    // Set timeout for receiving UDP datagrams
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Set socket to only receive datagrams from server
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
    while (1) menu();

    puts("Exiting");
    close(sock);

    return 0;
}

int auth(struct sockaddr_in* addr, socklen_t* slen) {
    // Get username and password input
    printf("Username: ");
    char user[AUTHSIZE];
    fgets(user, sizeof(user), stdin);
    user[strcspn(user, "\n")] = 0;
    printf("Password: ");
    char pass[AUTHSIZE];
    fgets(pass, sizeof(pass), stdin);
    pass[strcspn(pass, "\n")] = 0;

    // Sends credentials in plaintext and expects response
    char buffer[BUFFSIZE];
    int recvsize;
    do {
        snprintf(buffer, sizeof(buffer), "AUTH %s %s", user, pass);
        sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)addr, *slen);
        recvsize = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    } while (recvsize == -1);
    buffer[recvsize] = 0;
    if (strcmp(buffer, "SUCCESS") == 0) {
        // Receive communication permissions
        int perms;
        recvfrom(sock, &perms, sizeof(perms), 0, NULL, NULL);
        cs = perms / 100 % 10;
        p2p = perms / 10 % 10;
        multicast = perms % 10;
        return 1;
    }
    else return 0;
}


void menu() {
    puts("1 - Send a message via server");
    puts("2 - Send a message via P2P");
    puts("3 - Send a group message");
    puts("4 - Quit");
    
    printf(">> ");
    int opt;
    do {
        scanf("%d", &opt);
    } while (opt < 1 || opt > 4);
    
    switch (opt) {
        case 1:
            if (cs) puts("Imagine this works\n");
            else puts("You don't got permission fuckwad\n");
            break;
        case 2:
            if (p2p) puts("Imagine this works too\n");
            else puts("You don't got permission fuckwad\n");
            break;
        case 3:
            if (multicast) puts("Imagine this also works\n");
            else puts("You don't got permission fuckwad\n");
            break;
        case 4:
            close_client(0);
            break;
    }
}