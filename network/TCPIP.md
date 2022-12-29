# TCP/IP网络编程

## 理解网络编程和套接字

网络编程就是编程使两台联网的计算机能够进行数据交换。操作系统提供了套接字的部件，通过套接字可以完成数据传输，因此网络编程又称为套接字编程。

服务端常用API：

```c++
#include <sys/socket.h>
//成功返回文件描述符，失败返回-1
int socket(int domain, int type, int protocol);
//成功时返回0，失败返回-1
int bind(int sockfd, struct sockaddr *myaddr, socklen_t addrlen);
//成功时返回0，失败时返回-1
int listen(int sockfd, int backlog);
//成功时返回文件描述符，失败时返回-1；
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen;)
```

这是网络编程中接受连接请求的套接字的创建过程：

1. 调用socket函数创建套接字；
2. 调用bind函数分配IP地址和端口号；
3. 调用listen函数转为可接收请求状态；
4. 调用accept函数接受连接请求；

客户端常用API：

```c++
#include <sys/socket.h>
//成功返回0，失败时返回-1
int connect(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen);
```

客户端使用也需要两步：

- 创建套接字
- 调用connect向服务器发送连接请求

## 套接字类型与协议设置

创建网络套接字的接口如下：

```c++
int socket(int domain, int type, int protocol);
```

其中domain是协议族，type为套接字类型，protocol是协议信息。其中type分为两种类型，一种是面向连接的套接字，其是可靠的、按序传递的、基于字节的面向连接的数据传输方式的套接字，另一种是面向消息的套接字，其是不可靠的、无序的、以数据的高速传输为目的的套接字。

## 地址族与数据序列

IPv4的4字节IP地址分为网络地址和主机地址，且分为A、B、C、D、E等类型。网络地址是为了区分网络而设置的一部分IP地址，当传输数据时，一般先传输到网络地址的路由，之后路由再分发到主机。只需要IP地址的第一个字节即可判断网络地址占用的字节数：A类地址的首字节范围为0-127；B类地址的首字节范围为128-191；C类地址的首字节范围为192-223。还有如下的表述方式：A类地址的首位以0开始，B类地址的前两位以10开始，C类地址的前三位以110开始。

网络数据是不同的，有视频数据，有网页数据，那么将这些数据分配给合适的套接字就用到了端口号。端口号就是在同一操作系统中区分不同套接字而设置的。端口号由16位构成，范围为0-65535。

知道了ip地址和端口号，我们就可以传输网络数据，因此要将这些信息传递给网络套接字，正如前文所述，利用结构体传递给了bind函数：

```c++
struct sockaddr_in
{
	sa_family_t sin_family;
	uint16_t    sin_port;
	struct in_addr sin_addr;
	char        sin_zero[8];
};
```

其中in_addr是用来存放32位IP地址的结构体。

CPU向内存保存数据的方式有两种，这意味着CPU解析数据的方式也有两种：

- 大端序：高位字节存放到低位地址；
- 小端序：高位字节存放到高位地址；

为了防止网络两端解析数据的方式不同，统一了约定，这种约定称为网络字节序，统一为大端序。有以下帮助转换字节序的函数：

- unsigned short htons(unsigned short);
- unsigned short ntohs(unsigned short);
- unsigned long htonl(unsigned long);
- unsigned long ntohl(unsigned long);

其中h代表主机字节序，n代表网络字节序，另外，s代表short，l代表long。通常，以s作为后缀的函数中，s代表两个字节的short，因此用于端口号转换；以l作为后缀的函数中，l代表4个字节，因此用于IP地址转换。

以下函数可将字符串形式的IP地址转换为32整型数据，并同时进行网络字节序的转换：

```c++
#include <arpa/inet.h>
in_addr_t inet_addr(const char * string);
```

另外，inet_aton函数与inet_addr函数在功能上相同，但其利用了in_addr结构体，因此使用频率更高，函数原型如下：

```c++
int inet_aton(const char *string, struct in_addr *addr);
```

将网络字节序整数型ip地址转换为字符串可用以下函数：

```c++
char* inet_ntoa(struct in_addr adr);
```

结合以上内容，有以下常见的初始化方法：

```c++
struct sockaddr_in addr;
char *serv_ip = "211.217.168.13";
char *serv_port = "9190";
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = inet_addr(serv_ip);
addr.sin_port = htons(atoi(serv_port));
```

每次输入IP地址有些麻烦，可使用常数INADDR_ANY来分配服务器端的IP地址，可自动获取服务器端的计算机IP地址。

## 基于TCP的服务器端/客户端

