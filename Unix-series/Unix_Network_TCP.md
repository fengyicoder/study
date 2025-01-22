# 简介和TCP/IP

## 简介

客户和服务器之间通过某个网络协议进行通信，这样的通信通常涉及多个网络协议层。比如web客户与服务器之间使用TCP通信，TCP又使用IP通信，IP再通过某种数据链路层通信。如果客户与服务器处于同一个以太网，就有如下的通信层次：

![](..\image\Unix\1-1-3.png)

## OSI模型

模型如下：

![](..\image\Unix\1-1-14.png)

## 传输层：TCP、UDP和SCTP

## 用户数据包协议（UDP）

应用进程往一个UDP套接字写入一个消息，该消息随后被封装到一个UDP数据报，该UDP数据报进而又被封装到一个IP数据报，然后发送到目的地。UDP不保证UDP数据报会到达其最终目的地，不保证每个数据报的先后顺序跨网络后保持不变，也不保证每个数据报只到达一次。

每个UDP数据包都有一个长度，如果一个数据报正确到达目的地，那么该数据包的长度将随数据一道传递给接收端应用进程。

## 传输控制协议（TCP）

TCP提供了可靠性，当TCP向另一端发送数据时，要求对端返回一个确认，如果没有收到确认，TCP就自动重传数据并等待更长时间，在数次重传失败之后，TCP才会放弃，如此在尝试发送数据上所花的总时间一般是4-10分钟。

TCP含有用于动态估算客户和服务器之间的往返时间（RTT）的算法，以便它知道等待一个确认需要多少时间。举例来说RTT在一个局域网上大约是几毫秒，跨越一个广域网则可能是数秒钟。

TCP通过给其中每个字节关联一个序列号对发送数据进行排序。TCP也提供流量控制，总是告知对端在任何时刻它一次能够从对端接收多少字节的数据，这称为通告窗口。在任何时刻，该窗口指出接收缓冲区中当前可用的空间量，从而确保发送端发送的数据不会使得接收缓冲区溢出。该窗口时刻动态变化：当接收到来自发送端的数据时，窗口大小就减小，当接收端从缓冲区读取数据时该窗口就变大。

最后，TCP是全双工的。

## 流控制传输协议（SCTP）

SCTP在客户和服务器之间提供关联，并像TCP那样给应用提供可靠性、排序、流量控制以及全双工的数据传送。一个关联指代两个系统之间的一次通信，它可能因为SCTP支持多宿而涉及不止两个地址。

SCTP是面向消息的，提供各个记录的按序递送服务。与UDP一样，由发送端写入的每条记录的长度随数据一道传递给接收端应用。

SCTP能够在所连接的端点上提供多个流，每个流各自可靠的按序递送消息。一个流上某个消息的丢失不会阻塞同一关联其他流上的消息的投递。

SCTP还提供多宿特性，使得单个SCTP端点能够支持多个IP地址。该特性可以增强应对网络故障的健壮性。

## TCP连接的建立和终止

建立一个TCP连接会发生三路握手：

1. 服务器必须准备好接受外来的连接，通常通过调用socket、bind和listen这三个函数，称之为被动打开；
2. 客户调用connect发起主动打开，这导致客户TCP发送一个SYN（同步）分节，它告诉服务器客户将在连接中发送的数据的初始序列号。通常SYN分节不携带数据，其所在的IP数据包只含有一个IP首部、一个TCP首部及可能的TCP选项；
3. 服务器必须确认（ACK）客户的SYN，同时自己也得发送一个SYN分节，它含有服务器将在同一连接中发送的数据的初始序列号。服务器在单个分节中发送SYN和对客户SYN的ACK；
4. 客户必须确认服务器的SYN；

这种交换至少需要三个分组，如图：

![](..\image\Unix\1-2-2.png)

TCP建立一个连接需要3个分节，终止一个连接则需要4个分节：

1. 某个应用进程首先调用close，我们称该端执行主动关闭。该端的TCP发送一个FIN分节，表示数据发送完毕；
2. 接收到这个FIN的对端执行被动关闭，这个FIN由TCP确认，它的接收也作为一个文件结束符传递给接收端应用进程。
3. 一段时间后接收到这个文件结束符的应用进程调用close关闭它的套接字，导致它的TCP也发送一个FIN。
4. 接收这个最终FIN的原发送端TCP确认这个FIN。

如图：

![](..\image\Unix\1-2-3.png)

在步骤2和3之间，从执行被动关闭到执行主动关闭一端流动数据是可能的，这称为半关闭。

当套接字被关闭时，其所在端的TCP各自发送一个FIN。需要认识到，只要一个Unix进程终止，所有打开的描述符都被关闭，这也导致仍然打开的任何TCP连接上也发出一个FIN。

## TIME_WAIT状态

即客户端接受服务器close发送FIN之后TCP进入的状态，此状态的持续时间是最长分节生命期的两倍（2MSL），这个时间在1分钟到4分钟之间。MSL是任何IP数据报能够在因特网中存活的最长时间。

TIME_WAIT状态有两个存在的理由：

1. 可靠的实现TCP全双工连接的终止；
2. 允许老的重复分节在网络中消逝；

第一个理由可以通过假设最终的ACK丢失了来解释，服务器将重新发送它的最终的FIN，因此客户必须维护状态信息，以允许它重新发送那个ACK。第二个理由假设关闭一个连接后又在相同的地址和端口建立另一个连接，后一个连接称为前一个连接的化身，TCP必须防止旧连接的老的重复的连接在已终止之后再现，从而被误解为属于同一连接的某个新的化身，为做到这点，TCP将不给处于TIME_WAIT状态的连接发起新的化身。既然TIME_WAIT状态持续时间为2MSL，这足以让某个方向上的分组最多存活MSL秒即被丢弃，另一个方向上的应答也最多存活MSL秒被丢弃。

## SCTP关联的建立和终止

建立一个SCTP关联需要四路握手：

