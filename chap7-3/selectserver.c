/*
   selectserver.c -- a cheezy multiperson chat server 
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

#define PORT "8080"		// port we're listening on


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

int sendall(int s, char *buf, int *len) {
    int total = 0;              // how many bytes we've sent 
    int bytesleft = *len;       // how many we have left to send 
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if(n == -1) {break;}

        total += n;
        bytesleft -= n;
    }
    *len = total;               // return number actually sent here 
    return n == -1 ? -1 : 0;    // return -1 on failure, 0 on success
}

int main() {
	fd_set master; 		// master file descriptor list
	fd_set read_fds;	// temp file descriptor list for select()
    int fdmax;			// maximum file descriptor number 
	
	int listener;		// listening socket descriptor 
	int newfd;			// newly accept()ed socket descriptor 
	struct sockaddr_storage remoteaddr;		// client address 
    socklen_t addrlen;

	char buf[256];		// Buffer for client data 
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];
	int rv;

	FD_ZERO(&master);	// clear the master and temp sets 
	FD_ZERO(&read_fds);

	// Set up and get a listening socket 
    listener = get_listener_socket();

    if(listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
	
	// add the listener to the master set 
	FD_SET(listener, &master);

	// keep track of the biggest file descriptor 
	fdmax = listener;	// so far, it's this one 

	// main loop 
	while(1) {
		read_fds = master;	// copy it 
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read 
        for(int i = 0; i <= fdmax; ++i) {
            if(FD_ISSET(i, &read_fds)) {    // we got one!
                if(i == listener) {
                    // handle new connections 
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr*)&remoteaddr, &addrlen);
                    if(newfd == -1) {
                        perror("accept");
                    }
                    else {
                        // prevent spurious "ready-to-read"
                        int flags;
                        if((flags = fcntl(newfd, F_SETFL, 0)) < 0) { // 获取当前flags
                            perror("fcntl");
                            return 2;
                        }
                        flags |= O_NONBLOCK;    // 在原flags上添加非阻塞标志
                        if((rv = fcntl(newfd, F_SETFL, flags)) < 0) { // 设置新的flags
                            perror("fcntl");
                            return 2;
                        }

                        FD_SET(newfd, &master);     // add to master set 
                        if(newfd > fdmax) {         // keep track of the max 
                            fdmax = newfd;
                        }
                        inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *)&remoteaddr), remoteIP, INET6_ADDRSTRLEN);
                        printf("pollserver: new connection from %s(%d) on "
                                "socket %d\n",
                                remoteIP, get_in_port((struct sockaddr *)&remoteaddr), 
                                newfd);
                    }
                }
                else {      // handle data from client 
                    if((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client 
                        if(nbytes == 0) {
                            // connection closed 
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else {
                            perror("recv");
                        }
                        close(i);       // Bye! 
                        FD_CLR(i, &master);     // remove from master set 
                    }
                    else {
                        // we got some data from a client 
                        for(int j = 0; j <= fdmax; j++) {
                            // send to everyone 
                            if(FD_ISSET(j, &master)) {
                                // except the listener and ourselves 
                                if(j != listener && j != i) {
                                    if(send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }

                } // END handle data from client 
            } // END got new incoming connection
        } // END looping through file descriptor 
		
	} // END while(1) -- and you thought it would never end!

    return 0;
}

