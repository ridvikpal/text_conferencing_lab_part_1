#ifndef SESSION_H
#define SESSION_H

#include "user.h"
#include <stdbool.h>

// defined the session structure
typedef struct Session {
    // session information
    int id;

    // for the next session in the linked list of sessions
    struct Session *next;

    // the linked list of users associated with the session
    struct User *user;
} Session;

// initialize a new session and add it to the global session list
Session *initNewSession(Session *_sessionList, int _sessionID);

// remove a session from the global session list
Session *removeSession(Session *_sessionList, int _sessionID);

// search for a session in the session list and return it
Session *searchSession(Session *_sessionList, int _sessionID);

// check if a user is in a session
bool checkUserInSession(Session *_sessionList, int _sessionID, const struct User
*_user);

// insert a new user into the global session list
Session *insertNewUserIntoSession(Session *_sessionList, int _sessionID, const
struct User *_user);

// remove a user from the global session list
Session *removeUserFromSession(Session *_sessionList, int _sessionID, const
struct User *_user);

// completely delete the global session list and free memory
void deleteSessionList(Session *_sessionList);

#endif