1. 服务器准备好接受外来的关联，通常调用socket、bind和listen这三个函数来完成，称为被动打开；
2. 客户通过connect或者发送一个隐式打开该关联的消息主动打开，这使得客户SCTP发送一个INIT消息，该消息告诉服务器的IP地址清单、初始序列号、用于标识本关联中所有分组的起始标记、客户请求的外出流的数目以及客户能够支持的外来流的数目；
3. 服务器以一个INIT ACK消息确认客户的INIT消息，其中含有服务器的IP地址清单、初始序列号、起始标记、服务器请求的外出流的数目、服务器能够支持的外来流的数目以及一个状态cookie。状态cookie包含服务器用于确信本关联有效所需的所有状态，它是数字签名过的，以确保有效性。
4. 客户以一个COOKIE ECHO消息回射服务器的状态cookie；
5. 服务器以一个COOKIE ACK消息确认客户回射的cookie是正确的；

如下：

![](..\image\Unix\1-2-6.png)

四路握手过程结束时两端各自选择一个主目的地址。

SCTP不像TCP那样允许半关闭的关联，当一端关闭某个关联时另一端必须停止发送新的数据。

![](..\image\Unix\1-2-7.png)

SCTP没有类似TCP那样的TIME_WAIT状态，因为SCTP使用了验证标记，所有后续块都在捆绑它们的SCTP分组的公共首部标记了初始的INIT块和INIT ACK块中作为起始标记交换的验证标记；由来自旧连接的块通过所在SCTP分组的公共首部间接携带的验证标记对于新连接来说是不正确的。

## 端口号

以上所述的三种协议都使用16位整数的端口号来区分进程。端口号被划分为以下三段：

1. 众所周知的端口为0-1023，由IANA分配和控制，可能的话，相同端口号就分配给TCP、UDP和SCTP的同一给定服务；
2. 已登记的端口为1024-49151（49152是65536的四分之三），这些不受IANA控制，不过由IANA登记并提供它们的使用情况清单，可能的话，相同端口号也分配给TCP和UDP的同一给定服务；
3. 49152-65535是动态的或私用的端口。IANA不管这些端口，它们被称为临时端口；

注意，Unix系统有保留端口的概念，指的是小于1024的任何端口，这些端口只能赋予特权用户进程的套接字。

![](..\image\Unix\1-2-10.png)

一个TCP连接的套接字对是一个定义该连接的两个端点的四元组：本地IP地址、本地TCP端口号、外地IP地址、外地TCP端口号。就SCTP而言，一个关联由一组本地IP地址、一个本地端口、一组外地IP地址、一个外地端口标识。标识每个端点的两个值（IP和端口号）通常称为一个套接字。

# 套接字编程简介

## 套接字地址结构

ipv4套接字地址结构以sockaddr_in命名，定义在\<netinet/in.h\>头文件中，posix定义如下：

![](..\image\Unix\1-3-1.png)

注意ipv4地址和TCP或UDP端口号在套接字地址结构中总是以网络字节序来存储。

ipv6套接字结构如下所示：

![](..\image\Unix\1-3-2.png)

## 值结果参数

当往一个套接字函数传递一个套接字地址结构时，传递的总是指向该结构的一个指针。该结构的长度也作为一个参数来传递，不过其传递方式取决于该结构的传递方向：从进程到内核还是从内核到进程：

1. 从进程到内核传递套接字地址结构的函数有三个：bind、connect和sendto。这些函数的一个参数是某个套接字地址结构的整数大小；
2. 从内核到进程的有四个：accept、recvfrom、getsockname和getpeername，其中两个参数是指向某个套接字地址结构的指针和指向表示该结构大小的整数变量的指针；

这里变成使用指针的原因是当函数被调用时，结构大小是一个值，它告诉内核该结构大小，这样内核在写结构时不至于越界，当函数返回时，结构大小又是一个结果，告诉进程内核在该结构中究竟存了多少信息。这种类型的参数称为值-结果参数。

## 字节排序函数

考虑一个16位整数，它由两个字节组成。内存中存储这个字节有两个方法：一种是将低序字节存储在起始地址，这称为小端字节序，另一种是将告序字节存储在起始地址，这称为大端字节序。如：

![](..\image\Unix\1-3-9.png)

两种字节序转换使用以下四个函数：

![](..\image\Unix\1-3-10.png)

## 字节操纵函数

如下：

![](..\image\Unix\1-3-11.png)

![](..\image\Unix\1-3-12.png)

## inet_aton、inet_addr和inet_ntoa函数

inet_aton、inet_addr和inet_ntoa在点分十进制数串与它长度为32位的网络字节序二进制值间转换IPv4地址。

## inet_pton和inet_ntop函数

inet_pton和inet_ntop对于ipv4和ipv6地址都适用。

# 基本TCP套接字编程

## connect函数

如果是TCP套接字，调用connect会触发TCP的三路握手，而且仅在连接建立成功或出错时才返回，其中出错返回有以下几种情况：

1. 若TCP客户没有收到SYN分节的响应，则返回ETIMEOUT错误；比如4.4BSD内核发送一个SYN，若无响应则等待6s后再发送一个，若仍无响应则等待24s后再发送一个，若总共等待了75s后仍未收到响应则返回本错误；
2. 若对客户的SYN的响应是RST，则表明该服务器主机在我们指定的端口上没有进程在等待与之连接。这是一种硬错误，客户一接收到RST就马上返回ECONNERFUSED错误；产生RST的三个条件是：目的地为某端口的SYN到达，然而该端口上没有正在监听的服务器；TCP想取消一个已有连接‘TCP接收到一个根本不存在的连接上的分节。
3. 若客户发出的SYN在中间的某个路由器上引发了一个“destination unreachable” ICMP错误，则认为是一种软错误。客户主机内核保存该消息，并按第一种情况中所述的时间间隔继续发送SYN。若在某个规定的时间内仍未收到响应，则把保存的消息作为EHOSTUNREACH或ENETUNREACH错误返回给进程。

