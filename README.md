# Beej_NP

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

4. 