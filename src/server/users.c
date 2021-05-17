#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/users.h"

extern void sndmsg(char *msg);

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

// TODO delete user
void del_user(char* opt) {
    strtok(opt, " ");
    char *username = strtok(opt, "\n");
    if (!root) {
        sndmsg("[DEL] Registry is empty");
        return;
    }

    user* curr, * prev;
    if (strcmp(root->username, username) == 0) {
        curr = root->next;
        free(root);
        root = curr;
        return;
    }
    if (!root->next) return; 
    curr = root->next;
    prev = root;
    while (1) {
        if (strcmp(curr->username, username) == 0) {
            prev->next = curr->next;
            free(curr);
            return;
        }
        if (curr->next) {
            prev = curr;
            curr = curr->next;
        } else return;
    }
}

int add_user(char* username, char* pass, char* ip, bool server, bool p2p, bool multicast) {
    user *curr = root;
    if (root) {
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
    new->next = NULL;
    if (root) curr->next = new;
    else root = new;
    if (!modified) modified = true;
    return 0;
}

void list_users() {
    if (!root) {
        sndmsg("[LIST] Registry is empty");
        return;
    }

    user *curr = root;
    while (1) {
        printf("User: %s | IP: %s | Pass: %s | C-S: %d | P2P: %d | Multicast: %d\n", 
                curr->username, curr->pass, curr->ip, curr->server, curr->p2p, curr->multicast);
        if (curr->next) curr = curr->next;
        else break;
    }
}