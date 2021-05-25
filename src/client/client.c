// Redes de Comunicação 2020/2021 - Projeto Final
// Rodrigo Alexandre da Mota Machado - 2019218299
// Rui Bernardo Lopes Rodrigues - 2019217573

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUFFSIZE 512
#define AUTHSIZE 32
#define RCVPORT 9999

typedef enum { false, true } bool;

int sock = -1, rcv_sock = -1;
struct sockaddr_in addr;
struct ifreq iface;
socklen_t slen;
bool cs, p2p, multicast;
char username[AUTHSIZE];
char buff[BUFFSIZE];
pthread_t receiver;

int auth(struct sockaddr_in* addr, socklen_t* slen);
void menu();
void* incoming_msg();

void close_client(int code) {
    pthread_cancel(receiver);
    pthread_join(receiver, NULL);
    if (sock != -1) {
        snprintf(buff, sizeof(buff), "BYE %s", username);
        sendto(sock, buff, strlen(buff)+1, 0, (struct sockaddr*) &addr, slen);
        if (close(sock))
            perror("Failed to close socket");
    }
    if (rcv_sock != -1 &&close(rcv_sock))
        perror("Failed to close receiving socket");
    exit(code);
}

void sigint(int signo) {
    if (signo == SIGINT) close_client(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server.app <host> <port>\n");
        exit(1);
    }

    int code;

    // Store eth0 IP and index in iface
    iface.ifr_addr.sa_family = AF_INET;
    strncpy(iface.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(rcv_sock, SIOCGIFADDR, &iface);

    // Override SIGINT
    struct sigaction interrupt;
    interrupt.sa_handler = sigint;
    interrupt.sa_flags = 0;
    sigemptyset(&interrupt.sa_mask);
    sigaction(SIGINT, &interrupt, NULL);

    // Parse arguments
    char* hostname = *(argv + 1);
    int port = atoi(*(argv + 2));

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
    slen = sizeof(addr);

    // Create UDP socket for server opeartions
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        close_client(1);
    }

    if ((rcv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to open socket\n");
        close_client(1);
    }

    bool reuseaddr = true;
    code = setsockopt(rcv_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
    if (code == -1) perror("Failed to set SO_REUSEADDR");

    // Set timeout for receiving UDP datagrams
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Do not loopback multicast datagrams
    bool loop = false;
    code = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    if (code == -1) perror("Failed to set IP_MULTICAST_LOOP");

    // Increase Multicast TTL
    int multicastTTL = 255;
    code = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &multicastTTL, sizeof(multicastTTL));
    if (code == -1) perror("Failed to increase TTL");

    // Set outbound interface for multicast datagrams
    struct in_addr if_addr;
    if_addr.s_addr = htonl(INADDR_ANY);
    code = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &if_addr, sizeof(if_addr));
    if (code == -1) perror("Failed to set IP_MULTICAST_IF");

    // Set socket to only receive datagrams from server
    if (connect(sock, (struct sockaddr*)&addr, slen) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        close_client(1);
    }

    // Bind receiving socket to local port
    struct sockaddr_in rcvaddr;
    rcvaddr.sin_family = AF_INET;
    rcvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcvaddr.sin_port = htons(RCVPORT);
    if (bind(rcv_sock, (struct sockaddr*) &rcvaddr, sizeof(rcvaddr))) {
        fprintf(stderr, "Failed to bind receiving socket to local port");
        close_client(1);
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
        close_client(1);
    }

    pthread_create(&receiver, NULL, incoming_msg, NULL);
    snprintf(buff, sizeof(buff), "BIND %s", username);
    sendto(rcv_sock, buff, strlen(buff)+1, 0, (struct sockaddr*) &addr, slen);

    printf("C-S: %d | P2P: %d | Multicast: %d\n", cs, p2p, multicast);
    while (1) menu();

    close_client(1);

    return 0;
}

void* incoming_msg() {
    struct sockaddr_in rcvaddr;
    socklen_t addrlen;
    char buff[BUFFSIZE], ip[INET_ADDRSTRLEN];
    unsigned short int port;
    int recvlen;
    while (1) {
        memset(&rcvaddr, 0, sizeof(rcvaddr));
        recvlen = recvfrom(rcv_sock, buff, sizeof(buff), 0, (struct sockaddr*) &rcvaddr, &addrlen);
        buff[recvlen] = 0;
        inet_ntop(AF_INET, &(rcvaddr.sin_addr), ip, sizeof(ip));
        port = ntohs(rcvaddr.sin_port);
        printf("%s:%d >> %s\n", ip, port, buff);
    }
}

int auth(struct sockaddr_in* addr, socklen_t* slen) {
    // Get username and password input
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    printf("Password: ");
    char pass[AUTHSIZE];
    fgets(pass, sizeof(pass), stdin);
    pass[strcspn(pass, "\n")] = 0;

    // Sends credentials in plaintext and expects response
    char buffer[BUFFSIZE];
    int recvsize;
    do {
        snprintf(buffer, sizeof(buffer), "AUTH %s %s", username, pass);
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
    } else return 0;
}

