/*
    epollserver.cc -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unordered_set>


#define PORT "8080"     // Port we're listening on
#define MAXEVENTS 64
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


int make_socket_non_blocking(int sfd) {
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    flags = fcntl(sfd, F_SETFL, flags);
    if(flags == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

// get client's port number
unsigned int get_in_port(struct sockaddr* sa) {
    if(sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    else {
        return (((struct sockaddr_in6*)sa)->sin6_port);
    }
}

// Return a listening socket 
int get_listener_socket(void) {
    int listener;       // listening socket descriptor
    int yes = 1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listener < 0) {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if(make_socket_non_blocking(listener) == -1) {
            fprintf(stderr, "error getting NONBLOCK socket\n");
            return -1;
        }

        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    // If we got here, it means we didn't get bound
    if(p == NULL) {
        return -1;
    }

    freeaddrinfo(ai); // All done with this 

    // Listen 
    if(listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
} 

// Main
int main() {

    int listener;   // listening socket descriptor

    int newfd;      // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr;     // client address 
    socklen_t addrlen;

    char buf[256];      // Buffer for client data 
    char remoteIP[INET6_ADDRSTRLEN]; 

    // Set up and get a listening socket 
    listener = get_listener_socket();
    if(listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    
    int efd = epoll_create1(EPOLL_CLOEXEC);
    if(efd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event event;
    struct epoll_event* events;

    event.data.fd = listener;
    // EPOLLET (edge-triggered) flag
    event.events = EPOLLIN | EPOLLET;
    int rv;
    rv = epoll_ctl(efd, EPOLL_CTL_ADD, listener, &event);
    if(rv == -1) {
        perror("epoll_ctl");
        exit(1);
    }

    // Buffer where events are returned
    events = static_cast<epoll_event*>(calloc(MAXEVENTS, sizeof event));
    if(!events) {
        perror("calloc");
        exit(1);
    }

    std::unordered_set<int> uset;
    uset.insert(listener);

    // Main loop 
    while(1) {
        int epoll_count = epoll_wait(efd, events, MAXEVENTS, -1);

        if(epoll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read 
        for(int i = 0; i < epoll_count; ++i) {
            if((events[i].events & EPOLLERR) ||
               (events[i].events & EPOLLHUP) ||
               (!(events[i].events & EPOLLIN))) {
                /* An error has occured on this fd, or the socket is not
                    ready for reading 
                */
                    fprintf(stderr, "epoll error\n");
                    close(events[i].data.fd);
                    uset.erase(events[i].data.fd);
                    continue;
            }
            else if(listener == events[i].data.fd) {
                // If listener is ready to read, handle new connection(s)
                while(1) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if(newfd == -1) {

                        if((errno == EAGAIN) || 
                           (errno == EWOULDBLOCK)) {
                            /*
                                The socket is marked nonblocking and 
                                no connections are present to be accepted.
                            */
                            // We have processed all incoming connections.
                        }
                        else {
                            perror("accept");
                        }
                        break;
                    }
                    else {
                        inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *)&remoteaddr), remoteIP, INET6_ADDRSTRLEN);
                        printf("epollserver: new connection from %s(%d) on "
                                "socket %d\n",
                                remoteIP, get_in_port((struct sockaddr *)&remoteaddr), 
                                newfd);

                        // Make the incoming socket non-blocking and 
                        // and it to the list of fds to monitor.
                        if(make_socket_non_blocking(newfd) == -1) {
                            fprintf(stderr, "error getting NONBLOCK socket\n");
                            return -1;
                        }

                        event.data.fd = newfd;
                        event.events = EPOLLIN | EPOLLET;
                        int rv;
                        rv = epoll_ctl(efd, EPOLL_CTL_ADD, newfd, &event);
                        if(rv == -1) {
                            perror("epoll_ctl");
                            exit(1);
                        }
                        uset.insert(newfd);
                    }
                }
            }
            else {
                // we have data on the fd waiting to be read.
                while (1) {
                    int nbytes = recv(events[i].data.fd, buf, sizeof buf, 0);
                    int sender_fd = events[i].data.fd;

                    if(nbytes <= 0) {
                        // Got error or connection closed by client
                        if(nbytes == 0) {
                            // connection closed 
                            printf("pollserver: socket %d hung up\n", sender_fd);
                            // Closing the descriptor automatically removes it from the watched set of epoll instance efd.
                            close(sender_fd);      // Bye!
                            uset.erase(sender_fd);
                        }
                        else {
                            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                                close(sender_fd);      // Bye!
                                perror("recv");
                                uset.erase(sender_fd);
                            }
                        }
                        break;
                    }
                    else {
                        // We got some good data from a client
                        for(auto &fd : uset) {
                            // Send to everyone!
                            int dest_fd = fd;

                            // Except the listener and ourselves 
                            if(dest_fd != listener && dest_fd != sender_fd) {
                                if(send(dest_fd, buf, nbytes, 0) == -1) {
                                    if(errno != EAGAIN && errno != EWOULDBLOCK) {
                                        close(dest_fd);      // Bye!
                                        perror("send");
                                        uset.erase(sender_fd);
                                    }
                                }
                            }
                        }
                        break;
                    } // END handle data from client
                }
            } // END got ready-to-read from poll()
        } // END looping through file descriptor
    } // END while(1)--and you thought it would never end!

    return 0;
}