其TCP/IP协议栈共分4层，最上层为应用层，之后是TCP层与UDP层，再之后是IP层，最后是链路层。链路层是物理链接领域标准化的结果，可以理解成按照标准将各个实体进行了链接;IP层解决的是选择哪条路径向目标传递消息，IP本身是面向消息的、不可靠的协议，每次传递消息都会选择路径，但路径并不一致，如果路径错误会选择其他路径，但如果发生数据丢失或者错误，则无法解决；TCP和UDP层以IP层提供的路径为基础完成实际的数据传输，故又称为传输层，IP层只关注一个数据包的传输，因此无法保证数据包的顺序，这就交给了传输层来解决；根据程序特点确定服务端和客户端之间的数据传输规则就是应用层协议，网络编程的大部分就是设计并实现应用层协议。

### 实现基于TCP的服务端/客户端

TCP服务器的调用顺序如下：

![tcp_server](..\image\Network\tcp_server.png)

注意，只有调用了listen函数，客户端才能进入可发出连接请求的状态，即此时才能调用connect函数。

TCP客户端的调用顺序如下：

![tcp_client](..\image\Network\tcp_client.png)

客户端调用connect之后，发生以下情况之一才会返回：

- 服务器端接收连接请求；
- 发生断网等异常情况而中断连接请求；

需要注意的是，接收连接请求不意味着服务端调用accept，其实是服务端将连接请求信息记录到等待队列。

整个过程合起来可以如下表示：

![tcp](..\image\Network\tcp.png)

## 基于TCP的服务器端/客户端(2)

看下面客户端的一段代码：

```c++
write(sock, message, strlen(message));
str_len = read(sock, message, BUF_SIZE-1);
```

这样会存在一个问题，如果发送太多的消息，直接调用read可能会接收不全。解决方法很简单，代码如下：

```c++
str_len = write(sock, message, strlen(message));
recv_len = 0;
while(recv_len < str_len) {
	recv_cnt = read(sock, &message[recv_len], BUF_SIZE-1);
	if (recv_cnt == -1) error_handling("read() error!");
	recv_len += recv_cnt;
}
message[recv_len] = 0;
```

### TCP原理

有这样一个疑问，服务端通过一次write传输40个字节的数据，但客户端有可能调用四次read，每次读取10个字节，那么读取10个字节后剩下的30个字节存储在哪里？实际上，write与read调用后并非马上传输和接收数据，更准确的说，write函数调用瞬间，数据将移至输出缓冲，read函数调用瞬间从输入缓冲读取数据。调用write的时候，数据将移到输出缓冲，在适当的时候传向对方的输入缓冲，这时对方将调用read从输入缓冲读取数据，这些I/O缓冲特性可整理如下：

- I/O缓冲在每个TCP套接字中单独存在；
- I/O缓冲在创建套接字时自动生成；
- 即使关闭套接字也会继续传递输出缓冲中遗留的数据；
- 关闭套接字将丢失输入缓冲中的数据；

TCP在实际通信中会有3次对话过程，如图所示：

![three_way_handshaking](..\image\Network\three_way_handshaking.png)

套接字以全双工方式通信，即可以双向通信，因此收发数据需要做一些准备，首先请求连接的主机A向B传递消息，其中SEQ为1000，含义为现传递的数据包序号为1000，如果接收无误，请通知我传递1001号数据包，ACK为空。接下来，B向A传递消息，SEQ为2000的含义为现传递的数据包序号为2000，如果接收无误，请通知我传递2001号数据包，ACK为1001的含义为刚才传输的1000数据包无误，请传递1001号数据包；最后A向B传递消息，与上述同理。

三步握手完成了数据交换准备，接下来就是正式收发数据：

![data_exchange](..\image\Network\data_exchange.png)

首先A发送了SEQ为1200的数据包，大小为100个字节，B为了确认发送ACK为1301的消息，之所以为1301是加上了传过来的数据量，防止数据丢失，最后加1是为了告诉对方下一次要传递的SEQ号。如果发生错误，如下所示：

![tcp_timeout](..\image\Network\tcp_timeout.png)

即A未收到B对SEQ1301的确认ACK，此时TCP套接字会启动计时器等待ACK应答，若超时则重传。

套接字断开的过程如下：

![tcp_out](..\image\Network\tcp_out.png)

这个过程称为四次握手，双发各发送一次FIN消息断开连接。

## 基于UDP的服务端/客户端

UDP虽然不可靠，但比TCP的性能更好。UDP最重要的作用就是根据端口号将传给主机的数据包交付给最终的UDP套接字。当传递一些信息丢失一些也没关系时可以使用UDP，比如视频或音频时，轻微的抖动关系不大。但UDP并非每次都快于TCP，TCP比UDP满的原因主要有以下两点：

- 收发数据前后进行的连接设置与清楚过程；
- 收发数据过程中为保证可靠性增加的流控制；

### 实现基于UDP的服务端客户端

UDP套接字不会保持连接状态，因此每次传递数据都要添加目标地址信息，相关api如下：

```c++
#include <sys/socket.h>
ssize_t sendto(int sock, void* buff, size_t nbytes, int flags, struct sockaddr *to, socklent_t addrlen);
ssize_t recvfrom(int sock, void* buff, size_t nbytes, int flags, struct sockaddr *from, socklent_t addrlen);
```

