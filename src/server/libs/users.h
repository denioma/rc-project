#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define ENTRYSIZE 256
#define BUFFSIZE 1024

typedef enum { false, true } bool;

typedef struct user_struct {
    char username[BUFFSIZE];
    char pass[BUFFSIZE];
    char ip[INET_ADDRSTRLEN];
    bool server, p2p, multicast;
    struct sockaddr_in* addr; // This should be a full placeholder, not a pointer, me thinks
    struct user_struct* next;
} user;

user* root;
bool modified;
char* registry_file;

user* findUser(char *username, char* pass, char *ip);
void list_users();
int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast);
void del_user(char* opt);