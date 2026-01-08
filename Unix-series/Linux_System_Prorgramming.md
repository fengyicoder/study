# 简介和基础概念

## Linux编程概念

进程中的各个段最重要的和最通用的莫过于代码段、数据段和bss段。代码段包含可执行代码和只读数据，如常量，经常标记为只读和可执行。数据段包含已初始化的数据，如定义了值的C变量，通常标记为可读写的。bss段包含未初始化的全局数据，因为C标准规定了C变量的默认值全为，因此没有必要在磁盘上的目标代码中保存这些。

真实UID是启动进程的用户，有效UID可以使得进程在其他用户的权限下运行，保留UID保存原来的有效的UID，它的值决定了用户将切换到哪个有效的UID中，文件系统UID通常和有效UID相等，用来检测文件系统的访问权限。组用户的ID跟用户ID的用法类似。

# 文件和系统

## 打开文件

当使用open创建新文件时，如果使用了O_CREAT忘记提供mode参数，结果是未定义的。

当文件创建时，mode参数提供新建文件的权限，但系统并不在该次打开文件时检查权限，所以可以进行相反的操作，例如设置文件为只读权限，但却在打开文件后进行写操作。但如果文件已存在，此时权限检查会发生在open的时候。

## 用read来读

对read的调用可能会有很多可能的结果：

- 返回一个等于len的值，符合预期；
- 返回一个大于但小于len的值，可能读取被打断或者发生了一个错误，或者在读入len个字节之前已经抵达了EOF。
- 调用返回，标志着EOF，没有可以读取的数据；
- 调用阻塞，没有可用来读取的数据，在非阻塞模式下不会发生；
- 返回-1并且errno被设置为EINTR，表示在读入字节之前收到了一个信号，可以重新进行调用；
- 返回-1并且errno被设置为EAGAIN，表示读取会因为没有可用的数据而阻塞，而读请求应该在之后重开，只发生在非阻塞模式；
- 返回-1，并且errno被设置为不同于EINTR或EAGAIN，表示某种严重的错误；

## 用write来写

如果写的count比SSIZE_MAX还大，write调用的结果是未定义的。

当一个write调用返回时，内核已将所提供的缓冲区数据复制到了内核缓冲区中，但没有保证数据已经写入到目的文件。read读也会先从这个缓冲区读。

内核会尝试将延迟写的风险最小化，为保证数据适时写入，内核创建了一种缓存最大时效机制，并将所有脏的缓存在它们超过给定时效前写入磁盘，用户可以通过\proc\sys\vm\dirty_expire_centiseconds来配置这个值，这个值以十毫米计。

## 同步I/O

调用fsync可以保证fd对应文件的脏数据回写到磁盘上。该调用回写数据以及建立的时间戳和inode中的其他属性等元数据。在驱动器确认数据已经完全写入之前不会返回。

linux还提供了fdatasync系统调用，其完成的事情和fsync一样，区别在于它仅写入数据，该调用不保证元数据同步到磁盘上，故此可能快一些。

成功时都返回0，失败时都返回-2，并将errno设置为以下三个值之一：

EBADF：给定的文件描述符不是一个可写入的合法描述符；

EINVAL：给定的文件描述符不支持同步；

EIO：同步时发生了一个底层I/O错误；

一般来讲，即使在相应文件系统上实现了fdatasync()而未实现fsync()，在调用fsync时也会失败，偏执的应用可能会在fsync返回EINVAL时尝试用fdatasync()。

对于linux来说，sync()一定要等到所有的缓冲区都写入才返回，因此，调用一次sync就够了。

O_SYNC标志在open中使用，使得文件上的IO操作同步。它看起来就像是在每个write操作后都隐式的执行fsync，因此会存在一定的时间开销。POSIX为open定义了另外两个与同步相关的标志：O_DSYNC和O_RSYNC。在linux上，这些标志与O_SYNC同义。O_DSYNC指定在每次写操作后只有普通数据被同步，元数据则不同步。O_RSYNC要求读请求像写请求那样进行同步，因此该标志只能和O_SYNC或O_DSYNC一起使用。

## 直接I/O

在open中使用O_DIRECT会使内核最小化I/O管理的影响。使用该标志时，I/O操作将忽略页缓存机制，直接对用户空间缓冲区和设备进行初始化。所有的I/O将是同步的；操作在完成之前不会返回。当使用直接I/O时，请求长度，缓冲区对齐和文件偏移必须是设备扇区大小的整数倍。

## 定位读写

linux提供了两种read和write的变体来替代lseek，每个调用都需要读写的文件位置为参数。完成时不修改文件位置。

```c
#include <unistd.h>
ssize_t pread(int fd, void *buf, size_t count, off_t pos);
```

从fd的pos位置读取count个字节到buf中。

```c
#include <unistd.h>
ssize_t pwrite(int fd, const void *buf, size_t count, off_t pos);
```

从fd的pos文件位置写count个字节到文件中。

## I/O多路复用

linux提供了一个poll的近似调用ppoll：

```c
#include <sys/poll.h>
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
```

尽管poll和select完成一样的工作，但前者仍然优于后者：

- poll无需使用者计算最大的文件描述符值加一和传递该参数；
- poll在应对较大值的文件描述符使更具有效率；
- select的文件描述符集合是静态大小的，而poll的文件描述符数组大小可控；
- 若用select，文件描述符集合会在返回时重新创建，这样每个调用之后都必须重新初始化它们，poll分离了输入和输出，数组无需改变即可重用；
- select的timeout参数在返回时是未定义的，pselect没有这个问题；

但select也有几个优点：

- 有些Unix系统不支持poll所以select的可移植性更好；
- select提供了更好的超时方案，直到微秒级，ppoll和pselect理论上都提供纳秒级精度，但实际上没有任何调用可以可靠的提供哪怕微秒级的精度

## 内核内幕

Linux内核实现I/O主要关注三个主要的内核子系统：虚拟文件系统（VFS）、页缓存和页回写。虚拟文件系统是一种linux内核的文件操作的抽象机制，它允许内核在无需了解文件系统类型的情况下，使用文件系统函数和操作文件系统数据。实现的方法是使用一种通用文件模型。基于函数指针和各种面向对象方法，通用文件模型提供了一种linux内核文件系统必须遵循的框架。当应用发起了一个read系统调用，就开始了一段奇妙的旅程。C库提供了系统调用的定义，在编译器调用转化为适当的陷阱态，当一个用户空间进程转入内核态，则转交系统调用处理器处理，最终交给read系统调用，内核确认文件描述符对应的对象类型，内核调用与相关类型对应的read函数。

页缓存是一种在内存中保存最近在磁盘文件系统上访问过的数据的方式。交换和缓存间的平衡可以通过/proc/sys/vm/swappiness来调整。页缓存利用的是时间局部性，空间局部性则是数据连续使用的性质，基于这个原理，内核实现了页缓存预读技术。当内核从磁盘读取一块数据时，也会读取接下来的一两块数据。内核管理预读也是动态的，如果它注意到一个进程持续使用预读来的数据，内核就会增加预读窗口，进而预读更多的数据。预读窗口最小为16kb，最大为128kb。