具体的示例如下：

```c++
//服务端
int serv_sock;
char message[BUF_SIZE];
int str_len;
socklen_t clnt_adr_sz;
struct sockaddr_in serv_adr,clnt_adr;
serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
memset(&serv_adr, 0, sizeof(serv_adr));
serv_adr.sin_family = AF_INET;
serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
serv_adr.sin_port = htons(atoi(argv[1]));

if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1) error_handling("bind() error");
while(1) {
	clnt_adr_sz = sizeof(clnt_adr);
	str_len=recvfrom(serv_sock, message, BUF_SIZE, 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
	sendto(serv_sock, message, BUF_SIZE, 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
}
close(serv_sock);
return 0;

//客户端
int sock;
char message[BUF_SIZE];
int str_len;
socklen_t adr_sz;
struct sockaddr_in serv_adr,from_adr;
sock = socket(PF_INET, SOCK_DGRAM, 0);
memset(&serv_adr, 0, sizeof(serv_adr));
serv_adr.sin_family = AF_INET;
serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
serv_adr.sin_port = htons(atoi(argv[2]));

while(1) {
	fputs("Insert message(q to quit): ", stdout);
	fgets(message, sizeof(message), stdin);
	sendto(sock, message, strlen(message), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	adr_sz = sizeof(from_adr);
	str_len=recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&from_adr, &adr_sz);
	message[str_len] = 0;
}
close(serv_sock);
return 0;
```

在UDP中，调用sendto前应完成对套接字的地址分配工作，因此调用bind函数，bind函数不区分TCP还是UDP，如果调用sendto时发现未分配的地址信息，则在首次调用sendto函数时给相应套接字自动分配IP和端口，此时分配的地址一直保留到程序结束。

### UDP的数据传输特性和调用connect函数

TCP的数据传输中不存在边界，而UDP则是具有数据边界的协议，传输中调用I/O函数的次数非常重要，因此输出函数的调用次数应与输入函数的调用次数完全一致，这样才能保证数据的完整传输。

每次调用sendto函数会向UDP套接字注册目标IP和端口号，因此可重复利用同一UDP套接字向不同目标传输数据，这种未注册目标地址信息的套接字称为未连接套接字。然而，当与同一主机进行长时间通信时，将这种UDP套接字变为已连接套接字无疑会提高效率。创建已连接套接字的过程也较为简单，调用connect函数即可：

```c++
sock = socket(PF_INET, SOCK_DGRAM, 0);
memset(&adr, 0, sizeof(adr));
adr.sin_familay = AF_INET;
...
connect(sock, (struct sockaddr*)&adr, sizeof(adr));
```

## 优雅的断开套接字连接

### 基于TCP的半关闭

linux的close函数和windows的closesocket意味着完全断开连接，在某些情况下是不太优雅的，比如主机a断开了连接，但其实还有主机b发送的数据需要接收。为了解决这种问题，只关闭一部分数据交换中使用的流的方法应运而生，即可以传输数据但无法接收或者可以接收但无法传输。顾名思义，就是只关闭流的一半。两台主机建立了套接字的连接，则有输入和输出两个流，用来关闭其中一个流就要用到以下的api：

```c++
#include <sys/socket.h>
int shutdown(int sock, int howto);
```

其中，howto就代表传递断开方式信息，其值可能如下：SHUT_RD，断开输入流；SHUT_WR，断开输出流；SHUT_RDWR，同时断开输入输出流。

## 域名及网络地址

域名是赋予服务器端的虚拟地址，因此需要将虚拟地址转换为实际地址，这个过程由DNS服务器执行。所有计算机中都记录着默认DNS服务器的地址，就是通过这个默认DNS服务器得到相应域名的ip地址。通过命令nslookup可以得到默认的DNS服务器地址。计算机内置的默认DNS服务器不能知道所有域名的ip地址，若是该服务器不能解析，其会询问其他DNS服务器，并提供给用户。

### IP地址和域名之间的转换

通过以下的api可以利用域名获取ip地址：

```c++
#include <netdb.h>
struct hostent* gethostbyname(const char* hostname);
```

 返回时信息装入hostent结构体，如下：

```c++
struct hostent
{
	char* h_name;      //official name
	char** h_aliases;  //alias list
	int h_addrtype;    //host address type
	int h_length;      //address length
	char** h_addr_list; //address list
}
```

其中，h_name存有官方域名，h_aliases存有绑定的其他的域名，h_addrtype代表地址族信息，AF_INET代表IPv4，h_length保存IP地址的长度，h_addr_list，以整数形式保存ip地址。注意，还需要进行类型转换，如下：

```c++
inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
```

同样，我们也可以通过ip地址获取域名：

