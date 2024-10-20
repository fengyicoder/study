# 简介

## 概述

IPC是进程间通信的简称。在Unix过去30多年的演变中，消息传递经过了以下几个发展阶段：

- 管道，第一个广泛使用的IPC形式，既可以在程序中使用，又可以在shell使用。缺点是只能在具有共同祖先（指父子进程关系）的进程间使用，不过该问题已经被引入的FIFO（有名管道解决）；
- System V消息队列，可用在同一主机上有亲缘关系或无亲缘关系的进程之间。理论上说，所有Unix进程都跟init进程有亲缘关系，它是系统自举时启动所有初始化进程的祖先进程。
- Poxis消息队列；
- 远程过程调用（RPC），是从一个系统上某个程序调用；

## 进程线程与信息共享

进程之间信息共享有多种方式，按层级划分有如下关系：

![](..\image\Unix\2-1-1.png)

1. 进程共享文件系统中某个文件的信息，为了访问这些信息，每个进程都得穿越内核（例如read、write、lseek等）；
2. 共享的信息驻留在内核中，管道、System V消息队列、System V信号量都是如此；
3. 共享的信息存在一个双方都能访问的内存区；

## IPC对象的持续性

可以将任意类型的IPC的持续性定义成该类型的一个对象一直存在多长时间，如：

![](..\image\Unix\2-1-2.png)

1. 随进程持续的有管道和FIFO；
2. 随内核持续的有System V的消息队列、信号量和共享内存区；
3. 随文件系统持续的有如果是用映射文件的Posix消息队列、信号量和共享内存区；

各种类型IPC对象的持续性如下所示：

![](..\image\Unix\2-1-3.png)

## fork、exec和exit对IPC对象的影响

总结如下：

![](..\image\Unix\2-1-6.png)

# Posix IPC

## 概述

Posix IPC由Posix消息队列、Posix信号量、Posix共享内存区组成。相关函数总结如下：

![](..\image\Unix\2-2-1.png)

# System V IPC

## 概述

System V消息队列、System V信号量、System V共享内存区合称为System V IPC；其函数汇总如下：

![](..\image\Unix\2-3-1.png)

## 创建与打开IPC通道

创建或打开一个IPC对象的三个getxxx函数，第一个参数是类型为Key_t的IPC键，返回值identifier是一个整数标识符。对于key值有两个选择：

1. 调用ftok，传递pathname和id；
2. 指定key为IPC_PRIVATE，会保证创建一个新的、唯一的IPC对象；

其oflag的参数规则如下：

- 没有一对id和pathname的组合会产生IPC_PRVATE这个键值；
- 设置oflag参数的IPC_CREATE位但不设置IPC_EXCL位时，如果指定键的IPC对象不存在会创建，否则返回该对象；
- 同时设置oflag的IPC_CREAT和IPC_EXCL位时，如果指定键的IPC对象不存在就创建一个新的对象，否则返回一个EEXIST错误，因为该对象已存在；

## IPC权限

每创建一个新的IPC对象，以下信息就会保存到ipc_perm结构中：oflag参数中某些初始化ipc_perm结构的mode成员，表示了一些读写权限；cuid和cgid；uid和gid；

## 标识符重用

ipc_perm结构中还含有一个名为seq的变量，它是一个槽位使用情况序列号，是一个内核为系统中每个潜在的IPC对象维护的计数器。每当删除一个IPC对象时，内核就递增相应的槽位号，溢出则返回0；

该计数器的存在有两个原因。由于本章所述的IPC是系统级，如果采用小正数，很容易被一些进程试出来恶意操作IPC，因此IPC机制的设计者将标识符的可能范围扩大到所有的整数，这种扩大是这么实现：每次重用一个IPC表项时，把返回给调用进程的标识符值增加一个IPC表项数，例如系统配置最多50个消息队列，那么内核中第一个消息队列被删除且重用时，返回的标识符不再是0而是变成50。递增槽位使用情况序列号的另一个原因是为了避免短时间重用System V IPC标识符。

# 管道和FIFO

## 管道

所有的Unix都提供管道，由pipe函数创建，提供一个单路数据流。一个管道返回两个文件描述符，`fd[0]`打开读，`fd[1]`打开来写。宏S_ISFIFO可用来确定一个描述符或文件是管道还是FIFO。管道的使用情况一般是一个进程创建管道，然后调用fork派生一个自身的副本，之后父进程关闭这个管道的读出端，子进程关闭同一管道的写入端，这样就在父子进程之间提供了一个单向的数据流，如图：