内核使用缓冲区来延迟写操作，当一个进程发起写请求，数据被拷贝进一个缓冲区，并将缓冲区标记为脏的，这意味着内存中拷贝被磁盘上的新。最终脏数据要写回磁盘，以下两个条件触发回写：

- 当空闲内存小于设定阈值时；
- 当一个脏的缓冲区寿命超过设定阈值时；

回写由一些叫做pdflush的内核线程操作。当以上条件出现时，pdflush线程被唤醒并开始将脏的缓冲区提交给磁盘，直到没有触发条件被满足。可能同时存在多个pdflush线程在回写，这么做是为了更好的利用并行性。

# 缓冲输入输出

## 关闭流

fcloseall关闭所有和当前进程相关联的流，包括标准输入输出和错误：

```c
#include <stdio.h>
int fcloseall(void);
```

## 从流中读取数据

以下函数可以将字符放回流中：

```c
#include <stdio.h>
int ungetc(int c, FILE *stream);
```

成功时返回c，失败返回EOF。

## 清洗一个流

fflush只是把用户缓冲的数据写入到内核缓冲区，效果和没有用户缓冲区一样，这并不保证数据能够写入到物理介质，如果需要的话可以使用fsync这一类函数。一般而言是在调用fflush之后立即调用fsync。

# 高级文件I/O

## Event Poll接口

由于poll和select的局限，2.6内核引入了event poll机制。poll和select每次调用都需要所有被监听的文件描述符。内核必须遍历所有被监视的文件描述符。当这个表变得很大时，每次调用的遍历就成了明显的瓶颈。epoll把监听注册从实际监听中分离出来，一个系统调用初始化一个epoll上下文，另一个从上下文中加入或删除需要监视的文件描述符，第三个执行执行真正的事件等待。

创建一个新的epoll示例：

```c
#include <sys/epoll.h>
int epoll_create(int size)；
int epoll_create1(int flags);
```

成功后会创建一个epoll实例，返回与该实例相关联的文件描述符。size告诉内核需要监听的文件描述符数目，但不是最大值。出错时返回-1，设置errno为下列值之一：

- EINVAL：size不是正数
- ENFILE：系统达到打开文件数的上限
- ENOMEN：没有足够的内存来完成该次操作

epoll_create1不再需要文件描述符数目，flag为0代表默认行为，为EPOLL_CLOEXEC设置close-on-exec标志，防止子进程继承fd。

epoll_ctl可以向指定的epoll上下文加入或者删除文件描述符：

```c
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

头文件中定义了epoll event结构体：

```c
struct epoll_event {
	__u32 events;
	union {
		void *ptr;
		int fd;
		__u32 u32;
		__u64 u64;
	} data;
};
```

该调用成功将关联epoll实例和epfd，op指定要对fd进行的操作，以下是op的有效值：

- EPOLL_C TL_ADD：把fd指定的文件添加到epfd指定的epoll实例监听集中，监听event中定义的事件；
- EPOLL_CTL_DEL：把fd指定的文件从epfd指定的epoll监听集中删除；
- EPOLL_CTL_MOD：使用event改变已有fd上的监听行为；

epoll_event上的data字段由用户使用，确认监听事件后data会被返回给用户。

epoll_wait等待事件：

```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

等待epfd中的文件fd上的事件，时限为timeout毫秒。成功返回，events指向包含epoll_event结构体，返回值是事件数。出错返回-1，并将errno设置为以下值：

- EBADF：epfd是无效文件描述符；
- EFAULT：进程对events指向的内存无写权限；
- EINTR：系统调用在完成前被信号中断；
- EINVAL：epfd不是有效epoll实例或者maxevents太大；

如果timeout为0，即使没有事件发生调用也立即返回，如果为-1，调用将等待直到事件发生。

如果epoll_ctl的参数event中的events设置为EPOLLE，fd上的监听称为边沿触发，相反的称为水平触发，考虑下面场景：

1. 生产者向管道写入1kb数据；
2. 消费者在管道调用epoll_wait等待pipe出现数据从而可读；

对于水平触发的监听，对epoll_wait的调用将立即返回以表明pipe可读，对于边沿触发的监听直到步骤1发生后才会返回。水平触发是默认行为，也是poll和select的行为，边沿触发需要一个不同的方式来写程序，通常利用非阻塞I/O，并需要仔细检查EAGAIN。

## 存储映射

Linux提供了madvise系统调用，可以让进程在如何访问映射区域上给内核一定的提示：

```c
#include <sys/mman.h>
int madvise(void *addr, size_t len, int advice);
```

advice是提示信息，可参考书中内容。

## 普通文件IO提示

```c
#include <fcntl.h>
int posix_fadvise(int fd, off_t offset, off_t len, int advice);
```

调用该函数会给出内核在fd的[offset, offset+len)范围内操作提示。如果len为0，在提示作用区间为[offset, length of file]。advice见书中内容。

posix_fadvise是2.6内核新加入的系统调用，在此之前，readahead可以完成posix_fadvise使用POSIX_FADV_WILLNEED选项时同样的功能，但不同于后者，readahead是linux独有的：

```c
#include <fcntl.h>
ssize_t readahead(int fd, off64_t offset, size_t count);
```

通过向内核传递一些操作提示，一些应用的效率可以获得提升。例如在读取一个文件大部分内容时，进程可以通过设置POSIX_FADV_WILLNEED要求内核把文件预读进页缓存中，I/O操作将在后台异步进行，当应用最终要访问文件时，访问操作可以立即返回，不会被阻塞。相反，在读取或写入大量数据后（比如硬盘上连续的视频数据流），进程可以设置POSIX_FADV_DONTNEED要求内核丢弃缓存中的内容，这样页缓冲区不会被这些进程不会再访问的数据填满。一个进程试图读取整个文件时设置POSIX_FADV_SEQUENTIAL要求内核大量预读，相反，如果一个进程知道自己将随机访问文件，设置POSIX_FADV_RANDOM告诉内核预读没有用只会带来开销。

## 同步及异步操作

Linux实现了aio，其提供了一系列函数来实现异步I/O提交以及在完成时收到通知，具体方法可参阅书中内容。注意，linux只支持O_DIRECT标志打开的文件上的aio。如果想要在普通文件上使用aio，则必须自己实现，最常用的方式是线程，需要完成以下任务：

1. 创建一个线程池处理所有的I/O；
2. 实现将I/O操作加入工作队列的一系列函数；
3. 使这些函数返回唯一的I/O描述符；
4. 完成后把操作的结果加入到一个结果队列中；
5. 实现一系列从结果队列获取状态信息的函数，使用最初返回的I/O描述符区分每个操作；

## I/O调度器和I/O性能

I/O调度器实现两个基本操作：合并和排序。合并是将两个或多个相邻的I/O请求合并为一个，排序是选取两个操作中相对更重要一个，并按照块号递增的顺序重新安排等待的I/O请求。

