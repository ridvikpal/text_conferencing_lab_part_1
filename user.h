#ifndef USER_H
#define USER_H

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include "session.h"

#define USERNAME_LEN 32
#define PASSWORD_LEN 32
#define IP_LEN 16

typedef struct User {
    // authentication information
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];

    // client status
    int socketFD;
    pthread_t p;
    bool inSession;

    // session information
    Session *joinedSession;

    // the next user in the session
    struct User *next;

} User;

// add a user to the user list
User *addUserToList(User *_userList, User *_user);

// remove a user from the user list based on the username
User *removeUserFromList(User *_userList, const User *_user);

// Read the user data from the file into the user list
User *initUserList(FILE *_userListFP);

// Free a linked list of users in a session
void destroyUserList(User *_rootUser);

// authentication for user
int authenticateUser(const User *_userList, const User *_user);

// check if the user exists in the user list via username
bool userInUserList(const User *_userList, const User *_user);

#endif