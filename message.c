#include "message.h"
#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// define the buffer size for the string
#define BUFFER_SIZE 600

// convert the message packet to a fixed size string
void messageToString(const Message* _message, char* _string){
    // init the string buffer with 0 to be safe
    memset(_string, 0, sizeof(char) * BUFFER_SIZE);

    // initialize the cursor position
    // the cursor will control where we start copying from
    int cursor = 0;

    // copy the message type into the string
    sprintf(_string, "%d", _message->type);
    cursor = strlen(_string);
    memcpy(_string + cursor++, ":", sizeof(char));

    // copy the message size into the string
    sprintf(_string + cursor, "%d", _message->size);
    cursor = strlen(_string);
    memcpy(_string + cursor++, ":", sizeof(char));

    // copy the message source into the string
    sprintf(_string + cursor, "%s", _message->source);
    cursor = strlen(_string);
    memcpy(_string + cursor++, ":", sizeof(char));

    // copy the actual message data into the string
    memcpy(_string + cursor, _message -> data, strlen((char *)(_message->data)));
}

// convert a string to a packet
void stringToMessage(const char* _string, Message* _message){
    // initialize the data part of the message packet to 0
    // maybe init the entire message packet to 0?
    memset(_message->data, 0, DATA_SIZE);

    // if the string passed is empty, no need to convert to message packet
    if(strlen(_string) == 0) return;

    // compile Regex to match for the string/message separator ":"
    // we will then match for ":" in the string to separate the sections of
    // the message
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED)) {
        printf("Could not compile regex\n");
        return;
    }
    regmatch_t regexMatch[1];
    int cursor = 0;
    char tempBuffer[DATA_SIZE];     // Temporary buffer for regex matching

    // match the type and copy it into the message packet
    if(regexec(&regex, _string + cursor, 1, regexMatch, REG_NOTBOL)) {
        printf("Error matching type with regex\n");
        return;
    }
    memset(tempBuffer, 0, DATA_SIZE * sizeof(char));
    memcpy(tempBuffer, _string + cursor, regexMatch[0].rm_so);
    _message->type = atoi(tempBuffer);
    cursor += (regexMatch[0].rm_so + 1);

    // match the size and copy it into the message packet
    if(regexec(&regex, _string + cursor, 1, regexMatch, REG_NOTBOL)) {
        printf("Error matching size with regex\n");
        return;
    }
    memset(tempBuffer, 0, DATA_SIZE * sizeof(char));
    memcpy(tempBuffer, _string + cursor, regexMatch[0].rm_so);
    _message->size = atoi(tempBuffer);
    cursor += (regexMatch[0].rm_so + 1);

    // match the source and copy it into the message packet
    if(regexec(&regex, _string + cursor, 1, regexMatch, REG_NOTBOL)) {
        printf("Error matching source with regex\n");
        return;
    }
    memcpy(_message->source, _string + cursor, regexMatch[0].rm_so);
    _message->source[regexMatch[0].rm_so] = 0;
    cursor += (regexMatch[0].rm_so + 1);

    // load the data into the message packet
    // no need to match because data will be the last part of the string
    memcpy(_message -> data, _string + cursor, _message -> size);
}