每次读请求必须返回最新的数据，因此，当请求的数据不在页缓存中时，读请求在数据从磁盘读出前一直阻塞，将这种性能损失称为读延迟。

这与写请求形成了对比，写请求（在其默认的非同步状态下）直到未来某个时刻才需要发起磁盘I/O。因此，从用户空间看，写请求可以流式传输，不受磁盘性能限制。这种行为进一步加剧了读取问题：当写操作流式传输时它们会抢占内核和磁盘的资源。这种现象称为“写操作饿死读操作”问题。

如果I/O调度器总是按照插入顺序排序请求，那么可能会导致远离的块被无限期饿死，因此I/O调度器采用机制来防止饿死现象。

最简单的方法就像2.4内核那样采用linux电梯调度法，在该方法中，如果队列中有一定数量的旧的请求，则停止插入新的请求，这样整体上可以做到平等对待每个请求，但在读的时候却增加了读延迟。

Deadline I/O调度器是为了解决2.4调度程序及传统的电梯调度算法的问题。Linux电梯算法维护了一个经过排序的I/O等待列表，队列首的I/O请求是下一个被调度的。Deadline I/O调度器保留了这个队列，并增加了两个新的队列：读FIFO队列和写FIFO队列。队列中的项按提交时间排序。FIFO队列中的每个请求都设置一个过期时间，读FIFO队列的过期时间设置为500ms，写队列则为5s。

当一个新的I/O请求提交后它被按序插入到标准队列，然后相应加入到读或写队列队尾。通常情况下，硬盘总是先发送标准队列头的I/O请求。当一个FIFO队列头的请求超出了所在队列的过期时间，I/O调度器停止从标准I/O队列中的调度请求，转而调度这个FIFO队列的队首请求。

Deadline I/O调度器虽然表现很好但并不完美。现在假设应用突然提交一个读请求，然后它即将到截止时间，I/O调度器响应请求在硬盘查找数据然后返回，再处理队列中的其他请求。这样的前后查找可能持续很长时间。如果硬盘能够停下来等待下一个读请求而不处理排序队列的请求，性能能够得到一定提升，但这种情况下应用程序被调度并提交下一个独立的读请求器时，I/O调度器很可能已经移动磁头了，这就导致了每次搜索都要进行不必要的寻道操作。

anticipatory I/O调度器的工作原理很清晰，它像Deadline一样开始，但它具有预测机制，当一个读操作提交时，anticipatory I/O调度器在它的终止期限前调度它，不同于Deadline I/O, anticipatory I/O调度器会等待6ms，如果程序在6ms之内对硬盘同一部分发出另一次读请求，读请求立即被响应，anticipatory I/O调度器继续等待，如果6ms没有收到读请求，调度器确认预测错误然后返回正常操作。

使用CFQ I/O调度器时，每个进程都有自己的FIFO队列，每个队列分配一个时间片，I/O调度器使用轮转方式访问并处理队列中的请求，直到队列的时间片耗尽或所有请求都被处理完。后一种情况I/O调度器会空转一段时间（默认10ms）等待当前队列中的新请求。在每个进程的队列中，同步请求比异步请求具有更高的优先级。CFQ调度器适合高负载情况，并且是这种情况的第一选择。

Noop I/O调度器是目前最简单的调度器，无论何种情况它都不进行排序操作，只是简单的合并。

默认的I/O调度器可以在启动时通过内核参数iosched来指定，有效选项是as，cfq，deadline和noop，也可以在运行时针对每个块设备进行选择，可以修改/sys/block/device/queue/scheduler来完成。

为了使I/O请求能够以有利于寻址操作的顺序提交，程序可以做不同的处理：

1. 完整路径；
2. inode编号；
3. 文件的物理块；

按路径排序是最简单的也是效率最低的方法，其粗略接近文件在磁盘上的物理位置分布，但如果考虑文件系统的碎片，那作用就比较小。

按照inode排序比路径排序更有效，两者序号的大小反映了两者物理块位置的大小。

按物理块排序首先要通过stat调用获取文件中块的数量，之后对每个逻辑块使用ioctl调用获取与之相关的物理块。

# 进程管理

## 进程ID

缺省情况下，内核将进程ID的最大值限制为32768，这是为了和老的Unix系统兼容，因为这些系统只使用16位来表示进程ID。系统管理员可以设置/proc/sys/kernel/pid_max来突破这个缺省的限制，但会牺牲一些兼容性。

## 运行新进程

当调用了fork之后，现代的Unix系统例如linux，采用了写时复制的方法，而不是对父进程空间进行整体复制。它的前提很简单：如果有多个进程要读取它们自己那部分资源的副本，那么复制是不必要的，每个进程只要保存一个指向这个资源的指针即可。如果一个进程要修改自己的那份资源副本，那么就会复制那份资源，并把复制的那份提供给进程。

在使用虚拟内存的情况下，写时复制是以页为基础进行的，所以，只要进程不修改它全部的地址空间，那么就不必复制整个地址空间。写时复制在内核的实现非常简单，与内核页相关的数据结构可以被标记为只读和写时复制。如果有进程试图修改一个页，那么就会产生一个缺页中断。内核处理缺页中断的方式就是对该页进行一次透明复制，这时会清除页面的COW属性，表示它不再被共享。

在写时复制出现之前，为了避免fork之后立即exec所造成的地址空间的浪费，BSD的开发者们在3.0的BSD系统中引入了vfork系统调用，除了子进程必须要执行一次对exec的系统调用或者调用_exit()退出之外，对vfork的调用所产生的结果跟fork是一样的。vfork会挂起父进程直到子进程终止或运行了一个新的可执行文件的映像。在这个过程中父进程和子进程共享相同的地址空间和页表。

## 终止进程

POSIX和C89都定义了终止当前进程的函数：

```c
#include <stdlib.h>
void exit(int status);
```

status参数用来标示进程退出的状态，有EXIT_SUCCESS和EXIT_FAILURE两个宏，由用户设置。

在终止进程之前，C语言函数执行以下关闭进程的工作：

1. 以在系统注册的逆序调用由atexit()和on_exit()注册的函数；
2. 清空所有已打开的标准I/O流；
3. 删除由tmpfile()创建的所有临时文件；

在完成这些在用户空间需要做的事情后exit就可以调用\_exit()来让内核处理终止进程的剩余工作：

```c
#include <unistd.h>
void _exit(int status);
```

当进程退出时，内核会清理进程所创建的、不再用到的任何资源，包括但不限于：申请的内存、打开的文件和System V的信号量。清理完成后内核摧毁进程并告知父进程其子进程终止。

注意，vfork的使用者终止进程必须使用\_exit()，而不是exit()。

终止进程的典型方式是跳转到程序结尾处，在C语言中，这发生在main函数返回时，但其实这种方式还是进行了系统调用，编译器会简单在最后的代码中插入一个_exit()。在main函数返回时明确给出一个状态值或调用exit()是一个良好的编程习惯，shell会根据这个返回值来判断命令是否成功的执行了。

如果进程收到一个信号，并且这个信号对应的处理函数是终止进程，进程也会终止，这样的信号包括SIGTERM和SIGKILL。

