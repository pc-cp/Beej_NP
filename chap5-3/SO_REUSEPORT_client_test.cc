#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

int main(int argc, char *argv[]) {
    char *str;
    if(argc != 2) {
        fputs("usage: ./client message\n", stderr);
        exit(1);
    }
    str = argv[1];

    struct addrinfo hints, *res;
    int sockfd;

    // first, load up address structs with getaddrinfo:
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        // use IPv4 or IPv6，whichever
    hints.ai_socktype = SOCK_STREAM;    // TCP

    getaddrinfo("139.224.234.82", "8080", &hints, &res);

    // make a socket:
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0) {
        perror("socket");
        return 1;
    }
    // connect 
    connect(sockfd, res->ai_addr, res->ai_addrlen);
    
    while(true) {
        // 发送数据
        send(sockfd, str, strlen(str), 0);
        sleep(3);
    }

    // 关闭连接
    close(sockfd);
    return 0;
}