## fork和exec函数

fork被调用一次，返回两次。它在调用进程（父进程）中返回一次，返回值是新派生进程（子进程）的进程ID号，在子进程中又返回一次，返回值为0。子进程可以通过getppid获取父进程的进程ID，所以这里被设计为父进程返回子进程的进程ID。

父进程调用fork之前打开的所有文件描述符在fork返回之后由子进程共享。fork有两个典型用法：

1. 一个进程创建一个自身的副本，这样每个副本都可以处理各自的某个操作，这是网络服务器的典型用法；
2. 一个进程想要执行另一个程序，创建副本后其中一个副本执行exec把自身替换成新的程序；

exec函数有6个，其区别在于：

- 待执行的程序文件是由文件名还是路径名指定；
- 新程序的参数是一一列出还是由一个指针数组来引用；
- 把调用进程的环境传递给新程序还是给新程序指定新的环境；

如下：

```c
#include <unistd.h>
int execl(const char *pathname, const char *arg0, .../* (char *)0 */);
int execv(const char *pathname, char *const *argv[]);
int execle(const char *pathname, const char *arg0, .../* (char *)0, char *const envp[] */);
int execve(const char *pathname, char *const *argv[], char *const envp[]);
int execlp(const char *filename, const char *arg0, .../* (char *)0 */);
int execvp(const char *filename, char *const *argv[]); 
#成功不返回，出错则为-1
```

这些函数只有在出错时才返回到调用者，否则控制将被传递给新程序的起始点。一般来说，只有execve是内核中的系统调用，其他5个都是调用execve的库函数。

## close函数

close一个TCP套接字的默认行为是将该套接字标记为已关闭，然后立即返回到调用进程，该套接字描述符不能由调用进程使用，即不能再作为read或write的第一个参数。然而TCP将尝试发送已排队等待发送到对端的任何数据，发送完毕后发生的是正常的TCP连接终止序列。

## getsockname和getpeername函数

返回与某个套接字关联的本地协议地址和外地协议地址：

```c
#include <sys/socket.h>
int getsockname(int sockfd, struct sockaddr *localaddr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen);
```

# TCP客户/服务器程序示例

## POSIX信号处理

信号可以：

- 由一个进程发给另一个进程；
- 由内核发给某个进程；

可以用sigaction函数设定一个信号的处理，并有三种选择：

1. 可以提供一个函数用来处理特定信号，这样的函数称为信号处理函数，但有两个信号不能捕获，分别为SIGKILL和SIGSTOP，信号处理函数由信号值这个单一的整数参数来调用，且没有返回值，函数原型为`void handler(int signo);`对于大多数信号来说，调用sigaction函数指定信号处理函数即可，但SIGINO、SIGPOLL、SIGURG这些个别信号还要求捕获它们的进程做些额外工作；
2. 可以把某个信号的处置设定为SIG_IGN来忽略它，SIGKILL和SIGSTOP这两个信号不能被忽略；
3. 可以把某个信号的处置设定为SIG_DFL来启动默认处置，默认通常是收到信号后终止进程，但SIGCHLD和SIGURG则是默认为忽略；

对于一个可能永远阻塞的系统调用，比如accept，当某个进程阻塞在这种系统调用时捕获到了某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。有些内核自动重启某些被中断的系统调用（比如支持SA_RESTART标志）。为了便于移植，必须对慢系统调用返回EINTR有所准备。注意，有一个函数我们不能重启，connect，如果再次调用它会立即返回一个错误。当connect被一个捕获信号中断而不自动重启时我们必须调用select来等待连接完成。

## wait和waitpid函数

```c
#include<sys/wait.h>
pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);
```

waitpid的pid参数如果给定-1则等待第一个终止的子进程。其次options最常用的选项WNOHANG，告知内核在没有已终止子进程时不要阻塞。

如果有多个SIGCHLD同时发出，信号处理函数只会处理一次，就会出现僵尸进程，此时不适用wait而应该用waitpid：

```c++
#include "unp.h"
void sig_chld(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}
```

## accept返回前连接中止

即三路握手完成后accept之前，客户发了一个RST到达。这种情况，posix规定，accpet返回一个errno，值为ECONNABORTED。

## 服务器进程终止

场景如下，一个处理连接的服务器子进程终止，它所有的打开的描述符都关闭，会导致向客户发送一个FIN，客户则会响应一个ACK。之后，SIGCHLD信号被发送到服务器父进程被正确处理。客户上没有发生特殊的事情，应该接收服务器的FIN并响应一个ACK，然而客户现在阻塞在系统调用fgets上，对此没有反应。此时如果向客户键入文本，客户接着将数据发送给服务器。由于服务器原先打开套接字的进程已经终止，当接收客户数据时会响应以一个RST。然而客户进程看不到这个RST，因为它在调用writen之后立即调用readline，并且由于接收到了FIN，此时readline立即返回0（表示EOF），客户没有预期收到EOF因此终止。

## SIGPIPE信号

如果客户不理会readline返回的错误，继续写数据，即进程向某个已收到RST的套接字继续执行写操作，内核会向该进程发送一个SIGPIPE信号，该信号的默认行为是终止进程。不论该进程是捕获了该信号并处理返回还是简单的忽略该信号，写操作都返回EPIPE错误。

## 服务器主机崩溃

当服务器主机崩溃时，客户TCP将持续重传数据分节，当客户TCP最终放弃时，既然客户阻塞在readline调用上，因为服务器主机崩溃，从而对客户机分节根本没有响应，那么返回的错误为ETIMEDOUT，如果中间某个路由器判定主机不可达，那么返回的消息是EHOSTUNREACH或ENETUNREACH。

## 服务器主机崩溃重启

在前文中情况如果主机崩溃重启，它的TCP丢失了崩溃前的连接信息，因此服务器TCP对收到的数据分节响应一个RST，客户TCP收到RST时，由于正阻塞在readline上，该调用返回ECONNRESET错误。