最后一种是进程被内核惩罚性的终止。内核就会杀死执行非法指令引起一个段错误或内存耗尽的进程。

如果进程调用了exec，那么atexit注册的函数列表会被清除，如果进程是通过信号结束的，这些注册的函数也不会被调用。注意，注册的函数不能调用exit，否则会引起无限的递归调用，如果需要提前结束进程应该调用\_exit()，但一般不推荐这么做。

sunOS 4定义了一个和atexit等价的函数on_exit，linux的glibc提供了对它的支持：

```c
#include<stdlib.h>
 int on_exit(void (*function)(int,void*),void
 *arg);
```

这个函数的工作方式和atexit一样只是注册的函数原型不同。最新版本的Solaris不再支持这个函数，应该使用atexit。

XSI扩展了POSIX，而且linux提供了waitid：

```c
#include<sys/wait.h>
 int waitid(idtype_t idtype,id_t id,siginfo_t
 *infop,int options);
```

其提供了更多的选项可以等待pid，也可以等待组ID的进程。

# 高级进程管理

## 进程调度

线程是进程中的运行单元，每一个线程都独自占有一个虚拟处理器：独自的寄存器组，指令寄存器和处理器状态。

## 让出处理器

允许进程主动让出处理器，并通知调度器选择另一个进程来运行：

```c
#include<sched.h>
 int sched_yield(void);
```

注意，在多数情况下，系统中没有其他就绪进程，让出的进程会直接恢复运行，由于这种不确定性，这一系统调用并不经常使用。

调用成功返回0，失败返回-1。

在2.6内核之前，调用sched_yield产生一个很简单的作用，如果有其他就绪进程，内核会运行该进程，并把当前进程放到就绪队列的队尾，当然也很有可能没有其他进程，那么当前进程会继续运行。2.6内核做了调整：

1. 进程是实时进程吗？如果是将其放到就绪队列的队尾，返回，如果不是到下一步；
2. 把该进程从就绪队列移出放到到期进程队列，也就是说只有当所有就绪队列运行，耗光时间片后，该进程才能和所有的到期进程一道重新进入就绪队列；
3. 调度下一个就绪队列运行；

改变的原因之一是为了预防乒乓的不合理情况，即两个进程都调用了sched_yield，假设它们是仅有的两个就绪进程，就会轮流调度这两个进程。

## 进程优先级

Unix把这个优先级称为nice value。nice value是在进程运行的时候指定的，linux调度器基于这样的原则来调度：高优先级的程序先运行。同时，nice值也指明了进程的时间片长度。合法的优先级在-20到19之间，默认为0，越低优先级越高，时间片越长。

只有拥有CAP_SYS_NICE能力（实际上就是root所有的进程）才能够使用负值inc，减少友好度，增加优先级，因此非root进程只能降低优先级。遇到错误，nice()返回-1，但-1也可能是成功时的返回值，因为为了区别成功与否，在调用之前应该对errno置零，调用后检查。

更好的解决方案是使用getpriority和setpriority。

作为进程优先级的补充，linux还允许进程指定I/O优先级。缺省情况下，I/O调度器使用进程友好度决定I/O优先级。但linux内核还有两个系统调用来单独获取和设置I/O：

```c
int ioprio_get(int which,int who)
int ioprio_set(int which,int who,int ioprio)
```

然而，内核没有导出这两个系统调用，glibc也没有提供用户空间接口，没有glibc的支持，函数用起来相当麻烦。

## 处理器亲和度

linux在进行进程调度的时候，如果一个进程曾在某个CPU上运行，进程调度器还应该尽量把它放在同一CPU上，因为处理器间的进程迁移会带来性能损失，其中最大的损失在于迁移带来的缓存效应。

处理器亲和度表明一个进程停留在同一处理器的可能性。进程从父进程继承处理器亲和度，linux提供两个系统调用来获取和设定进程的硬亲和度：

```c
#define _GNU_SOURCE
 #include <sched.h>
 typedef struct cpu_set_t;
 size_t CPU_SETSIZE;
 void CPU_SET (unsigned long cpu, cpu_set_t *set);
 void CPU_CLR (unsigned long cpu, cpu_set_t *set);
 int CPU_ISSET (unsigned long cpu, cpu_set_t *set);
 void CPU_ZERO (cpu_set_t *set);
 int sched_setaffinity (pid_t pid, size_t setsize,
 const cpu_set_t *set);
 int sched_getaffinity (pid_t pid, size_t setsize,
 const cpu_set_t *set);
```

如果pid为0，则得到当前进程的亲和度。具体用法见书中相关内容。

## 实时系统

如果一个系统受到操作期限，即请求和响应之间的最小量和命令次数的支配，就称该系统是实时的。

linux提供了两类实时调度策略作为正常默认策略的补充，头文件<sched.h>中的预定义宏表示各个策略：分别是SCHED_FIFO、SCHED_RR、SCHED_OTHER。

每一个进程都有一个与nice值无关的静态优先级，对于普通程序，值为0，对于实时程序，它为1到99。linux调度器始终选择最高优先级（静态优先级数值最大的）。如果一个优先级为50的进程在运行，此时一个优先级为51的进程就绪，调度器就会直接抢占当前进程，转而运行新到的高优先级进程。

先进先出策略是没有时间片时的简单的实时策略，只要没有高优先级进程就绪，FIFO类型进程就会持续运行，用宏SCHED_FIFO表示，因为缺少时间片，所以操作策略相当简单：

- 一个就绪的FIFO进程如果是系统中的最高优先级进程就会一直保持运行，特别的，如果一个FIFO进程就绪，它会直接抢占普通进程；
- FIFO进程持续运行直到阻塞或者调用sched_yield()，或者高优先级进程就绪；
- 当FIFO进程阻塞时，调度器将其移出就绪队列，当它恢复时，被插到相同优先级进程队列的末尾，因此，它必须等到高优先级或同等优先级进程停止运行；
- 当FIFO型进程调用sched_yield时，调度器将其放到同等优先级队列的末尾，因此，它必须等待其他同等优先级进程停止运行，如果没有其他同等优先级进程，sched_yield将没有作用；
- 当FIFO进程被抢占时，它在优先级队列中的位置不变，因此，一旦高优先级进程停止运行，被抢占的FIFO进程就会继续运行；
- 当一个进程变为FIFO型或者进程静态优先级改变，它将会被放到相应优先级队列的头部，因此，新来的FIFO进程能够抢占同等优先级进程。

轮转策略类似FIFO类型，仅仅引入了处理同等优先级进程的附加规则，使用SCHED_RR表示。调度器给每个RR进程分配一个时间片，当RR型进程耗光时间片时，调度器将其放在所在优先级队列的末尾。可以认为RR进程等同于FIFO进程，仅仅是在时间片耗尽的时候停止运行，排到同等优先级队列的末尾。

SCHED_OTHER代表标准调度策略，适用于默认的非实时进程，所有这些进程的静态优先级都为0。

SCHED_BATCH是批调度或空闲调度的策略，这种类型的进程只在系统中没有其他就绪进程时才会运行，即使那些进程已经耗光时间片。