```c++
#include<netdb.h>
struct hostent* gethostbyaddr(const char* addr, socklen_t len, int family);
```

其中addr为含有ip地址信息的in_addr结构体指针，len为地址信息的字节数，family是地址族信息，IPv4时是AF_INET，IPv6时为AF_INET6。有以下的示例：

```c++
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
addr.sin_addr.s_addr = inet_addr(argv[1]);
host = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);
```

## 套接字的多种可选项

以下是一部分可设置套接字的部分可选项：

![socket_options](..\image\Network\socket_options.png)

我们可以针对所有的可选项进行读取和设置，主要通过以下的api：

```c++
#include <sys/socket.h>
int getsockopt(int sock, int level, int optname, void* optval, socklen_t *optlen);
int setsockopt(int sock, int level, int optname, void* optval, socklen_t *optlen);
```

其中，sock是套接字文件描述符，level为要更改的可选项协议层，optname是要更改的可选项名，optval保存选项信息的缓存地址值，optlen是optval保存信息的字节数。

SO_RCVBUF是输入缓冲大小相关可选项，SO_SNDBUF是输出缓冲大小相关可选项，用这两个可选项既可以读取当前I/O缓冲大小，也可以更改。

### SO_REUSEADDR

以下是time-wait状态下的套接字：

![time_wait](..\image\Network\time_wait.png)

即服务器先终止程序，显然，服务器会有一段等待时间，之后才会close。注意，其实客户端也会有time-wait状态，但由于客户端每次都会动态分配端口号，因此无需关心。至于为什么会有这个状态，就是为了防止最后发向B的消息丢失，即使丢失，B会重发消息也会收到A的回复，B会正常终止。

然而time-wait有时候也会耽误事情，比如服务器紧急重启就会耽误时间，下图演示了四次握手不得不延长time-wait：

![time_wait2](..\image\Network\time_wait2.png)

这种由于消息丢失而引起的time-wait计时器重启显然是不合适的，解决方法就是在套接字的可选项中更改SO_REUSEADDR的状态。适当调整该参数，可以将time-wait状态下的套接字端口号重新分配给新的套接字：

```c++
optlen = sizeof(option)
option = TRUE;
setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);
```

### TCP_NODELAY

Nagle算法用于防止因数据包过多而发生网络过载，其应用于TCP层，其使用与否会导致如下的差异：

![nagle](..\image\Network\nagle.png)

可见，只有收到前一条数据的ACK消息时，nagle算法才发送下一条数据。TCP套接字默认使用nagle算法。但需要注意，nagle算法不是什么情况都适用，当网络流量未受太大影响时，不使用nagle算法比使用它时速度要快，比如传输大文件的时候（图中第二种情况是每隔一段时间发送数据可能会出现的，大文件输出到输出缓冲会很快，因此，即使不使用nagle算法也会很快装满输出缓冲）。禁用nagle算法也比较简单，只需要将套接字可选项TCP_NODELAY设为1即可，如下：

```c++
int opt_val = 1;
setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&opt_val, sizeof(opt_val));
```

## 多进程服务器

具有代表性的并发服务器端实现模型和方法有如下几种：

- 多进程服务器：通过创建多个进程提供服务；
- 多路复用服务器：通过捆绑并统一管理I/O对象提供服务；
- 多线程服务器：通过生成与客户端等量的线程提供服务；

进程是占用内存空间的正在运行的程序，所有进程都会从系统分配到id，称为进程id，是大于2的整数，1要分配给操作系统启动后的首个进程。创建进程的方法如下：

```c++
#include <unistd.h>
pid_t fork(void);
```

其会复制正在运行的调用fork函数的进程。要根据如下特点区分程序执行流程：

- 父进程：fork函数返回子进程ID；
- 子进程：fork函数返回0；

### 进程和僵尸进程

如果未正确销毁进程，其会变成僵尸进程。以下是调用fork函数产生子进程的终止方式：

- 传递参数并调用exit函数；
- main函数中执行return语句并返回值；

这两种方式都不会销毁子进程，直到这些值传递给产生该子进程的父进程，这之前的状态就是僵尸进程。只有父进程主动发起请求，操作系统才会传递该值，因此如果父进程未主动要求获得子进程的结束状态值，操作系统将一直保存，并让子进程长时间处于僵尸进程状态。因此，为了销毁子进程，父进程应主动请求获取子进程的返回值，共有两种方式。

第一种可以利用wait函数，如下：

```c++
#include<sys/wait.h>
pid_t wait(int* statloc);
```

该函数调用时如果已有子进程终止，会将终止时传递的返回值保存到statloc所指的内存空间，但statloc所指的内存空间还有其他信息，因此需要通过宏来分离：WIFEXITED，子进程正常终止时返回真；WEXITSTATUS，返回子进程的返回值，使用如下：

```c++
if (WIFEXITED(status))
{
	puts("Normal termination");
	printf("Child pass num: %d", WEXITSTATUS(status));
}
```