![](..\image\Unix\2-4-4.png)

注意为子进程waitpid，因为子进程在管道写入最终数据后调用exit首先终止，它随后变成一个僵尸进程：自身已终止但父进程仍在运行且尚未等待该子进程的进程。当子进程终止时，内核还给父进程一个SIGCHLD信号，但父进程没有捕获这个信号，该信号的默认行为就是忽略。之后不久，父进程在从管道读取最终数据后返回，随后调用waitpid取得已终止子进程（僵尸进程）的终止状态。如果父进程没有调用waitpid，而是直接终止，那么子进程将成为托孤给init进程的孤儿进程，内核将向init进程发送另一个SIGCHLD信号，init进程随后取得该僵尸进程的终止状态。

## 全双工管道

某些系统提供全双工管道：SVR4的pipe函数以及许多内核都提供的socketpair函数。全双工管道的真实实现如下：

![](..\image\Unix\2-4-13.png)

## popen和pclose函数

作为另一个关于管道的例子，标准I/O函数库提供了popen函数，其创建一个管道并启动另一个进程，该进程要么从该管道读取标准输入，要么往该管道写入标准输出。

## FIFO

FIFO称为有名管道，由mkfifo创建。再创建出一个FIFO之后，必须打开来读或者打开来写，所用的可以是open或者fopen。FIFO不能打开既读又写，因为它是半双工的。

管道在所有进程最终都关闭它之后自动消失，FIFO的名字则只有通过调用unlink才从文件系统中删除。

unlink尽管可以从文件系统删除指定的路径名，但先前已经打开该路径名、目前仍打开着的描述符却不受影响。

## 管道和FIFO的额外属性

一个描述符能以两种方式设置成非阻塞：

1. 调用open时指定O_NONBLOCK标志：

   ```
   writefd = Open(FIFO1, O_WRONLY | O_NONBLOCK, 0);
   ```

   

2. 如果一个描述符已经打开，可以调用fcntl启用O_NONBLOCK标志，对于管道来说必须使用这种技术，因为其没有open调用，在pipe调用中也无法启动，示例：

   ```c++
   int flags;
   if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
   	err_sys("F_GETFL error");
   flags |= O_NONBLOCK;
   if ((flags = fcntl(fd, F_SETFL, flags)) < 0)
   	err_sys("F_SETFL error");
   ```

   建议使用以上方式，防止简单设置标志而将原来的设置清空。

非阻塞标志的影响如下：

![](..\image\Unix\2-4-21.png)

以下是关于管道或FIFO的读写的若干额外规则：

- 如果请求读出的数据量多于管道或FIFO当前可用数据量，那么只返回这些可用数据；

- 如果请求写入的数据的字节数小于或等于PIPE_BUF（一个Posix限制值），那么write操作保证是原子的。如果大于PIPE_BUF，那么就不能保证。*Posix.1要求PIPE_BUF至少为512字节，常见的值处于1024到5120之间。*

- O_NONBLOCK对write操作的原子性没有影响。但如果设置成非阻塞时，来自write的返回值取决于待写的字节数以及该管道或FIFO中当前可用空间大小。如果待写字节数小于等于PIPE_BUF：

  1. 如果有足以存放所请求字节数的空间，那么所有数据写入。
  2. 如果没有足够的空间，那么立即返回一个EAGAIN错误。

  如果待写的字节数大于PIPE_BUF:

  1. 有空间写多少返回多少。
  2. 如果没有空间返回一个EAGAIN错误。

- 如果向一个没有为读打开着的管道或FIFO写入，内核将产生一个SIGPIPE信号：
  1. 如果调用进程既没有捕获也没有忽略该信号，那么默认行为就是终止该进程。
  2. 如果调用进程忽略了该信号或者捕获了该信号并从信号处理程序中返回，那么write返回一个EPIPE错误。

## 对比迭代服务器与并发服务器

默认情况下当客户还没有打开字节的FIFO时，服务器是阻塞在对该FIFO的open调用，这意味着恶意的客户可以让服务器处于停顿状态，方法是发送一个请求却不打开字节的FIFO来读，这称为拒绝服务（DoS）型攻击。为避免这种攻击，要留意服务器可能在哪阻塞以及可能阻塞多久。处理这种问题方法之一是在特定操作设置一个超时时钟，但把服务器程序变成并发服务器更为简单。

