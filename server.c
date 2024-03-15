#include "user.h"
#include "session.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

// define the input string buffer size
#define BUFFER_SIZE 64
// define the # of pending connections the queue can hold
#define PENDING_CONN 10
// define the authentication file that contains the username and password

// use mutex to lock pthreads to modify global variables
pthread_mutex_t sessionListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userLoginMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessionCountMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userConnectedCountMutex = PTHREAD_MUTEX_INITIALIZER;

#define USER_LIST "users.txt"
// create global variables
char port[6] = {0}; // for port in getaddrinfo
int numOfSessions = 1;
int numOfConnectedUsers = 0;
User *globalUserList = NULL;
User *globalUserListLoggedIn = NULL; // 
Session *globalSessionList = NULL;

char globalInputBuffer[BUFFER_SIZE] = {0};

// get the sockaddr from Beej's network guide to programming
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

// Function to run when handling new client for server
void *handle_new_client(void *args) {
    User *new_user = (User *) args;
    Session *sessionJoined = NULL;

    // setup message parameters
    char recvBuffer[BUFFER_SIZE];
    char source[SOURCE_SIZE];
    int bytesSent, bytesRecieved;
    Message messageSend, messageRecieved;

    printf("New thread created\n");

    bool loggedIn = false; // if the user is logged in to a session
    bool toExit = false; // if we need to end the connection
    bool toSend = false; // if we need to send a message back

    while (1) {
        // intially clear all buffers and message packets
        memset(recvBuffer, 0, sizeof(char) * BUFFER_SIZE);
        memset(&messageRecieved, 0, sizeof(Message));
        memset(&messageSend, 0, sizeof(Message));

        // we do buffer size - 1 because we need to add null character to the
        // end of the buffer to make it a standard c string
        bytesRecieved = recv(new_user->socketFD, recvBuffer, BUFFER_SIZE - 1,
                             0);
        if (bytesRecieved == -1) {
            printf("There was an error recieving the data from recv\n");
            return NULL;
        }

        // add null character to make the data a string
        recvBuffer[bytesRecieved] = '\0';
        printf("Message recieved via recv: %s\n", recvBuffer);

        // parse the recieved string into a actual message packet
        stringToMessage(recvBuffer, &messageRecieved);

        // if we receive 0 bytes or if we receive exit message, then we can
        // exit and close this client's connection
        if (bytesRecieved == 0 || messageRecieved.type == EXIT) toExit = 1;

        // if the user is not logged in, then log them in first
        // don't do anything until the user is logged in
        if (loggedIn == false) {
            if (messageRecieved.type == LOGIN) {
                // get the username and password provided by the client
                strcpy(new_user->username, (char *) (messageRecieved.source));
                strcpy(new_user->password, (char *) (messageRecieved.data));

                printf("Attempting to authenticate user\n");
                printf("User Name: %s\n", new_user->username);
                printf("User password: %s\n", new_user->password);

                // Check if user has already logged in
                bool alreadyJoined = userInUserList(globalUserListLoggedIn,
                                                    new_user);

                // authenticate the user
                bool validUser = authenticateUser(globalUserList, new_user);

                // Clear user password for memory safety
                memset(new_user->password, 0, PASSWORD_LEN);

                if (validUser & !alreadyJoined) {
                    printf("User is valid\n");

                    // setup the message packet to send back
                    messageSend.type = LO_ACK;
                    toSend = true;
                    loggedIn = true;

                    // add the user to the global user list of logins
                    // we need a temporary user for the pthread locking
                    User *tempUser = malloc(sizeof(User));
                    memcpy(tempUser, new_user, sizeof(User));

                    // lock the pthread mutexes to add the user to the logged
                    // in user list
                    pthread_mutex_lock(&userLoginMutex);
                    globalUserListLoggedIn = addUserToList
                            (globalUserListLoggedIn, tempUser);
                    pthread_mutex_unlock(&userLoginMutex);

                    // save the username in the source
                    strcpy(source, new_user->username);

                } else {
                    printf("Login Error\n");

                    // setup message to send back
                    messageSend.type = LO_NAK;
                    toSend = true;

                    if (alreadyJoined) {
                        strcpy((char *) (messageSend.data), "Already logged "
                                                            "in\n");
                    } else if (userInUserList(globalUserList, new_user)) {
                        strcpy((char *) (messageSend.data), "Incorrect "
                                                            "password\n");
                    } else {
                        strcpy((char *) (messageSend.data), "Incorrect "
                                                            "username, user "
                                                            "not registered\n");
                    }

                    // we need to exit to restart the connection
                    toExit = true;
                }
            } else {
                // setup message to send back
                messageSend.type = LO_NAK;
                toSend = true;

                strcpy((char *) (messageSend.data),
                       "Error: Must login first\n");
            }
        } else {
            if (messageRecieved.type == JOIN) {
                int sessionID = atoi((char *) (messageRecieved.data));
                printf("User %s is trying to join session %s\n",
                       new_user->username, sessionID);

                // check if the session id exists
                Session *matchingSession = searchSession(globalSessionList,
                                                         sessionID);
                if (matchingSession == NULL) {
                    // setup the packet to send back
                    messageSend.type = JN_NAK;
                    toSend = true;
                    int cursor = sprintf((char *) (messageSend.data), "%d",
                                         sessionID);
                    strcpy((char *) (messageSend.data + cursor),
                           " Session does not exist");
                    printf("Error: Failed to join session %d because session "
                           "does not exist\n", sessionID);
                } else if (checkUserInSession(globalSessionList, sessionID,
                                              new_user)) {
                    // setup the packet to send back
                    messageSend.type = JN_NAK;
                    toSend = true;
                    int cursor = sprintf((char *) (messageSend.data), "%d",
                                         sessionID);
                    strcpy((char *) (messageSend.data + cursor),
                           " Session already joined");
                    printf("Error: Failed to join session %d because user %s "
                           "has already joined this session\n",
                           new_user->username, sessionID);
                } else {
                    // Update messageSend
                    messageSend.type = JN_ACK;
                    toSend = 1;
                    sprintf((char *) (messageSend.data), "%d", sessionID);

                    // update the global sessionList
                    pthread_mutex_lock(&sessionListMutex);
                    globalSessionList = insertNewUserIntoSession(
                            globalSessionList, sessionID, new_user);
                    pthread_mutex_unlock(&sessionListMutex);

                    // Update private sessJoined
                    sessionJoined = initNewSession(sessionJoined, sessionID);

                    printf("User %s has joined session %d\n",
                           new_user->username, sessionID);

                    // Update user status in userConnected
                    pthread_mutex_lock(&userLoginMutex);
                    for (User *user = globalUserListLoggedIn;
                         user != NULL; user = user->next) {
                        if (strcmp(user->username, source) == 0) {
                            user->inSession = true;
                            user->joinedSession = initNewSession(
                                    user->joinedSession,
                                    sessionID);
                        }
                    }
                    pthread_mutex_unlock(&userLoginMutex);
                }

            } else if (messageRecieved.type == LEAVE_SESS) {
                printf("User %s is leaving all sessions\n", new_user->username);
                // Iterate until all session left
                while (sessionJoined != NULL) {
                    int currentSessionID = sessionJoined->id;

                    // Free private sessionJoined
                    Session *current = sessionJoined;
                    sessionJoined = sessionJoined->next;
                    free(current);

                    // Free global sessionList
                    pthread_mutex_lock(&sessionListMutex);
                    globalSessionList = removeUserFromSession(globalSessionList,
                                                              currentSessionID,
                                                              new_user);
                    pthread_mutex_unlock(&sessionListMutex);

                    printf("User %s has left session %d\n", new_user->username,
                           currentSessionID);
                }
                messageToString(&messageSend, recvBuffer);

                // Update user status in userConnected;
                pthread_mutex_lock(&userLoginMutex);
                for (User *user = globalUserListLoggedIn;
                     user != NULL; user = user->next) {
                    if (strcmp(user->username, source) == 0) {
                        deleteSessionList(user->joinedSession);
                        user->joinedSession = NULL;
                        user->inSession = 0;
                        break;
                    }
                }
                pthread_mutex_unlock(&userLoginMutex);
            } else if (messageRecieved.type == NEW_SESS) {
                printf("User %s is making a new session\n", new_user->username);

                // update the global session list
                messageToString(&messageSend, recvBuffer);
                pthread_mutex_lock(&sessionListMutex);
                globalSessionList = initNewSession(globalSessionList,
                                                   numOfSessions);
                pthread_mutex_unlock(&sessionListMutex);

                // join the current user to the session that was just created
                sessionJoined = initNewSession(sessionJoined, numOfSessions);
                pthread_mutex_lock(&sessionListMutex);
                globalSessionList = insertNewUserIntoSession(globalSessionList,
                                                             numOfSessions,
                                                             new_user);
                pthread_mutex_unlock(&sessionListMutex);

                // Update user status in the global user logged in list;
                pthread_mutex_lock(&userLoginMutex);
                for (User *user = globalUserListLoggedIn;
                     user != NULL; user = user->next) {
                    if (strcmp(user->username, source) == 0) {
                        user->inSession = 1;
                        user->joinedSession = initNewSession(
                                user->joinedSession,
                                numOfSessions);
                    }
                }
                pthread_mutex_unlock(&userLoginMutex);

                // update the message to send
                messageSend.type = NS_ACK;
                toSend = true;
                // we have to send the number of sessions back to the client
                sprintf((char *) (messageSend.data), "%d", numOfSessions);

                // increase the number of sessions since we made a new one
                pthread_mutex_lock(&sessionCountMutex);
                numOfSessions = numOfSessions + 1;
                pthread_mutex_unlock(&sessionCountMutex);

            } else if (messageRecieved.type == MESSAGE) {

                printf("User %s is sending message \"%s\"\n",
                       new_user->username, messageSend.data);

                // Session to send to
                int currentSession = atoi(messageSend.source);

                // Prepare message to be sent
                memset(&messageSend, 0, sizeof(Message));
                messageSend.type = MESSAGE;
                strcpy((char *) (messageSend.source), new_user->username);
                strcpy((char *) (messageSend.data),
                       (char *) (messageRecieved.data));
                messageSend.size = strlen((char *) (messageSend.data));

                // store the message in recv buffer
                memset(recvBuffer, 0, sizeof(char) * BUFFER_SIZE);
                messageToString(&messageSend, recvBuffer);
                printf("sending message %s to session\n",
                       recvBuffer);

                // Send though local session list
                for (Session *current = sessionJoined;
                     current != NULL; current = current->next) {
                    // Find corresponding session in global sessionList
                    Session *sessToSend;
                    if ((sessToSend = searchSession(globalSessionList,
                                                    current->id)) == NULL)
                        continue;
                    printf("%d", sessToSend->id);
                    for (User *user = sessToSend->user;
                         user != NULL; user = user->next) {
                        if ((bytesSent = send(user->socketFD, recvBuffer,
                                              BUFFER_SIZE - 1, 0)) == -1) {
                            printf("error sending the packet\n");
                            exit(1);
                        }
                    }
                }
                printf("\n");
                toSend = false;
            } else if (messageRecieved.type == QUERY) {
                printf("User %s is making a query\n", new_user->username);

                int cursor = 0;
                messageSend.type = QU_ACK;
                toSend = true;

                for (User *user = globalUserListLoggedIn;
                     user != NULL; user = user->next) {
                    cursor += sprintf((char *) (messageSend.data) + cursor,
                                      "%s", user->username);
                    for (Session *sess = user->joinedSession;
                         sess != NULL; sess = sess->next) {
                        cursor += sprintf((char *) (messageSend.data) + cursor,
                                          "\t%d", sess->id);
                    }
                    messageSend.data[cursor++] = '\n';
                }

                printf("Query Result:\n%s", messageSend.data);
            }
        }

        if (toSend == true) {
            // add source and size for messageSend and then send packet
            memcpy(messageSend.source, new_user->username, USERNAME_LEN);
            messageSend.size = strlen((char *) (messageSend.data));

            memset(recvBuffer, 0, BUFFER_SIZE);
            messageToString(&messageSend, recvBuffer);
            if ((bytesRecieved = send(new_user->socketFD, recvBuffer,
                                      BUFFER_SIZE - 1, 0)) == -1) {
                printf("error sending the message packet\n");
            }
        }
        printf("\n");
        if (toExit == true) break;
    }

    // close the new_user socket
    close(new_user->socketFD);

    // check if the user is logged in
    if (loggedIn == 1) {
        // Leave all session before user exits, remove from sessionList
        for (Session *current = sessionJoined;
             current != NULL; current = current->next) {

            pthread_mutex_lock(&sessionListMutex);
            globalSessionList = removeUserFromSession(globalSessionList,
                                                      current->id, new_user);
            pthread_mutex_unlock(&sessionListMutex);
        }

        // Remove from global userLoggedin
        for (User *usr = globalUserListLoggedIn; usr != NULL; usr = usr->next) {
            if (strcmp(source, usr->username) == 0) {
                deleteSessionList(usr->joinedSession);
                break;
            }
        }
        globalUserListLoggedIn = removeUserFromList(globalUserListLoggedIn,
                                                    new_user);

        // remove private session list, free memory
        deleteSessionList(sessionJoined);
        free(new_user);

        // Decrement userConnectedCnt
        pthread_mutex_lock(&userConnectedCountMutex);
        numOfConnectedUsers = numOfConnectedUsers - 1;
        pthread_mutex_unlock(&userConnectedCountMutex);

        printf("User has exited\n");
    }
    return NULL;
}

