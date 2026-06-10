#define MAX_MESSAGE_SIZE 4096

#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

/*
    length - Length of msg in message
    type   - msg_type being sent

*/
struct msg_header
{
    uint32_t length;
    uint32_t type;
};

/*
    msg - Char buffer array to hold bytes being sent
    msg_length - Length of msg
    msg_type - Type of message being sent
*/
struct message
{
    char msg[MAX_MESSAGE_SIZE];
    uint32_t msg_length;
    uint32_t msg_type;
};

/*
    Attempts to create & send a message + msg_header to the given socketFD

    Returns 0 if message successfully sent
    Else -1 & sets errno
*/
int send_message(int socketFD, uint32_t type, const void* msg, uint32_t length)
{
    if(length >= MAX_MESSAGE_SIZE)  // msg must be MAX_MESSAGE_SIZE - 1, 1 byte reserved for null terminator
    {
        errno = EMSGSIZE;
        return -1;
    }

    struct msg_header header;
    header.length = htonl((uint32_t)length);
    header.type = htonl((uint32_t)type);

    struct message m;
    strcpy(m.msg, msg); // NOTE: I think I need to make m.msg into network order?
    m.msg_length = htonl((uint32_t)length);
    m.msg_type = htonl((uint32_t)type);

    ssize_t writtenBytes = 0;
    while(writtenBytes < sizeof header)
    {
        ssize_t bytes = send(socketFD, &header + writtenBytes, sizeof header - writtenBytes, 0);
        if(bytes < 0)
        {
            return -1;
        }
        writtenBytes += bytes;
    }

    writtenBytes = 0;
    while(writtenBytes < sizeof m)
    {
        ssize_t bytes = send(socketFD, &m + writtenBytes, sizeof m - writtenBytes, 0);
        if(bytes < 0)
        {
            return -1;
        }
        writtenBytes += bytes;
    }

    return 0;


}

int recv_message(int socketFD, struct message* msg)
{

}