## 字节流与消息

如果想要识别消息的边界，可以采用以下的技巧：

1. 带内特殊终止序列：比如以换行符来分隔消息。
2. 显式长度：每个记录前冠以它的长度；
3. 每次连接一个记录：应用通过关闭与其对端的连接来指示一个记录的结束；

## 管道和FIFO限制

系统给与管道和FIFO的唯一限制为：

OPEN_MAX：一个进程在任意时刻打开的最大描述符数（Posix要求至少为16）；

PIPE_BUF：可原子写往一个管道或FIFO的最大数据量；

OPEN_MAX的值可通过sysconf函数查询，可通过ulimit或limit命令从shell修改，也可通过调用setrlimit函数从一个进程中修改。PIPE_BUF的值可以在运行时通过调用pathconf或fpathconf取得。

# Posix消息队列

## 概述

Posix消息队列和System V消息队列的差别如下所示：

- Posix消息队列的读总是返回最高优先级的最早消息，对System V消息队列的读则可以返回任意指定优先级的消息；
- 当往一个空队列放置一个消息时，Posix消息队列允许产生一个信号或启动一个线程，System V消息队列则不提供类似机制；

队列中的每个消息具有如下属性：

- 一个无符号整数优先级（Posix）或一个长整数类型（System V）；

- 消息的数据部分长度（可以为0）；
- 数据本身；

## mq_open、mq_close和mq_unlink

mq_close可以关闭一个mq_open打开的消息队列，但其消息队列并不从系统删除。要从系统中删除就必须调用mq_unlink。每个消息队列有一个保存其打开着的描述符数的引用计数器，因此能够实现类似于unlink函数删除一个文件的机制：当一个消息队列的引用技术仍大于0，其能够执行删除操作，但析构要到最后一个mq_close发生时才进行。getopt函数用来解析传入到进程的参数。

## mq_send和mq_receive函数

这两个函数分别用于往一个队列中放置一个消息和从一个队列中取走一个消息。每个消息有个优先级，它是一个小于MQ_PRIO_MAX的无符号整数，Posix要求这个上限至少为32。mq_receive总是返回所指定队列中最高优先级的最早消息，而且该优先级能随该消息的内容及长度一同返回。

## 消息队列限制

创建队列时会建立两个限制：

- mq_maxmsg：队列中最大消息数
- mq_msgsize：给定消息的最大字节数

消息队列的实现定义了两位两个限制：

- MQ_OPEN_MAX：一个进程能够同时拥有的打开着消息队列的最大数目（Posix要求至少为8）；
- MQ_PRIO_MAX：任意消息的最大优先级值加一（Posix要求至少为32）；

## mq_notify函数

Posix消息队列允许异步事件通知，告知何时有一个消息放到了某个空消息队列中，这种通知有两种方式选择：

- 产生一个信号；
- 创建一个线程来执行一个特定的函数；

这种通知通过调用mq_notify建立。任意时刻只有一个进程可以被注册为接受某个队列的通知，且接收到通知之后这种注册即被取消。注意，在mq_receive调用中的阻塞比任何通知的注册都优先。

注意，信号处理函数中只可调用异步信号安全的函数，所有标准I/O函数和pthread_xxx都不在其中。本书涵盖的IPC函数中，只有sem_post、read和write在其中。

sigprocmask可以用来阻塞某个信号，sigwait可以用来阻塞等待某个信号。对于上述信号处理函数的写法，一种是利用sigwait来等待某个信号，一种是在信号处理函数利用write来写入管道通过select监视信号的变化。

异步事件通知的另一种方式是把sigev_notify设置为SIGEV_THREAD，这回创建一个新线程，调用sigev_notify_function指定的函数。

## Posix实时信号

信号可分为两大组：

- 值在SIGRTMIN和SIGRTMAX之间的实时信号，Posix要求至少提供RTSIG_MAX种实时信号，该常值最小为8；
- 其他信号：SIGALRM等。

针对sigaction调用中是否指定了新的SA_SIGINFO标志，有如下差异：

![](..\image\Unix\2-5-16.png)

实时行为有如下意义：

