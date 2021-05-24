#include <netdb.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define ENTRYSIZE 256
#define BUFFSIZE 1024

typedef enum { false, true } bool;

typedef struct user_struct {
    char username[ENTRYSIZE];
    char pass[ENTRYSIZE];
    char ip[INET_ADDRSTRLEN];
    bool server, p2p, multicast;
    // IDEA Consider storing only the port -> smaller than storing sockaddr_in struct
    struct sockaddr_in msg_addr;
    short int msgport;
    struct user_struct* next;
} user;

user* root;
bool modified;
char* registry_file;

user* findUser(char* username, char* ip);
user* matchUser(char *username, char* pass, char *ip);
void list_users();
int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast);
void del_user(char* opt);