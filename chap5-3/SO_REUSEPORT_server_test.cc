#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

#define MAXLINE 80 // 缓冲区的大小

int main() {
    struct addrinfo hints, *res;
    int sockfd;

    // first, load up address structs with getaddrinfo:
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        // use IPv4 or IPv6，whichever
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // fill in my IP for me 

    getaddrinfo(NULL, "8080", &hints, &res);

    // make a socket:
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }
    // bind it to the port we passed in to getaddrinfo()
    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    listen(sockfd, 2);

    printf("Listening on port 8080...\n");
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    struct sockaddr_in cliaddr;                 // 定义客户端地址结构体
    socklen_t cliaddr_len = sizeof(cliaddr);    // 客户端地址长度
    while (true) {
        int clientfd = accept(sockfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        while(clientfd >= 0) {

            printf("received from %s at PORT %d:", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
            int n = recv(clientfd, buf, MAXLINE, 0);
            printf("content: %s\n", buf);
            if(n <= 0) {
                perror("recv");
                break;
            }
        }
        close(clientfd);    
    }
    close(sockfd);
    return 0;
}