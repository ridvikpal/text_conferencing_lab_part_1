#ifndef USER_H
#define USER_H

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include "session.h"

#define USERNAME_LEN 32
#define PASSWORD_LEN 32

typedef struct User {
    // authentication information
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];

    // client status
    int socketFD;
    pthread_t pthread;
    bool inSession;

    // the session the user has joined
    Session *joinedSession;

    // the next user in the session
    struct User *next;

} User;

// Read the user data from the file into the user list
User *initUserList(FILE *_userListFP);

// add a user to the user list
User *addUserToList(User *_userList, User *_user);

// remove a user from the user list based on the username
User *removeUserFromList(User *_userList, const User *_user);

// Free a linked list of users in a session
void deleteUserList(User *_rootUser);

// authentication for user
bool authenticateUser(const User *_userList, const User *_user);

// check if the user exists in the user list via username
bool userInUserList(const User *_userList, const User *_user);

#endif