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

// define the input buffer size
#define BUFFER_SIZE 64
// define the # of pending connections the queue can hold
#define PENDING_CONN 10
// define the authentication file that contains the username and password
#define USER_LIST "users.txt"



// Function to run when adding new client
void add_new_client(){
    
}


int main(){
    // Get the user input

    // Load the authentication file (a file of usernames and passwords)

    // Setup the server

    // Listen on the incoming port

    // Accept incoming connections loop

    // Free global memory on exit

    return 0;
}