#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFSIZE 512
#define AUTHSIZE 32

int sock;

int auth(struct sockaddr_in* addr, socklen_t* slen);

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

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        exit(-1);
    }
  
    if (connect(sock, (struct sockaddr*)&addr, slen) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(-1);
    }

    // Request user authentication
    int attempts;
    for (attempts = 0; attempts < 3; attempts++) {
        if (auth(&addr, &slen)) {
            puts("Authentication successful");
            break;
        } else puts("Authentication failed");
    }
    if (attempts == 3) {
        puts("Failed authentication too many times");
        close(sock);
        exit(1);   
    }

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
    snprintf(buffer, sizeof(buffer), "AUTH %s %s", user, pass);
    sendto(sock, buffer, strlen(buffer)+1, 0, (struct sockaddr*) addr, *slen);

    int recvsize = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    buffer[recvsize] = 0; 
    if (strcmp(buffer, "SUCCESS") == 0) return 1;
    else return 0;
}