## 服务器主动关机

Unix系统关机时，init进程通常先给所有进程发送SIGTERM信号，等待一段固定时间后（往往5到20s之间），再给所有仍在运行的进程发送SIGKILL信号。

# I/O复用：select和poll函数

## I/O模型

Unix下可用的5中I/O模型如下：

- 阻塞式I/O；
- 非阻塞式I/O；
- I/O复用（select和poll）；
- 信号驱动式I/O（SIGIO）；
- 异步I/O（Posix的aio_系列函数）；

默认情形下，所有套接字都是阻塞的。以数据报套接字为例，有：

![](..\image\Unix\1-6-1.png)

进程把一个套接字设置成非阻塞是在通知内核：当所请求的I/O操作非得把本进程投入睡眠才能完成时，不要把本进程投入睡眠，而是返回一个错误。

![](..\image\Unix\1-6-2.png)

I/O复用是进程阻塞在select或poll这两个系统调用上，而不是真正的I/O系统调用。

![](..\image\Unix\1-6-3.png)

信号驱动式I/O是用信号让内核在描述符就绪时发送SIGIO信号通知我们。

![](..\image\Unix\1-6-4.png)

这种需要开启套接字的信号驱动式I/O功能，并通过sigaction系统调用安装一个信号处理函数。该系统调用将立即返回，进程将继续工作。

异步I/O模型一般是告知内核启动某个操作，并让内核在整个操作完成后通知我们。其与信号驱动式I/O的区别是信号驱动式是由内核通知我们何时可以启动一个I/O操作，而异步I/O模型是由内核通知我们I/O操作何时完成。

![](..\image\Unix\1-6-5.png)

## select函数

该函数允许进程指示内核等待多个事件中的任何一个发生，并只有在一个或多个事件发生或经历一段指定的时间后才唤醒它。

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int maxfdpl, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout); //若有就绪描述符则返回其数目，若超时则为0，出错则返回-1
```

满足下列四个条件中的任何一个时，一个套接字准备好读：

1. 该套接字接收缓冲区中的数据字节数大于等于套接字接收缓冲区低水位标记的当前大小，对这样的套接字执行读操作不会阻塞并将返回一个大于0的值。我们可以使用SO_RCVLOWAT套接字选项设置该套接字的低水位标记，对于TCP和UDP套接字来说其默认值为1；
2. 该连接的读半部关闭（也就是接收了FIN的TCP连接）,对这样的套接字的读操作将不阻塞并返回0；
3. 该套接字是一个监听套接字且已完成的连接数不为0；
4. 其上有一个套接字错误待处理，这样的套接字读操作不阻塞返回-1，这些待处理错误也可以通过指定SO_ERROR套接字选项调用getsockopt获取并清除；

满足下列四个条件之一时一个套接字准备好写：

1. 该套接字发送缓冲区中的可用空间字节数大于等于套接字发送缓冲区低水位标记的当前大小，并且或者该套接字已连接，或者该套接字不需要连接（比如UDP套接字），这意味着如果把这种套接字设置非阻塞，写操作将不阻塞并返回一个正值。可以用SO_SNDLOWAT套接字选项设置低水位标记。
2. 该连接的写半部关闭，对这样的套接字的写操作将产生SIGPIPE信号；
3. 使用非阻塞connect的套接字已建立连接或者connect已经以失败告终；
4. 其上有一个套接字错误待处理；

如果一个套接字存在带外数据或者仍处于带外标记，那么有异常条件待处理。

## shutdown函数

终止网络连接的通常方法是使用close函数，不过close有两个限制，可以使用shutdown来避免：

1. close把描述符的引用计数减一，但仅在该计数为0时才关闭套接字，使用shutdown可以不管引用计数就激发TCP的正常连接终止序列；
2. close终止读和写两个方向的数据传送，但有时候我们需要告诉对方我们已经完成了数据发送，即使对方仍有数据要发送给我们；

```c
#include <sys/socket.h>
int shutdown(int sockfd, int howto); //成功返回0，出错返回-1
```

对于howto参数，SHUT_TD，会关闭连接的读这一半，套接字不再有数据接收，而且套接字接收缓冲区的现有数据都被丢弃。进程不能再对这样的套接字调用任何读函数；SHUT_WR，关闭连接的写这一半，对于TCP，这称为半关闭，当前留在套接字发送缓冲区的数据将被发送，后跟TCP的正常连接终止序列。SHUT_RDWR，连接的读半部和写半部都关闭。

## pselect函数

由posix发明，现在有许多unix变种支持：

```c
#include <sys/select.h>
#include <signal.h>
#include <time.h>