- 信号是排队的；
- 当有多个SIGRTMIN到SIGRTMAX范围内的解阻塞信号排队时值较小的信号先于值较大的信号递交；
- 当某个非实时信号递交时，传递给它的信号处理程序的唯一参数是该信号的值，实时信号比其他信号携带更多的信息。

**待续**

## System V消息队列

System V消息队列使用消息队列标识符标识。

## msgget函数

msgget需要指定一个key，其既可以是ftok的返回值，也可以是常值IPC_PRIVATE.当创建一个新消息队列时，msgid_ds结构的如下成员被初始化：

- msg_perm结构的uid和cuid成员被设置成当前进程的有效用户ID，gid和cgid成员被设置成当前进程的有效组ID；
- oflag中的读写权限位存放在msg_perm.mode中；
- msg_qnum、msg_lspid、msg_lrpid、msg_stime和msg_rtime被置为0；
- msg_ctime被设置成当前时间；
- msg_qbytes被设置成系统限制值；

## msgsnd函数

flag参数可以是0也可以是IPC_NOWAIT。IPC_NOWAIT标志使得msgsnd调用非阻塞：如果没有存放新消息的可用空间，该函数马上返回，可能发生的情形包括：

- 在指定的队列中已有太多的字节；
- 在系统范围内存在太多的消息；

如果条件满足且IPC_NOWAIT指定，msgsnd会返回一个EAGAIN错误，如果IPC_NOWAIT没有指定，那么调用线程会陷入睡眠，直到：

- 具备存放新消息的空间；
- 由msqid标识的消息队列从系统删除（这种情况返回一个EIDRM错误）；
- 调用线程被某个捕获的信号所中断（这种情况返回一个EINTR错误）；

## msgrcv函数

type参数指定希望从所给定的队列中读出什么样的消息：

- 如果type为0，就返回队列中的第一个消息；
- 如果type大于0，就返回其类型值为type的第一个消息；
- 如果type小于0，返回其类型值小于或等于type参数的绝对值的消息中类型值最小的第一个消息；

如果所请求类型的类型不在所指定的队列中，如果设置了flag的IPC_NOWAIT位，msgrcv函数立即返回一个ENOMSG错误，否则，调用者被阻塞到下列某个事件发生为止：

1. 有一个所请求类型的消息可获得；
2. 由msqid标识的消息队列被从系统中删除（这种情况下返回一个EIDRM错误）；
3. 调用线程被某个捕获的信号中断（这种情况返回EINTR错误）；

如果设置了MSG_NOERROR，那么所接收的消息的真正数据部分大于length参数时，msgrcv函数只是截短数据消息，而不返回错误，否则返回E2BIG错误。

成功返回时，msgrcv返回的是接收消息中数据的字节数，不包括消息类型那几个字节。

## msgctl函数

msgctl函数提供如下命令：

- IPC_RMID：从系统删除由msqid指定的消息队列并丢弃消息；
- IPC_SET：给指定的消息队列设置msqid_ds的以下四个成员：msg_perm.uid、msg_perm.gid、msg_perm.mode和msg_qbytes。
- IPC_STAT：给调用者返回指定队列对应的msqid_ds结构；

## 消息队列限制

如图：

![](..\image\Unix\2-6-25.png)

# 互斥锁和条件变量

如果一个互斥锁或条件变量存放在多个进程间共享的某个内存区，那么Posix还允许它用于这些进程间同步。

## 互斥锁：上锁与解锁

posix互斥锁被声明为具有pthread_mutex_t数据类型的变量，如果互斥锁变量是静态分配的，我们可以将其初始化为常值PTHREAD_MUTEX_INITIALIZER，如果是动态分配的，则必须通过调用pthread_mutex_init函数来初始化。pthread_mutex_trylock是pthread_mutex_lock的非阻塞函数，如果该互斥锁已经锁住，其会返回一个EBUSY错误。不同线程可被赋予不同的优先级，优先唤醒优先级最高的阻塞线程。

## 条件变量：定时等待和广播

pthread_cond_broadcast可唤醒阻塞在相应条件变量上的所有线程。

## 互斥锁和条件变量的属性

互斥锁属性的数据类型为pthread_mutexattr_t，条件变量属性的数据类型为pthread_condattr_t。一旦这些属性对象被初始化，就通过不同函数启用或禁止特定的属性：

```c++
#include <pthread.h>
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * attr, int *valptr);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int value);
int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *valptr);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int value);
```

