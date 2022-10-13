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