int pselect(int maxfdpl, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timespec *timeout, const sigset_t *sigmask);
```

pselect使用timespec而不是timeval，其指定纳秒数，它还增加了sigmask参数，一个指向信号掩码的指针，该参数允许先禁止递交某些信号，再测试由这些被禁止信号的处理函数设置的全局变量，然后调用pselect，告诉它重新设置信号掩码。

## poll函数

```c
#include <poll.h>
int poll(struct pollfd *fdarray, unsigned long nfds, int timeout); //若有就绪描述符则返回其数目，超时返回0，出错返回-1
```

指定events标志和revents标志的一些常值如下：

![](..\image\Unix\1-6-23.png)

如果我们不再关心某个描述符，可以将其对应的pollfd结构的fd成员设置为一个负值，poll函数将忽略这样的events成员，返回时将它的revents成员的值置为0。

对select来说，设计之初操作系统通常为每个进程可用的最大描述符数设置了上限（4.2BSD的限制为31）,而事实上现如今的Unix允许每个进程使用无限数目的描述符，poll就没有这个限制，因为分配一个pollfd结构数组并把该数组的元素的数目通知内核成了调用者的责任，内核不再需要知道类似fd_set的固定大小的数据类型。

# 套接字选项

## getsockopt和setsockopt函数

这两个函数仅用于套接字

```c++
#include <sys/socket.h>
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
```

## 通用套接字选项

**SO_BROADCAST**:开启或禁止进程发送广播消息的能力，只有数据包套接字支持广播，且还必须是在支持广播消息的网络上（例如以太网、令牌环网等）；

**SO_DEBUG**：仅由TCP支持，开启时内核将为TCP在该套接字发送和接收的所有分组保留详细跟踪信息，这些信息保存在内核的某个环形缓冲区，并可以用trpt程序检查；

**SO_DONTROUTE**：规定外出的分组将绕过底层协议的正常路由机制；

**SO_ERROR**：进程可以通过访问此套接字选项获取so_error的值，之后so_error由内核复位为0；

**SO_KEEPALIVE**：设置后如果2小时内该套接字的任一方向没有数据交换，TCP就自动给对端发送一个保持存活探测分节，这是一个对端必须响应的TCP分节，它将导致以下三种情况之一：

1. 对端以期望的ACK响应，应用进程得不到通知，在又经过无动静的2小时之后，TCP将发出另一个探测分节；
2. 对端以RST响应，它告知本端TCP：对端已崩溃且已重新启动，该套接字的待处理错误被置为ECONNRESET，套接字本身被关闭；
3. 对端没有任何响应，源自Berkeley的TCP将另外发送8个探测分节，两两相隔75s，试图得到一个响应，TCP在发出第一个探测分节后11分15秒内没有得到任何响应则放弃。

如果根本没有对TCP的探测分节的响应，该套接字的待处理错误就被置为ETIMEOUT，套接字本身则被关闭，如果该套接字接收到一个ICMP错误作为某个探测分节的响应，那就返回相应的错误，套接字本身也被关闭。

本选项一般由服务器使用，为了处理客户主机崩溃等情况，这种情况下服务器进程永远不会知道，并继续等待不会到达的输入，这被称为半开连接，保持存活选项将检测出这些半开连接并终止它们。

**SO_LINGER**：指定close对面向连接的协议如何操作。默认是close立即返回，如果有数据残留在套接字发送缓冲区，系统将试着将这些数据发送给对方。

![](..\image\Unix\1-7-6.png)

设置SO_LINGER套接字选项后，close的成功返回只是代表先前发送的数据（和FIN）以由对端TCP确认，而不能告诉我们对端应用进程是否已经读取数据。让客户知道服务器已读取数据的一个方法是改用shutdown（并设置它的第二个参数SHUT_WR），而不是调用close，并等待对端close连接的当地段（服务器端），如图所示：

![](..\image\Unix\1-7-10.png)

下图汇总了shutdown的两种可能调用和对close的三种可能调用：

![](..\image\Unix\1-7-12.png)

**SO_OOBINLINE**：开启时带外数据将被留在正常的输入队列中。

**SO_RCVBUF和SO_SNDBUF**：TCP套接字接收缓冲区不可能溢出，因为不允许对端发出超过本端所通告窗口大小的数据，这就是TCP的流量控制，如果对端无视窗口大小而发出了超过本端所通告窗口大小的数据，本端TCP将丢弃它们。这两个选项可以改变两个缓冲区的默认大小。注意，由于TCP的窗口规模选项是在建立连接时用SYN分节与对端互换得到，对于客户，这意味着选项必须在调用connect之前设置，对于服务器，必须在调用listen之前设置。TCP套接字缓冲区的大小至少应该是相应连接的MSS的值的四倍。

**SO_RCVLOWAT和SO_SNDLOWAT**：接收低水位标记是让select返回可读时套接字接收缓冲区中所需的数据量，对于TCP、UDP、SCTP套接字其默认值为1，发送低水位标记是让select返回可写时套接字发送缓冲区中所需的可用空间，对TCP其默认值为2048。

**SO_RCVTIMEO和SO_SNDTIMEO**：给套接字的接收和发送设置一个超时值。

**SO_REUSEADDR和SO_REUSEPORT**：SO_REUSEADDR应该在bind之前调用，其允许启动一个监听服务器并绑定其众所周知的端口，即使以前建立的将端口用作它们的本地端口的连接仍存在，这个条件通常是这样碰到：

1. 启动一个监听服务器；
2. 连接请求到达，派生一个子进程来处理这个客户；
3. 监听服务器停止，但子进程继续为现有连接服务；
4. 重启监听服务器；

其也允许在同一端口启动同一服务器的多个实例，只要每个实例绑定一个不同的本地ip地址即可。

其允许单个进程捆绑同一端口到多个套接字，只要每次捆绑指定不同的本地ip地址即可。

其允许完全重复的捆绑：当一个IP地址和端口已绑定到某个套接字上时，如果传输协议支持，同样的ip地址和端口还可以绑定到另一个套接字上。

**SO_TYPE**：返回套接字的类型

**SO_USELOOPBACK**：仅用于路由域的套接字，开启时相应套接字将接收在其上发送的任何数据包的一个副本。

## TCP套接字选项

**TCP_MAXSEG**：设置TCP连接的最大分节大小（MSS）。返回值是我们的TCP可以发送给对端的最大数据量，它通常是由对端使用SYN分节通告的MSS，除非我们的TCP选择使用一个比对端通告的MSS小些的值。

**TCP_NODELAY**：禁止TCP的Nagle算法。

## fcntl函数

fcntl可执行各种描述符控制操作，小结如下：

![](..\image\Unix\1-7-20.png)

非阻塞式I/O，可将一个套接字设置为非阻塞型，信号驱动式I/O，可以把一个套接字设置成一旦其状态发生变化，内核就产生一个SIGIO信号。

# 基本UDP套接字编程

## 概述

使用UDP编写的一些常见的应用程序有：DNS（域名系统）、NFS（网络文件系统）和SNMP（简单网络管理协议）。下图给出了典型的UPD的函数调用，客户不与服务器建立连接，而是只管使用sendto函数给服务器发送数据报，其中必须指定目的地的地址作为参数。

![](..\image\Unix\1-8-1.png)

## recvfrom和sendto函数

```c
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *buff, size_t nbytes, int flags,
				struct sockaddr *from, socklen_t *addrlen);
