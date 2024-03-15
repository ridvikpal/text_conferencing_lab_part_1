#ifndef PACKET_H
#define PACKET_H

#define SOURCE_SIZE 32
#define DATA_SIZE 512

enum MessageType {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

typedef struct Message {
    unsigned int type; // actually the MessageType enum
    unsigned int size;
    unsigned char source[SOURCE_SIZE];
    unsigned char data[DATA_SIZE];
} Message;

// convert the message packet to a fixed size string
void messageToString(const Message* _message, char* _string);

// convert a string to a packet
void stringToMessage(const char* _string, Message* _message);

#endif