注意，调用wait函数时如果没有已终止的子进程，那么程序将阻塞，直到有子进程终止，因此使用需谨慎。

第二种方式可以使用waitpid函数，可以防止阻塞：

```c++
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int* statloc, int options);
```

其中，statloc与wait中参数有着相同的意义。

### 信号处理

不能调用waitpid函数以等待子进程终止，因为父进程也很繁忙，可以使用信号，依赖注册信号完成。当进程发现自己的子进程终止时，可以请求操作系统调用特定函数，该请求通过以下函数调用实现：

```c++
#include<signal.h>
void (*signal(int signo, void(*func)(int)))(int);
```

函数名为signal，参数为int signo与void(*func)(int)，返回值是参数为int，返回void的函数指针。调用该函数时，第一个参数是特殊情况信息，第二个参数是特殊情况下将要调用的函数，以下是部分特殊情况：

- SIGALRM：已到通过调用alarm函数注册的时间；
- SIGINT：输入CTRL+C；
- SIGCHLD：子进程终止；

如果子进程终止则调用mychild函数，则代码如下：

```c++
signal(SIGCHLD, mychild);
```

以下介绍alarm函数：

```c++
#include<unistd.h>
unsigned int alarm(unsigned int seconds);
```

返回0或以秒为单位的距SIGALRM信号发生所剩时间。此函数作用为指定时间后发送SIGALRM信号，如果向该函数传递0，则之前对SIGALRM信号的预约取消。如果预约信号后未指定该信号的处理函数，则（通过调用signal函数）终止进程，不做任何处理。

sigaction函数类似signal函数，而且可以完全取代，且更稳定，其在不同的操作系统中完全相同：

```c++
#include<signal.h>
int sigaction(int signo, const struct sigaction* act, struct sigaction* oldact);

struct sigaction
{
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
}

//使用
struct sigaction act;
act.sa_handler = timeout;
sigemptyset(&act.sa_mask);
act.sa_flags = 0;
sigaction(SIGALRM, &act, 0);
```

其中sa_handler保存信号处理函数的指针值。

### 分割TCP的I/O程序

即客户端的父进程负责接收数据，额外创建的子进程负责发送数据，分割后，不同进程分别负责输入和输出，这样，无论客户端是否接收完数据都可以进行传输。分割I/O程序还有两一个好处，那就是可以提高频繁交换数据的程序性能。示例如下：

```c++
pid = fork();
if (pid == 0) write_routine(sock buf);
else read_routine(sock, buf);

void read_routine(int sock, char* buf) {
	while(1) {
		int str_len = read(sock, buf, BUF_SIZE);
		if (str_len == 0) return;
		buf[str_len] = 0;
		printf("Message from server:%s", buf);
	}
}

void write_routine(int sock, char* buf) {
    while(1) {
        fgets(buf, BUF_SIZE, stdin);
        if(!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")) {
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}
```

## 进程间通信

进程间通信可以使用管道，管道并非属于进程，而是和套接字一样属于操作系统，以下是创建管道的api：

```c++
#include<unistd.h>
int pipe(int filedes[2]);
```

其中filedes[0]是通过管道接收数据时使用的文件描述符，即管道出口，filedes[1]是通过管道传输数据时使用的文件描述符，即管道入口，使用示例如下：

```c++
#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char* argv[]) {
	int fds[2];
	char str[] = "Who are you?";
	char buf[BUF_SIZE];
	pid_t pid;
	pipe(fds);
	pid = fork();
	if (pid == 0) write(fds[1], str, sizeof(str));
	else {
		read(fds[0], buf, BUF_SIZE);
		puts(buf);
	}
}
```

当然，也可以通过1个通道进行双向通信，比如：

```c++
#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char* argv[]) {
	int fds[2];
	char str1[] = "Who are you?";
	char str2[] = "Thank you for your message";
	char buf[BUF_SIZE];
	pid_t pid;
	pipe(fds);
	pid = fork();
	if (pid == 0) {
		write(fds[1], str1, sizeof(str1));
		sleep(2);
		read(fds[0], buf, BUF_SIZE);
		printf("Child proc output: %s\n", buf);
	}
	else {
		read(fds[0], buf, BUF_SIZE);
		printf("Parent proc output: %s \n", buf);
		write(fds[1], str2, sizeof(str2));
		sleep(3);
	}
	return 0;
}
```

注意，如果将`sleep(2);`注释掉，将会发生错误，因为向管道传递数据时，先读的进程会把数据取走，如果注释掉上述的那行代码，则父进程调用read函数后将无限期等待数据进入管道，因此，最好使用两个管道进行双向通信，如下：

