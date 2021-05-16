#include <stdio.h>
#include <string.h>
#include "libs/users.h"

user* findUser(char *username, char* pass, char *ip) {
    printf("[AUTH] Trying to authenticate %s: user: %s, pass: %s\n", ip, username, pass);
    if (!root) return NULL;
    user* curr = root;
    while (curr != NULL) {
        if (strcmp(username, curr->username) == 0) {
            if (strcmp(pass, curr->pass) == 0) {
                if (strcmp(ip, curr->ip) == 0) return curr;
            }
        } else curr = curr->next;
    }
    return NULL;
}