#include "session.h"
#include <stdlib.h>
#include <string.h>

// initialize a new session and add it to the global session list and return
// the new session just created
Session *initNewSession(Session *_sessionList, int _sessionID) {
    Session *newSession = calloc(sizeof(Session), 1);
    newSession->id = _sessionID;
    // insert the new session to head of the list
    newSession->next = _sessionList;
    return newSession;
}

// remove a session from the global session list and return the new session
// list head
Session *removeSession(Session *_sessionList, int _sessionID) {
    // if there are no sessions in the session list, return immediately
    if (_sessionList == NULL);

    // if the head of session list matches, then immediately return after
    // removing session
    if (_sessionList->id == _sessionID) {
        Session *newHead = _sessionList->next;
        free(_sessionList);
        return newHead;
    } else {
        // standard linked list search for matching session in session list
        // we need previous to change the next pointer properly in the
        // linkedlist
        Session *current = _sessionList;
        Session *previous;
        while (current != NULL) {
            if (current->id == _sessionID) {
                previous->next = current->next;
                free(current);
                break;
            } else {
                previous = current;
                current = current->next;
            }
        }
        return _sessionList;
    }
}

// check for a session in the session list and return it
Session *searchSession(Session *_sessionList, int _sessionID) {
    // standard linkedlist search for session with matching session id
    Session *current = _sessionList;
    while (current != NULL) {
        if (current->id == _sessionID) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// check if a user is in a session and return that user's ID
bool checkUserInSession(Session *_sessionList, int _sessionID, const struct User
*_user) {
    // get the correct session
    Session *session = searchSession(_sessionList, _sessionID);
    // if the session exists then check if the user is in the session and
    // return that user
    if (session != NULL) {
        return userInUserList(session->user, _user);
    } else {
        return false;
    }
}

// insert a new user into the corresponding session id
// also have to insert the user into the session of each corresponding user
// in its thread
Session *insertNewUserIntoSession(Session *_sessionList, int _sessionID, const
struct User *_user) {
    // find the correct session
    Session *session = searchSession(_sessionList, _sessionID);
    if (session == NULL) {
        printf("session with session id %d, not found\n", _sessionID);
        return NULL;
    }

    // malloc for new user
    User *newUser = malloc(sizeof(User));
    memcpy((void *) newUser, (void *) _user, sizeof(User));

    // Insert into session list via:
    // 1. Insert user into user list
    // 2. Insert the user list into the session
    session->user = addUserToList(session->user, newUser);

    return _sessionList;
}

// remove a user from the corresponding session id
Session *removeUserFromSession(Session *_sessionList, int _sessionID, const
struct User *_user) {
    // get the session with matching session id
    Session *session = searchSession(_sessionList, _sessionID);
    if (session == NULL) {
        printf("session with session id %d, not found\n", _sessionID);
        return NULL;
    }

    // check if the user actually exists
    if (!(userInUserList(session->user, _user))) {
        printf("User not found\n");
        return NULL;
    }
    // if the user exists, remove it from the user list for this session
    session->user = removeUserFromList(session->user, _user);

    // If the session user list is empty, remove the session
    if (session->user == NULL) {
        _sessionList = removeSession(_sessionList, _sessionID);
    }
    return _sessionList;
}

// completely delete the global session list and free memory
void deleteSessionList(Session *_sessionList) {
    // standard iterative linked list delete
    Session *current = _sessionList;
    Session *next = current;

    while (current != NULL) {
        // make sure to delete the entire user list for the session as well
        deleteUserList(current->user);
        next = current->next;
        free(current);
        current = next;
    }
}