value的值可以是PTHREAD_PROCESS_PRIVATE或PTHREAD_PROCESS_SHARED。后者也称为进程间共享属性。下面是初始化一个互斥锁以便其能在进程间共享的过程：

```c++
pthread_mutex_t *mptr;
pthread_mutexattr_t mattr;
...
mptr = /* some value that points to shared memory */；
Pthread_mutexattr_init(&mattr);
#ifdef _POSIX_THREAD_PROCESS_SHARED
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
#else
# erorr this implementation does not suppose _POSIX_THREAD_PROCESS_SHARED
#endif
pthread_mutex_init(mptr, &mattr);
```

当在进程间共享一个互斥锁时，持有该互斥锁的进程在持有期间终止的可能总是有的，没有办法让系统在进程终止时自动释放锁持有的锁。进程终止时内核总是自动清理的唯一同步锁类型是fcntl记录锁。使用System V信号量时，应用程序可以选择进程终止时内核是否自动清理某个信号量锁。

# 读写锁

## 获取与释放读写锁

读写锁的数据类型是pthread_rwlock_t。如果这个类型的变量是静态分配的，那么可以赋常值PTHREAD_RWLOCK_INITIALIZER来初始化。

跟互斥锁一样，也可以定义PTHREAD_PROCESS_SHARED来指定相应的读写锁在不同进程间共享。

**待续**

# 记录上锁

## 概述

本章所述的锁是读写锁的一种扩展类型，可用于有亲缘关系或无亲缘关系的进程之间共享某个文件的读与写。被锁住的文件通过其描述符访问，执行上锁操作的函数为fcntl。这种类型锁通常在内核维护，其属主是由属主的进程ID标识。

## Posix fcntl记录上锁

记录上锁的Posix接口是fcntl函数。用于记录上锁的cmd参数共有三个值，这三个命令要求第三个参数arg是指向某个flock结构的指针：

```c++
struct flock {
	short l_type; /* F_RDLCK, F_WRLCK, F_UNLCK */
	short l_whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
	off_t l_start; /* relative starting offset in bytes */
	off_t l_len; /* bytes; 0 means until end-of-file */
	pid_t l_pid; /* PID returned by F_GETLK */
};
```

这三个命令如下：

- F_SETLK：获取（l_type成员为F_RDLCK或F_WRLCK）或释放（l_type成员为F_UNLCK）由arg指向的flock结构所描述的锁。如果无法将该锁授予调用进程，该函数立即返回一个EACCES或EAGAIN错误而不阻塞；
- F_SETLKW：与上一个命令类似，不过如果无法将所请求的锁授予调用进程，调用线程将阻塞到该锁能够授予为止。
- F_GETLK：检查由arg指向的锁以确定是否有某个已存在的锁会妨碍将新锁授予调用进程。如果当前没有这样的锁存在，由arg指向的flock结构的l_type成员将被设置为F_UNLCK。否则，关于这个已存在的锁的信息将在arg指向的flock结构中返回，其中包括该锁的进程ID。

锁不能通过fork由子进程继承。记录上锁不应该同标准I/O函数库一起使用，因为该函数库会执行内部缓冲。当某个文件需要上锁时，为避免问题，应对它使用read和write。

## 劝告性上锁

Posix记录上锁称为劝告性上锁，其含义是内核维护着已由各进程上锁的所有文件的正确信息，但它不能防止一个进程写已由另一个进程读锁定的某个文件。一个继承能够无视一个劝告性锁而写一个读锁定文件，或者读一个写锁定文件，前提是该进程有读或写该文件的足够权限。

## 强制性上锁

使用强制性上锁后，内核检查每个read和write请求，以验证其操作不会干扰由某个进程持有的某个锁。对于通常的阻塞式描述符，与某个强制性锁冲突的read或write将把调用进程投入睡眠，直到该锁释放为止，对于非阻塞式描述符，与某个强制性锁冲突的read或write将导致他们返回一个EAGAIN错误。为对某个特定文件施行强制性上锁，应满足：

- 组成员执行位必须关掉；
- SGID位必须打卡；

在支持强制性记录上锁的系统上，ls命令查找权限位的这种特殊组合，并输出l或L以指示相应文件的强制性上锁是否启用，类似，chmod命令接受l这个指示符以给某个文件启用强制性上锁。