int main() {
    // Get the user input
    memset(globalInputBuffer, 0, BUFFER_SIZE * sizeof(char));
    fgets(globalInputBuffer, BUFFER_SIZE, stdin);
    char *compare;
    if ((compare = strstr(globalInputBuffer, "server ")) != NULL) {
        memcpy(port, compare + 7, sizeof(char) * (strlen(compare + 7) - 1));
    } else {
        printf("Usage: server -<TCP port number to listen on>\n");
    }
    printf("Server: starting at port %s...\n", port);

    // Load the authentication file (a file of usernames and passwords)
    FILE *filePointer;
    if ((filePointer = fopen(USER_LIST, "r")) == NULL) {
        printf("Can't open input file %s\n", USER_LIST);
    }
    globalUserList = initUserList(filePointer);
    fclose(filePointer);
    printf("Server: Userdata loaded\n");

    // Setup the server
    int sockFD;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;    // Use TCP
    hints.ai_flags = AI_PASSIVE;        // use my IP
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockFD = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("Server: socket");
            continue;
        }
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            printf("there was an error with setsockopt");
        }
        if (bind(sockFD, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockFD);
            printf("there was an error with the server binding to a "
                   "port");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        printf("Server failed to bind\n");
    }

    // Server exits after a while if no user online
    struct timeval timeout;
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
    if (setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof
            (timeout)) < 0) {
        printf("setsockopt failed\n");
    }

    // Listen on incoming port
    if (listen(sockFD, PENDING_CONN) == -1) {
        printf("error listening on socket fd");
    }
    printf("Server: waiting for connections\n");

    // Accept incoming connections loop
    do {
        while (1) {

            // Accept new incoming connections
            User *newUsr = calloc(sizeof(User), 1);
            sin_size = sizeof(their_addr);
            newUsr->socketFD = accept(sockFD, (struct sockaddr *) &their_addr,
                                      &sin_size);
            if (newUsr->socketFD == -1) {
                perror("accept");
                break;
            }
            inet_ntop(their_addr.ss_family,
                      get_in_addr((struct sockaddr *) &their_addr), s,
                      sizeof(s));
            printf("Server: got connection from %s\n", s);

            // Increment userConnectedCnt
            pthread_mutex_lock(&userConnectedCountMutex);
            numOfConnectedUsers = numOfConnectedUsers + 1;
            pthread_mutex_unlock(&userConnectedCountMutex);

            // Create new thread to handle the new socket
            pthread_create(&(newUsr->pthread), NULL, handle_new_client,
                           (void *) newUsr);

        }
    } while (numOfConnectedUsers > 0);

    // Free global memory on exit
    deleteUserList(globalUserList);
    deleteUserList(globalUserListLoggedIn);
    deleteSessionList(globalSessionList);
    close(sockFD);
    printf("Server closed\n");
    return 0;
}