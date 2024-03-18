#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include "message.h"
#include "session.h"
#include "user.h"

#define BUFFER_SIZE 64

bool insession = false;
char buffer[BUFFER_SIZE];

void *receive(void *sockfd) {
    // Receive may get: JN_ACK, JN_NAC, NS_ACK, QU_ACK, MESSAGE
    // Only LOGIN packet is not listened by receive
    int *socketfd_p = (int *)sockfd;
    int bytes;
    Message message;
    while (true) {
        if ((bytes = recv(*socketfd_p, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return NULL;
        }
        if (bytes == 0) continue;
        buffer[bytes] = 0;
        stringToMessage(buffer, &message);
        if (message.type == JN_ACK) {
            printf("Success: Successfully joined session %s.\n", message.data);
            insession = true;
        } else if (message.type == JN_NAK) {
            printf("Error: Join failure. Detail: %s\n", message.data);
            insession = false;
        } else if (message.type == NS_ACK) {
            printf("Success: Successfully created and joined session %s.\n", message.data);
            insession = true;
        } else if (message.type == MESSAGE) {
            printf("%s: %s\n", message.source, message.data);
        } else if (message.type == QU_ACK) {
            printf("Success: User id\t\tSession ids\n%s", message.data);
        } else {
            printf("Warning: Unexpected packet received: type %d, data %s\n", message.type, message.data);
        }
        // Flush stdout to ensure immediate display of messages
        fflush(stdout);
    }
    return NULL;
}


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void login (char *command, int *sockfd, pthread_t *thread_p){
    char *username, *password, *server_ip, *server_port;
    command = strtok(NULL, " ");
    username = command;
    command = strtok(NULL, " ");
    password = command;
    command = strtok(NULL, " ");
    server_ip = command;
    command = strtok(NULL, " ");
    server_port = command;
    printf ("Username:%s Password:%s Server IP:%s Server Port:%s\n", username, password, server_ip, server_port);
    if (username == NULL || password == NULL || server_ip == NULL || server_port == NULL){
        printf ("Invalid login command. /login <client_id> <password> <server_ip> <server_port>\n");
        return;
    }else if (*sockfd != -1){
        printf ("You cannot login to multiple servers at the same time\n");
        return;
    }else {
        // Prepare to connect through TCP protocol
        int address_resolution_result;
        struct addrinfo address_resolution_hints, *server_address_info, *current_address_info;
        char server_address_string[INET6_ADDRSTRLEN];
        memset(&address_resolution_hints, 0, sizeof address_resolution_hints);
        address_resolution_hints.ai_family = AF_UNSPEC;
        address_resolution_hints.ai_socktype = SOCK_STREAM;

        // Get address info for the server
        if ((address_resolution_result = getaddrinfo(server_ip, server_port, &address_resolution_hints, &server_address_info)) != 0) {
            perror("getaddrinfo");
            return;
        }
        printf ("Address resolution result: %d\n", address_resolution_result);

        // Iterate over the list of server addresses and connect
        for(current_address_info = server_address_info; current_address_info != NULL; current_address_info = current_address_info->ai_next) {
            if ((*sockfd = socket(current_address_info->ai_family, current_address_info->ai_socktype, current_address_info->ai_protocol)) == -1) {
                perror("socket");
                continue;
            }
            if (connect(*sockfd, current_address_info->ai_addr, current_address_info->ai_addrlen) == -1) {
                close(*sockfd);
                perror("connect");
                continue;
            }
            break;
        }

        // Check if connection succeeded
        if (current_address_info == NULL) {
            perror("connect");
            close(*sockfd);
            *sockfd = -1;
            return;
        }

        // Print connection information
        inet_ntop(current_address_info->ai_family, get_in_addr((struct sockaddr *)current_address_info->ai_addr), server_address_string, sizeof server_address_string);
        printf("Client is connecting to %s\n", server_address_string);
        freeaddrinfo(server_address_info);

        int bytes;
        Message login_message;
        login_message.type = LOGIN;
        strncpy(login_message.source, username, SOURCE_SIZE-1);
        strncpy(login_message.data, password, SOURCE_SIZE-1);
        login_message.size = strlen(login_message.data);
        messageToString(&login_message, buffer);
        if ((bytes = send(*sockfd, buffer, BUFFER_SIZE-1, 0))==-1){
            perror("send");
            close (*sockfd);
            *sockfd = -1;
            return;
        }
        if ((bytes = recv(*sockfd, buffer, BUFFER_SIZE-1, 0)) == -1){
            perror("recv");
            close (*sockfd);
            *sockfd = -1;
            return;
        }
        buffer[bytes] = 0;
        stringToMessage(buffer, &login_message);

        // Handle login response
        if (login_message.type == LO_ACK && pthread_create(thread_p, NULL, receive, sockfd) == 0) {
            printf("Successfully logged in\n");
        } else if (login_message.type == LO_NAK) {
            printf("Login failed. Detail: %s\n", login_message.data);
            close(*sockfd);
            *sockfd = -1;
            return;
        } else {
            printf("Error: message received: type %d, data %s\n", login_message.type, login_message.data);
            close(*sockfd);
            *sockfd = -1;
            return;
        }
    }
}

void logout(int *sockfd, pthread_t *thread_p) {
    // Check if the client is logged in
    if (*sockfd == -1) {
        printf("Error: not logged in to any server.\n");
        return;
    }

    // Send an exit packet to the server
    int bytes;
    Message exit_message;
    exit_message.type = EXIT;
    exit_message.size = 0;
    messageToString(&exit_message, buffer);

    if ((bytes = send(*sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("send");
        printf("Error: Failed to send exit packet to the server.\n");
        return;
    }

    // Cancel the receive thread
    if (pthread_cancel(*thread_p)) {
        perror("pthread_cancel");
        printf("Error: Failed to cancel the receive thread.\n");
    } else {
        printf("Success: Receive thread canceled successfully.\n");
    }

    insession = false;
    close(*sockfd);
    *sockfd = -1;

    printf("Success: Logout completed.\n");
}


void join_session(char *command, int *sockfd) {
    // Check if the client is logged in
    if (*sockfd == -1) {
        printf("Error: You have not logged in to any server.\n");
        return;
    }
        // Check if the client has already joined a session
    else if (insession) {
        printf("Error: You have already joined a session.\n");
        return;
    }

    // Extract session ID from the command
    char *session_id;
    command = strtok(NULL, " ");
    session_id = command;

    // Check if session ID is provided
    if (session_id == NULL) {
        printf("Usage: /joinsession <session_id>\n");
    } else {
        // Send join session request to the server
        int bytes;
        Message join_message;
        join_message.type = JOIN;
        strncpy(join_message.data, session_id, SOURCE_SIZE);
        join_message.size = strlen(join_message.data);
        messageToString(&join_message, buffer);
        if ((bytes = send(*sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("send");
            printf("Error: Failed to send join session request to the server.\n");
            return;
        }
    }
}



void leave_session(int sockfd) {
    // Check if the client is logged in
    if (sockfd == -1) {
        printf("Error: You have not logged in to any server.\n");
        return;
    }
        // Check if the client has joined a session
    else if (!insession) {
        printf("Error: You have not joined a session yet.\n");
        return;
    }

    // Send leave session request to the server
    int bytes;
    Message leave_message;
    leave_message.type = LEAVE_SESS;
    leave_message.size = 0;
    messageToString(&leave_message, buffer);
    if ((bytes = send(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("send");
        printf("Error: Failed to send leave session request to the server.\n");
        return;
    }

    // Update client state to reflect leaving the session
    insession = false;
}



void create_session(int sockfd) {
    // Check if the client is logged in
    if (sockfd == -1) {
        printf("Error: You have not logged in to any server.\n");
        return;
    }
        // Check if the client has already joined a session
    else if (insession) {
        printf("Error: You have already joined a session.\n");
        return;
    }

    // Send create session request to the server
    int bytes;
    Message create_message;
    create_message.type = NEW_SESS;
    create_message.size = 0;
    messageToString(&create_message, buffer);
    if ((bytes = send(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("send");
        printf("Error: Failed to send create session request to the server.\n");
        return;
    }
}



void list(int sockfd) {
    // Check if the client is logged in
    if (sockfd == -1) {
        printf("Error: You have not logged in to any server.\n");
        return;
    }

    // Send query request to the server to list sessions
    int bytes;
    Message list_message;
    list_message.type = QUERY;
    list_message.size = 0;
    messageToString(&list_message, buffer);
    if ((bytes = send(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("send");
        printf("Error: Failed to send query request to the server.\n");
        return;
    }
}


void send_message(int sockfd) {
    // Check if the client is logged in
    if (sockfd == -1) {
        printf("Error: You have not logged in to any server.\n");
        return;
    }
        // Check if the client is engaged in a session
    else if (insession == -1) {
        printf("Error: You have not been engaged in any session yet.\n");
        return;
    }

    // Send the message to the server
    int bytes;
    Message message;
    message.type = MESSAGE;
    // Format the session ID as the source of the message
    bytes = snprintf(message.source, SOURCE_SIZE - 1, "%d", insession);
    strncpy(message.data, buffer, SOURCE_SIZE - 1);
    message.size = strlen(message.data);
    messageToString(&message, buffer);
    if ((bytes = send(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("send");
        printf("Error: Failed to send message to the server.\n");
        return;
    }
}


int main(){
    char *command;
    bool quit = false;
    int sockfd = -1;
    pthread_t thread_p;

    while (!quit){
        fgets(buffer, BUFFER_SIZE-1, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline if present
        command = buffer;
        //printf ("%s\n", buffer);
        while (*command == ' '){
            command++;
        }
        if (*command == 0){
            continue;
        }
        command = strtok(buffer, " ");
        int command_length = strlen(command);
        if (strcmp(command, "/login") == 0){
            //Login
            login(command, &sockfd, &thread_p);
        }else if (strcmp(command, "/logout") == 0){
            //Logout
            logout(&sockfd, &thread_p);
        }else if (strcmp(command, "/joinsession") == 0){
            //Join session
            join_session(command, &sockfd);
        }else if (strcmp(command, "/leavesession") == 0){
            //Leave session
            leave_session(sockfd);
        }else if (strcmp(command, "/createsession") == 0) {
            //Create Session
            create_session(sockfd);
        }else if (strcmp (command, "/list") == 0){
            //List
            list (sockfd);
        }else if (strcmp(command,  "/quit") == 0){
            //Quit
            logout(&sockfd, &thread_p);
            quit = true;
        }else {
            //Send message
            buffer[command_length] = ' ';
            send_message(sockfd);
        }
    }
    return 0;
}