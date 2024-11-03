#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>

int main() {

    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    /*
        struct sockaddr_in {
            short int                   sin_family; // Address family, AF_INET
            unsigned short int          sin_port;   // Port number
            struct in_addr              sin_addr;   // Internet Address 
            unsigned char               sin_zero[8];    // Same size as struct sockaddr 
        };

        // IPv4 only
        // Internet address (a structure for histories reasons)
        struct in_addr{
            uint32_t s_addr;            // that's a 32-bit int (4 bytes)
        };
    */
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;

    inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
    inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr));

    inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
    printf("The IPv4 address is: %s\n", ip4);

    inet_ntop(AF_INET6, &(sa6.sin6_addr), ip6, INET6_ADDRSTRLEN);

    printf("The address is: %s\n", ip6);
    return 0;
}