void cs_message() {
    char user[AUTHSIZE], payload[BUFFSIZE-AUTHSIZE-5];
    printf("To: ");
    fgets(user, sizeof(user), stdin);
    user[strcspn(user, "\n")] = 0;
    printf("Message: ");
    fgets(payload, sizeof(payload), stdin);
    payload[strcspn(payload, "\n")] = 0;
    char msg[BUFFSIZE];
    snprintf(msg, sizeof(msg), "MSG %s %s", user, payload);
    int recvsize;
    do {
        sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr*) &addr, slen);
        recvsize = recvfrom(sock, buff, sizeof(buff), 0, NULL, NULL);
        if (recvsize >= 0) puts(buff);
    } while (recvsize == -1);
}

void peer() {
    char username[AUTHSIZE], buff[BUFFSIZE-AUTHSIZE-4], msg[BUFFSIZE];
    printf("To: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    if (strcmp(username, "") == 0) {
        puts("Username cannot be empty");
        return;
    }
    snprintf(buff, sizeof(buff), "PEER %s", username);
    int rec1, rec2;
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    do {
        sendto(sock, buff, strlen(buff)+1, 0, (struct sockaddr*) &addr, slen);
        rec1 = recvfrom(sock, ip, sizeof(ip), 0, NULL, NULL);
        rec2 = recvfrom(sock, &peer_addr.sin_port, sizeof(peer_addr.sin_port), 0, NULL, NULL);
    } while (rec1 != sizeof(ip) || rec2 != sizeof(peer_addr.sin_port));
    inet_pton(AF_INET, ip, &peer_addr.sin_addr.s_addr);
    printf("Message: ");
    fgets(buff, sizeof(buff), stdin);
    buff[strcspn(buff, "\n")] = 0;
    snprintf(msg, sizeof(msg), "[%s] %s", username, buff);
    sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr*) &peer_addr, sizeof(peer_addr));
}

void join_group() {
    char buff[AUTHSIZE], msg[BUFFSIZE];
    printf("Name: ");
    fgets(buff, sizeof(buff), stdin);
    buff[strcspn(buff, "\n")] = 0;
    if (strcmp(buff, "") == 0) {
        puts("Group name cannot be empty");
        return;
    }
    snprintf(msg, sizeof(msg), "GROUP %s", buff);
    unsigned long multicast_ip, recvsize;
    do {
        sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr*) &addr, slen);
        recvsize = recvfrom(sock, &multicast_ip, sizeof(multicast_ip), 0, NULL, NULL);
    } while (recvsize != sizeof(multicast_ip));
    if (multicast_ip == 0) {
        puts("Server failed to get group");
        return;
    }
    struct ip_mreqn multiopt;
    multiopt.imr_multiaddr.s_addr = multicast_ip;
    multiopt.imr_address.s_addr = htonl(INADDR_ANY);
    multiopt.imr_ifindex = 0;
    int code = setsockopt(rcv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multiopt, sizeof(multiopt));
    if (code == -1) perror("Failed to join multicast group");
}

void group_msg() {
    char group[AUTHSIZE], buff[BUFFSIZE-AUTHSIZE-4], msg[BUFFSIZE];
    printf("Name: ");
    fgets(group, sizeof(group), stdin);
    group[strcspn(group, "\n")] = 0;
    snprintf(msg, sizeof(msg), "GROUP %s", group);
    unsigned long multicast_ip, recvsize;
    do {
        sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr*) &addr, slen);
        recvsize = recvfrom(sock, &multicast_ip, sizeof(multicast_ip), 0, NULL, NULL);
    } while (recvsize != sizeof(multicast_ip));
    
    struct sockaddr_in multiaddr;
    multiaddr.sin_family = AF_INET;
    multiaddr.sin_addr.s_addr = multicast_ip;
    multiaddr.sin_port = htons(RCVPORT);
    
    printf("Message: ");
    fgets(buff, sizeof(buff), stdin);
    buff[strcspn(buff, "\n")] = 0;
    snprintf(msg, sizeof(msg), "[%s] %s", username, buff);
    sendto(sock, buff, strlen(buff)+1, 0, (struct sockaddr*) &multiaddr, sizeof(multiaddr));
}

void menu() {
    puts("1 - Send a message via server");
    puts("2 - Send a message via P2P");
    puts("3 - Join a multicast group");
    puts("4 - Send a group message");
    puts("5 - Quit");

    printf(">> ");
    int opt;
    do {
        scanf("%d", &opt);
    } while (opt < 1 || opt > 5);
    getchar(); // Clear stdin after scanf
    switch (opt) {
    case 1:
        if (cs) cs_message();
        else puts("Permission denied\n");
        break;
    case 2:
        if (p2p) peer();
        else puts("Permission denied\n");
        break;
    case 3:
        if (multicast) join_group();
        else puts("Permission denied\n");
        break;
    case 4:
        if (multicast) group_msg();
        else puts("Permission denied\n");
        break;
    case 5:
        close_client(0);
        break;
    }
}