```c++
#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char* argv[]) {
	int fds1[2]， fds2[2];
	char str1[] = "Who are you?";
	char str2[] = "Thank you for your message";
	char buf[BUF_SIZE];
	pid_t pid;
	pipe(fds1), pipe(fds2);
	pid = fork();
	if (pid == 0) {
		write(fds1[1], str1, sizeof(str1));
		read(fds2[0], buf, BUF_SIZE);
		printf("Child proc output: %s\n", buf);
	}
	else {
		read(fds1[0], buf, BUF_SIZE);
		printf("Parent proc output: %s \n", buf);
		write(fds2[1], str2, sizeof(str2));
		sleep(3);
	}
	return 0;
}
```

## I/O复用

模型中引入复用技术，可以减少进程数，即使用一个进程向多个客户端提供服务。

### 理解select函数并实现服务器端

运行select函数是最具有代表性的实现复用服务器端的方法，使用select函数时可以将多个文件描述符集中到一起同一监视，其调用过程如下所示：

![select](..\image\Network\select.png)

使用select可以同时监视多个文件描述符，当然也可以视为监视套接字，使用时将要监视的文件描述符集中到一起，需要按照监视项（接收、传输、异常）来区分，将监视项分成三类，使用fd_set数组变量执行此项操作，要监视哪一个文件描述符，就将哪个文件描述符代表的位置为1。在fd_set中注册或更改值的操作都由以下宏来操作：

- FD_ZERO(fd_set *fdset)：将fd_set变量的所有位初始化为0；
- FD_SET(int fd, fd_set* fdset)：在参数fdset指向的变量中注册文件描述符fd的信息；
- FD_CLR(int fd, fd_set *fdset)：从参数fdset指向的变量中清除文件描述符fd的信息；
- FD_ISSET(int fd, fd_set* fdset)：若参数fdset指向的变量中包含文件描述符fd的信息，则返回真；

功能如下：

![fd_set_function](..\image\Network\fd_set_function.png)

select的api如下所示：

```c++
#include <sys/select.h>
#include <sys/time.h>

int select(int maxfd, fd_set* readset, fd_set* writeset, fd_set* exceptset, const struct timeval* timeout);
```

由于每次新建文件描述符，其值都会+1，因此maxfd可只设置为最大的文件描述符+1，因为文件描述符从0开始。另外如何知道哪些文件描述符发生了变化，可以通过那些文件描述符集合来看，其值没有置零就是发生了变化，即发生了监视的事件。以下是调用的示例：

```c++
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#define BUF_SIZE 30

int main() {
	fd_set reads, temps;
	int result, str_len;
	char buf[BUF_SIZE];
	struct timeval timeout;
	FD_ZERO(&reads);
	FD_SET(0, &reads);
	timeout.tv_sec = 5; 
	timeout.tv_usec = 5000;
	while(1) {
		temps = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		result = select(1, &temps, 0, 0, &timeout);
		if (result == -1) {
			puts("select() error");
			break;
		}
		else if(result == 0) {
			puts("Time-out");
		}
		else {
			if(FD_ISSET(0, &temps)) {
				str_len = read(0, buf, BUF_SIZE);
				buf[str_len] = 0;
				printf("message from console: %s", buf);
			}
		}
	}
	return 0;
}
```

## 多种I/O函数

### send&recv函数

linux的相关api如下：

```c++
#include <sys/socket.h>
ssize_t send(int sockfd, const void* buf, size_t nbytes, int flags);
ssize_t recv(int sockfd, void* buf, size_t nbytes, int flags);
```

其中flag是可选项，有以下的种类和含义：

![send&recv](..\image\Network\send&recv.png)

其中，MSG_OOB用于发送紧急消息，使用示例如下：

```c++
send(sock, "4", strlen("4"), MSG_OOB);
...
fcntl(recv_sock, F_SETOWN, getpid());
state = sigaction(SIGURG, &act, 0);

void urg_handler(int signo) {
	int str_len;
	char buf[BUF_SIZE];
	str_len = recv(recv_sock, buf, sizeof(buf)-1, MSG_OOB);
	buf[str_len] = 0;
	printf("Urgent message: %s\n", buf);
}
```

当收到MSG_OOB紧急消息时，操作系统会产生SIGURG信号，并调用注册的信号处理函数，至于fcntl函数，其用于控制文件描述符，上述代码中将recv_sock的所有权指派给了把getid函数返回值用作ID的进程。然而，通过这个选项，并不能加快数据传输速度，而且信号处理函数urg_handle也只能读一个字节，剩余的字节只能通过未设置MSG_OOB的普通读取函数读取。MSG_OOB的真正意义在于督促数据接收对象尽快处理数据。

同时设置MSG_PEEK和MSG_DONTWAIT选项，以验证输入缓冲中是否存在接收的数据，设置MSG_PEEK选项并调用recv函数时，即使读取了输入缓冲的数据也不会删除，因此，常与MSG_DONTWAIT合作，用于调用非阻塞方式验证待读数据存在与否的函数。

### readv&writev函数

