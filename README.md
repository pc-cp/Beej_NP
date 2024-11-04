# Beej_NP
[Beej's Guide to Network Programming Using Internet Sockets](https://beej.us/guide/bgnet/)

QA

## chap7-1
1. 有人说recv只是与socket的接收缓冲区打交道，而不是recv本身从对方接收数据。同时send只是与发送缓冲区打交道，而不是send本身将数据发给对方。如何理解？

    这句话强调recv和send这两个操作实际上并不是直接与对端的程序打交道，而是与OS的网络缓冲区打交道。
    1. recv和接收缓冲区
    
        当你调用recv时，recv从操作系统内核中的接收缓冲区(socket's receive buffer)读取数据。这个缓冲区是OS为该套接字分配的一个临时存储区域。

        流程如下：
        - 当数据从网络上到达你的主机时，OS会将这些数据放到套接字的接收缓冲区中。
        - recv调用会读取这些已经在缓冲区的数据，而不是直接去网络上“取”数据。
        - **如果接收缓冲区里有数据，recv会立即返回这些数据**。
        - **如果接收缓冲区为空，recv在阻塞模式下会等待数据到达**（或在**非阻塞模式下会立即返回一个错误，如EAGAIN或EWOULDBLOCK**）。

    2. send和发送缓冲区

        当你调用send时，send并不会直接把数据发给对方，而是将数据放入发送缓冲区(socket's send buffer)。

        流程如下：
        - 当你调用send时，数据会被copy到发送缓冲区。
        - OS随后在后台将发送缓冲区中的数据通过网络发送出去，这个过程是**异步**的。
        - **如果发送缓冲区有足够空间，send会立即成功，将数据放入缓冲区后返回。**
        - **如果发送缓冲区已满，send在阻塞模式下会等待**缓冲区有空余（或在非阻塞模式下会立即返回错误，如EAGAIN或EWOULDBLOCK）。

    3. 关键点
        - recv和send与对端程序没有直接交互，它们只是和OS的缓冲区交互。
        - OS的缓冲区在网络数据的传输过程中充当一个缓冲角色，使得发送和接收过程能够异步进行，提升了效率。
        - **recv和send的调用结果依赖于缓冲区的状态（是否有数据，是否有空余）**，而非直接依赖于对端是否发送或接收。
2. `int fcntl(int fd, int op, ... /* arg */ );`
        
    如果将server端的通信套接字设置为非阻塞，那么影响的是client的recv和send还是server的recv和send？
        
    影响的是服务器端的recv和send操作，而不是客户端的recv和send操作
    1. 服务器端的recv：当服务器调用recv去接收数据时，如果没有数据可用，**recv会立即返回-1**，并将errno设置为EAGAIN或EWOULDBLOCK，表示当前没有数据可读取。服务器可以继续执行其他操作而无需等待数据到达

    2. 服务器端的send：当服务器调用send发送数据时，如果发送缓冲区已满，send会立即返回-1，并将errno设置为EAGAIN或EWOULDBLOCK，表示无法立即发送。服务器可以选择稍后重试发送数据。

3. 理解阻塞和非阻塞模式对recv的影响（send同理）：
    1. 非阻塞模式的行为：当recv调用的接收缓冲区为空时，recv立即返回-1，并将errno设置为EAGAIN或EWOULDBLOCK，表示没有数据可供读取。其避免了进程被挂起，但会导致程序不断尝试recv，即所谓的“忙等待”现象，这会耗费CPU资源。因此，在实际使用中，非阻塞模式通常结合select或poll等系统调用，以高效地等待数据而避免忙等待。
        ```shell
        client: connecting to 139.224.234.82
        recv: Resource temporarily unavailable
        client: received 'Hello, World!'
        recv: Resource temporarily unavailable
        recv: Resource temporarily unavailable
        recv: Resource temporarily unavailable
        recv: Resource temporarily unavailable
        client: received 'Hello, World!'
        ```
    2. 阻塞模式的行为：当接收缓冲区为空，recv会阻塞进程，直到数据到达。这会将该进程挂起，使CPU可调度其他任务。此时，OS会将进程置于等待队列中，等待数据到达并解除阻塞。这种方式不会导致忙等待，但会影响响应时间，因为recv调用会一直阻塞直到有数据可读。
        ```shell
        client: connecting to 139.224.234.82
        client: received 'Hello, World!'
        client: received 'Hello, World!'
        client: received 'Hello, World!'
        ```
    3. 适用场景：非阻塞模式适用于需要实时响应的情况，例如实施数据采集或多客户端的服务器。在这种情况下，结合select或poll等多路复用机制，服务器可以高效地管理多个客户端的连接。
## chap7-3
1. select的BUGS(`man select`)
    >Under Linux, select() may **report a socket file descriptor as "ready for reading", while  nevertheless  a subsequent  read  blocks**. This could for example happen when data has arrived but upon examination has wrong checksum and is discarded. There may be other circumstances in which a file descriptor is  **spuriously reported as ready**. Thus it may be safer to use **O_NONBLOCK** on sockets that should not block.
2. 尽管select和poll API强调可同时监听多种类型的事件，但是往往在使用时只设置"ready for reading"事件，这是为什么？
    1. 常见场景主要是等待客户端的请求
        许多网络程序（Webserver）最常见的任务是等待客户端的请求或数据（可读事件）。服务端往往先等待客户端发送的数据，接收后再决定如何处理并响应。可读事件常常是触发业务逻辑的关键。
    2. 可写事件的触发频率较高
        在默认情况下，一个socket在没有阻塞的情况下，通常会一直处于可写状态。这意味着，如果我们监听可写事件，则poll或select可能会频繁地返回该事件，即使没有需要写入的数据。这种频繁的返回会浪费资源，增加不必要的CPU占用。
    3. 写操作通常在需要时直接进行
        由上述可知，大多数应用在有数据需要写入时会/可以直接执行写操作。
3. 返回值rv和errno的关系
    1. 大多数系统调用在失败时会返回-1或NULL，并设置errno
    2. errno提供了具体的错误原因，但在每次成功的函数调用后不会自动清零（手动设0以便于检测新的错误）。
    3. errno的错误信息可以通过perror或strerror(errno)来获取用户友好的描述。
4. send函数在阻塞socket下会保证发送完全部数据后返回，而在非阻塞socket下需要手动保证：
    > **[阻塞 socket 是否能确保所有数据都发送完？]**
    ```cpp
    int sendall(int s, char *buf, int *len) {
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
    ```


