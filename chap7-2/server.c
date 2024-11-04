/*
 server.c -- a stream socket server demo 
 // 这个程序运行有问题
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>

#define PORT "8080"     // the port users will be connecting to
#define BACKLOG 10      // how many pending connecting queue will hold

void sigchld_handler(int s) {
    (void) s;   // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    // 清理所有已终止的子进程，避免产生僵尸进程。
    // WNOHANG表示非阻塞模式，这样当没有已终止的子进程时，waitpid会立即返回0，而不会
    // 挂起程序。
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
// sockaddr_storage --> sockaddr <--> sockaddr_in/sockaddr_in6
void* get_in_addr(struct sockaddr* sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    else {
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
}

unsigned int get_in_port(struct sockaddr* sa) {
    if(sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    else {
        return (((struct sockaddr_in6*)sa)->sin6_port);
    }
}

int main(void) {
    int sockfd, new_fd;         // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information 
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;        // use my IP

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can 
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);     // all done with this structure

    if(p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if(listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;        // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    struct pollfd *pfds, *old_pfds;
    int num_clients = 1, old_clients; // listen socket
    pfds = (struct pollfd*)malloc(sizeof(struct pollfd) * num_clients);
    memset(pfds, 0x00, sizeof (struct pollfd));
    pfds[0].fd = sockfd;        
    pfds[0].events = POLLIN;        // Tell me when ready to read
    old_pfds = pfds;
    old_clients = num_clients;
    while(1) {  // main accept() loop
        int num_events = poll(old_pfds, old_clients, -1);
        if(num_events == -1) {
            perror("poll");
            exit(1);
        }
        pfds = NULL;
        num_clients = old_clients;
        for(int i = 0; i < old_clients; ++i) {
            if(old_pfds[i].fd == sockfd) { // new client
                sin_size = sizeof their_addr;
                new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
                if(new_fd == -1) {
                    perror("accept");
                    continue;
                }

                inet_ntop(their_addr.ss_family,
                        get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
                printf("server: got connection from %s(%d)\n", s, get_in_port((struct sockaddr *)&their_addr));

                struct pollfd* pfds_copy = (pfds ? pfds : old_pfds);

                pfds = (struct pollfd*)malloc(sizeof(struct pollfd) * (num_clients + 1));
                memset(pfds, 0x00, sizeof (struct pollfd) * (num_clients + 1));
                for(int i = 0; i < num_clients; ++i) {
                    pfds[i].fd = pfds_copy[i].fd;
                    pfds[i].events = pfds_copy[i].events;
                }
                pfds[num_clients].fd = new_fd;
                pfds[num_clients].events = POLLOUT; // just care send 
                num_clients ++;

                if(pfds_copy != old_pfds) {
                    free(pfds_copy);
                }
            }
            else {
                // int pollout_happened = old_pfds[i].revents & POLLOUT;
                // if(pollout_happened) {
                    if(send(old_pfds[i].fd, "Hello, World!", 13, 0) == -1) 
                        perror("send");
                    else {
                        printf("File descriptor %d is sent.\n", pfds[i].fd);
                    }
                // }
                // else {
                    // printf("Unexpected event occurred: %d\n", pfds[0].revents);
                // }
            }
        }
        if(pfds) {
            free(old_pfds);
            old_pfds = pfds;
            old_clients = num_clients;
        }
        if(num_events == 0) {
            printf("no events\n");
        }
    }
    return 0;
}