进程可以通过sched_getscheduler()和sched_setscheduler()来操作调度策略：

```c
#include <sched.h>
 struct sched_param {
 /* ... */
 int sched_priority;
 /* ... */
 };
 int sched_getscheduler (pid_t pid);
 int sched_setscheduler (pid_t pid, int policy,
 const struct sched_param *sp);
```

POSIX定义的sched_getparam()和sched_setparam()可以获取和设置已有调度策略的相关参数：

```c
 #include <sched.h>
 struct sched_param {
 /* ... */
 int sched_priority;
 /* ... */
 };
以
设
int sched_getparam (pid_t pid, struct sched_param
 *sp);
 int sched_setparam (pid_t pid, const struct
 sched_param *sp);
```

linux提供两个系统调用来获得优先级范围，一个返回最小值，一个返回最大值，主要用于开发程序的兼容性：

```c
#include<sched.h>
intsched_get_priority_min(intpolicy);
 intsched_get_priority_max(intpolicy);
```

posix定义了获得进程时间片长度的接口：

```c
#include <sched.h>
 struct timespec {
 time_t tv_sec;
 /* seconds */
 long tv_nsec; /* nanoseconds */
 };
 int sched_rr_get_interval (pid_t pid, struct
 timespec *tp);
```

posix规定这个函数只有工作于SCHED_RR进程，但linux上它可以获得任意进程的时间片长度。

## 资源限制

linux提供了两个操作资源限制的系统调用，两个都是posix的标准调用，分别用getlimit和setlimit获取和设置：

```c
 #include <sys/time.h>
 #include <sys/resource.h>
 struct rlimit{
 rlim_t rlim_cur;/*softlimit*/
 rlim_t rlim_max;/*hardlimit*/
 };
 int getrlimit(int resource,struct rlimit *rlim);
 int setrlimit(int resource,const struct rlimit
 *rlim);
```

# 文件与目录管理

## 文件及其元数据

inode存储了与文件有关的元数据，可以使用ls命令的-i选项来获取一个文件的inode编号。

## 目录

在读取目录内容之前，需要创建一个由DIR对象指向的目录流：

```c
#include <sys/types.h>
 #include <dirent.h>
 DIR* opendir(const char *name);
```

之后，可以在给定目录流中获取该目录的文件描述符：

```c
#define _BSD_SOURCE /*or_SVID_SOURCE*/
 #include <sys/types.h>
 #include <dirent.h>
 int dirfd(DIR *dir);
```

成功返回目录流的文件描述符，错误时返回-1。注意，该函数是BSD的扩展，并不是标准POSIX函数。

可以使用readder来从给定目录流来依次返回目录项：

```c
#include <sys/types.h>
 #include <dirent.h>
 struct dirent* readdir(DIR *dir);
```

该结构体定义如下：

```c
struct dirent{
 ino_t d_ino;/*inodenumber*/
 off_t d_off;/*offsettothenextdirent*/
 unsigned short d_reclen;/*lengthofthis
 record*/
 unsigned char d_type;/*typeoffile*/
 char d_name[256];/*filename*/
};
```

POSIX只需要字段d_name。程序应连续调用readdir来获取目录每个文件，直到它们发现它们搜索的文件或直到目录读完，此时返回NULL。失败时，也返回NULL，为了区分这点，必须在每次调用readdir之前将errno设置为0，并在之后检查返回值和errno。

使用closedir关闭由opendir打开的目录流：

```c
#include <sys/types.h>
#include <dirent.h>
int closedir(DIR *dir);
```

成功调用会关闭由dir指向的目录流，包括目录的文件描述符，并返回0，失败时返回-1，并设置errno为EBADF。

这些都是C库提供的标准POSIX函数，在这些函数内部，则使用系统调用readdir()和getdents()：

```c
#include <unistd.h>
 #include <linux/types.h>
 #include <linux/dirent.h>
 #include<linux/unistd.h>
 #include<errno.h>
 /*
 *Not defined for user space: need to
 *use the _syscall3() macro to access.
 */
 int readdir(unsigned int fd, struct dirent
 *dirp,unsigned int count);
 int getdents(unsigned int fd,struct dirent
 *dirp,unsigned int count);
```

## 链接

目录中每个名字至inode的映射称为链接。链接数为0的文件在文件系统上没有对应的目录项，当文件链接数达到0时，文件被标记为空闲，并且磁盘块可重用。当进程已打开这样一个文件时，文件仍在文件系统中保留，若没有进程打开该文件，文件会被移除。

Linux内核通过使用链接计数和使用计数来进行管理，使用计数是文件被打开的实例数的计数，文件直到链接计数和使用计数均为0时才会从文件系统中移除。

另外一种链接是符号链接，它不是文件系统中文件名和inode的映射，而是运行时解释的更高层次的指针，符号链接可跨越文件系统。

## 设备节点

设备节点是应用程序与设备驱动交互的特殊文件。当程序在设备节点执行一般的Unix I/O时，内核以不同于普通文件I/O的方式处理请求。内核将该请求转交给设备驱动，设备驱动处理这些操作并返回结果。

每个设备节点都具有两个数值属性，分别是主设备号和次设备号。主次设备号与对应的设备驱动映射表已载入内核。如果设备节点的主次设备号不对应内核的设备驱动，在设备节点上的open请求返回-1并设置errno为ENODEV，称这样的设备是不存在的设备。

所有的linux系统上都有几个特殊的设备节点，这些设备节点是linux开发环境的一部分，并且作为linux abi的一部分出现。

空设备位于/dev/null，主设备号是1，次设备号是3，该设备文件的所有者是root但所有用户均可读写。内核会忽略所有对该设备的写请求，所有对该文件的读请求则返回文件终止符（EOF）。

零设备位于/dev/zero，主设备号为1，次设备号为5，与空设备一样，内核会忽略所有对零设备的写请求，读取该设备时会返回无限null字节流。

满设备位于/dev/full，主设备号为1，次设备号为7，与零设备一样读请求返回null字符，写请求却总触发ENOSPC错误，表明设备已满。

内核的随机数生成数设备位于/dev/random和/dev/urandom，它们的主设备号1，次设备号分别是8和9。内核的随机数生成器从设备驱动和其他源中收集噪声，内核将收集的噪声连结并做单向哈希，其结果存储在内核熵池中，内核保持池中预算数量的熵比特数。

读取/dev/random时则返回该池中的熵。该结果适于作为随机数生成器的种子，用来生成密钥或其他需要强熵加密的任务。

理论上，能从熵池获取足够数据并成功破解单向哈希的攻击者能够获得足够多的熵池中剩余熵的状态信息，尽管这样的攻击只是理论上存在，但内核仍能通过减少池中熵预算数量来应对这种可能的攻击，当预算数达到0时，读请求将阻塞，且对熵的预算足以能满足读取请求。

/dev/urandom不具有该特性，即使内核熵预算数量不足以完成请求，对该设备的读取请求仍会成功，仅仅那些对安全性要求较高的程序需要关注加密强熵，大多数程序使用/dev/urandom而非/dev/random。在没有填充内核熵池的I/O行为发生时，读取/dev/random会阻塞一段很长的时间，这种情况在无盘、无头服务器中比较常见。

