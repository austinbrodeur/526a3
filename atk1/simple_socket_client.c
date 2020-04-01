#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> // for error handling
#include <string.h> // string lib
#include <netdb.h> // network and databse operations
#include <sys/types.h> // basic types
#include <netinet/in.h> // internet addresses
#include <sys/socket.h> // sockets
#include <arpa/inet.h> // internet operations

#define PORT "37952" // port
#define MAXDATASIZE 100 // max num of bytes

/*

Uses big endian.

Addrinfo function definition:

int getaddrinfo(const char* hostname,
                const char* service,
                const struct addrinfo* hints,
                struct addrinfo** res);

Addrinfo struct definition:

struct addrinfo {
    int       ai_flags;             additional options
    int       ai_family;            desired address family, such as ipv4 or ipv6
    int       ai_socktype;          preferred socket type, could be SOCK_DGRAM for UDP, for example
    int       ai_protocol;          specifies protocol used, 0 indicates any field can be used by getaddrinfo
    socklen_t ai_addrlen;
    struct    sockaddr* ai_addr;
    char*     ai_canonname;         canonical name 
    struct    addrinfo* ai_next;    this struct can form a linked list
};
*/


// get sockaddr, ipv4 or ipv6
void *get_in_addr (struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) { // to get ipv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr); // to get ipv6
}

int main (int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // initializes the hints struct
    hints.ai_family = AF_UNSPEC; // protocol family accepted, AF_UNSPEC means will accept any protocol
    hints.ai_socktype = SOCK_STREAM; // set socktype, SOCK_STREAM usually used for TCP, datagrams don't have to be manually made

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) { // gets server info
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) { // will iterate through multiple addresses if the host returns multiple, for example, google
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);

    close(sockfd);

    return 0;

}