通过writev，可以将分散保存在多个缓冲的数据一起发送，通过readv，可以由多个缓冲分别接收，因此，适当使用这两个函数可以减少I/O函数的调用次数。相关api如下：

```c++
#include <sys/uio.h>
ssize_t writev(int filedes, const struct iovec* iov, int iovcnt);
ssize_t readv(int filedes, const struct iovec* iov, int iovcnt);
struct iovec {
    void* iov_base; //缓冲地址
    size_t iov_len;  //缓冲大小
}
```

## 多播与广播

多播方式的数据传输是基于UDP完成的，其与UDP服务端/客户端的区别在于，UDP数据传输以单一目标进行，多播则可以同时向多个主机传递数据。多播的数据传输特点如下所示：

- 多播服务器针对特定多播组，只发送1次数据；
- 即使只发送一次数据，该组内所有客户端都可以收到数据；
- 多播组数可在IP地址范围内任意增加；
- 加入特定组即可接收发往该多播组的数据；

多播适合向大量客户端发送相同数据，例如多媒体的实时传输。为了传递多播数据包，必须设置TTL（Time to Live，生存时间）。TTL用整数表示，每经过一个路由器就减一，当TTL变为0时该数据包就不会被传输而是会销毁。与设置TTL相关的协议层为IPPROTO_IP，选项名为IP_MULTICAST_TTL，示例如下：

```c++
int send_sock;
int time_live = 64;
...
send_sock = socket(PF_INET, SOCK_DGRAM, 0);
setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));
```

加入多播组也通过设置套接字选项完成：

```c++
int recv_sock;
struct ip_mreq join_adr;
recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
...
join_adr.imr_muttiaddr.s_addr = "多播组地址信息"；
join_adr.imr_interface.s_addr = "加入多播组的主机地址信息"；
setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr));

...
struct ip_mreq {
    struct in_addr imr_multiaddr; //组ip地址
    struct in_addr imr_interface; //加入该组的套接字所属主机的IP地址，也可以使用INADDR_ANY
}
```

### 广播

广播也是一次性向多个主机发送数据，但其只能向同一网络中的主机传输数据。广播也是基于UDP完成的，根据传输数据时使用的IP地址的形式，分为直接广播和本地广播。二者的差别主要在ip地址，直接广播的ip地址除了网络地址外，其余主机地址全部设置为1，反之，本地广播中使用的ip地址限定为255.255.255.255。使用如下：

```c++
int send_sock;
int bcast = 1;  //对变量初始化以将SO_BRADACAST选项设置为1
...
send_sock = socket(PF_INET, SOCK_DGRAM, 0);
...
setsockopt(send_sock, SOL_SOCKET, SO_BRODACAST, (void*)&bcast, sizeof(bcast));
```

## 套接字和标准IO

标准IO函数具有以下的优点：具有良好的移植性；可以利用缓冲提高性能。使用示例如下：

```c++
#include <stdio.h>
#define BUF_SIZE 3

int main(int argc, char* argv[]) {
	FILE *fp1;
	FILE *fp2;
	char buf[BUF_SIZE];
	fp1 = fopen("new.txt", "r");
	fp2 = fopen("copy.txt", "w");
	while (fgets(buf, BUF_SIZE, fp1)!=NULL) {
		fputs(buf, fp2);
	}
	fclose(fp1);
	fclose(fp2);
	return 0;
}
```

当然，标准IO也有缺点：不容易进行双向通信；有时可能频繁调用fflush函数；需要以FILE结构体指针形式返回文件描述符。

### 使用标准io函数

创建套接字会返回文件描述符，为了使用标准IO函数，需要先将其转换为FILE结构体指针，可使用如下api：

```c++
#include <stdio.h>
FILE *fdopen(int fildes, const char* mode);
```

常用的参数模式有读模式r和写模式w，使用如下：

```c++
#include <stdio.h>
#include <fcntl.h>

int main(void) {
	FILE *fp;
	int fd = open("data.dat", O_WRONLY|O_CREAT|O_TRUNC);
	if (fd == -1) {
		fputs("file open error", stdout);
		return -1;
	}
	fd = dfopen(fd, "w");
	fputs("Network C programming\n", fp);
	fclose(fp);
	return 0;
}
```

另外，还可以使用fileno将其转换会文件描述符：

```c++
#include <stdio.h>
int fileno(FILE* stream);
```

实际使用如下：

```
...
readfp = fdopen(clnt_sock, "r");
writefp = fdopen(clnt_sock, "w");
while(!feof(readfp)) {
	fgets(message, BUF_SIZE, readfp);
	fputs(message, writefp);
	fflush(writefp);
}
```

注意，由于标准IO函数内部提供了额外的缓冲，若不立即调用fflush函数，将无法保证数据立即传输到客户端。

## 关于IO流分离的其他内容

终止流时无法半关闭的原因：

![FILE](..\image\Network\FILE.png)