ssize_t sendto(int sockfd, const void *buff, size_t nbytes, int flags,
				const struct sockaddr *to, socklen_t *addrlen);	
                //成功返回字节数，出错返回-1
```

sendto的to参数指向一个含有数据包接收者的协议地址（例如ip地址和端口号）的套接字地址，其大小由addrlen指定，recvfrom同理。

## 数据报的丢失

UDP客户/服务器例子是不可靠的，如果一个客户数据丢失，客户将永远阻塞在recvfrom调用上，设置超时是一个解决方法。

## 验证接收到的响应

使用recvfrom得到的ip跟发送ip一致来验证是发往服务器的应答是不充分的，如果服务器主机是多宿的该客户就可能失败，一个解决方法是得到recvfrom的ip之后，客户通过在DNS中查找服务器主机的名字来验证该主机的域名，另一个方法是UDP服务器给主机上配置的每个ip地址创建一个套接字，用bind捆绑每个ip到各自的套接字，然后再所有的这些套接字上使用select，再从可读的套接字给出应答。

## 服务器进程未运行

对于一个UDP套接字，由它引发的异步错误并不返回给它，除非它已连接。

## UDP的connect函数

可以给UDP套接字调用connect，但没有三路握手过程。内核只是检查是否存在立即可知的错误，记录对端的IP地址和端口号，然后立即返回到调用进程。

对于已连接的UDP套接字，发生的变化如下：

1. 不能给输出操作指定目的IP和端口，即不使用sendto，而使用write或者read；
2. 不必使用recvfrom获知数据包的发送者，而改用read、recv或recvmsg；
3. 对已连接UDP套接字引发的异步错误会返回给它们所在的进程，而未连接的UDP套接字不接受任何异步错误；

给一个已连接UDP套接字的进程可出于下列两个目的之一再次调用connect：

- 指定新的ip地址和端口号；
- 断开套接字；

第一个目的不同于TCP套接字中的connect的使用，因为对于TCP套接字，connect只能调用一次。为了断开一个已UDP套接字连接，我们再次调用connect时把套接字地址结构的地址族成员设置为AF_UNSPEC，这么做可能会返回一个EAFNOSUPPORT错误，不过没有关系，使套接字断开连接的是在已连接UDP套接字上调用connect的进程。

当应用进程在一个未连接的UDP套接字上调用sendto时，给两个数据报调用sendto时涉及内核执行以下六个步骤：

- 连接套接字；
- 输出第一个数据报；
- 断开套接字连接；
- 连接套接字；
- 输出第二个数据报；
- 断开套接字连接；

而如果已知要给同一个地址发报，调用connect之后：

- 连接套接字；
- 输出第一个数据报；
- 输出第二个数据报；

UDP套接字没有流量控制。

# 基本SCTP套接字编程

## 接口模型

开发SCTP的一到一形式是为了方便将现有TCP应用程序移植到SCTP上，以下是两者必须搞清楚的差异：

1. 任何TCP套接字必须转换成等效的SCTP套接字选项，两个常见的选项是TCP_NODELAY和TCP_MAXSEG，它们必须映射成SCTP_NODELAY和SCTP_MAXSEG；
2. SCTP保存消息边界，因而应用层消息边界并非必需；
3. 有些TCP应用进程使用半关闭来告知对端去往它的数据流已经结束，将这样的应用程序移植到SCTP需要额外重写应用层协议，让应用程序在应用数据流中告诉对端该传输数据流已经结束；
4. send函数能够以普通方式使用；

![](..\image\Unix\1-9-1.png)

在一到多形式套接字上，用以标识单个关联的是一个关联标识，关联标识是一个类型为sctp_assoc_t的值，通常是一个整数。用户应该掌握以下几点：

1. 当一个客户关闭一个关联时，其服务器也将自动关闭同一个关联，服务器内核中不再有该关联的状态；
2. 可用于致使在四路握手的第三个或第四个分组中捎带用户数据的唯一办法就是使用一到多形式；
3. 对于任何一个与它还没有关联存在的IP地址，任何以它为目的地的sendto、sendmsg或sctp_sendmsg将导致对主动打开的尝试，从而建立一个与该地址的新关联；
4. 用户必须使用sendto、sendmsg或sctp_sendmsg这三个分组发送函数，而不能使用send或write这两个分组发送函数，除非已经使用sctp_peeloff函数从一个一到多套接字剥离出一个一到一式套接字；
5. 任何时候调用其中任何一个分组发送函数时，所用的目的地址是由系统在关联建立阶段选定的主目的地址，除非调用者在所提供的sctp_sendrcvinfo中设置了MSG_ADDR_OVER标志，为了提供这个结构，调用者必须使用伴随辅助数据的sendmsg函数或sctp_sendmsg函数；
6. 关联事件可能被启用，如果应用进程不希望收到这些事件，可以用SCTP_EVENTS套接字选项显式禁止它们。默认情况下唯一启用的事件是sctp_data_io_event，它给recvmsg和sctp_recvmsg调用提供辅助数据，这个默认设置同时适用于一到一和一到多形式。

![](..\image\Unix\1-9-2.png)

## sctp_bindx函数

sctp_bindx函数允许SCTP套接字捆绑一个特定地址子集：

```c
#include <netinet/sctp.h>
int sctp_bindx(int sockfd, const struct sockaddr *addr, int addrcnt, int flags);
#成功返回0出错返回-1c
```

此函数既可以用于已绑定的套接字，也可以用于未绑定的套接字。对于已绑定的套接字，若指定SCTP_BINDX_ADD_ADDR则把额外的地址加入到套接字描述符，若指定SCTP_BINDX_REM_ADDR则从套接字描述符的已加入地址中移除给定地址。如果在一个监听套接字上执行sctp_bindx，那么将来产生的关联将使用新的地址配置，已经存在的关联不受影响。所有套接字地址结构的端口号必须相同，而且必须与已经绑定的端口号匹配，否则调用就会失败，返回EINVAL错误码。

## sctp_connectx函数

```c
#include <netinet/sctp.h>
int sctp_connectx(int sockfd, const struct sockaddr* addr, int addrcnt);
#成功返回0，出错为-1
```

用于连接到一个多宿对端主机，该函数在addrs参数中指定addrcnt个全部属于同一对端的地址。

## sctp_getpaddrs函数

getpeername不是为支持多宿概念的传输协议设计的，当用于SCTP时它仅仅返回主目的地址，如果需要知道对端所有地址，应该使用sctp_getaddrs函数：

```c
#include <netinet/sctp.h>
int sctp_getpaddrs(int sockfd, sctp_assoc_t id, struct sockaddr* addr); #成功返回存放在addrs中的对端地址数，否则为-1
```

## sctp_freeaddrs函数

用来释放由sctp_getpaddrs函数分配的资源：

```c
#include <netinet/sctp.h>
void sctp_freepaddrs(struct sockaddr* addr); 
```

## sctp_sendmsg函数

```c
#include <netinet/sctp.h>
void sctp_sendmsg(int sockfd, const void *msg, size_t mgssz, const struct sockaddr *to, socklen_t tolen, uint32_t ppid, uint32_t flags, uint16_t stream, uint32_t timetolive, uint32_t context); #成功返回所写字节数，出错则为-1 
```

## sctp_recvmsg函数

```c
#include <netinet/sctp.h>
void sctp_recvmsg(int sockfd, const void *msg, size_t mgssz, const struct sockaddr *from, socklen_t *fromlen, struct sctp_sndrcvinfo *sinfo, int *msg_flags); #成功返回所写字节数，出错则为-1 
```

## sctp_opt_info函数

此函数是为无法为SCTP使用getsockopt函数的那些实现提供的。

```c
#include <netinet/sctp.h>
int sctp_opt_info(int sockfd, sctp_assoc_t assoc_id, int opt, void *arg, socklen_t *siz); #成功返回0否则为-1
```

## sctp_peeloff函数

```c
#include <netinet/sctp.h>
int sctp_peeloff(int sockfd, sctp_assoc_t id); #成功返回0否则为-1
```

## shutdown函数

当相互通信的两个SCTP端点中任何一个发起关联终止序列时，这两个端点都得把已排对的任何数据发送掉，然后关闭关联。与TCP不同，新的套接字打开之前不必调用close，SCTP允许一个端点调用shutdown，shutdown结束之后这个端点就可以重用原套接字。注意，如果这个端点没有等到SCTP关联终止序列结束，新的连接就会失败。

下图标出用户接收MSG_NOTIFICATION事件，如果用户未曾预订接收这些事件，那么返回的是结果长度为0的read调用，对于SCTP，shutdown函数的howto参数语义如下：

SHUT_RD：与TCP的语义等同；

SHUT_WR：禁止后续发送操作，激活SCTP关联终止进程，以此终止当前关联，注意，本操作不提供半关闭状态，不过允许本地端点读取已经排队的数据，这些数据是对端在收到SCTP的SHUTDOWN消息之前发送的；

SHUT_RDWR：禁止所有的read操作和write操作，激活SCTP关联终止进程，传送到本地端点的任何已经排队的数据都得到确认，然后丢弃。

![](..\image\Unix\1-9-5.png)

## 通知

使用SCTP_ENVENTS套接字选项可以预定8个事件，其中7个事件产生称为通知的额外数据，通知本身可由普通的套接字描述符获取。为了区分来自对端的数据和由事件产生的通知，用户应该使用recvmsg函数和sctp_recvmsg函数，如果返回的数据是一个事件通知，那么返回的msg_flags参数将含有NSG_NOTIFICATION标志。

# 名字与地址转换

## 域名系统

域名系统（Domain Name System, DNS）主要用于主机名字和ip地址之间的映射。主机名可以是一个简单名字，例如solaris或bsdi，也可以是一个全限定域名，例如solaris.unpbook.com。

DNS中的条目称为资源记录（RR），我们感兴趣的RR类型有若干个：

- A：A记录把一个主机名映射成一个32位的IPV4地址；
- AAAA：把一个主机名映射成一个128位的IPv6地址；
- PTR：称为指针记录，把IP地址映射成主机名。对于IPv4地址，32位地址的4个字节先反转顺序，每个字节转换成各自的十进制ASCII值（0-255）后，再添上in-addr.arpa，结果字符串用于PTR查询，对于IPv6地址，128位地址中的32个四位组先反转顺序，每个四位组都被转换成相应的十六进制ASCII值（0-9，a-f）后，再添上ip6.arpa。
- MX：MX记录把一个主机指定作为给定主机的邮件交换器。其按照优先级顺序使用，值越小优先级越高；
- CNAME：代表"canonical name"（规范名字），常见用法是为常用服务指派CNAM记录。

以下是应用进程、解析器和名字服务器之间的一个典型关系：

![](..\image\Unix\1-11-1.png)

解析器代码通过读取其系统相关配置文件确定本组织的名字服务器们的位置，文件/etc/resolv.conf通常包含本地名字服务器主机的ip地址。解析器使用UDP向本地名字服务器发出查询，如果本地名字服务器不知道答案，会使用UDP在整个因特网查询其他名字服务器，如果答案太长超出了UDP消息的承载能力，本地名字服务器和解析器会自动切换到TCP。

不适用DNS也可能获取名字和地址信息，常用的替代方法有静态主机文件（通常是/etc/hosts文件）、网络信息系统（NIS）以及轻权目录访问协议（LDAP）。

## gethostbyname函数

查找主机名最基本的函数是gethostbyname，如果调用成功，返回一个指向hostent结构的指针，该结构含有所查找主机的所有IPv4地址。这个函数的局限是只能返回ipv4地址。

```c
#include <netdb.h>
struct hostent *gethostbyname(const char *hostname); //成功返回非空指针，否则返回NULL且设置h_errno
```

hostent结构如下：

```c
struct hostent {
	char *h_name; //officical name of host
	char **h_aliases; // pointer to array of pointers to alias names
	int h_addrtype; //host address type: AF_INET
	int h_length; //length of address: 4
	char **h_addr_list; // ptr to array of ptrs with ipv4 addrs
};
```

gethostbyname与其他套接字函数的不同在于当发生错误时，它不设置errno变量，而是将全局整数变量h_errno设置为在头文件netdb.h中定义的以下常值之一：

- HOST_NOT_FOUND；
- TRY_AGAIN；
- NO_RECOVERY；
- NO_DATA；

NO_DATA表示指定的名字有效，但它没有A记录。

## gethostbyaddr函数

此函数试图由一个二进制的IP地址找到主机名。

```c
#include <netdb.h>
struct hostent *gethostbyaddr(const char *addr, socklen_t len, int family); //成功返回非空指针，否则返回NULL且设置h_errno
```

addr参数实际上是一个指向存放IPV4地址的某个in_addr结构的指针，len参数是这个结构的大小：对于IPv4地址为4，family参数为AF_INET。按照DNS的说法，gethostbyaddr在in_addr.arpa域中向一个名字服务器查询PTR记录。

## getservbyname和getservbyport函数

如果在代码中通过其名字而不是其端口号来指代一个服务，而且从名字到端口号的映射关系保存在一个文件中（通常是/etc/service），那么即使端口号发生变动，那么需修改的仅仅是/etc/services文件中的一行，而不必重新编译程序。getservbyname用于根据跟定名字查找相应服务。

```c
#include <netdb.h>
struct servent *getservbyname(const char *servname, const char *protoname);
//成功返回非空指针，出错返回NULL
```

其中servent结构如下：

```c
struct servent {
	char *s_name; //official service name
	char **s_aliases; // alias list
	int s_port; // port number, network byte order
	char *s_proto; //protocol to use
};
```

getservbyport用于根据给定端口号和可选协议查找相应服务。

```c
#include <netdb.h>
struct servent *getservbyport(int port, const char *protoname);
//成功返回非空指针，出错返回NULL
```

port参数的值必须为网络字节序。

## getaddrinfo函数

gethostbyname和gethostbyaddr仅支持ipv4，而getaddrinfo可以避免这个问题，其能够处理名字到地址以及服务到端口这两种转换，返回的是一个sockaddr结构而不是一个地址列表，这些sockaddr结构随后可由套接字函数直接使用。

```c
#include <netdb.h>
int getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result); //成功则返回0，否则为非0
```

## gai_strerror函数

此函数以getaddrinfo返回的非0错误值为参数，返回一个指向对应出错信息串的指针。

```c
#include <netdb.h>
const char *gai_strerror(int error);
```

## freeaddrinfo函数

由getaddrinfo返回的所有存储空间都是动态获取的，包括addrinfo结构、ai_addr结构和ai_canonname字符串，这些存储空间通过freeaddrinfo返还给系统。

```c
#include <netdb.h>
void freeaddrinfo(struct addrinfo *ai);
```

ai参数指向由getaddrinfo返回的第一个addrinfo结果。

## getnameinfo函数

是getaddrinfo的互补函数，以一个套接字地址为参数，返回描述其中主机的一个字符串和描述其中的服务的另一个字符串。本函数以协议无关的方式提供这些信息。

```c
#include <netdb.h>
int getnameinfo(const struct sockaddr *sockaddr, socklen_t addrlen,
				char *host, socklen_t hostlen,
				char *serv, socklen_t servlen, int flags);
