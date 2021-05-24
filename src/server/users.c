#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/users.h"

extern void sndmsg(char *msg);

user* findUser(char* username, char* ip) {
    printf("[DEBUG] Matching %s\n", username);
    if (ip == NULL) puts("[DEBUG] No IP");
    if (!root) return NULL;
    user* curr = root;
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
    if (!root) return NULL;
    user* curr = findUser(username, ip);
    if (curr && strcmp(pass, curr->pass) == 0) return curr;
    else return NULL;
}

// TODO delete user
void del_user(char* opt) {
    if (!root) {
        sndmsg("[DEL] Registry is empty");
        return;
    }

    strtok(opt, " ");
    char *username = strtok(NULL, "\n");
    if (!username) return;
    printf("[DEL] Attempting to remove user %s\n", username);
    user* curr, * prev;
    if (strcmp(root->username, username) == 0) {
        curr = root->next;
        free(root);
        root = curr;
        if (modified == false) modified = true;
        return;
    }
    if (!root->next) return; 
    curr = root->next;
    prev = root;
    while (1) {
        if (strcmp(curr->username, username) == 0) {
            prev->next = curr->next;
            free(curr);
            if (modified == false) modified = true;
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
    new->msgport = 0;
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

    char buff[BUFFSIZE];
    user *curr = root;
    while (1) {
        snprintf(buff, sizeof(buff), "User: %s | IP: %s | Port: %d | Pass: %s | C-S: %d | P2P: %d | Multicast: %d\n", 
                curr->username, curr->ip, curr->msgport, curr->pass, curr->server, curr->p2p, curr->multicast);
        sndmsg(buff);
        if (curr->next) curr = curr->next;
        else break;
    }
}