## 带外通信

有时候需要与基本数据流外的文件通信，比如，对串口设备来讲读取将对串口远端硬件读取，写入讲向那硬件发送数据。

通过系统调用ioctl来进行带外通信：

```c
#include<sys/ioctl.h>
 int ioctl(int fd,int request,...);
```

## 监视文件事件

linux提供为监视文件接口inotify，利用它可以监控文件的移动、读取、写入和删除操作。inotify机制在内核2.6.13中被引入，由于程序在操作普通文件时的操作（尤其是select和poll）也使用inotify，因此该机制非常灵活且易于使用。

在使用之前必须初始化，系统调用inotify_init()来初始化并返回初始化实例指向的文件描述符：

```c
#include<inotify.h>
int inotify_init(void);
```

错误时返回-1，并使用下列代码设置errno：

- EMFILE：inotify达到用户最大的实例数限制；
- ENFILE：文件描述符数达到系统最大限制；
- ENOMEM：剩余内存不足，无法完成请求；

初始化之后会设置监视，监视由监视描述符表示，由一个标准的UNIX路径和一个相关联的监视掩码组成，该掩码通知内核，该进程关心何种事件。其可以监视文件和目录，监视目录时，inotify会报告目录本身和该目录下所有文件的事件。

系统调用inotify_add_watch在文件或目录path上增加一个监视，监视事件由mask确定，监视实例由fd指定：

```c
#include <inotify.h>
int inotify_add_watch(int fd, const char *path,
 uint32_t mask);
```

成功时返回新建的监视描述符，失败时返回-1，并使用下列值设置errno：

- EACCESS：不允许读取path指定的文件；
- EBADF：fd是无效的inotify实例；
- EFAULT：无效的path指针；
- EINVAL：监视掩码mask包含无效的事件；
- ENOMEM：剩余内存不足；
- ENOSPC：inotify监视总数达到用户限制

监视掩码由一个或多个inotify事件的二进制或运算生成，其定义在\<inotify.h\>：

- IN_ACCESS：文件读取；
- IN_MODIFY：文件写入；
- IN_ATTRIB：文件元数据（例如所有者、权限或扩展属性）已改变；
- IN_CLOSE_WRITE：文件已关闭且曾以写入模式打开；
- IN_CLOSE_NOWRITE：文件已关闭且未曾以写入模式打开；
- IN_OPEN：文件已打开；
- IN_MOVED_FROM：文件已从监视目录中移出；
- IN_MOVED_TO：文件已移入监视目录；
- IN_CREATE：文件已在监视目录创建；
- IN_DELETE：文件已从监视目录删除；
- IN_DELETE_SELF：监视对象本身已删除；
- IN_MOVE_SELF：监视对象本身已移动；

下面的事件已定义，单个值中包括两个或多个事件：

- IN_ALL_EVENTS：所有合法的事件；
- IN_CLOSE：所有涉及关闭的事件；
- IN_MOVE：所有涉及移动的事件；

使用定义在\<inotify.h\>中的结构inotify_event来描述inotify事件：

```c
#include<inotify.h>
 structinotify_event{
     int wd; /*watch descriptor*/
     uint32_t mask; /*mask of events*/
     uint32_t cookie; /*unique cookie*/
     uint32_t len; /*size of ’name’ field*/
     char name[]; /*null-terminated name*/
 };
```

注意，len与字符串name长度不一样，name使用null字符进行填充，以保证后续inotify_event能够正确对齐，因此在计算数组中下个inotify_event结构的偏移时，必须使用len，而不能使用strlen。

获取inotify事件很简单，仅需要读取与inotify实例关联的文件描述符即可。

删除inotify监视，通过inotify_rm_watch：

```c
#include <inotify.h>
int inotify_rm_watch(int fd, uint32_t wd);
```

未处理事件队列大小可以通过在inotify实例文件描述符上执行ioctl来获取，请求的第一个参数获得以无符号整数表示的队列的字节长度。示例：

```c
unsigned int queue_len;
int ret;
ret = ioctl(fd, FIONREAD, &queue_len);
if(ret < 0)
 	perror(”ioctl”);
else
 	printf(”%ubytespendinginqueue\n”,
 		queue_len);
```

注意，请求返回的是队列的字节大小，而非队列的事件数。

销户inotify实例及其关联的监视和关闭实例的文件描述符一样简单：

```c
int ret;
/* ’fd’ was obtained via inotify_init( ) */
ret = close (fd);
if (fd ==-1)
	perror (”close”);
```

# 内存管理

## 进程地址空间

一个进程不能访问一个处在二级存储中的页，除非这个页和物理内存中的页相关联。如果一个进程尝试访问这样的页面，那么存储器管理单元（MMU）会产生一个页错误。然后内核会透明的从二级存储换入需要的页面。因为一般来说虚拟存储器总是比物理内存大，所以内核也经常把页面从物理内存换出到二级存储，从而为将要换入的页面腾出空间。内核总是倾向于将未来最不可能使用的页换出，以此来优化性能。

当一个进程试图写某个共享的可写页时，可能发生两种情况。最简单的是内核允许这个操作，这种情况下所有共享这个页的进程都能看到这次写操作的结果。另一种情况是MMU会截取这次写操作并产生一个异常：作为回应，内核会透明的创造一份这个页的拷贝以供该进程进行写操作。我们将这种方法叫做写时拷贝。

内核将某些具有相同特征的页组织成块，例如读写权限，这些块叫做存储器区域、段或者映射。下面是一些在每个进程都可以看到的存储器区域：

- 文本段：包含一个进程的代码、字符串、常量和一些只读数据。在linux中文本段被标记为只读，并且直接从目标文件映射到内存中；
- 堆栈段包括一个进程的执行栈，随着栈的深度动态的伸长或收缩，执行栈包括了程序的局部变量和函数的返回值；
- 数据段又叫堆，包含一个进程的动态存储空间，这个段是可写的，并且大小可变，这部分空间往往是由malloc分配的；
- BSS段包含了没有被初始化的全局变量。linux从两个方面优化这些变量，首先因为附加段是用来存放没有被初始化的数据的，所以链接器ld实际不会讲特殊的值存放在对象文件中，这样可以减小二进制文件的大小，其次，当这个段被加载到内存时内核只需要简单的根据写时复制原则将它们映射到一个全是0的页上，这样就十分有效的设置了这些变量的初始值；
- 大多数地址空间含有很多映射文件，可以看看/proc/self/maps或者pmap程序的输出。

## 动态内存分配

数据的对齐是数据地址和由硬件确定的内存块之间的关系。一个变量的地址是它大小的倍数时就叫自然对齐。例如一个32bit长的变量，如果它的地址是4（字节）的倍数（即地址的低两位是0），那么就是自然对齐了。所以，如果一个类型的大小是2n个字节，那么它的地址中至少低n位是0。

