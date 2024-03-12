#include "message.h"
#include <regex.h>

// define the buffer size
#define BUFFER_SIZE 600

// convert the message packet to a fixed size string
void messageToString(const Message* _message, char* _string){
    // clear the string
    memset(_string, 0, sizeof(char)* BUFFER_SIZE);

    int cursor = 0;

    // load the message type into the string
    cursor = sprintf(dest, "%d", _message->type);
    // cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    // load the message size into the string
    cursor = sprintf(dest+cursor, "%d", _message->size);
    // cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    // load the message source into the string
    sprintf(dest+cursor, "%s", _message->source);
    // cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    // load the message data into the string
    memcpy(dest+cursor, _message->data, strlen((char*)(_message->data)));
    // cursor = strlen(dest);
}

// convert a string to a packet
void messageToPacket(const char* _string, Message* _message){
    // clear the data in the message
    memset(_message->data, 0, DATA_SIZE);

    // compile regex to match for :
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED) != 0){
        printf("There was an error compiling regex\n");
    }
    regmatch_t regmatch[1];
    int cursor = 0;
    char regexBuffer[DATA_SIZE];

    // match the type and load it into the message
    if (regexec(&regex, _string, 1, regmatch, REG_NOTBOL) != 0) {
        printf("Error matching regex type");
        return;
    }
    memset(regexBuffer, 0, DATA_SIZE*sizeof(char));
    memcpy(regexBuffer, _string, regmatch[0].rm_so);
    _message->type = atoi(regexBuffer);
    cursor += regmatch.rm_so + 1;

    // match the size and load it into the message
    if (regexec(&regex, _string+cursor, 1, regmatch, REG_NOTBOL) != 0){
        printf("Error matching regex size");
        return;
    }
    memset(regexBuffer, 0, DATA_SIZE*sizeof(char));
    memcpy(regexBuffer, _string+cursor, regmatch[0].rm_so);
    _message->size = atoi(regexBuffer);
    cursor += regmatch.rm_so + 1;

    // match the source and load it into the message
    if (regexec(&regex, _string+cursor, 1, regmatch, REG_NOTBOL) != 0){
        printf("Error matching regex source");
        return;
    }
    memcpy(_message->source, _string+cursor, regmatch[0].rm_so);
    _message->source[regmatch[0].rm_so] = 0;
    cursor += regmatch.rm_so + 1;

    // load the data into the message
    memcpy(_message->data, _string+cursor, _message->size);
}