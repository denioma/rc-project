// Redes de Comunicação 2020/2021 - Projeto Final
// Rodrigo Alexandre da Mota Machado - 2019218299
// Rui Bernardo Lopes Rodrigues - 2019217573

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/users.h"

extern void sndmsg(char *msg);

/* ----- User List Interfaces ----- */

user* findUser(char* username, char* ip) {
    printf("[DEBUG] Matching %s\n", username);
    if (ip == NULL) puts("[DEBUG] No IP");
    if (!uroot) return NULL;
    user* curr = uroot;
    while (curr != NULL) {
        if (strcmp(username, curr->username) == 0) {
            puts("[DEBUG] Username match");
            if (ip == NULL || strcmp(ip, curr->ip) == 0) return curr;
            else {
                puts("[DEBUG] IP is not a match");
                return NULL;
            }
        } else {
            puts("[DEBUG] Username is not a match");
            curr = curr->next;
        }
    }
    return NULL;
}

user* matchUser(char *username, char* pass, char *ip) {
    if (!uroot) return NULL;
    user* curr = findUser(username, ip);
    if (curr && strcmp(pass, curr->pass) == 0) return curr;
    else return NULL;
}

// TODO delete user
void del_user(char* opt) {
    if (!uroot) {
        sndmsg("[DEL] Registry is empty\n");
        return;
    }
    
    strtok(opt, " ");
    char *username = strtok(NULL, "\n");
    if (!username) return;
    // printf("[DEL] Attempting to remove user %s\n", username);
    user* curr, * prev;
    if (strcmp(uroot->username, username) == 0) {
        curr = uroot->next;
        free(uroot);
        uroot = curr;
        if (modified == false) modified = true;
        sndmsg("[DEL] User deleted\n");
        return;
    }
    if (!uroot->next) return; 
    curr = uroot->next;
    prev = uroot;
    while (1) {
        if (strcmp(curr->username, username) == 0) {
            prev->next = curr->next;
            free(curr);
            if (modified == false) modified = true;
            sndmsg("[DEL] User deleted\n");
            return;
        }
        if (curr->next) {
            prev = curr;
            curr = curr->next;
        } else return;
    }
    sndmsg("[DEL] Failed to delete user\n");
}

int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast) {
    user *curr = uroot;
    if (uroot) {
        while (1) {
            if (strcmp(curr->username, username) == 0) return 1;
            if (curr->next) curr = curr->next;
            else break;
        }
    }
    user *new = malloc(sizeof(user));
    if (!new) return 1;
    strncpy(new->username, username, sizeof(new->username));
    strncpy(new->pass, pass, sizeof(new->pass));
    strncpy(new->ip, ip, sizeof(new->ip));
    new->server = server;
    new->p2p = p2p;
    new->multicast = multicast;
    new->msgport = 0;
    new->next = NULL;
    if (uroot) curr->next = new;
    else uroot = new;
    if (!modified) modified = true;
    return 0;
}

void list_users() {
    if (!uroot) {
        sndmsg("[LIST] Registry is empty\n");
        return;
    }

    char buff[BUFFSIZE];
    user *curr = uroot;
    while (1) {
        snprintf(buff, sizeof(buff), "User: %s | IP: %s | Port: %d | Pass: %s | C-S: %d | P2P: %d | Multicast: %d\n", 
                curr->username, curr->ip, curr->msgport, curr->pass, curr->server, curr->p2p, curr->multicast);
        sndmsg(buff);
        if (curr->next) curr = curr->next;
        else break;
    }
}

void free_users(user* node) {
    if (!node) return;
    if (node->next) free_users(node->next);
    free(node);
}

/* ----- Multicast Groups Interfaces ----- */

unsigned long baseIp = 0xEF000000;
const unsigned long maxIp = 0xEFFFFFFF;

group* get_group(char* name) {
    group* curr = groot;
    while (curr) {
        if (strcmp(curr->name, name) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

group* new_group(char* name) {
    if (baseIp == maxIp+1) {
        // TODO COMPLAIN THAT YOU HAVE REACHED AN IMPOSSIBLE NUMBER OF MULTICAST GROUPS
        return NULL;
    }
    group* curr = groot;
    if (groot) {
        while (1) {
            if (strcmp(curr->name, name) == 0) return curr;
            else if (curr->next) curr = curr->next;
            else break;
        }
    }
    group* new = malloc(sizeof(group));
    new = malloc(sizeof(group));
    strncpy(new->name, name, sizeof(new->name));
    new->ip = htonl(baseIp++);
    if (groot) curr->next = new;
    else groot = new;
    return new;
}
