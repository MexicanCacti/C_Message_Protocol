#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/in.h>

#include "protocol.c"

#define BACKLOG 10
#define LISTEN_PORT "4567"
#define LISTEN_IP "127.0.0.1"

int get_readable_ip(struct addrinfo** addrInfo, struct sockaddr_in** ipInfo, char* ipStringBuffer, socklen_t len);

int create_and_bind_socket(struct addrinfo** addrInfo, int* socketFD);

int main(int argc, char** argv)
{
    int status, returnCode = 0;
    int socketFD = -1;
    
    struct addrinfo     hints;
    struct addrinfo     *serverInfo = NULL;
    struct sockaddr_in  *ipInfo = NULL;

    char                ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;    // fill in IP for me

    if ( (status = getaddrinfo(LISTEN_IP, LISTEN_PORT, &hints, &serverInfo) ) != 0)
    {
        returnCode = status;
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        goto free;
    }

    // Convert & store IP address for display after setup
    returnCode = get_readable_ip(&serverInfo, &ipInfo, ipstr, sizeof ipstr);
    if(returnCode != 0) goto free;

    returnCode = create_and_bind_socket(&serverInfo, &socketFD);
    if(returnCode != 0) goto close;

    status = listen(socketFD, BACKLOG);
    if(status == -1)
    {
        returnCode = errno;
        perror("listen error");
        goto close;
    }

    printf("Server listening on IP: %s and Port: %hu\n", ipstr, ntohs(ipInfo->sin_port));

    // loop & accept
    int connectionFD;
    socklen_t addr_size;
    struct sockaddr_storage connection_addr;
    
    while(1)
    {
        addr_size = sizeof connection_addr;
        connectionFD = accept(socketFD, (struct sockaddr *)&connection_addr, &addr_size);
        if(connectionFD == -1)
        {
            perror("accept");
            continue;
        }

        char connectionIP[INET_ADDRSTRLEN];
        char connectionPort[NI_MAXSERV];

        status = getnameinfo( (struct sockaddr *)&connection_addr, addr_size, connectionIP, sizeof(connectionIP), connectionPort, sizeof(connectionPort), NI_NUMERICHOST | NI_NUMERICSERV); // Flag return numeric vals

        if(status != 0) continue;
        printf("Connection from IP: %s and Port: %s\n", connectionIP, connectionPort);

        while(1)
        {
            ssize_t recvBytes;
            struct message msg;
            do
            {
                status = recv_message(connectionFD, &msg);
                if(status == 0) printf("Message Length: %u, Message Type: %u, Message Contents: %s\n", msg.msg_length, msg.msg_type, msg.msg);

            } while (status == 0);
            
            close(connectionFD);
            break;
        }
    }

close:
    close(socketFD);

free:
    freeaddrinfo(serverInfo);

    return returnCode;
}

int get_readable_ip(struct addrinfo** addrInfo, struct sockaddr_in** ipInfo, char* ipStringBuffer, socklen_t len)
{
    int returnVal = 0;
    *ipInfo = (struct sockaddr_in *)(*addrInfo)->ai_addr;
    if(inet_ntop((*addrInfo)->ai_family, &((*ipInfo)->sin_addr), ipStringBuffer, len) == NULL)
    {
        returnVal = errno;
        perror("ipv4 conversion error");
    }

    return returnVal;
}

int create_and_bind_socket(struct addrinfo** addrInfo, int* socketFD)
{
    int returnVal = 0;

    *socketFD = socket((*addrInfo)->ai_family, (*addrInfo)->ai_socktype, (*addrInfo)->ai_protocol);
    if(*socketFD == -1)
    {
        returnVal = errno;
        perror("socket creation error");
        goto cleanCreate;
    }

    int yes = 1;
    setsockopt(*socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    returnVal = bind(*socketFD, (*addrInfo)->ai_addr, (*addrInfo)->ai_addrlen);
    if(returnVal != 0)
    {
        perror("socket binding error");
    }

cleanCreate:
    return returnVal;
}