```

# IPv4与IPv6的互操作性

![](..\image\Unix\1-12-2.png)

![](..\image\Unix\1-12-3.png)

![](..\image\Unix\1-12-4.png)

# 守护进程和inetd超级服务器

## 概述

守护进程是在后台运行且不与任何控制终端关联的进程。它们通常由系统初始化脚本启动，也可能由用户键入命令启动：

1. 系统启动阶段，许多守护进程由系统初始化脚本启动，这些脚本通常位于/etc目录或者以/etc/rc开头的某个目录中，由这些脚本启动的守护进程一开始就拥有超级用户特权；
2. 许多网络服务器由inetd超级服务器启动，它本身由某个脚本启动。inetd监听网络请求，每当有一个请求到达时，启动相应服务器（Telnet服务器、FTP服务器等）
3. cron守护进程按照规则定期执行一些程序，其启动执行的程序同样作为守护进程运行。cron自身由第一条启动方法的某个脚本启动。
4. at命令用于指定将来某个时刻的程序执行，这些程序的执行时刻到来时通常由cron守护进程启动执行。
5. 守护进程还可以从用户终端或在前台或后台启动。

## syslogd守护进程

由某个系统初始化脚本启动，而且在系统工作期间一直执行，其启动时执行以下步骤：

1. 读取配置文件，通常为/etc/syslog.conf的配置文件指定本守护进程可能收取的各种日志怎么处理；
2. 创建一个Unix域数据报套接字，给它捆绑路径名/var/run/log（某些系统是/dev/log）
3. 创建一个UPD套接字，绑定端口514；
4. 打开路径名/dev/klog；

syslogd守护进程在一个无限循环中运行：调用select等待它的3个描述符之一变成可读。

## syslog函数

从守护进程中登记消息的常用技巧是syslog函数。

```c
#include <syslog.h>
void syslog(int priority, const char *message, ...);
```

priotity是级别和设施的结合。

# 高级I/O函数

## 套接字超时

在涉及套接字的I/O操作上设置超时有以下三种方法：

1. 调用alarm，在指定超时期满时产生SIGALRM信号；
2. 在select上阻塞等待I/O；
3. 使用较新的SO_RCVTIMEO和SO_SNDTIMEO套接字选项；

