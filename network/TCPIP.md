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

UDP虽然不可靠，但比TCP的性能更好。