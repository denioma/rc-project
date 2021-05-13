#define ENTRYSIZE 256
#define BUFFSIZE 1024

typedef enum { false, true } bool;

typedef struct user_struct {
    char username[BUFFSIZE];
    char pass[BUFFSIZE];
    char ip[INET_ADDRSTRLEN];
    bool server, p2p, multicast;
    struct user_struct* next;
} user;

user* root;
bool modified;
char* registry_file;
