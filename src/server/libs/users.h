// Redes de Comunicação 2020/2021 - Projeto Final
// Rodrigo Alexandre da Mota Machado - 2019218299
// Rui Bernardo Lopes Rodrigues - 2019217573

#include <netdb.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define ENTRYSIZE 256
#define BUFFSIZE 1024
#define MULTIP "224.0.0.1"

typedef enum { false, true } bool;

typedef struct user_struct {
    char username[ENTRYSIZE];
    char pass[ENTRYSIZE];
    char ip[INET_ADDRSTRLEN];
    bool server, p2p, multicast;
    struct sockaddr_in msg_addr;
    unsigned short int* msgport;
    struct user_struct* next;
} user;

user* uroot;
bool modified;
char* registry_file;

user* findUser(char* username, char* ip);
user* matchUser(char *username, char* pass, char *ip);
int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast);
void list_users();
void free_users(user* node);

typedef struct group_struct {
    char name[ENTRYSIZE];
    unsigned long ip;
    struct group_struct* next;
} group;

group* groot;
unsigned long baseIp;
const unsigned long maxIp;

void del_user(char* opt);
group* get_group(char* name);
group* new_group(char* name);