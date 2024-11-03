#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
struct addrinfo {
    int                 ai_flags;       // AI_PASSIVE, AI_CANINNAME, etc.
    int                 ai_family;      // AF_INET, AF_INET6, AF_UNSPEC
    int                 ai_socktype;    // SOCK_STREAM, SOCK_DGRAM
    int                 ai_protocol;    // use 0 for "any"
    size_t              ai_addrlen;     // size of ai_addr in bytes 
    struct sockaddr    *ai_addr;        // struct sockaddr_in or _in6 
    char               *ai_canonname;   // full canonical hostname(完整规范主机名)

    struct addrinfo    *ai_next;        // linked list, next node
};
*/

/*
struct sockaddr {
    unsigned short       sin_family;        // address family, AF_INET
    char                 sa_data[14];       // 14 bytes of protocol address   
};
*/

/*
struct sockaddr_in {
    short int                   sin_family; // Address family, AF_INET
    unsigned short int          sin_port;   // Port number
    struct in_addr              sin_addr;   // Internet Address 
    unsigned char               sin_zero[8];    // Same size as struct sockaddr 
};
*/

/*
IPv4 only
Internet address (a structure for histories reasons)

struct in_addr{
    uint32_t s_addr;            // that's a 32-bit int (4 bytes)
};
*/

/*
int getaddrinfo(const char* node,                   // e.g. "www.example.com" or IP 
                const char* service,                // e.g. "http" or prot number 
                const struct addrinfo* hints,       // 
                struct addrinfo **res);
*/

/*
getaddrinfo 的最佳实践：
1. 用于服务器端（绑定本地地址）

如果你的程序是服务器端，需要监听来自客户端的连接，则：
•	第一个参数应为 NULL：这样可以表示绑定到所有本地网络接口的 IP 地址，方便在多网卡或多 IP 环境中使用。
•	设置 hints.ai_flags = AI_PASSIVE：这样会生成适合绑定的本地地址（如 0.0.0.0 或 ::），而不是某个具体远程主机的地址。
•	指定具体的端口号：将第二个参数设置为你要监听的端口号，比如 "3490"。这会返回一个绑定到该端口号的 addrinfo 列表。

2. 用于客户端（连接远程服务器）

如果你的程序是客户端，需要连接到远程服务器，则：
•	第一个参数应指定目标主机名或 IP 地址：例如，getaddrinfo("example.com", "80", &hints, &res)。这样会返回 example.com 的地址信息，用于连接到远程服务器。
•	省略 AI_PASSIVE：客户端无需设置 AI_PASSIVE，因为它不需要绑定到本地接口，而是要获取一个合适的远程地址。
•	指定服务或端口：可以指定服务名（如 "http"）或端口号（如 "80"）。这样 getaddrinfo 会返回适合用于连接的地址列表，并填入该端口号。

3. 第二个参数为空的情况

在实践中，第二个参数为空的情况确实较少见，通常用于以下场景：
•	仅获取 IP 地址信息：比如做 DNS 解析，或者你只想获取对方的 IP 地址，不关心特定的端口号。可以用 getaddrinfo("example.com", NULL, &hints, &res)，这种方式返回的 addrinfo 中端口号是未定义的。
•	手动设置端口：在某些特殊情况下，获取到地址后可能会手动设置端口号，而不是在调用 getaddrinfo 时指定。
*/