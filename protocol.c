#define MAX_MESSAGE_SIZE 4096

#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

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


int send_message(int socketFD, uint32_t type, const void* msg, uint32_t length);
int recv_message(int socketFD, struct message* msg);

/*
    Attempts to create & send a message + msg_header to the given socketFD
    First: Sends the msg header
    Second: Sends a stream of bytes for the message that is of size length

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

    // Send the Header
    ssize_t writtenBytes = 0;
    while(writtenBytes < sizeof header)
    {
        ssize_t bytes = send(socketFD, ((char*)&header) + writtenBytes, sizeof header - writtenBytes, 0);
        if(bytes < 0)
        {
            return -1;
        }
        writtenBytes += bytes;
    }

    printf("Header sent\n");

    // Send the Message
    writtenBytes = 0;
    while(writtenBytes < length)
    {
        ssize_t bytes = send(socketFD, ((char*)msg) + writtenBytes, length - writtenBytes, 0);
        if(bytes < 0)
        {
            return -1;
        }
        if(bytes == 0)
        {
            errno = ECONNRESET;
            return -1;
        }
        writtenBytes += bytes;
    }

    printf("Message sent\n");
    return 0;
}



/*
    Reads a socket for the following...
    1. Reads a byte stream to construect a msg_header object
    2. Reads a byte stream for a message
    3. Uses the msg_header + message to fill out the msg field passed in

    Returns 0 on Success
    -1 on error, sets errno
*/
int recv_message(int socketFD, struct message* msg)
{
    int returnCode = 0;
    char    readBuffer[MAX_MESSAGE_SIZE];
    char    messageHeaderBuffer[sizeof(struct msg_header)];

    // Recv the header
    ssize_t readBytes = 0;
    ssize_t byteLength = 0;
    do
    {
        readBytes = read(socketFD, &readBuffer + byteLength, sizeof (struct msg_header) - byteLength);
        if(readBytes < 0)
        {
            returnCode = -1;
            perror("read");
            goto cleanup;
        }
        if(readBytes == 0)
        {
            errno = ECONNRESET;
            return -1;
        }

        memcpy(messageHeaderBuffer + byteLength, &readBuffer, readBytes);
        byteLength += readBytes;
    } while (byteLength < sizeof(struct msg_header));

    printf("Header received\n");
    messageHeaderBuffer[byteLength] = '\0';

    struct msg_header* header = (struct msg_header*)messageHeaderBuffer;

    // convert header fields from network byte order 
    uint32_t header_length = ntohl(header->length);
    uint32_t header_type = ntohl(header->type);
    printf("Header Length: %u, Header Type: %u\n", header_length, header_type);

    if(header_length >= MAX_MESSAGE_SIZE)
    {
        errno = EMSGSIZE;
        returnCode = -1;
        goto cleanup;
    }

    // Recv the message
    memset(&readBuffer, '\0', sizeof readBuffer);
    char    messageBuffer[MAX_MESSAGE_SIZE];
    readBytes = 0;
    byteLength = 0;
    do
    {
        readBytes = read(socketFD, &readBuffer, MAX_MESSAGE_SIZE - 1);
        if(readBytes < 0)
        {
            returnCode = -1;
            perror("read");
            goto cleanup;
        }

        if(readBytes == 0)
        {
            returnCode = -1;
            errno = ECONNRESET;
            goto cleanup;
        }
        memcpy(&messageBuffer + byteLength, &readBuffer, readBytes);
        byteLength += readBytes;
    } while(byteLength < header_length);
    messageBuffer[byteLength] = '\0';


    memcpy(msg->msg, messageBuffer, byteLength);
    msg->msg_length = header_length;
    msg->msg_type = header_type;

    cleanup:
    return returnCode;
}