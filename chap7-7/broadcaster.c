/*
    broadcaster.c -- a datagram "client" like talker.c,
                     this one can broadcast
    在本地同时运行listener.c,
    需要加
    “
        int opt = 1;

        // 允许地址复用
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(1);
        }

        // 允许端口复用（如果系统支持 SO_REUSEPORT）
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
            perror("setsockopt(SO_REUSEPORT) failed");
            exit(1);
        }
    ”

    然后本地运行命令./broadcaster 255.255.255.255 "Your message here"
    两个listener.c接收到
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "8080"       // the port users will be connecting to 

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in their_addr;      // connector's address information
    struct hostent *he;
    int numbytes;
    int broadcast = 1;

    if(argc != 3) {
        fprintf(stderr, "usage: talker broadcaster hostname message\n");
        exit(1);
    }
    if((he = gethostbyname(argv[1])) == NULL) { // get the host info 
        perror("gethostbyname");
        exit(1);
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // this call is what allows broadcast packets to be sent:
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
                sizeof(broadcast)) == -1) {
                    perror("setsockopt (SO_BROADCAST");
                    exit(1);
                }

    their_addr.sin_family = AF_INET;                // host byte order
    their_addr.sin_port = htons(SERVERPORT);        // short, network byte order 
    their_addr.sin_addr = *((struct in_addr*)he->h_addr);

    memset(&their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

    
    if((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, (struct sockaddr*)&their_addr, sizeof their_addr)) == -1) {
        perror("sendto");
        exit(1);
    }

    printf("sent %d bytes to %s\n", numbytes, inet_ntoa(their_addr.sin_addr));

    close(sockfd);
    return 0;
}