大多数情况下，编译器和C库会自动处理对齐问题，Posix规定通过malloc、calloc和realloc返回的内存空间对于C中的标准类型都应该是对齐的。在linux中这些函数返回的地址在32位系统以8字节为边界对齐，在64位系统是以16字节为边界对齐。

对于更大的边界，比如页面，需要动态的对齐。Posix 1003.1d提供一个叫做posix_memalign的函数：

```c
#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <stdlib.h>
int posix_memalign (void **memptr,
 size_t alignment,
 size_t size);
```

对于这个函数，errno不会被设置，而是直接在返回值中给出。由posix_memalign获得的内存通过free释放。

在posix定义posix_memalign之前，BSD和SunOS分别提供了如下了接口：

```c
void * valloc (size_t size);
void *memalign(size_t boundary,size_t size);
```

valloc的功能和malloc一样，但返回的地址是页面对齐的。使用如下：

```c
structship *pirate,*hms;
pirate = valloc(sizeof(structship));
if(!pirate){
  perror(”valloc”);
  return-1;
}
hms=memalign(getpagesize( ),sizeof(struct
 ship));
if(!hms){
 perror(”memalign”);
 free(pirate);
 return-1;
}
 /*use’pirate’and ’hms’...*/
free(hms);
free(pirate)
```

在linux中这两个函数获得的内存可以通过free释放，但在别的unix系统中却未必，一些系统没有提供一个足够安全的机制来释放这些内存。只有为了移植到更老的系统上时，才可以使用这两个函数，否则应该优先选择posix_memalign。

非标准类型的对齐要求比自然对齐有着更多的要求：

- 一个结构的对齐要求和它的成员中最大的那个类型是一样的；
- 结构体也引入了填充的需求，以此来保证每一个成员都符合各自的对齐要求；
- 一个联合的对齐和联合里最大的类型一致；
- 一个数组的对齐和数组里的元素类型一致；

## 数据段的管理

```c
#include<unistd.h>
int brk(void* end);
void* sbrk(intptr_tincrement);
```

这些函数继承了一些老版本Unix系统中函数的名字，那时堆栈还在同一个段，堆和栈的分界线叫做中断或中断点。调用brk会设置中断点的地位为end，成功返回0，失败返回-1并设置errno为ENOMEM。调用sbrk将数据段末端增加increment字节，increment可正可负。sbrk返回修改后的断点。尽管posix和c都没有定义这些函数，但几乎所有的Unix系统都至少支持其中之一。

## 匿名存储器映射

glibc的内存分配使用了数据段和内存映射。实现malloc最经典的方法就是将数据段分为一系列大小为2的幂的块，返回最小的符合要求的那个块来满足请求。释放则是简单将这块标记为未使用。如果相邻的分区都是空闲的，它们会被合成一个更大的分区。如果堆的最顶端是空的，系统可以用brk来降低断点，使堆收缩，将内存返回给系统。对于较大的分配，glibc并不使用堆而是创建一个匿名内存映射。实际上，一个匿名内存映射只是一块已经用0初始化的大的内存块。

其他Unix系统（例如BSD），并没有MAP_ANONYMOUS标记，相反，它们使用特殊的设备文件/dev/zero实现了一个类似的方案，linux也支持此种方案。

## 高级存储器分配

调用mallopt会将由param确定的存储管理相关的参数设置为value：

```c
#include <malloc.h>
int mallopt(int param,int value);
```

Linux目前支持六种param值，均被定义在malloc.h中。程序必须要在调用malloc或是其他内存分配函数前使用mallopt。

Linux提供了两个用来控制glibc内存分配系统的底层函数，第一个函数允许程序查询一块已分配内存中由多少可用字节：

```c
#include <malloc.h>
size_t malloc_usable_size(void *ptr);
```

第二个函数允许程序强制glibc归还所有可释放的动态内存给内核：

```c
#include <malloc.h>
int malloc_trim(size_t padding);
```

## 调试内存分配

程序可以设置MALLOC_CHECK环境变量开启存储系统额外的调试功能。

Linux提供了mallinfo函数来获得关于动态存储分配系统的统计数据：

```c
#include <malloc.h>
struct mallinfo mallinfo (void);
```

Linux也提供了stats函数，将跟内存相关的统计数据打印到标准错误输出：

```c
#include <malloc.h>
void malloc_stats (void);
```

## 基于栈的分配

如果要在一个栈中实现动态分配内存，使用系统调用alloca：

```c
#include <alloca.h>
void* alloca(size_t size);
```

由于这块内存是在栈中的，当调用它的函数返回时，这块内存将被自动释放。

注意，不能使用由alloca得到的内存来作为一个函数调用的参数。

alloca常见的用法是用来临时复制一个字符串，由于这种需求比较多，linux专门提供了strdup来将一个给定的字符串复制到栈中：

```c
#define _GNU_SOURCE
#include <string.h>
char* strdupa(const char *s);
char* strndupa(const char *s, size_t n);
```

## 存储器操作

最常用的是memset，用来把从s指向区域开始的n个字节设置为c，并返回s：

```c
#include <string.h>
void* memset(void *s,int c,size_tn);
```

bzero是早期由BSD引入的相同功能的函数，现在已被淘汰。

memcmp比较两块内存是否相等：

```c
#include <string.h>
int memcmp(const void* s1, const void* s2,
size_t n);
```

BSD同样提供了相同功能的接口bcmp。

memmove复制src的前n个字节到dst返回dst：

```c
#include <string.h>
void* memmove(void* dst,const void* src,
size_t n);
```

同样，BSD提供的是bcopy。

memcpy是变种，强制dst和src不能重叠，如果重叠，结果是未定义的：

```c
#include <string.h>
void* memcpy(void *dst,const void* src,size_t
n);
```

另一个安全的复制函数是memccpy，当它发现字节c在src指向的前n个字节中时会停止拷贝，返回指向dst中c后一个字节的指针，或者当没有找到c时返回NULL：

```c
#include <string.h>
void* memccpy(void* dst,const void* src,int
c,size_t n);
```

最后可以使用mempcpy来跨过拷贝的内存：

```c
#define _GNU_SOURCE
#include <string.h>
void* mempcpy(void* dst,const void* src,
size_t n);
```

memchr和memrchr可以在内存块中搜索一个给定的字节：

```c
#include <string.h>
void* memchr(const void* s, int c, size_t n);
#define _GNU_SOURCE
void* memrchr(const void* s,int c, size_t n);
```

memrchr与memchr不同的是，它从s指定的内存开始反向搜索n个字节。对于更加复杂的搜索，可以使用memmem函数，它可以在一块内存中搜索任意的字节数组。

memfrob将s指向的位置开始的n个字节，每个都与42进行异或操作来对数据加密，函数返回s，再次对相同的区域调用memfrob可以将其转换回来：

```c
#define _GNU_SOURCE
#include <string.h>
void* memfrob(void* s,size_t n);
```

这个函数用于数据加密是不合适的，它的使用仅限于对于字符串的简单处理，它是GNU标准函数。

## 内存锁定

mlock将锁定addr开始长度为len个字节的虚拟内存，来保证它们不会被交换到磁盘：

