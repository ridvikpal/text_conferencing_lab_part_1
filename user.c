#include "user.h"
#include <string.h>
#include <stdlib.h>

// Read the user data from the file into the user list
User *initUserList(FILE *_userListFP) {
    int numMatches;

    User *userListHead = NULL;
    while (1) {
        // allocate memory for user
        User *user = calloc(1, sizeof(User));
        // scan the username and password from the file and store into the
        // user object
        numMatches = fscanf(_userListFP, "%s %s\n", user->username,
                            user->password);

        // if the numMatches = EOF, we have reached the end of the file and
        // should exit
        if (numMatches == EOF) {
            free(user);
            break;
        }
        // user -> next = userListHead;
        // userListHead = user;

        // add the user to the head of the user list
        userListHead = addUserToList(userListHead, user);
    }
    return userListHead;
}

// add a user to the user list
// assume user has already been malloced
User *addUserToList(User *_userList, User *_user) {
    _user->next = _userList;
    return _user;
}

// remove a user from the user list based on the username
User *removeUserFromList(User *_userList, const User *_user) {
    // if the user list is empty, then return NULL
    if (_userList == NULL) return NULL;

        // check if the head of the user list matches the desired user via
        // username and remove that user directly
    else if (strcmp(_userList->username, _user->username) == 0) {
        User *newHead = _userList->next;
        free(_userList);
        return newHead;
    } else {
        // standard linkedlist search for the user with matching username
        User *current = _userList;
        User *previous;
        while (current != NULL) {
            if (strcmp(current->username, _user->username) == 0) {
                previous->next = current->next;
                free(current);
                break;
            } else {
                previous = current;
                current = current->next;
            }
        }
        return _userList;
    }
}

// Free a linked list of users in a session
void deleteUserList(User *_rootUser) {
    // standard linkedlist delete
    User *current = _rootUser;
    User *next = current;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

// authenticate user with provided username and password matching with the
// user list
bool authenticateUser(const User *_userList, const User *_user) {
    // search the linkedlist for a user with matching username or password
    const User *current = _userList;
    while (current != NULL) {
        if (strcmp(current->username, _user->username) == 0 &&
            strcmp(current->password, _user->password) == 0) {
            return true;
        }
        current = current->next;
    }
    // if no user with matching username or password is found, then return false
    return false;
}

// check if the user exists in the user list via username
bool userInUserList(const User *_userList, const User *_user) {
    // standard linkedlist search for user with matching username
    const User *current = _userList;
    while (current != NULL) {
        if (strcmp(current->username, _user->username) == 0) {
            return true;
        }
        current = current->next;
    }

    // if no user with matching username is found, return false
    return false;
}