## 文件作锁用

Posix.1保证，如果O_CREAT和O_EXCL标志调用open函数，一旦该文件已经存在，该函数就返回一个错误，所以这种技巧创建的文件可作为锁使用，释放这样的锁只需要unlink文件。除此之外，还有link和指定O_TRUNC标志也可用于创建锁。但这些都有各自的缺点，还是应该使用fcntl记录上锁。

# Posix信号量

## 概述

本书讨论三种类型的信号量：

- Posix有名信号量：使用Posix IPC名字标识，可用于进程或线程同步；
- Posix基于内存的信号量：存放在共享内存区，可用于进程或线程同步；
- System V信号量：在内核中维护，可用于进程或线程间的同步；

信号量、互斥锁、条件变量之间的差异：

1. 互斥锁必须由给它上锁的线程解锁，信号量的挂出却不必由执行过它的等待操作的同一线程执行；
2. 互斥锁要么锁住要么被解开；
3. 既然信号量有一个与之关联的状态，那么信号量的挂出操作总是被记住，但当向一个条件变量发送信号时，如果没有线程等待在该条件变量，那么该信号将丢失。

有名信号量与无名信号量使用函数比较如下：

![](..\image\Unix\2-10-6.png)

## sem_open、sem_close和sem_unlink函数

一个进程终止时，内核对其上仍然打开着的所有有名信号量自动执行信号量关闭操作，无论该进程是否自愿关闭。关闭一个信号量并没有将其从系统删除，除非使用sem_unlink函数。如果在调用sem_open返回之后接着调用fork，其信号量仍会在子进程中打开。

## sem_wait和sem_trywait

两者的差别是，当指定的信号量的值已经是0时，后者并不将调用线程陷入睡眠，相反则是返回一个EAGAIN错误。如果别某个信号中断，sem_wait可能过早的返回，返回的错误为EINTR。

## sem_post和sem_getvalue

在各种各样的同步技巧中，能够从信号处理函数中安全调用的唯一函数是sem_post。

## sem_init和sem_destroy函数

api如下：

![](..\image\Unix\2-10-8.png)

如果`shared`为0，待初始化的信号量被同一进程的各个线程间共享，否则在进程间共享。当`shared`非

0时，该信号量必须存放在某种类型的共享内存区内。注意，对一个已经初始化的变量再次调用sem_init结果是未定义的。

## 信号量限制

Posix定义了两个信号量限制：SEM_NSEMS_MAS，一个进程可同时打开着的最大信号量数；SEM_VALUE_MAX，一个信号量的最大值。

**待续**

# System V信号量

System V信号量通过定义如下概念给信号量增加了另外一级复杂度：计数信号量集，即一个或多个信号量（构成一个集合），其中每个都是计数信号量。每个集合的信号量数存在一个限制，一般在25个数量级上。

# 共享内存区介绍

共享内存区是可用IPC形式中最快的，一旦这样的内存区映射到共享它的进程的地址空间，这些进程间的数据传递就不再涉及内核（这里指进程不再通过执行任何进入内核的系统调用来彼此传递数据）。然而往该共享内存区存放信息或取走信息的进程间通常需要某种形式的同步。

普通的ipc涉及到了内核到进程之间的数据转移，如：

![](..\image\Unix\2-12-1.png)

注意，默认情况下，通过fork派生的子进程并不与其父进程共享内存区。

## mmap、munmap和msync函数

mmap函数把一个文件或一个Posix共享内存区对象映射到调用进程的内存空间，使用该函数有三个目的：

1. 使用普通文件以提供内存映射I/O；
2. 使用特殊文件以提供匿名内存映射；
3. 使用shm_open以提供无亲缘关系进程间的Posix共享内存区；

```c
#include<sys/mman.h>
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset); #返回若成功则为被映射区的起始地址，出错则为MAP_FAILED
```

其中addr指定fd应被映射到的进程内空间的起始地址，通常指定为空指针。映射关系展示如下：

![](..\image\Unix\2-12-6.png)

内存映射区的保护由prot参数指定，使用的值如下：

![](..\image\Unix\2-12-7.png)

flags使用的值如下：

![](..\image\Unix\2-12-8.png)

MAP_SHARED或MAP_PRIVATE这两个标志必须指定一个。可移植的代码应把addr指定为空指针，并且不指定MAP_FIXED。