可见，读模式跟写模式的指针都是根据同一个文件描述符创建的，因此针对任意一个FILE指针调用fclose函数都会关闭文件描述符。既然如此，要怎么进入半关闭状态，方法是创建FILE指针先复制文件描述符即可：

![FILE1](..\image\Network\FILE1.png)

复制文件描述符可以使用以下两个函数之一：

```c++
#include <unistd.h>
int dup(int fildes);
int dup2(int fildes, int fildes2);
```

使用示例如下：

```c++
#include <stdio.h>
#include <unistd.h>
int main(int argc, char* argv[]) {
	int cfd1, cfd2;
	char str1[] = "Hi~ \n";
	char str2[] = "It's nice day~\n";
	cfd1 = dup(1);
	cfd2 = dup2(cfd1, 7);
	printf("fd1=%d, fd2=%d\n", cfd1, cfd2);
	write(cfd1, str1, sizeof(str1));
	write(cfd2, str2, sizeof(str2));
	close(cfd1);
	close(cfd2);
	write(1, str1, sizeof(str1));
	close(1);
	write(1, str2, sizeof(str1));
	return 0;
}
```

用这种方法实现流的分离：

```c++
...
readfp = fdopen(clnt_sock, "r");
writefp = fdopen(dup(clnt_sock), "w");
fputs("xxxx", writefp);
fflush(writefp);
shutdown(fileno(writefp), SHUT_WR);
fclose(writefp);
fgets(buf, sizefo(buf), readfp);
fputs(buf, stdout);
fclose(readfp);
...
```

## 优于select的epoll

基于select的IO复用服务器端慢的原因主要有以下两点：调用select后常见的针对所有文件描述符的循环语句；每次调用select函数都需要向该函数传递监视对象的信息。而linux下的epoll具有克服select函数的优点：无需编写以监视状态为目的的针对所有文件描述符的循环语句；调用对应于select函数的epoll_wait时不需要每次传递监视对象信息。

epoll服务器端实现需要以下三个函数：

- epoll_create：创建保存epoll文件描述符的空间；
- epoll_ctl：向空间注册并注销文件描述符；
- epoll_wait：与select类似，等待文件描述符发生变化；

相关的结构体如下：

```c++
struct epoll_event {
	__uint32_t events;
	epoll_data_t data;
}

typedef union epoll_data {
	void *ptr;
	int fd;
	__uint32_t u32;
	__uint64_t u64;
}epoll_data_t;
```

epoll_create的api如下：

```c++
#include <sys/epoll.h>
int epoll_create(int size);
```

调用此函数创建的文件描述符保存空间称为epoll例程，通过size参数来决定epoll例程的大小。epoll_create创建的资源与套接字相同，也由操作系统管理，也会返回文件描述符，因此终止时也需要close函数。

生成epoll例程后需要注册需要监视对象文件描述符，api如下：

```c++
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
```

示例如下：

```c++
epoll_ctl(A, EPOLL_CTL_ADD, B, C);
epoll_ctl(A, EPOLL_CTL_DEL, B, NULL);
```

分别代表在epoll例程中注册和删除文件描述符B，C指的是要监视的事件。op有以下的选项：

- EPOLL_CTL_ADD：将文件描述符注册到epoll例程；
- EPOLL_CTL_DEL：从epoll例程中删除文件描述符；
- EPOLL_CTL_MOD：更改注册的文件描述符的关键事件发生情况；

epoll_event可以注册要关注的事件，使用如下：

```c++
struct epoll_event event;
event.events = EPOLLIN;
event.data.fd = sockfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
```

events中可以保存的常量及所指事件的类型有：

- EPOLLIN：需要读取数据的情况；
- EPOLLOUT：输出缓冲为空，可以立即发送数据的情况；
- EPOLLPRI：收到OOB数据的情况；
- EPOLLRDHUP：断开连接或者半关闭的情况；
- EPOLLERR：发生错误的情况；
- EPOLLET：以边缘触发的方式得到事件通知；
- EPOLLONESHOT：发生一次事件后相应的文件描述符不再收到事件通知，因此需要EPOLL_CTL_MOD再次设置事件；

最后一个api是：

```c++
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
```

使用如下：

```
int event_cnt;
struct epoll_event *ep_events;
...
ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);
...
event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
```

### 条件触发和边缘触发

条件触发方式中，只要输入缓冲中有数据就会一直通知该事件，而边缘触发中输入缓冲收到数据时仅注册一次该事件，即使输入缓冲中还有数据也不会再进行注册。

epoll默认以条件触发方式工作，但也可按以下的方式更改换成以边缘触发的方式工作：

```c++
event.events = EPOLLIN|EPOLLET
```

下面介绍将套接字改为非阻塞方式的方法，linux可使用如下的方式来更改或读取文件属性：

```c++
int fcntl(int filedes, int cmd, ...);
```

使用如下：

```c
int flag = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flag|O_NONBLOCK);
```

