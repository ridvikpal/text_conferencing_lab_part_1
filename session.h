#ifndef SESSION_H
#define SESSION_H

#include "user.h"

// defined the session structure
typedef struct Session {
    // session information
    int id;
    struct Session* next;

    // the user list associated with the session
    struct User* user;
} Session;

// initialize a new session and add it to the global session list
Session* initNewSession(Session* _sessionList, int sessionID);

// remove a session from the global session list
Session* removeSession(Session* _sessionList, int sessionID);

// check if a session is valid
Session* checkValidSession(Session* _sessionList, int _sessionID);

// check if a user is in a session
int checkUserInSession(Session* _sessionList, int _sessionID, const User* _user);

// insert a new user into the global session list
Session* insertNewUserIntoSession(Session* _sessionList, int sessionID, const User* _user);

// remove a user from the global session list
Session* removeUserFromSession(Session* _sessionList, int sessionID, const User* _user);

// completely delete the global session list and free memory
void deleteSessionList(Session* _sessionList);

#endif