父子进程之间共享内存区的方法之一是父进程在调用fork之前先指定MAP_SHARED调用mmap，Posix.1保证父进程的内存映射关系存留到子进程中，而且父进程做的修改子进程能看到。

mmap成功返回后fd参数可以关闭。

为从某个进程的地址空间删除一个映射关系，应调用munmap：

```c
#include<sys/mman.h>
int munmap(void *addr, size_t len); #返回成功为0，出错为-1
```

如果调用munmap之后再访问这个地址addr，将导致向调用进程产生一个SIGSEGV信号。

对于一个MAP_SHARED内存区，我们修改了共享内存区的内容，内核将在稍后某个时刻相应的更新文件，然而有时候我们希望硬盘上的文件内容与内存映射区的内容一致，于是调用msync来执行这种同步：

```c
#include<sys/mman.h>
int msync(void *addr, size_t len, int flags); 返回若成功则为0否则为-1
```

flags参数是以下常值的组合：

![](..\image\Unix\2-12-9.png)

MS_ASYNC和MS_SYNC这两个常值必须指定一个，但不能都指定，其差别在于一旦写操作已由内核排入队列，MS_ASYNC立即返回，而MS_SYNC需要等到写操作完成才返回。

使用内存映射文件的好处是所有的I/O操作都在内核掩盖下操作，我们不必调用read、write和lseek。注意，不是所有文件都能进行内存映射，比如试图将一个访问终端或套接字的描述符映射到内存将导致返回一个错误，这些类型的描述符必须通过read和write（或者它们的变体）来访问。mmap的另一个作用是在无亲缘关系的进程间提供共享的内存区。

## 4.4BSD匿名内存映射

如果意图调用mmap提供一个穿越fork由父子进程共享的映射内存区，也可以不必先打开一个文件，具体方法取决于实现：

1. 4.4BSD提供匿名内存映射，彻底避免了文件的创建和打开，其方法是mmap的flags参数指定为MAP_SHARED|MAP_ANON，把fd参数指定为-1，offset参数则被忽略。这样的内存区初始化为0；
2. SVR4提供/dev/zero设备文件，open它之后可在mmap调用中使用得到的描述符，从该设备读时返回的字节全为0，写往该设备的任何字节则被丢弃；

## 访问内存映射的对象

文件大小和内存映射区大小可以不同。但内核知道底层支撑对象的大小，一旦超出这个范围就被发出SIGBUS或SIGSEGV信号，如下：

![](..\image\Unix\2-12-18.png)

如果要处理一个持续增长的文件，则指定一个大于该文件大小的内存映射区大小，跟踪该文件的当前大小（注意要确保不访问当前文件尾以远的部分），然后让该文件大小随着每次写入数据而增长（通过ftruncate函数增加文件大小）。

# Posix共享内存区

## 概述

Posix.1提供了两种在无亲缘关系进程间共享内存区的方法：

1. 内存映射文件：由open函数打开，由mmap把得到的描述符映射到当前进程空间的一个文件。
2. 共享内存区对象：由shm_open打开一个Posix.1 IPC名字，由mmap把得到的描述符映射到当前进程的地址空间；

Posix把两者合称为内存区对象，如下：

![](..\image\Unix\2-13-1.png)

## shm_open和shm_unlink函数

函数api如下：

```c
#include<sys/mman.h>
int shm_open(const char *name, int oflag, mode_t mode); #成功返回非负描述符，出错则为-1
int shm_unlink(const char *name);#成功返回0，出错则为-1
```

shm_unlink跟上文中的所有unlink一样，删除一个名字不会影响对于其底层支撑对象的现有引用，直到对于该对象的引用全部关闭为止。

### ftruncate和fstat函数

处理mmap的时候，普通文件或共享内存区对象的大小都可以通过调用ftruncate修改:

```c
#include<unistd.h>
int ftruncate(int fd, off_t length) #成功返回0，出错返回-1
```

此函数对普通文件与共享内存区对象的处理稍有不同：

- 对于一个普通文件：如果该文件大小大于length参数，额外的数据就被丢弃，如果小于length，那么该文件是否修改以及大小是否增长是未加说明的。实际上对一个普通文件把它的大小扩展到length字节的可移植方法是：先lseek到偏移为length-1处，然后write 1个字节的数据。所幸的是几乎所有Unix实现都支持使用ftruncate扩展一个文件。
- 对于一个共享区对象：该函数把该对象的大小设置成length字节。