```c
#include <sys/mman.h>
int mlock(const void* addr,size_t len);
```

一个由fork产生的子进程不从父进程继承锁定的内存，然而由于linux对地址空间的写时复制，子进程的页面被锁定在内存中直到子进程对它们执行写操作。

如果一个进程想在物理内存中锁定它的全部地址空间，可以使用mlockall：

```c
#include <sys/mman.h>
int mlockall(int flags);
```

flags参数的行为如下：

- MCL_CURRENT：如果设置该值，会将所有已被映射的页面锁定进程地址空间中；
- MCL_FUTURE：如果设置该值，会将未来映射的页面也锁定在进程地址空间中；

POSIX提供了两个接口来将页从内存中解锁，允许内核根据需要将页换出至硬盘中：

```c
#include <sys/mman.h>
int munlock(const void* addr,size_t len);
int munlockall(void);
```

因为内存的锁定影响一个系统的整体性能，所以linux对于一个进程能够锁定的页面数进行了限制。拥有CAP_IPC_LOCK权限的进程能够锁定任意多的页面。没有这个权限的进程只能锁定RLIMIT_MEMLOCK个字节。

出于调试的需要，linux提供了mincore函数来确定一个给定范围的内存是在物理内存还是被交换到了硬盘中：

```c
#include <unistd.h>
#include <sys/mman.h>
int mincore(void *start,
size_t length,
unsigned char* vec);
```

# 时间

内核从三个角度来度量时间：

- 墙上时间（或真实时间）：真实世界的实际时间和日期；
- 进程时间：进程消耗的时间；
- 单调时间：时间源严格线性增长，包括linux的大多数系统，使用计算机的正常运行时间；

posix定义了一种在运行时确定系统计时器频率的机制：

```c
long hz;
hz = sysconf(_SC_SCK_TCK);
```

## 时间的数据结构

最简单的数据结构是time_t，其表示自大纪元以来流逝的秒数。

timeval结构对time_t进行了扩展，达到了毫秒级精度，定义如下：

```c
#include <sys/time.h>
struct timeval{
	time_t tv_sec;/*seconds*/
	suseconds_t tv_usec;/*microseconds*/
};
```

timespec将精度提高到了纳秒级：

```c
#include <time.h>
struct timespec{
	time_t tv_sec;/*seconds*/
	long tv_nsec;/*nanoseconds*/
};
```

## POSIX时钟

clockid_t类型表示了特定的POSIX时钟，linux支持其中四种：

- CLOCK_MONOTONIC：一个不可被任何进程设置的单调增长的时钟；
- CLOCK_PROCESS_CPUTIME_ID：一个处理器提供给每个进程的高精度时钟；
- CLOCK_REALTIME：系统级真实时钟；
- CLOCK_THREAD_CPUTIME_ID：和每个进程的时钟类似，但是是线程独有的；

## 时间源精度

SIX定于了clock_getres来取得给定时间源的精度：

```c
#include<time.h>
int clock_getres(clockid_t clock_id,
struct timespec *res);
```

## 取得当前时间

最简单的方式是time函数：

```c
#include <time.h>
time_t time (time_t *t);
```

返回当前时间，以从大纪元以来用秒计的秒数来表示。

gettimeofday扩展了time，在其基础上提供了微妙精度支持：

```c
#include <sys/time.h>

int gettimeofday (struct timeval *tv,
struct timezone *tz);
```

POSIX提供了clock_gettime来取得一个指定时间源的时间，然而更有用的是该函数可以达到纳秒级精度：

```c
#include <time.h>

int clock_gettime (clockid_t clock_id,
struct timespec *ts);
```

times系统调用取得正在运行的当前进程及其子进程的进程时间：

```c
#include <sys/times.h>
struct tms {
	clock_t tms_utime; /* user time consumed */
	clock_t tms_stime; /* system time consumed */
	clock_t tms_cutime; /* user time consumed by
		children */
	clock_t tms_cstime; /* system time consumed
		by children */
};
clock_t times (struct tms *buf);
```

## 设置当前时间

与time相对的是stime：

```c
#define_SVID_SOURCE
#include<time.h>
int stime(time_t *t);
```

调用需要发起者拥有CAP_SYS_TIME权限，一般只有root用户才有该权限。

与gettimeofday对应的是settimeofday：

```c
#include <sys/time.h>

int settimeofday (const struct timeval *tv ,
const struct timezone *tz);
```

另外还有clock_settime：

```c
#include <time.h>
int clock_settime(clockid_t clock_id,
	const struct timespec* ts);
```

## 玩转时间

asctime将tm结构体分解时间转换成一个ASCII字符串：

```c
#include<time.h>
char* asctime(const struct tm* tm);
char* asctime_r(const struct tm* tm,char* buf);
```

mktime将tm结构体转换为一个time_t：

```c
#include<time.h>
time_t mktime(struct tm* tm);
```

ctime将一个time_t转换为一个ASCII表示：

```c
#include<time.h>
char* ctime(const time_t* timep);
char* ctime_r(const time_t* timep,char* buf);
```

gmtime将给出的time_t转换到tm结构体：

```c
#include<time.h>
struct tm* gmtime(const time_t* timep);
struct tm* gmtime_r(const time_t* timep,struct
tm* 0result);
```

localtime和localtime_r将给出的time_t表示为用户时区：

```c
#include<time.h>
struct tm* localtime(const time_t* timep);
struct tm* localtime_r(const time_t* timep,
struct tm* result);
```

difftime返回两个time_t的差值，并转换到双精度浮点型：

```c
include<time.h>
double difftime(time_t time1,time_t time0);
```

## 调校系统时钟

Unix提供了adjtime以指定的增量逐渐的调整时间。

## 睡眠和等待

sleep让发起进程睡眠指定秒数：

```c
#include<unistd.h>
unsigned int sleep(unsigned int seconds);
```

usleep实现微妙精度的睡眠。

linux不赞成使用usleep，提供了纳秒级精度的函数nanosleep：

```c
#define_POSIX_C_SOURCE199309
#include<time.h>
int nanosleep(const struct timespec* req,
struct timespec* rem);
```

posix提供了一个高级的睡眠函数clock_nanosleep：

```c
#include<time.h>
int clock_nanosleep(clockid_t clock_id,
int flags,
const struct timespec* req,
struct timespec* rem);
```

## 定时器

alarm是最简单的定时器接口，到时间后会将SIGALRM信号发送给调用进程。

在POSIX标准中，setitimer可以提供比alarm更多的控制，而且能够重启自身：

```c
#include <sys/time.h>
int getitimer (int which,
	struct itimerval *value);
int setitimer (int which,
	const struct itimerval* value,
	struct itimerval* ovalue);
```

最强大的定时器接口，毫无疑问来自于POSIX的时钟函数：timer_create建立定时器，timer_settime初始化定时器，timer_delete销毁它。

可以使用timer_gettime获取一个定时器的过期时间而不重新设置它。

POSIX中的接口timer_getoverrun可以来确定一个给定定时器目前的超时值。