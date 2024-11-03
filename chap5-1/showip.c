/*
showip.c -- show IP addresses for a host given on the command line 
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if(argc != 2) {
        fprintf(stderr, "usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    /*
        getaddrinfo函数中，第二个参数指定服务名称或端口号。当
        该参数为NULL时，表示没有特定的服务或端口要求，通常用于仅
        获取目标地址信息（如IP），而不是用于建立连接。

        so 第二个参数为NULL很少见。
    */
    if((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    printf("IP addresses for %s:\n\n", argv[1]);

    /*  
        一个域名（例如 www.baidu.com）可能对应多个 IP 地址（IPv4 和 IPv6）：
            域名对应多个 IP 地址可以帮助提高访问速度、增加可靠性、分担负载、并适应不同的网络协议（IPv4 和 IPv6）。
    */
    for(p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6
        if(p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "Ipv4";
        }
        else {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("    %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res);      // free the linked list
    return 0;
}