当打开一个已存在的共享内存区对象时，可以使用fstat来获取有关该对象的信息：

```c
#include<sys/types.h>
#include<sys/stat.h>
int fstat(int fd, struct stat *buf); #成功返回0，出错为-1
```

# System V共享内存区

## 概述

类似于Posix共享内存区，先调用shmget，再调用shmat。

## shmget函数

shmget创建一个新的共享内存区或者访问一个已存在的共享内存区。

```c
#include<sys/shm.h>
int shmget(key_t key, size_t size, int oflag); #成功返回共享区对象，出错返回-1
```

返回值是一个称为共享内存区标识符的整数。key既可以是ftok的返回值，也可以是IPC_PRIVATE。

## shmat函数

该函数将共享内存区附加到调用进程的地址空间：

```c
#include<sys/shm.h>
void *shmat(int shmid, const void* shmaddr, int flag);成功返回映射区的起始地址，出错返回-1；
```

如果shmaddr是一个空指针，那么系统替调用者选择地址（推荐），如果是一个非空地址，那么返回地址取决于调用者是否给flag参数指定了SHM_RND值，如果没有指定SHM_RND，那么相应共享内存附加到shmaddr参数指定的地址，如果指定了SHM_RND，那么相应的共享内存区附接到shmaddr参数指定的地址向下舍入一个SHMLBA常值，LBA代表低端边界地址。

## shmdt函数

进程完成共享内存区使用时调用shmdt断接这个内存区。

```c
#include<sys/shm.h>
int shmdt(const void* shmaddr); 成功返回0否则为-1
```

当一个进程终止时，当前附接着的所有共享内存区都自动断接掉。注意，本函数不删除所指定的共享内存区，这个删除工作通过以IPC_RMID命令调用shmctl完成。

## shmctl函数

```c
#include<sys/shm.h>
int shmctl(int shmid, int cmd, struct shmid_ds *buff); 成功返回0否则为-1
```

该函数提供了三个命令：

- IPC_RMID：从系统中删除由shmid标识的共享内存区并拆除它；
- IPC_SET：给所指定的共享内存区设置其shmid_ds结构的以下三个成员：shm_perm.uid、shm_perm.gid和shm_perm.mode，它们的值来自buff参数指向的结构中的相应成员。shm_ctime的值也用当前时间替换；
- IPC_STAT：通过buff参数向调用者返回指定共享内存区当前的shmid_ds结构。

## 共享内存区限制

![](..\image\Unix\2-14-5.png)

# 门

## 概述

客户-服务器过程调用的三种类型：

![](..\image\Unix\2-15-1.png)

1. 本地过程调用：被调用过程与调用过程处于同一个进程；
2. 远程过程调用（RPC）：被调用过程与调用过程处于不同进程，这也是门提供的能力。
3. RPC通常允许一台主机上某个客户调用另一个主机上的某个服务器进程，只要这两个主机以某种形式的网络连接着；

门是Solaris中的实现，目前linux系统中并没有相关实现，对于linux上该机制的移植实现见 [Linux door]( http://www.rampant.org/doors)。

在 Linux 系统中，`fattach` 函数用于将一个 STREAMS 或 doors-based 文件描述符附加到文件系统中的一个对象上，实际上是将一个名字与文件描述符关联起来。

## door_call函数

```c
#include<door.h>
int door_call(int fd, door_arg_t *argp);
```

该函数由客户调用，会在服务器进程的地址空间中执行一个服务器过程。

## door_create函数

```c
#include<door.h>
typedef void Door_server_proc(void *cookie, char *dataptr, size_t datasize, door_desc_t *descptr, size_t ndesc);
int door_create(Door_server_proc *proc, void *cookie, u_int attr);
```

## door_return函数

```c
#include<door.h>
int door_return(char *dataptr, size_t datasize, door_desc_t *descptr, size_t *ndesc);
```

服务器过程完成通过调用该函数返回，这会使得客户中相关联的door_call调用返回。

## door_cred函数

```c
#include<door.h>
int door_cred(door_cred_t *cred);
```

服务器过程能够通过该函数获取每个调用对应的客户凭证。

## door_info函数

```c
#include<door.h>
int door_info(int fd, door_info_t *info);
```

客户通过该函数获取有关服务器的信息。

