# Unix基础知识

## 登录

系统在其口令文件（通常是/etc/passwd文件）中查看登录名，口令文件中的登录项由7个以冒号分隔的字段组成，依次是：登录名、加密口令、数字用户ID、数字组ID、注释字段、起始目录以及shell程序（bin/ksh）。系统从口令文件中相应用户登录项的最后一个字段了解应该为该登录用户执行哪一个shell。

## 文件和目录

目录是一个包含目录项的文件，逻辑上可以认为每个目录项都包含一个文件名，同时还包含说明该文件属性的信息。stat和fstat函数返回包含所有文件属性的一个信息结构。对于文件名，只有斜线（/）和空字符这两个字符不能出现在文件名中。

## 输入和输出

每当运行一个新程序，所有shell都为其打开3个文件描述符，即标准输入、标准输出以及标准错误。函数Open、read、write、lseek以及close提供了不带缓冲的I/O。这些函数都使用文件描述符。

## 出错处理

c标准定义了两个函数，用于打印出错信息：

```c
#include <string.h>
#incluce <stdio.h>
char *strerror(int errnum);
void perror(const char *msg);
```

strerror将errnum映射成一个出错消息字符串，并且返回此字符串的指针，perror基于当前errno的当前值，在标准错误上产生一条出错消息，然后返回。

## 用户标识

组文件通常是/etc/group。可调用getuid和getgid返回用户id和组id。

## 时间值

Unix使用过两种不同的时间值。日历时间，是自协调世界时1970年一月一日0时0分0秒一来经过的秒数累计值，使用time_t来保存这种值；进程时间，以时钟滴答计算，用clock_t来保存。time命令可以用来获取时钟时间、用户CPU时间和系统CPU时间。

# 文件I/O

## 文件描述符

对于内核而言，所有打开的文件都通过文件描述符引用。文件描述符的变化范围是0-OPEN_MAX-1。

## 函数open和openat

使用open或openat可以打开或者创建一个文件。

```c
#include <fcntl.h>
int open(const char *path, int oflag, .../* mode_t mode */);
int openat(int fd, const char *path, int oflag, .../* mode_t mode */);\
//成功返回文件描述符，否则为-1
```

oflag参数可用来说明此函数的多个选项，在头文件<fcntl.h>中定义：

- O_RDONLY：只读打开；
- O_WRONLY：只写打开；
- O_RDWR：读写打开；
- O_EXEC：只执行打开；
- O_SEARCH：只搜索打开；

这些常量中必须指定一个且指定指定一个，以下常量是可选的：

- O_APPEND：每次写时追加到文件尾端；
- O_CLOEXEC：把FD_CLOEXEC常量设置为文件描述符标志；
- O_CREAT：若此文件不存在则创建，使用此函数需要说明第三个参数mode，用mode指定新文件的访问权限位；
- O_DIRECTORY：如果path引用的不是目录，则出错；
- O_EXCL：如果同时指定了O_CREAT，而文件已存在，则出错。用此可测试一个文件是否存在，如果不存在则创建，这使得测试和创建两者成为一个原子操作；
- O_NOCTTY：如果path引用的是终端设备，则不将该设备分配作为此进程的控制终端；
- O_NOFOLLOW：如果path引用是一个符号链接，则出错；
- O_NONBLOCK：如果path引用的是一个FIFO、一个块特殊文件或一个字符特殊文件，则此选项为文件的本次打开操作和后续的I/O操作设置非阻塞方式；
- O_SYNC：使得每次write等待物理I/O操作完成，包括由该write操作引起的文件属性更新所需要的I/O；
- O_TRUNC：如果文件存在而且为只写或读写成功打开，则将其长度截断为0；
- O_DSYNC：每次write要等待物理I/O操作完成，如果该写操作不影响读取刚写入的数据，则不需要等待文件属性更新；
- O_RSYNC：使得每一个以文件描述符作为参数的read操作等待，直至所有对文件同一部分挂起的写操作都完成；

fd参数把open和openat函数区分开来，共有以下三种情况：

1. path指定的是绝对路径名，此时两者等价；
2. path指定的是相对路径名，fd参数指出了相对路径名在文件系统中的开始地址；
3. path指定的是相对路径名，fd参数具有特殊值AT_FDCWD，这种情况路径名在当前工作目录获取；

openat的存在希望解决两个问题，第一个是让线程可以使用相对路径打开目录中的文件，而不再只能打开当前工作目录，同一进程的不同线程共享当前的工作目录，因此很难让同一进程的线程在同一时间工作在不同目录，第二，可以避免time-of-check-to-time-of-use(TOCTTOU)错误。TOCTTOU错误的基本思想是如果有两个基于文件的函数调用，其中第二个依赖第一个调用的结果，那么程序是脆弱的，因为两个调用不是原子操作。

常量_POSIX_NO_TRUNC决定是要截断过长的文件名或是路径名，还是返回一个出错。

## 函数creat

可用creat函数创建一个新文件：

```c
#include <fctnl.h>
int creat(const char *path, mode_t mode);//成功返回只写打开的文件描述符，否则为-1
```

此函数等效于`open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);`

## 函数close

close函数关闭一个打开文件。

```c
#include <unistd.h>
int close(int fd);
```

关闭一个文件时还会释放该进程加在该文件上的所有记录锁。当一个进程终止时，内核自动关闭它所有的打开文件。

## 函数lseek

显式的为一个打开文件设置偏移量。

```c
#include <unistd.h>
off_t lseek(int fd, off_t offset, int whence);//成功返回新的文件偏移量，否则为-1
```

若whence是SEEK_SET，则将该文件的偏移量设置为距文件开始处offset个字节；若whence是SEEK_CUR，则将该文件的偏移量设置为其当前值加offset，offset可正可负；若为SEEK_END，则将偏移量设置为文件长度加offset，offset可正可负。

如果文件描述符指向一个管道、FIFO或网络套接字，则lseek返回-1，并将errno设置为ESPIPE。

## 函数read

该函数从打开文件中读数据。

```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t nbytes);//成功返回读到的字节数，否则为-1
```

## 函数write

该函数向打开文件写数据

```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t nbytes);//成功返回已写字节数，否则为-1
```

## 文件共享

内核使用三种数据结构表示打开文件，它们之间的关系决定了在文件共享方面一个进程对另一个进程可能产生的影响：

1. 每个进程在进程表中都有一个记录项，记录项中包含一张打开文件描述符表，可将其视为一个矢量，每个描述符占用一项，与每个文件描述符相关联的是文件描述符标志与指向一个文件表项的指针；
2. 内核为所有打开文件维持一张文件表，每个文件表项包含：文件状态标志、当前文件偏移量、指向该文件v节点表项的指针；
3. 每个打开文件或设备有一个v节点。v节点包含了文件类型和对此文件进行各种操作函数的指针。对于大多数文件，v节点还包含了该文件的i节点。

下图说明了这三张表的关系：

![](..\image\Unix\3-3-7.png)

如果两个进程打开了同一个文件，则有如下的关系：

![](..\image\Unix\3-3-8.png)

注意文件描述符和文件状态标志在作用范围方面的区别，前者只用于一个进程的一个描述符，后者则用于指向该给定文件表项的任何进程中的所有描述符。

## 原子操作

追加到一个文件，Unix系统提供了一种原子操作方法，即在打开文件时设置O_APPEND标志，这样就不用每次写之前调用lseek。

Single UNIX Specification包括了XSI扩展，该扩展允许原子性定位并执行I/O，pread和pwrite就是这种扩展。

```c
#include <unistd.h>
ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset); //返回读到的字节数，出错返回-1；
ssize_t pwrite(int fd, const void *buf, size_t nbytes, off_t offset); //返回已写的字节数，出错返回-1；
```

## 函数dup和dup2

下面两个函数都可以用来复制一个现有的文件描述符：

```c
#include <unistd.h>
int dup(int fd);
int dup2(int fd, int fd2);
//若成功返回新的文件描述符，出错返回-1
```

由dup返回的新文件描述符一定是当前可用文件描述符中的最小数值，对于dup2，可以用fd2指定新的描述符的值。如果fd2已打开，则先将其关闭，如果fd等于fd2，则dup2返回fd2，而不关闭它。否则，fd2的FD_CLOEXEC文件描述符标志就被清楚，这样fd2在进程调用exec时是打开状态。每个文件描述符都有它自己的一套文件描述符标志，新描述符的执行时关闭标志总是由dup函数清除。

复制一个描述符的另一种方法是fcntl函数，调用dup(fd)等效于fcntl(fd, F_DUPFD, 0)，而调用dup2(fd, fd2)等效于close(fd2); fcntl(fd, F_DUPFD, fd2);后一种情况并不完全相同，区别在于：

1. dup2是一个原子操作，而close和fcntl包括两个函数调用，有可能在close和fcntl之间调用了信号捕获函数，它可能修改文件描述符。
2. dup2和fcntl有一些不同的errno;

## 函数sync、fsync、和fdatasync

传统的Unix系统实现在内核中设有缓冲区高速缓存或页高速缓存，大多数磁盘I/O都通过缓冲区进行，当向文件写入数据时，内核通常先将数据复制到缓冲区，然后排入队列，晚些时候写入磁盘，这种方式叫做延迟写。通常，当内核需要重用缓冲区来存放其他磁盘块数据时，它会把所有延迟写数据块写入磁盘，为了保证磁盘实际文件系统与缓冲区中内容的一致性，系统提供了这几个函数：

```c
#include <unistd.h>
int fsync(int fd);
int fdatasync(int fd);
//成功返回0，否则为-1
void sync(void);
```

sync只是将所有修改过的块缓冲区排入写队列，然后就返回，并不等待实际写磁盘结束。通常，称为update的系统守护进程周期性（一般每隔30s）的调用sync函数，这就保证了定期冲洗内核的块缓冲区，命令sync(1)也调用sync函数。

fsync只对文件描述符fd指定的一个文件起作用，并且等待写磁盘操作结束才返回。

fdatasync类似fsync，但它只影响文件的数据部分，而除数据外，fsync还会同步更新文件的属性。

## 函数fcntl

此函数可以改变已经打开文件的属性。

```c
#include <unistd.h>
int fcntl(int fd, int cmd, .../* int arg */);
```

此函数有以下五种功能：

1. 复制一个已有的描述符（cmd=F_DUPFD或F_DUPFD_CLOEXEC）；
2. 获取/设置文件描述符标志（cmd=F_GETFD或F_SETFD）；
3. 获取/设置文件状态标志（cmd=F_GETFL或F_SETFL）；
4. 获取/设置异步I/O所有权（cmd=FGETOWN或FSETOWN）；
5. 获取/设置记录锁（cmd=F_GETLK、F_SETLK或F_SETLKW）；

## 函数ioctl

终端I/O是使用ioctl最多的地方。

```c
#include <unistd.h> /* System V*/
#include <sys/ioctl.h> /* BSD and linux */
int ioctl(int fd, int request, ...);
```

每个设备驱动程序可以定义它自己专用的一组ioctl命令，系统则为不同种类的设备提供通用的ioctl命令，下图总结了FreeBSD支持的通用ioctl命令的一些类别：

![](..\image\Unix\3-3-15.png)

## /dev/fd

打开文件/dev/fd/n等效于复制描述符n。

在下列函数调用中：

```c
fd = open("/dev/fd/0", mode);
```

大多数系统忽略它所指定的mode，而另外一些系统要求mode必须是所引用文件初始打开时所使用的打开模式的子集。因为上面的打开等效于`fd = dup(0);`

也可以用/dev/fd作为路径名参数调用creat，这与调用open时用O_CREAT作为第2个参数作用相同。

# 文件和目录

## 函数stat、fstat、fstatat和lstat

```c
#include <sys/stat.h>
int stat(const char *restrict pathname, struct stat *restrict buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *restrict pathname, struct stat *restrict buf);
int fstatat(int fd, const char *restrict pathname, struct stat *restrict buf, int flag);
//若成功返回0，否则-1
```

一旦给出pathname，stat函数返回与此命名文件有关的信息结构，fstat获得已在文件描述符fd上打开文件的有关信息，lstat类似于stat，但当命名的文件是一个符号链接时，返回该符号链接的有关信息，而不是该符号链接引用的文件的信息，fstatat为一个相对于当前打开目录（由fd参数指向）的路径名返回文件统计信息，flag参数控制着是否跟随着一个符号链接。当AT_SYMLINK_NOFOLLOW标志被放置，fstatat不会跟随符号链接，而是返回符号链接本身的信息，否则，默认情况返回是符号链接指向的实际文件的信息，如果fd参数的值是AT_FDCWD，并且pathname参数是一个相对路径名，fstatat会计算相对于当前目录的pathname参数，如果pathname是一个绝对路径，fd参数会被忽略。

buf指向一个我们必须提供的结构，其基本形式是：

```c
struct stat {
    dev_t     st_dev;     /* ID of device containing file */
    ino_t     st_ino;     /* inode number */
    mode_t    st_mode;    /* file type and mode */
    nlink_t   st_nlink;   /* number of hard links */
    uid_t     st_uid;     /* user ID of owner */
    gid_t     st_gid;     /* group ID of owner */
    dev_t     st_rdev;    /* device ID (if special file) */
    off_t     st_size;    /* total size, in bytes */
    blksize_t st_blksize; /* block size for filesystem I/O */
    blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
    time_t    st_atime;   /* time of last access */
    time_t    st_mtime;   /* time of last modification */
    time_t    st_ctime;   /* time of last status change */
};

```

timespec结构按照秒和纳秒定义了时间，至少包括以下两个字段：

```c
time_t tv_sec;
long tv_nsec;
```

## 文件类型

文件类型包括如下几种：

1. 普通文件。这种文件包含了某种形式的数据，至于数据是文本还是二进制，于UNIX内核并无区别，内容的解释由应用进行。
2. 目录文件。包含了其他文件的名字以及指向与这些文件有关信息的指针。对一个目录文件具有读权限的任一进程都可以读取该目录的内容，但只有内核可以之间写目录文件。
3. 块特殊文件。这种类型的文件提供对设备带缓冲的访问，每次访问以固定长度为单位进行。
4. 字符特殊文件。这种文件提供对设备不带缓冲的访问，每次访问长度可变。系统中的所有设备呀么是字符特殊文件，要么是块特殊文件。
5. FIFO。这种类型的文件用于进程间通信。
6. 套接字。这种类型文件用于网络通信。
7. 符号链接。这种类型的文件指向另一个文件。

文件类型信息包含在stat结构的st_mode成员中。可以用书中宏来确定文件类型。

## 设置用户ID和设置组ID

与一个进程相关联的ID有6个或更多，如图：

![](..\image\Unix\3-4-5.png)

- 实际用户ID和实际组ID标识我们是谁，这两个字段在登录时取自口令文件中的登录项。通常，在一个登录会话起期间这些值并不改变，但是超级用户进程有方法改变它们；
- 有效用户ID、有效组ID以及附属组ID决定了我们的文件访问权限；
- 保存的设置用户ID和保存的设置组ID在执行一个程序时包含了有效用户ID和有效组ID的副本。

每个文件有一个所有者和组所有者，所有者由stat结构中的st_uid指定，组所有者则由st_gid指定。

当执行一个程序文件时，进程的有效用户ID通常就是实际用户ID，有效组ID通常是实际组ID。但是可以在文件模式字st_mode中设置一个特殊标志，其含义是当执行此文件时将进程的有效用户ID设置为文件所有者的用户ID，与此类似，也可以将此文件的进程的有效组ID设置为文件的组所有者ID。在文件模式字中这两位被称为设置用户ID和设置组ID位。

## 文件访问权限

st_mode值也包含了对文件的访问权限位，每个文件都有9个访问权限位，可将其分为3类，如图：

![](..\image\Unix\3-4-6.png)

使用方式汇总如下：

- 第一个规则是当我们用名字打开任一类型的文件时，对该名字包含的每一个目录，包括隐含的当前目录都应具有执行权限，这就是为什么对于目录其执行权限位常被称为搜索位；
- 对一个文件的读权限决定了我们能否打开现有文件进行读操作；
- 对一个文件的写权限决定了我们能否打开现有文件进行写操作；
- 为了在open函数中对一个文件指定O_TRUNC标志，必须对该文件具有写权限；
- 为了在一个目录中创建一个新文件，必须对该目录具有写权限和执行权限；
- 为了删除一个现有文件，必须对包含该文件的目录具有写权限和执行权限，对该文件本身不需要读写权限；
- 如果用7个exec函数的任一一个执行某个文件，必须对该文件具有执行权限，该文件还必须是一个普通文件；

进程每次打开、创建或删除一个文件，内核都会进行如下测试：

1. 若进程的有效用户ID是0（超级用户），则允许访问；
2. 若进程的有效用户ID等有文件的所有者ID（也就是进程拥有此文件），那么如果所有者适当的访问权限位被设置，则允许访问，否则拒绝访问呢。适当的权限位指的是若进程为读而打开该文件，则用户读位应为1；若进程为写打开该文件，则用户写位应为1；若进程将执行该文件，则用户执行位应为1；
3. 若进程的有效组ID或进程的附属组ID之一等于文件的组ID，那么如果组适当的访问权限被设置，则允许访问，否则拒绝；
4. 若其他用户适当的访问权限位被设置则允许访问，否则拒绝；

## 新文件和目录的所有权

新文件的用户ID设置为进程的有效用户ID，组ID，POSIX.1允许实现选择下列之一为新文件的组ID：

1. 新文件的组ID可以是进程的有效组ID；
2. 新文件的组ID可以是它所在目录的组ID；

## 函数access和faccessat

正如前文所述，当用open打开一个文件时，内核以进程的有效用户id和有效组id为基础执行其访问权限测试。有时进程也希望按其实际用户id和实际组id来测试其访问能力。例如当一个用户设置用户id或者设置组id作为另一个用户运行时，比如一个进程设置用户以超级用户权限运行，它仍可能想验证其实际用户能够访问一个给定的文件，access和faccessat就是按实际用户id和实际组id进行访问权限测试的。

```c
#include <unistd.h>
int access(const char* pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
//若成功返回0，否则返回-1
```

如果测试文件是否已经存在，mode就为F_OK，否则mode就为图示的常值：

![](..\image\Unix\3-4-5.png)

在以下两种情况下，二者是相同的：一种是pathname参数是绝对路径，另一种是fd参数为AT_FDCWD，而pathname参数为相对路径。否则，faccessat计算相对于打开目录的pathname。

flag参数可以用于改变faccessat的行为，如果flag设置为AT_EACCESS，访问检查用的是调用进程的有效用户ID和有效组ID，而不是实际用户ID和实际组ID。

## 函数mask

umask函数为进程设置文件模式创建屏蔽字，并返回之前的值。

```c
#include <sys/stat.h>
mode_t umask(mode_t cmask);
```

参数cmask是上文列出的9个常量中的若干个按位或构成的。在进程创建一个新文件或新目录时，就一定会使用文件模式创建屏蔽字，在文件模式创建屏蔽字中为1的位，在文件mode中相应位一定被关闭。

## 函数chmod、fchmod、和fchmodat

这三个函数可以更改现有文件的访问权限：

```c
#include <sys/stat.h>
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);
//成功返回0，否则为-1
```

fchmodat和chmod在以下两种情况下是相同的：一种是pathname是绝对路径，另一种是fd取值为AT_FDCWD而pathname是相对路径。否则，fchmodat计算相对于打开目录的pathname。flag参数用来改变fchmodat的行为，当设置了AT_SYMLINK_NOFOLLOW标志时，fchmodat并不会跟踪符号链接。

为了改变一个文件的权限位，进程的有效用户ID必须等于文件所有者ID，或者该进程必须具有超级用户权限。参数mode是以下常量的按位与：

![](..\image\Unix\3-4-11.png)

## 粘着位

在Unix尚未使用分页技术的早期版本中，如果一个可执行程序的S_ISVTX被设置，那么当该程序第一次执行之后终止时，改程序正文部分（机器码）的一个副本仍被保存在交换区，以备下次执行能够很快的装载入内存。

现今的系统扩展了该位的使用范围，如果对一个目录设置了粘着位，只有对该目录具有写权限且满足以下条件之一，才能删除或重命名该目录下的文件：

- 拥有此目录；
- 拥有此文件；
- 是超级用；

目录/tmp和/var/tmp是设置粘着位的典型候选者。

## 函数chown、fchown、fchownat和lchown

该函数组用于改变文件的用户ID和组ID：

```c
#include <unistd.h>
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group, int flag);
int lchown(const char *pathname, uid_t owner, gid_t group);
//成功返回0，否则返回-1
```

注意，在符号链接情况下，lchown和fchownat（设置了AT_SYMLINK_NOFOLLOW标志）更改符号链接本身的所有者，而不是该符号链接所指向的文件的所有者。

fchmodat与chown和lchown在以下两种情况下是相同的：一种是pathname是绝对参数，一个是fd取值为AT_FDCWD而pathname是相对路径。在这两种情况下，如果flag参数设置了AT_SYMLINK_NOFOLLOW，fchownat与lchown行为相同，如果flag参数清除了AT_SYMLINK_NOFOLLOW标志，则fchownat与chown行为相同。如果fd参数设置为打开目录的文件描述符，并且pathname是一个相对路径名，fchownat计算相对于打开目录的pathname。

基于BSD的系统规定只有超级用户才能更改一个文件的所有者，这样做的原因是防止用户改变其文件的所有者从而摆脱磁盘空间限额对它们的限制。System V则允许任一用户更改他们所拥有文件的所有者。按照_POSIX_CHOWN_RESTRICTED的值，Posix.1允许在这两种形式的操作中选用一种，默认是施加限制。

\_POSIX_CHOWN_RESTRICTED常量可选的定义在\<unistd.h\>中，而且总是可以用pathconf或fpathconf函数查询。若_POSIX_CHOWN_RESTRICTED对指定的文件生效，则：

1. 只有超级用户进程能更改该文件的用户ID；
2. 如果进程拥有此文件（其有效用户ID等于该文件的用户ID），参数owner等于-1或文件的用户ID，并且参数group等于进程的有效组ID或进程的附属组ID之一，那么一个非超级用户进程可以更改该文件的组ID；

## 文件长度

stat结构成员st_size表示以字节为单位的文件的长度，此字段只对普通文件、目录文件和符号链接有意义。对于目录，文件长度通常是一个数（如16或512）的整倍数，对于符号链接，文件长度是在文件名中的实际字节数。

如今大多数现代的UNIX系统提供字段st_blksize和st_blocks。其中第一个是对文件I/O中较合适的块长度，第二个是分配的实际512字节块块数。

普通文件可以包含空洞，空洞是由所设置的偏移量超过文件尾端，并写入了某些数据后造成的。

## 文件截断

将一个文件的长度截断为零是一个特例，在打开文件时使用O_TRUNC标志可以做到这一点。为了截断文件可以调用函数truncate和ftruncate。为了截断文件可以调用函数truncate和ftruncate。

```c
#include <unistd.h>
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
```

这两个函数将现有文件长度截断为length，如果该文件原来的长度大于length，则超过该长度的数据将不再能访问，如果小于length，文件长度将增加。

## 函数link、linkat、unlink、unlinkat和remove

任何一个文件可以有多个目录项指向其i节点，创建一个指向现有文件的链接的方法是使用link函数或linkat函数：

```c
#include <unistd.h>
int link(const char *existingpath, const char *newpath);
int linkat(int efd, const char *existingpath, int nfd, const char *newpath, int flag);
//若成功返回0否则返回-1
```

当现有文件是符号链接时由flag参数来控制linkat函数是创建指向现有符号链接的链接还是创建指向现有符号链接所指向的文件的链接。如果flag参数设置了AT_SYMLINK_FOLLOW标志，就创建指向符号链接目标的链接，如果这个标志被清除了，则创建一个指向符号链接本身的链接。

创建新目录项和增加链接计数应当是一个原子操作。

虽然POSIX.1允许支持跨越文件系统的链接，但大多数实现要求现有的和新建的两个路径名在同一个文件系统中。

为了删除一个现有的目录项，可以调用unlink函数：

```c
#include <unistd.h>
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
//若成功返回0否则返回-1
```

这两个函数删除目录项，并将由pathname所引用的文件的链接计数减一，如果对该文件还有其他链接，则仍可以通过其他链接访问该文件的数据。如果出错，则不对该文件做任何更改。

只有当链接计数达到0时，该文件的内容才可以被删除。如果有进程打开了该文件，内容也不能被删除。

如果fd设置为AT_FDCWD，那么通过相对于调用进程的当前工作目录来计算路径名。

当flag参数被设置为AT_REMOVEDIR时，unlinkat可以类似于rmdir一样删除目录。

unlink这种特性经常被程序用来确保即使程序崩溃它创建的临时文件也不会遗留。进程用open或create创建一个文件，然后立即调用unlink，因为该文件仍旧是打开的，所以不会将其内容终止，只有当进程关闭文件或该文件终止时该文件内容才会被删除。

也可以用remove解除对一个文件或目录的链接，对于文件，remove的功能与unlink相同，对于目录，remove功能与rmdir相同。

```c
#include <unistd.h>
int remvoe(const char *pathname); //若成功返回0否则返回-1
```

## 函数rename和renameat

```c
#include <unistd.h>
int rename(const char *oldname, const char *newname);
int unlinkat(int oldfd, const char *oldname, int newfd, const char *newname);
//若成功返回0否则返回-1
```

需要注意以下几点：

1. 如果oldname指的是一个文件而不是目录，那么为该文件或符号链接重命名，在这种情况下如果newname已经存在，则它不能引用一个目录。如果newname已存在，而且不是一个目录，则先将该目录项删除后然后将oldname重命名为newname，对包含oldname的目录以及包含newname的目录调用进程必须具有写权限，因为将更改这两个目录。
2. 如果oldname指的是一个目录，那么为该目录重命名，如果newname已存在，则它必须引用一个目录，而且该目录应当是空目录，如果newname存在（而且是一个空目录），则先将其删除，然后将oldname重命名为newname。另外当为一个目录重命名时，newname不能包含oldname作为其路径前缀。
3. 如果oldname或newname引用符号链接，则处理的是符号链接本身。
4. 不能对.和..重命名。
5. 作为一个特例，如果oldname和newname引用同一文件，则函数不做更改而成功返回。

## 符号链接

硬链接直接指向文件的i节点，引入符号链接是为了避开硬链接的一些限制：

- 硬链接通常要求链接和文件位于同一文件系统；
- 只有超级用户才能创建指向目录的硬链接；

下图列出了各个函数是否处理符号链接：

![](..\image\Unix\3-4-17.png)

一个例外是同时用O_CREATE和O_EXCL两者调用open函数，这种情况下如果路径名引用符号链接，open将出错返回，errno将设置为EEXIST。

## 创建和读取符号链接

可用symlink或symlinkat创建一个符号链接：

```c
#include <unistd.h>
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
//若成功返回0否则返回-1
```

在创建此符号链接时并不要求actualpath已经存在，并且actualpath不需要跟sympath位于同一文件系统。如果sympath指定的是绝对路径或者fd参数设置了AT_FDCWD，那么symlinkat等同于symlink函数。因为open函数跟随符号链接，所有需要一种方法打开该链接本身，可以用readlink或readlinkat。

```c
#include <unistd.h>
ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *restrict pathname, char *restrict buf, size_t bufsize);
//若成功返回读取的字节数否则返回-1
```

这两个函数组合了open、read和close的所有操作，如果成功执行，则返回读入buf的字节数，在buf中返回的符号链接的内容不以null字节终止。

当pathname指定的是绝对路径或者fd参数的值为AT_FDCWD，readlinkat函数的行为与readlink相同。如果fd是一个打开目录的有效文件描述符并且pathname参数是相对路径名，则readlinkat计算相对于由fd代表的打开目录的路径名。

## 文件的时间

每个文件维护三个时间字段，如下：

![](..\image\Unix\3-4-19.png)

注意，修改时间（st_mtim）是文件内容最后一次被修改的时间，状态更改时间（st_ctim）是该文件的i节点最后一次被修改的时间。有很多影响i节点的操作，比如更改文件的访问权限，更改用户id，更改链接数等。

注意，系统并不维护对一个i节点的最后一次访问时间，所以access和stat函数并不更改这三个时间中的任一个。

另外，还有如下的总结：

![](..\image\Unix\3-4-20.png)

## 函数futimens、utimensat和utimes

futimens和utimensat可以指定纳秒级别的时间戳。

```c
#include <sys/stat.h>
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2], int flag);
//若成功返回读取的字节数否则返回-1
```

时间戳可以按以下方式指定：

1. 如果times是一个空指针，则访问时间和修改时间都设置为当前时间；
2. 如果times指向两个timespec结构的数组，任一数组元素的tv_nsec字段的值为UTIME_NOW，相应的时间戳就设置为当前时间，忽略相应的tv_sec字段；
3. 如果times参数指向两个timespec结构的数组，任一数组元素的tv_nsec字段的值为UTIME_OMIT，相应的时间戳保持不变，忽略相应的tv_sec字段；
4. 如果times参数指向两个timespec结构的数组，且tv_nsec字段的值既不是UTIME_OMIT又不是UTIME_NOW，相应的时间戳设置为相应的tv_sec和tv_nsec字段的值。

## 函数mkdir、mkdirat和rmdir

函数签名为：

```c
#include <sys/stat.h>
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
//若成功返回0否则返回-1
```

用rmdir可以删除一个空目录：

```c
#include <unistd.h>
int rmdir(const char *pathname); //若成功返回0否则返回-1
```

## 读目录

![](..\image\Unix\3-4-21.png)

上述7个函数用DIR结构来保存当前正在被读的目录的有关信息，其作用类似于FILE结构，FILE结构由标准I/O维护。

## 函数chdir、fchdir和getcwd

进程调用chdir和fchdir可以更改当前工作目录：

```c
#include <unistd.h>
int chdir(const char *pathname);
int fchdir(int fd); //若成功返回0否则返回-1
```

getcwd则可以获得当前工作目录：

```c
#include <unistd.h>
char *getcwd(char *buf, size_t size); //若成功返回buf否则返回NULL
```

该缓冲区必须有足够的长度以容纳绝对路径名再加上一个终止null字节，否则返回出错。

如果更换目录后想要返回原目录，可以用open打开当前工作目录，然后保存返回的文件描述符，之后将该文件描述符传送给fchdir即可。

## 设备特殊文件

系统中与每个文件名关联的st_dev是文件系统的设备号，只有字符特殊文件和块特殊文件才有st_rdev值，此值包含实际设备的设备号。

## 文件访问权限位小结

![](..\image\Unix\3-4-26.png)

# 标准I/O库

## 流和FILE对象

对于标准I/O库，它的操作是围绕流进行的。当用标准I/O库打开或创建一个文件时，已经使得一个流与一个文件相关联。

流的定向决定了所读写的字符是单字节或多字节的。当一个流最初被创建时，它并没有定向，其是由相应的I/O函数决定。只有两个函数可以改变流的定向，freopen清除一个流的定向，fwide用于设置流的定向：

```c
#include <stdio.h>
#include <wchar.h>
int fwide(FILE *fp, int mode); //若流是宽定向的，返回正值，若是字节定向的，返回负值，未定向返回0
```

如果mode参数为负，此函数将试图使得指定的流是字节定向的，为正，则试图使其宽定向，为0则不设置定向，但返回标识该流定向的值。

注意，fwide并不改变已定向流的定向。

当打开一个流时，标准I/O函数fopen返回一个指向FILE对象的指针，该指针指向的结构包含了I/O库管理该流的所有信息。

## 缓冲

标准I/O库提供缓冲的目的是尽可能的减少read和write调用的次数，它也对每个I/O流自动的进行缓冲管理。其提供了以下三种类型的缓冲：

1. 全缓冲：这种情况下在填满I/O缓冲区后才进行实际I/O操作，对于驻留在磁盘上的文件通常是由标准I/O库实施全缓冲的。在一个流上执行第一次I/O操作时，相关标准I/O通常调用malloc获得需使用的缓冲区。缓冲区可以由标准I/O例程自动flush，也可以调用fflush操作。
2. 行缓冲：在输入和输出中遇到换行符时标准I/O库执行I/O操作，这允许我们一次输出一个字符，但只有在写了一行后才实际进行I/O操作，当涉及一个终端时通常使用行缓冲。对于行缓冲有两个限制，一是由于缓冲区的长度是固定的，所以只要填满了缓冲区，即使没有换行符也会进行I/O操作，二是只要通过标准I/O库要求从一个不带缓冲的流或者一个行缓冲的流得到输入数据，那么就会冲洗所有行缓冲输出流；
3. 不带缓冲，即立即输出，标准错误流通常是不带缓冲的。

ISO C有以下要求：

- 当且仅当标准输入和标准输出不指向交互式设备时他们才是全缓冲的；
- 标准错误绝不会是全缓冲的；

可以使用以下函数来更改缓冲类型：

```c
#include <stdio.h>
void setbuf(FILE *restrict fp, char *restrict buf);
void setvbuf(FILE *restrict fp, char *restrict buf, int mode, size_t size);
//成功返回0，出错返回非0
```

这些函数应该在流打开之后且在执行任何操作前调用。

可以使用setbuf来打开或关闭缓冲机制，为了带缓冲进行I/O，buf必须指向一个长度为BUFSIZE的缓冲区（该常量通常定义在stdio.h中），通常在此之后流就是全缓冲的，但如果该流与一个终端设备有关，那么某些系统也可以将其设置为行缓冲的。为了关闭缓冲，将buf设置为NULL。

使用setvbuf可以精确说明需要的缓冲类型，通过mode参数实现：

- _IOFBF：全缓冲；
- _IOLBF：行缓冲；
- _IONBF：不带缓冲；

如果指定一个不带缓冲的流则忽略buf和size参数。如果指定全缓冲或行缓冲，则buf和size则选择一个缓冲区和长度。如果该流是带缓冲的，而buf是NULL，则标准I/O库会自动给其分配适当长度的缓冲区。适当长度通常是BUFSIZE的值。

这两个函数的行为总结如下：

![](..\image\Unix\3-5-1.png)

注意，如果在一个函数内分配一个自动变量类的标准I/O缓冲区，则从该函数返回时必须关闭该流。一般而言，最好由系统选择缓冲区的长度并自动分配缓冲区。

任何时候，都可以强制冲洗一个流：

```c
#include <stdio.h>
int fflush(FILE *fp);
```

此函数使得该留所有未写的数据都被传送到内核，如果fp是NULL，则所有输出流被冲洗。

## 打开流

以下函数打开一个标准I/O流：

```c
#include <stdio.h>
FILE *fopen(const char *restrict pathname, const char *restrict type);
FILE *freopen(const char *restrict pathname, const char *restrict type, FILE *restrict fp);
FILE *fdopen(int fd, const char *type);
//成功返回文件指针，出错返回NULL
```

函数区别如下：

1. fopen通过路径名打开一个指定的文件；
2. freopen在一个指定的流上打开一个指定的文件，如果该流已经打开，则先关闭该流，若该流已定向，则使用freopen清除该定向。此函数一般用于将一个指定的文件打开为一个预定义的流：标准输入、标准输出或标准错误；
3. fdopen将一个标准I/O流与一个已有的文件描述符相结合。一般用于创建管道和网络通信通道返回的文件描述符，因为这些文件描述符不能用fopen打开。

fopen和freopen是ISO C所属部分，而ISO C不涉及文件描述符，所以仅有POSIX.1具有fdopen。

type参数指定流的读写方式，总结如下：

![](..\image\Unix\3-5-2.png)

fdopen的type参数稍有不同，因为其文件描述符受打开时的设置控制。

当以读和写类型打开一个文件时，具有以下限制：

- 如果中间没有fflush、fseek、fsetpos或rewind，则在输出后面不能直接跟随输入
- 如果中间没有fseek、fsetpos或rewind，或者要给输入操作没有到达文件尾端，则输入之后不能直接跟随输出；

打开流的6种不同方式：

![](..\image\Unix\3-5-3.png)

注意在指定w和a类型创建一个新文件时，我们无法说明该文件的访问权限位。

调用fclose关闭一个打开的流：

```c
#include <stdio.h>
int fclose(FILE *fp);
```

在文件被关闭之前，冲洗缓冲区中的输出数据，缓冲区内的任何输入数据被丢弃。

## 读和写流

一旦打开了流，可以在3种不同类型的非格式化I/O中进行选择，对其进行读写操作：

1. 每次一个字符的I/O；
2. 每次一行的I/O。如果想要一次读或写一行，则使用fgets和fputs，每行都以一个换行符终止。调用fgets时应说明能处理的最大行长。
3. 直接I/O。fread和fwrite支持这种类型的I/O。每次操作读或写某种数量的对象，而每个对象具有指定的长度，这两个函数常用于从二进制文件中每次读或写一个结构。

以下三个函数可以用于一次读一个字符：

```c
#include <stdio.h>
int getc(FILE *fp);
int fgetc(FILE *fp);
int getchar(void);
//若成功返回下一个字符，若到达尾端或出错，返回EOF
```

getchar等同于getc(stdin)。前两个函数的区别是getc可被实现为宏，而fgetc不能。

这三个函数在返回下一个字符的时候，将其unsigned char类型转换为int类型。注意，不管是出错还是到达尾端，这三个函数都返回同样的值，为了区分这两种情况，必须调用ferror或feof：

```c
#include<stdio.h>
int ferror(FILE *fp);
int feof(FILE *fp);
//若条件为真返回非0否则返回0
void clearerr(FILE *fp);
```

大多数实现都为每个流在FILE对象中维护了出错标志和文件结束标志，调用clearerr可以清除这两个标志。

从流中读取数据之后可以调用ungetc将字符再压送回流中：

```c
#include <stdio.h>
int ungetc(int c, FILE *fp);
```

不能回送EOF，但是可以在到达文件尾端的时候仍回送一个字符，下次读将返回该字符，再读返回EOF。之所以可以这么做的原因是一次成功的ungetc会清除该流的文件结束标志。

对应上面每个输入函数都有一个输出函数：

```c
include <stdio.h>
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int putchar(int c);
```

## 每次一行I/O

由以下函数提供：

```c
#include <stdio.h>
char* fgets(char *restrict buf, int n, FILE *restrict fp);
char* gets(char *buf);
//若成功返回buf，若到达尾端或出错，返回NULL
```

gets从标准输入读，而fgets从指定的流读。对于fgets必须指定缓冲区长度，此函数一直读到换行符为之，但不超过n-1个字符，该缓冲区以null字节结尾。gets不推荐使用，因为不能指定缓冲区长度可能造成缓冲区溢出。

fputs和puts提供每次输出一行的功能：

```c
#include <stdio.h>
int fputs(const char *restrict str, int n, FILE *restrict fp);
int puts(const char *str);
//若成功返回非负值，若出错返回EOF
```

fputs将一个以null字节终止的字符串写到指定的流，null不写出。puts将一个以null字节终止的字符串写到标准输出，终止符不写出，但是puts随后又将一个换行符写到标准输出。

## 二进制I/O

```c
#include <stdio.h>
int fread(void *restrict ptr, size_t size, size_t nobj, FILE *restrict fp);
int fwrite(const void *restrict ptr, size_t size, size_t nobj, FILE *restrict fp);
//返回读或写的对象数
```

有以下常见用法：

1. 读或写一个二进制数组；
2. 读或写一个结构；

对于读如果出错或到达文件尾端，数字可能少于nobj，因此应该调用ferror或feof判定究竟是哪种情况。对于写如果返回值少于要求的nobj，则出错。

使用二进制I/O的基本问题是只能用于读在同一系统上已写的数据，但现在有了网络通信，在一个系统上写的数据可能在另一个系统上使用，这种情况这两个函数就可能出错，比如不同系统的偏移量或对齐不同，又比如二进制格式也随着系统不同而不同，这种情况下就需要使用网络协议使用的交换二进制数据的某些技术。

## 定位流

有三种方法定位标准I/O流：

1. ftell和fseek函数，它们都假定文件的位置可以存放在一个长整型中。
2. ftello和fseeko函数，使文件偏移量可以不必一定使用长整型，它们使用off_t代替了长整型；
3. fgetpos和fsetpos函数，使用一个抽象数据类型fpos_t 记录文件位置。

需要移植到非UNIX系统上运行的程序应当使用fgetpos和fsetpos：

```c
#include <stdio.h>
long ftell(FILE *fp) //成功返回当前文件位置指示，出错返回-1L
int fseek(FILE *fp, long offset, int whence); //成功返回0，否则返回-1
void rewind(FILE *fp)
//返回读或写的对象数
```

whence有lseek函数中的相同：SEEK_SET表示从文件起始位置开始，SEEK_CUR表示从当前文件位置开始，SEEK_END表示从文件尾端开始。

对于文本文件，它们文件当前位置可能不是以简单的字节偏移量度量。为了度量一个文本文件，whence一定要是SEEK_SET，而且offset只能是0或者是该文件的ftell所返回的值。使用rewind也可以将一个流设置到文件的起始位置。

除了偏移量的类型是off_t而非long之外，ftello与ftell相同，fseeko与fseek相同。

```c
#include <stdio.h>
off_t ftell(FILE *fp) //成功返回当前文件位置，出错返回(off_t)-1
int fseeko(FILE *fp, off_t offset, int whence); //成功返回0，否则返回-1
```

以下函数是由ISO C标准引入。

```c
#include <stdio.h>
int fgetpos(FILE *restrict fp, fpos_t *restrict pos);
int fsetpos(FILE *fp, const fpos_t *pos); //成功返回0，否则返回非0
```

## 格式化I/O

格式化输出是由5个printf函数来处理的：

```c
#include <stdio.h>
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict fp, const char *restrict format, ...);
int dprintf(int fd, const char *restrict format, ...);
//这三个函数若成功返回字符数，出错返回负值
int sprintf(char *restrict buf, const char *restrict format, ...);
int snprintf(char *restrict buf, size_t n, const char *restrict format, ...);
//若缓冲区足够大返回要存入数组的字符数，若编码出错返回负值
```

sprintf将格式化的字符送入数组buf中，且在该数组的尾端自动加一个null字节，但该字符不包含在返回值中。注意，sprintf可能会造成由buf指向的缓冲区的溢出，因此调用者有责任保证该缓冲区足够大。为了解决这个问题，引入了snprintf。

注意，使用dprintf不需要调用fdopen将文件描述符转换为文件指针（fprintf需要）。

格式转换的格式如下：`%[flags][fldwidth][precision][lenmodifier]convtype`

各种标志的总结如下：

![](..\image\Unix\3-5-7.png)

fldwidth说明最小字段宽度，转换后若小于宽度用空格补齐，字段宽度是一个非负十进制或是一个星号。precision说明整型转换后最少输出数字位数、浮点数转换后小数点后最少位数、字符串转换后最大字节数。lenmodifier说明参数长度，可能的值如下：

![](..\image\Unix\3-5-8.png)

以下是五种printf的变体：

```c
#include <stdio.h>
#include <stdarg.h>
int vprintf(const char *restrict format, va_list arg);
int vfprintf(FILE *restrict fp, const char *restrict format, va_list arg);
int vdprintf(int fd, const char *restrict format, va_list arg);
//这三个函数若成功返回字符数，出错返回负值
int vsprintf(char *restrict buf, const char *restrict format, va_list arg);
int vsnprintf(char *restrict buf, size_t n, const char *restrict format, va_list arg);
//若缓冲区足够大返回要存入数组的字符数，若编码出错返回负值
```

格式化输出的三个函数

```c
#include <stdio.h>
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict fp, const char *restrict format, ...);
int sscanf(const char *restrict buf, const char *restrict format, ...)
```

其相应变体如下：

```c
#include <stdio.h>
#include <stdarg.h>
int scanf(const char *restrict format, va_list arg);
int fscanf(FILE *restrict fp, const char *restrict format, va_list arg);
int sscanf(const char *restrict buf, const char *restrict format, va_list arg)
```

## 实现细节

在Unix中，标准I/O最终都要调用第三章中的说明例程，每个标准I/O都有一个相关联的文件描述符，可以对一个流调用fileno来获取其文件描述符：

```c
#include<stdio.h>
int fileno(FILE *fp);
```

如果要调用dup或fcntl等函数则需要此函数。

## 临时文件

ISO C标准I/O库提供了两个函数以帮助创建临时文件：

```c
#include<stdio.h>
char *tmpnam(char *ptr); //返回指向唯一路径名的指针
FILE *tmpfile(void); //成功返回文件指针，出错返回NULL
```

tmpnam产生一个与现有文件名不同的一个有效路径名字符串，每次调用都产生一个不同的路径名，最多调用TMP_MAX，定义在stdio.h中。若ptr是NULL，则产生的路径名存放在一个静态区中，指向该静态区的指针作为函数值返回。后续调用tmpnam时，会重写该静态区。若ptr不是NULL，则认为它的长度至少是L_tmpnam个字符的数组，所产生的路径名存在该数组中，ptr也作为函数值返回。

tmpfile创建一个临时二进制文件（类型wb+），在关闭该文件或程序结束时自动删除这种文件。

一般使用方法是，先调用tmpnam产生一个唯一的路径名，然后用该路径名创建一个文件，并立即unlink它。

Single Unix Specification为处理临时文件定义了另外两个函数，是XSI的扩展部分：

```c
#include<stdio.h>
char *mkdtemp(char *template); //成功返回指向目录名的指针，出错返回NULL；
int mkstemp(char *template); //成功返回文件描述符，出错返回-1
```

mkdtemp创建一个目录，该目录有一个唯一的名字，mkstemp创建一个文件，该文件有一个唯一的名字。mkdtemp创建的目录使用以下访问权限位集：S_IRUSR | S_IWUSR | S_IXUSR。注意调用进程的文件模式创建屏蔽字可以进一步限制这些权限。mkstemp创建的文件使用访问权限位S_IRUSR | S_IWUSR。与tempfile不同，mkstemp创建的临时文件不会自动删除。

使用tmpnam有一个缺点，那就是创建路径名和创建文件之间存在时间窗口，另一进程可以使用相同名字创建文件，因此应该使用tmpfile和mkstemp。

## 内存流

在SUSv4中支持了内存流，虽然仍使用FILE指针进行访问，但其实没有底层文件，所有I/O都是通过缓冲区和主存之间来回传送字节完成，它们的某些特征使其更适用于字符串操作。

```c
#include <stdio.h>
FILE *fmemopen(void *restrict buf, size_t size, const char *restrict type);
//成功返回流指针，错误返回NULL
```

type控制如何使用流：

![](..\image\Unix\3-5-14.png)

无论何时以追加写方式打开内存流，当前文件位置设置为缓冲区的第一个null字节。如果不存在null字节，当前位置就设置为缓冲区结尾的后一个字节。当流不是以追加写的方式打开，当前位置设置为缓冲区的开始位置。内存流不适合存储二进制数据（二进制数据在数据尾端之前就可能包含多个null字节）。如果buf是一个null指针，打开流进行读或写都没有意义，因为缓冲区是通过fmemopen分配的。任何时候需要增加流缓冲区中数据量以及调用fclose、fflush、fseek、fseeko以及fsetpos都会在当前位置写入一个null字节。

另外用于创建内存流的函数如下：

```c
#include <stdio.h>
#include <wchar.h>
FILE *open_memstream(char **bufp, size_t *sizep);
FILE *open_wmemstream(wchar_t **bufp, size_t *sizep)
//成功返回流指针，出错返回NULL
```

这两个函数与fmemopen的区别在于：

- 创建的流只能写打开；
- 不能指定自己的缓冲区，但可以分配通过bufp和sizep访问缓冲区地址和大小；
- 关闭流后需要自行释放缓冲区；
- 对流添加字节会增加缓冲区大小；

对缓冲区地址和大小也有一些原则：缓冲区地址和大小必须在调用fclose或fflush后才有效，另外这些值只有在下一次流写入或调用fclose前才有效。因为缓冲区可以增长，可能出现重新分配。

由于避免了缓冲区溢出，内存流非常适用于创建字符串，因为内存流只访问主存，所有将其用于临时文件来说会有很大的性能提升。

标准I/O的一个不足是效率不高，这和需要复制的数据量有关。当使用每次一行函数fgets和fputs时，通常需要复制两次数据：内核和标准I/O缓冲区一次，标准I/O缓冲区和用户程序中的行缓冲区一次。

# 系统数据文件和信息

## 口令文件

口令文件包含了以下所示的各字段，定义在<pwd.h>的passwd结构中。

![](..\image\Unix\3-6-1.png)

由于历史原因，口令文件是/etc/passwd，而且是一个ASCII文件，每一行包含上图的各字段，字段之间用冒号分隔。在linux系统中，该文件可能有以下4行：

```bash
root:x:0:0:root:/root:/bin/bash
squid:x:23:23::/var/spool/squid:/dev/null
nobody:x:65534:65534:Nobody:/home:/bin/sh
sar:x:205:105:Stephen Rago:/home/sar:bin/bash
```

为了阻止一个特定用户登录系统，除了使用/dev/null之外，还可以将/bin/false用作登录shell，还可以用bin/true来禁止一个账户。使用nobody用户名的一个目的是任何人都可以登录系统，但不提供任何特权。

POSIX.1定义了两个获取口令文件项的函数：

```c
#include <pwd.h>
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name); //成功返回指针，出错返回NULL
```

如果要查看的只是登录名或用户ID，那么这两个函数能满足要求，但如果要查看整个口令文件，可以用以下的函数：

```c
#include <pwd.h>
struct passwd *getpwent(void); //成功返回指针，出错或到达文件尾端，返回NULL
void setpwent(void);
void endpwent(void);
```

调用getpwent，它返回口令文件中的下一个记录项，setpwent反绕它所使用的文件，endpwent则关闭这些文件。

## 阴影口令

该文件至少要包含用户名和加密口令。阴影口令不应是一般用户可以读取的，仅有少数几个程序需要访问加密口令，如login和passwd。以下是访问阴影口令的函数：

```c
#include <shadow.h>
struct spwd *getspnam(const char *name);
struct spwd *getspent(void);

void setspent(void);
void endspent(void);
```

## 组文件

组文件包含了以下字段，定义在<grp.h>中的group结构中：

![](..\image\Unix\3-6-4.png)

gr_mem是一个指针数组，其中每个指针指向一个属于改组的用户名，该数组以null指针结尾。

可以用以下两个由POSIX.1定义的函数来查看组名或者数值组ID：

```c
#include <grp.h>
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);
//成功返回指针，出错返回null
```

如果需要搜索整个组文件，则需要另外几个函数：

```c
#include <grp.h>
struct group *getgrent(void);
//成功返回指针，出错或到达文件尾端，返回NULL
void setgrent(void);
void endgrent(void);
```

## 附属组ID

在V7中，每个用户任何时候只属于一个组，当用户登录时，系统就按照口令文件中记录的数值组ID赋给它实际组ID，可以在任何时候执行newgrp更改组ID。之后4.2BSD引入了附属组ID的概念，我们不仅可以属于口令文件记录项中组ID所对应的组，还可以属于多至16个另外的组。文件访问权限检查相应更改为：不仅将进程的有效组ID与文件的组ID比较，而且也将所有附属组ID与文件的组ID比较。

为了获取和设置附属组ID，提供了以下三个函数：

```c
#include <unistd.h>
int getgroups(int gidsetsize, gid_t grouplist[]); //成功返回组ID数量，出错返回-1；
#include <grp.h> /* on linux */
#include <unistd.h> /* on FreeBSD, Mac OS x and SOlaris */
int setgroups(int ngroups, const gid_t grouplist[]);
#include <grp.h> /* on linux */
#include <unistd.h> /* on FreeBSD, Mac OS x and SOlaris */
int initgroups(const char *username, gid_t basegid);
//成功返回0，出错返回-1
```

## 实现区别

![](..\image\Unix\3-6-5.png)

## 登录账户记录

大多数Unix系统都提供以下两个数据文件：utmp文件记录当前登录到系统的各个用户；wtmp文件跟踪各个登录和注销事件。在V7中每次写入这两个文件中的是包含下列结构的一个二进制记录：

```c
struct utmp {
	char ut_line[8];
	char ut_name[8];
	long ut_time;
};
```

登录时login程序填写此类结构，然后将其写入utmp文件中，同时也将其添加到wtmp文件中。注销时init进程将utmp文件中相应的记录擦除，并将一个新记录写到wtmp文中中。

## 系统标识

POSIX.1定义了uname函数，返回与主机和操作系统有关的信息：

```c
#include <sys/utsname.h>
int uname(struct utsname *name);
```

结果在utsname结构中，有如下字段：

```c
struct utsname {
    char sysname[];
    char nodename[];
    char release[];
    char version[];
    char machine[];
}
```

utsname中的信息通常可由uname(1)命令打印。

历史上BSD派生的系统提供gethostname函数，它只返回主机名。

```c
#include <unistd.h>
int gethostname(char *name, int namelen);
```

现在该函数已在POSIX.1中定义，指定最大主机名长度为HOST_NAME_MAX。

hostname(1)命令可以用来获取和设置主机名。

## 时间和日期例程

由UNIX内核提供的基本时间服务是计算自协调世界时（1970.01.01 00:00:00）这一特定时间以来经过的秒数，以数据类型time_t表示，称为日历时间。日历时间包括时间和日期。

time函数返回当前时间和日期：

```c
#include <time.h>
time_t time(time_t *calptr); //成功返回时间值，出错返回-1
```

时间值作为函数值返回，如果参数非空，时间值也存放在calptr指向的单元内。

时钟通过clockid_t标识系统时钟，标准值如下：

![](..\image\Unix\3-6-8.png)

clock_gettime用于获取指定时钟的时间，返回的时间在timespec结构中，它把时间表示为秒跟纳秒。

```c
#include <sys/time.h>
int clock_gettime(clockid_t clock_id, struct timespec *tsp); //成功返回0，否则为-1
int clock_getres(clockid_t clock_id, struct timespec *tsp); //成功返回0，否则为-1
```

clock_getres将tsp指向的timespect结构初始化为与clock_id参数对应的时钟精度。

要对特定时钟设置时间，可以调用clock_settime函数：

```c
#include <sys/time.h>
int clock_settime(clockid_t clock_id, const struct timespec *tsp); //成功返回0，出错为-1
```

SUSv4指定gettimeofday已经弃用，但一些程序仍旧使用这个函数，因为与time相比其提供了更高的精度（可到微妙级）：

```c
#include <sys/time.h>
int gettimeofday(struct timeval *restrict tp, void *restrict tzp); //总是返回0
```

tzp的唯一合法值是NULL，其他值将产生不确定的结果。

两个函数localtime和gettime将日历时间转换为分解的时间，并存放在tm结构中。

![](..\image\Unix\3-6-9.png)

```c
#include <time.h>
struct tm *gmtime(const time_t *calptr);
struct tm *localtime(const time_t *calptr);
//成功返回指向tm结构的指针，出错返回NULL
```

这两个函数的区别是：localtime会将日历时间转换成本地时间（考虑到本地时区和夏令时标志），而gmtime则将日历时间转换成协调统一时间的年月日等。

mktime将本地时间的年月日等转换为time_t值：

```c
#include <time.h>
time_t mktime(struct tm *tmptr);
//成功返回日历时间出错返回-1
```

strftime可用于打印时间值：

```c
#include <time.h>
size_t strftime(char *restrict buf, size_t maxsize, const char *restrict format, const struct tm *restrict tmptr);
size_t strftime_l(char *restrict buf, size_t maxsize, const char *restrict format, const struct tm *restrict tmptr, locale_t locale);
//若有空间返回存入数组的字符数，否则返回0
```

format格式说明：

![](..\image\Unix\3-6-10.png)

# 进程环境

## main函数

当内核执行c程序时，在调用main之前先调用一个特殊的启动例程。可执行文件将此启动例程指定为程序的起始地址，这是由连接编辑器设置的，连接编辑器则是由C编译器调用。启动例程从内核取得命令行参数和环境变量值。

## 进程终止

有八种方式使程序退出，五种为正常终止：

1. 从main返回；
2. 调用exit；
3. 调用\_exit或\_Exit；
4. 最后一个线程从其启动例程返回；
5. 从最后一个线程调用pthread_exit；

异常终止有三种：

1. 调用abort；
2. 接到一个信号；
3. 最后一个线程对取消请求做出响应；

三个函数用于正常终止一个程序：\_exit和\_Exit立即进入内核，exit则先执行一些清理处理，然后返回内核：

```c
#include <stdlib.h>
void exit(int status);
void _Exit(int status);
#include <unistd.h>
void _exit(int status);
```

由于历史原因，exit函数总是执行一个标准I/O库的清理关闭操作：对于所有打开流调用fclose函数。

如果调用这些函数时不带终止状态，或main执行了一个误返回值的return语句，或main没有声明返回类型是整型，该进程的终止状态是未定义的。正常退出时终止状态为0。

根据ISO C规定，一个进程可以登记多至32个函数，将由exit自动调用，而登记这些函数是由atexit来进行的。

```c
#include <stdlib.h>
int atexit(void (*func)(void)); //成功返回0，否则返回非0
```

exit调用这些函数的顺序与登记顺序相反，同一函数如果被登记多次，也会被多次调用。

根据ISO C和POSIX.1，exit首先调用各终止程序，然后关闭所有打开流。POSIX.1扩展了ISO C标准，它说明如果程序调用了exec程序族中任一函数，则将清理所有已安装的终止处理程序。下图演示了一个C程序是如何启动的：

![](..\image\Unix\3-7-2.png)

注意，内核使程序执行的唯一方法是调用一个exec函数，进程自愿终止的唯一方法是显式或隐式（通过调用exit）调用\_exit或\_Exit。

## 环境表

每个程序都接收到一张环境表，与参数表一样，环境表也是一个字符指针数组，每个指针包含一个以null接收的C字符串的地址。全局变量environ包含了该指针数组的地址`extern char **environ;`通常用getenv和putenv函数来访问特定的环境变量，而不是用environ变量，但是如果要查看整个环境，必须用environ指针。

## C程序的存储空间布局

布局如下所示：

![](..\image\Unix\3-7-6.png)

## 存储空间分配

ISO C说明了三个用于存储空间动态分配的函数：

1. malloc，分配指定字节数的存储区，此存储区中的初始值不确定；
2. calloc，为指定数量指定长度的对象分配存储空间，每一位都初始化为0；
3. realloc，增加或减少以前分配区的长度；

```c
#include <stdlib.h>
void *malloc(size_t size);
void *calloc(size_t nobj, size_t size);
void *realloc(void *ptr, size_t newsize);
//成功返回非空指针，出错返回NULL
void free(void *ptr);
```

这三个分配函数返回的指针一定是适当对齐的，使其可以用于任何数据对象。这些分配例程通常用sbrk系统调用实现。

## 环境变量

ISO C定义了getenv，可以用来获取环境变量值：

```c
#include <stdlib.h>
char *getenv(const char *name); //返回与name关联的value指针，若未找到，返回NULL
```

Unix实现使用了很多的环境变量：

![](..\image\Unix\3-7-7.png)

以下是不同标准实现的如何设置环境变量的函数：

![](..\image\Unix\3-7-8.png)

中间三个函数的原型为：

```c
#include <stdlib.h>
int putenv(char *str); //成功返回0，否则为非0
int setenv(const char *name, const char *value, int rewrite);
int unsetenv(const char *name);
//成功返回0，否则为-1
```

putenv取形式为name=value的字符串，将其放在环境表中，如果name已经存在，则先删除原来的定义；setenv将name设置为value，如果name已经存在，若rewrite非0则首先删除原来的定义，若为0则不删除其原来的定义，即不起作用；unsetenv删除name的定义。注意，putenv不会创建存储区，而是直接使用传入的参数，而setenv则会根据参数创建存储区。

## 函数setjmp和longjmp

在C中，goto语句不能跨越函数，而setjmp和longjmp可以。其可以在栈上跳过若干帧，返回当前函数调用路径上的某个函数中：

```c
#include <setjmp.h>
int setjmp(jmp_buf env); //直接调用返回0，若从longjmp返回则为非0
void longjmp(jmp_buf env, int val);
```

## 函数getrlimit和setrlimit

每个进程都有一组资源限制，一些可以用这两个函数查询和更改：

```c
#include <sys/resource.h>
int getrlimit(int resource, struct rlimit *rlptr);
int setrlimit(int resource, const struct rlimit *rlptr); //成功返回0，出错返回非0
```

更改资源限制时，必须遵循以下规则：

1. 任何一个进程都可以将其软限制值更改为小于或等于其硬限制值；
2. 任何一个进程都可以降低其硬限制值，但其必须大于或等于软限制值，这种降低，对普通用户来说是不可逆的；
3. 只有超级用户进程可以提高硬限制值；

常量RLIM_INFINITY指定了一个无限量的限制。下图显示了Single UNIX Specification定义并由四种UNIX系统实现支持的资源限制：

![](..\image\Unix\3-7-15.png)

# 进程控制

## 进程标识

每个进程都有一个非负整型表示的唯一进程ID。虽然是唯一的，但是进程ID是可以复用的。系统中有一些专用进程，ID为0的进程通常是调度进程，也被称为交换进程，是内核的一部分，并不执行任何磁盘上的程序，因此也被称为系统进程。ID为1的通常是init进程，在自举过程结束时由内核调用，该进程的程序文件在Unix的早期版本中是/etc/init，在较新版本中是/sbin/init。此进程负责在自举内核后启动一个UNIX系统。init通常读取与系统有关的初始化文件（/etc/rc*文件或/etc/inittab文件，以及在etc/init.d中的文件），并将系统引导到一个状态（如多用户）。init进程绝不会终止，它是一个普通的用户进程，但以超级用户特权运行。

每个Unix系统都有一套提供操作系统服务的内核进程，在某些Unix的虚拟存储器实现中，进程ID2是页守护进程，负责支持虚拟存储器系统的分页操作。

获取进程一些标识符的函数如下：

```c
#include <unistd.h>
pid_t getpid(void); //返回调用进程的进程ID；
pid_t getppid(void); //返回调用进程的父进程ID；
uid_t getuid(void); //返回调用进程的实际用户ID；
uid_t geteuid(void); //返回调用进程的有效用户ID；
gid_t getgid(void); //返回调用进程的实际组ID；
gid_t getegid(void); //返回调用进程的有效组ID；
```

## 函数fork

一个现有的进程可以调用fork创建一个新进程：

```c
#include <unistd.h>
pid_t fork(void); //子进程返回0，父进程返回子进程ID，出错返回-1
```

子进程是父进程的副本，获得父进程数据空间、堆和栈的副本，但共享正文段。注意，fork的一个特性是父进程的所有打开文件描述符都被复制到了子进程中，父子进程的每个相同的打开的文件描述符都共享一个文件表项。

考虑下述情况，一个进程具有三个不同的打开文件，它们是标准输入、标准输出、标准错误，在fork返回之后，就有了如下的结构：

![](..\image\Unix\3-8-2.png)

重要的一点是，父进程和子进程共享同一个文件偏移量。

fork有以下两种用法：

1. 一个父进程希望复制自己，使得父进程和子进程执行不同的代码段，这在网络服务中比较常见；
2. 一个进程要执行一个不同的程序。这对shell是常见的情况，这种情况从fork返回之后立即调用exec；

## 函数vfork

vfork的调用序列和返回值与fork相同，但语义不同。注意，可移植的应用程序不应该使用该函数，因为有些系统删除了该函数。

vfork用于创建一个新进程，该新进程的目的是exec一个新程序。该子进程在调用exec或者exit之前在父进程的空间中运行，但如果子进程修改数据、进行函数调用或者没有调用exec或exit就返回可能会带来未知的结果。vfork与fork的另一区别是vfork保证子进程先运行，在它调用exec或exit之后父进程才可能被调用运行。

## 函数exit

在Unxi中，一个已经终止，但其父进程尚未对其处理的进程称为僵死进程。ps命令将其状态打印为Z。

## 函数wait和waitpid

当一个进程正常或异常终止时，内核就向其父进程发送SIGCHLD信号。

对于调用wait或waitpid的进程：

- 如果所有的子进程都还在运行，则阻塞；
- 如果一个子进程已终止，正等待父进程获取其终止状态，则取得该子进程的终止状态立即返回；
- 如果没有任何子进程则立即出错返回；

如果进程由于接收到SIGCHLD信号而调用wait，我们期望wait会立即返回，但如果在随机时间点调用wait，则进程可能会阻塞。

```c
#include <sys/wait.h>
pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);
//成功返回进程ID，出错返回0或-1
```

区别如下：

- 在一个子进程终止前，wait使其调用者阻塞，而waitpid有选项可使其不阻塞；
- waitpid并不等待在其调用之后第一个终止子进程，它有若干个选项可以控制它所等待的进程；

statloc是一个整型指针，如果其不是一个空指针，则终止进程的状态就存在在它所指向的单元内，如果不关心终止状态，则可将该参数指定为空指针。POSIX.1规定终止状态用定义在<sys/wait.h>中的各个宏来查看：

![](..\image\Unix\3-8-4.png)

waitpid中的pid参数的作用解释如下：

- pid==-1：等待任一子进程；
- pid>0：等待进程id与pid相等的子进程；
- pid==0：等待组ID等于调用进程组ID的任一子进程；
- pid<-1：等待组ID等于pid绝对值的任一子进程；

options参数或者为0或者是如下常量按位或的结果：

![](..\image\Unix\3-8-7.png)

waitpid提供了wait没有的三个功能：

1. waitpid可以等待一个特定的进程，而wait则返回任一终止子进程的状态；
2. waitpid提供了一个wait的非阻塞版本；
3. waitpid通过WUNTRACED或WCONTINUED支持作业控制；

如果一个进程fork一个子进程，但不要它等待子进程，也不希望子进程处于僵死状态直到父进程终止，实现这个的方案是调用fork两次：

```c
#include "apue.h"
#include <sys/wait.h>
int main(void) {
 	pid_t pid;
 	if ((pid = fork()) < 0) {
 		err_sys("fork error");
 	} else if (pid == 0) {
 		if ((pid = fork()) < 0) {
 			err_sys("fork error");
 		}
 		else if (pid > 0) {
 			exit(0);
 		}
 		sleep(2);
 		printf("second child, parent pid = %ld\n", (long)getppid());
 		exit(0);
 	}
 	
 	if (waitpid(pid, NULL, 0) != pid) err_sys("waitpid error");
 	exit(0);
}
```

由于第二个子进程的父进程调用exit终止，因此其被init进程接管。

## 函数waitid

Sing UNIX Specification提供了另一个取得进程终止状态的函数waitid，其比waitpid更加灵活。

```c
#include <sys/wait.h>
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
//成功返回0，否则返回-1
```

与waitpid相似，它允许指定要等待的进程，并使用两个参数来表示要等待的子进程所属的类型。idtype类型如下图所示：

![](..\image\Unix\3-8-9.png)

options参数是下列各标志的按位或运算：

![](..\image\Unix\3-8-10.png)

WCONTINUED、WEXITED或WSTOPPED这三个常量之一必须在options参数中指定。

## 函数wait3和wait4

大多数UNIX系统实现了另外两个函数wait3和wait4，历史上这两个函数是从UNIX的BSD分支上沿袭下来的，其比wait、waitpid和waitid提供的功能要多一个，这与附加参数有关，该参数允许内核返回由终止进程及其所有子进程使用的资源概况。

```c
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

pid_t wait3(int *statloc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *statloc, int options, struct rusage *rusage);
//成功返回进程ID，出错返回-1
```

资源统计信息包括用户CPU时间总量、系统CPU时间总量、缺页次数、接收到信号的次数等，细节可参阅getrusage(2)手册页。各个wait函数支持的参数如下所示：

![](..\image\Unix\3-8-11.png)

## 竞争条件

当多个进程都企图对共享数据进行某种处理，而最后的结果又取决于进程运行的顺序时，可以认为是发生了竞争条件。

## 函数exec

fork创建子进程之后，子进程往往要调用exec以执行另一个程序。当进程调用exec时该进程执行的程序完全替换为新程序，而新程序则从main开始执行。由于exec不创建新进程，所有进程id并未改变，其只是用磁盘上一个新程序替代了当前进程的正文段、数据段、堆段和栈段。

exec函数族如下所示：

```c
#include <unistd.h>
int execl(const char *pathname, const char *arg0, ... /* (char *)0 */);
int execv(const char *pathname, char *const argv[]);
int execle(const char *pathname, const char *arg0, ... /* (char *)0, char *const envp[] */);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execlp(const char *filename, const char *arg0, ... /* (char *)0 */);
int execvp(const char *filename, char *const argv[]);
int fexecve(int fd, char *const argv[], char *const envp[]);
//出错返回-1， 成功不返回
```

当指定filename作为参数时：

- 如果filename包含/，就将其视为路径名；
- 否则就按照PATH环境变量，在它所指定的各目录中搜寻可执行文件；

如果execlp和execvp使用路径前缀中（即环境变量中包含的）的一个找到了一个可执行文件，但是该文件不是由链接编辑器产生的机器可执行文件，则认为该文件是一个shell脚本，会试着调用/bin/sh，并以该filename作为shell的输入。

每个系统对参数表和环境表的总长度都有一个限制，这种限制是由ARG_MAX给出的。在POSIX.1系统中，此值至少为4096字节。

执行新程序时对打开文件的处理与每个描述符的执行时关闭标志值有关，若设置了FD_CLOEXEC，则在执行exec时关闭该描述符，否则仍打开，除非特定用fcntl设置了该执行时关闭标志，否则默认是exec之后仍旧保持描述符打开。

POSIX.1明确要求在exec时关闭打开目录流，这通常是由opendir实现的，它调用fcntl为对应打开目录流的描述符设置执行时关闭标志。

在exec前后实际用户ID和实际组ID保持不变，但有效ID是否改变取决于所执行程序文件的设置用户ID位和设置组ID位是否设置。如果新程序的设置用户ID位已设置，则有效用户ID变成程序文件所有者的ID，否则有效用户ID不变，对组ID的处理方式与此相同。

在很多UNIX实现中，这7个函数中只有execve是内核的系统调用，另外6个只是库函数，它们最终都要调用该系统调用，它们的关系如图所示：

![](..\image\Unix\3-8-15.png)

## 更改用户ID和更换组ID

在Unix系统中，特权以及访问控制是基于用户ID和组ID的。可以用setuid来设置实际用户ID和有效用户ID，类似，可以用setgid设置实际组ID和有效组ID。

```c
#include <unistd.h>
int setuid(uid_t uid);
int setgid(gid_t gid);
```

注意：

1. 若进程拥有超级用户特权，则setuid将实际用户id、有效用户id以及保存的设置用户id设置为uid；
2. 若没有超级用户特权，但uid等于实际用户id或保存的设置用户id，则setuid只将有效用户id设置为uid，不更改实际用户id和保存的设置用户id；
3. 如果上面条件都不满足，则errno设置为EPERM，并返回-1；

在此假定_POSIX_SAVED_IDS为真，如果没有提供此功能，则上面说的关于保存的设置用户id部分无效。

关于内核维护的三个用户ID，需要注意以下几点：

1. 只有超级用户进程可以更改实际用户id，通常，实际用户id是在用户登录时，由login(1)程序设置且绝不会改变；
2. 仅当对程序文件设置了设置用户ID位时，exec才能设置有效用户ID。任何时候都可以调用setuid，将有效用户ID设置为实际用户ID或保存的设置用户ID；
3. 保存的设置用户ID是由exec复制有效用户id得到的，如果设置了文件的设置用户id位，则在exec根据文件的用户id设置了进程的有效用户id后，这个副本就被保存起来。

还有以下两个函数：

```c
#include <unistd.h>
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);
//成功返回0，否则为-1
```

历史上，BSD支持setreuid，来交换实际用户id和有效用户id。如果任一参数为-1，则表示相应的id应当保持不变。

seteuid和setegid类似setuid和setgid，但只改变有效用户ID和有效组ID。

```c
#include <unistd.h>
int seteuid(uid_t uid);
int setegid(gid_t gid);
//成功返回0，否则返回-1
```

各函数作用如下所示：

![](..\image\Unix\3-8-19.png)

## 解释器文件

解释器文件是文本文件，其起始行的形式为`#! pathname [optional-argument]`，感叹号和pathname之间的空格是可选的，最常见的解释器文件以下列行开始：

`#！ /bin/sh`

pathname通常是绝对路径名，对它不进行什么特殊处理（不适用PATH路径进行路径搜索），对这种文件的识别是由内核作为exec系统调用处理的一部分来完成的。注意，内核使调用exec的进程实际执行的是该解释器文件中pathname所指定的文件。

## 函数system

定义如下：

```c
#include <stdlib.h>
int system(const char *cmdstring);
```

如果cmdstring是一个空指针，则仅当该命令可用时，system返回非0值，由此可以判断一个系统是否支持system函数。在UNIX中，system总是可用的。因为system在其实现中调用了fork、exec和waitpid，因此由三种返回值：

1. fork失败或者waitpid返回除EINTR之外的出错，system返回-1，并且设置errno指示错误类型；
2. 如果exec失败，其返回值如同shell执行了exit一样；
3. 否则都成功，则返回值是shell的终止状态；

## 进程会计

大多数UNIX都提供了一个选项以进行进程会计处理，启用后，每当进程结束时内核就写一个会计记录，一般包含总量较小的二进制数据，包括命令名、所使用的CPU时间总量、用户ID和组ID、启动时间等。

函数acct启用和禁用进程会计，唯一使用这一函数的是accton命令。

## 用户标识

getlogin函数可以获取登录名：

```c
#inlcude <unistd.h>
char *getlogin(void); //成功返回指向登录名字字符串的指针，出错返回NULL
```

## 进程调度

进程可以通过nice函数获取或者更改它的nice值，使用这个函数，进程只能影响字节的nice值：

```c
#include <unistd.h>
int nice(int incr); //成功返回新的nice值NZERO，出错返回-1
```

getpriority函数可以获取进程的nice，还可以获取一组相关进程的nice值：

```c
#include <sys/resource.h>
int getpriority(int which, id_t who); //成功返回-NZERO-NZERO-1之间的nice值，出错返回-1
```

which参数可以是以下的值：PRIO_PROCESS表示进程，PRIO_PGRP表示进程组，PRIO_USER表示用户ID。which参数控制who参数是如何解释的，如果who为0表示调用进程、进程组或用户，当which设为PRIO_USER并且who为0时，使用调用进程的实际用户ID，如果which作用于多个进程，则返回所有进程中优先级最高的。

setpriority可用于为进程、进程组和属于特定用户ID的所有进程设置优先级。

```c
#include <sys/resource.h>
int setpriority(int which, id_t who, int value); //成功返回0，出错返回-1
```

## 进程时间

```c
#include <sys/times.h>
clock_t times(struct tms *buf); 成功返回流逝的墙上时钟时间，出错返回-1
```

# 进程关系

## 进程组

进程组是一个或多个进程的组合，通常，它们是在同一作业中结合起来的，同一进程组的各进程接收来自同一终端的各种信号。每个进程组有一个唯一的进程组ID。getpgrp返回调用进程的进程组ID：

```c
#include <unistd.h>
pid_t getpgrp(void);
```

Single Unix Specification定义了getpgid函数模仿此行为：

```c
#include <unistd.h>
pid_t getpgid(pid_t pid); //成功返回进程组ID，否则返回-1
```

若pid是0，返回调用进程的进程组ID。

每个进程组有一个组长进程，其进程ID等于进程组ID。进程组组长可以创建一个进程组、创建该组中的进程，然后终止，只要某个进程组中有一个进程存在，该进程组就存在。

进程调用setpgid可以加入一个现有的进程组，也可以创建一个新进程组：

```c
#include <unistd.h>
int setpgid(pid_t pid, pid_t pgid); //成功返回0，出错返回-1
```

setpgid将pid进程的进程组ID设置为pgid，如果这两个参数相等，则pid指定的进程变成进程组长，如果pid是0，则使用调用者的进程id，如果pgid是0，则由pid指定的进程ID用作进程组ID。

一个进程只能为它自己或者它的子进程设置进程组ID，在它的子进程调用exec之后，它就不再更改该子进程的进程组ID。

## 会话

会话是一个或多个进程组的集合。通常是由shell的管道将几个进程编成一组的。

进程调用setsid建立一个新会话：

```c
#include <unistd.h>
pid_t setsid(void); //成功返回进程组ID，出错返回-1
```

如果调用此函数的进程不是一个进程组的组长，此函数会创建一个新会话，具体会发生以下三件事：

1. 该进程变成新会话的会话首进程，此时，该进程是新会话的唯一进程；
2. 该进程成为一个新进程组的组长进程，新进程组id是该调用进程的进程id；
3. 该进程没有控制终端，如果在调用该函数之前进程有一个控制终端，那么这种联系也被切断；

如果该调用进程已经是一个进程组的组长，那么函数返回出错。为了保证不处于这种情况，通常操作是fork出一个子进程，之后关闭父进程，子进程继续，因为子进程继承了父进程的进程组id，而进程id又是新分配的，两者不可能相等，这就保证了子进程不是一个进程组的组长。

getsid返回会话首进程的进程组id：

```c
#include <unistd.h>
pid_t getsid(pid_t pid); //成功返回会话首进程的进程组ID，出错返回-1
```

如果pid是0，getsid返回调用进程的会话首进程的进程组id，如果pid不属于调用者所在的会话，那么就不能获得该会话首进程的进程组ID。

## 控制终端

会话和进程组还有一些其他特性：

- 一个会话可以有一个控制终端；
- 建立与控制终端连接的会话首进程称为控制进程；
- 一个会话中的几个进程组可被分成一个前台进程组以及一个或多个后台进程组；
- 如果一个会话有一个控制终端，则它有一个前台进程组，其他进程组为后台进程组；
- 无论何时键入终端的中断键，都会将中断信号发送至前台进程组的所有进程；
- 无论何时键入终端的退出键，都会将退出信号发送至前台进程组的所有进程；
- 如果终端接口检测到调制解调器或网络已经断开连接，那么则将挂断信号发送至控制进程（会话首进程）

保证程序能与控制终端对话的方法是open文件/dev/tty。在内核中，此特殊文件是控制终端的同义语。自然地，如果程序没有控制终端，则对于此设置的open将失败。

## 函数tcgetpgrp、tcsetpgrp和tcgetsid

```c
#include <unistd.h>
pid_t tcgetpgrp(int fd); //成功返回前台进程组ID，出错返回-1
int tcsetpgrp(int fd, pid_t pgrpid); //成功返回0，否则返回-1
```

tcgetpgrp返回前台进程组ID，它与在fd上打开的终端相关联。如果进程有一个控制终端，则该进程可以调用tcsetpgrp将前台进程组ID设置为pgrpid，pgrpid应当是同一会话中的一个进程组ID，fd必须引用该会话的控制终端。

给出控制TTY的文件描述符，通过tcgetsid可以获得会话首进程的进程组ID。

```c
#include <termios.h>
pid_t tcgetsid(int fd); //成功返回会话首进程的进程组ID，出错返回-1
```

## 作业控制

这是BSD在1980年左右增加的一个特性，它允许在一个终端上启动多个作业（进程组），它控制哪一个作业可以访问该终端以及哪些作业在后台运行。

我们可以键入一个影响前台作业的特殊字符——挂起键，与终端驱动程序交互，使得驱动程序将信号SIGTSTP发送至前台进程组中的所有进程，后台进程组作业不受影响，有三个特殊字符可以使得终端驱动程序产生信号，它们是：

- 中断字符（一般是Delete或Ctrl+C）产生SIGINT；
- 退出字符（一般采用Ctrl+\）产生SIGQUIT；
- 挂起字符（一般采用Ctrl+Z）产生SIGTSTP；

# 信号

## 信号概念

信号是软件中断。每个信号的名字都以SIG开头。比如SIGABRT是夭折信号，进程调用abort函数产生这种信号，SIGALRM是闹钟信号，由alarm函数设置定时器超时产生。在头文件signal.h中，信号都被定义成正整数常量。不存在编号为0的信号，POSIX.1将此种信号编号称为空信号。

信号相关的动作如下：

1. 忽略此信号，大多数信号都可采用这种方式处理，但两种信号除外：SIGKILL和SIGSTOP。其不能被忽略的原因是他们向内核和超级用户提供了使得进程终止或停止的可靠方法。
2. 捕捉信号。
3. 执行系统默认动作。

在图示中，SUS列中，点表示此种信号定义为基本POSIX.1规范部分，XSI表示该信号定义在XSI扩展部分。在系统默认动作列，终止+core表示在进程当前工作目录的core文件中复制了该进程的内存映像。

![](..\image\Unix\3-10-1.png)

## 函数signal

```c
#include <signal.h>
void (*signal(int signo, void(*func)(int)))(int); //成功返回以前的信号处理配置，出错返回SIG_ERR
```

signo是信号名，func的值通常是常量SIG_IGN、常量SIG_DFL或接到此信号时要调用的函数地址。如果指定SIG_IGN，则向内核表示忽略此信号，如果指定SIG_DFL则表示接到此信号后的动作是系统默认动作。

## 中断的系统调用

早期UNIX系统的一个特性是：如果进程在执行一个低速系统调用而阻塞期间捕捉到一个信号，则该系统调用就被中断不再执行（这里指的是内核中执行的系统调用）。该系统调用出错，其errno设置为EINTR。

为了支持这种特性，将系统调用分为两类：低速系统调用和其他系统调用。低速系统调用是可能会使得进程永远阻塞的一类系统调用包括：

- 如果某些类型文件（如读管道、终端设备和网络设备）的数据不存在，则读操作可能会使调用者永远阻塞；
- 如果这些数据不能被相同的类型文件立即接受，则写操作可能会使调用者永远阻塞；
- 在某种条件发生之前打开某些类型文件可能会发生阻塞（例如打开一个终端设备，需要先等待与之连接的调制解调器应答）；
- pause函数和wait函数；
- 某些ioctl操作；
- 某些进程间通信函数；

在这些低速系统调用中，值得注意的例外是与磁盘I/O有关的系统调用，虽然读写一个磁盘文件可能暂时阻塞调用者，但除非发生硬件错误，I/O操作总会很快返回，并使得调用者不再处于阻塞状态。

为了帮助应用程序使其不必处理被中断的系统调用，4.2BSD引进了某些被中断系统调用的自动重启动，包括：ioctl、read、readv、write、writev、wait和waitpid。如前所述，前五个函数只有对低速设备进行操作时才会被信号中断，而wait和waitpid在捕捉到信号时总是被中断。4.3BSD也允许禁用中断后重启动。

## 可重入函数

即在信号处理操作中可以保证调用安全的函数，具体见书本所述。

## SIGCLD语义

BSD的SIGCHLD的语义与其他信号语义相似，子进程状态改变后产生此信号，父进程需要调用一个wait函数以检测发生了什么。

System V处理SIGCLD信号的方式不同于其他信号，其早期处理方式是：

1. 如果进程明确将该信号的配置设置为SIG_IGN，则调用进程的子进程将不产生僵死进程。子进程在终止时将其状态丢弃。如果调用进程随后调用一个wait函数，那么它将阻塞直到所有子进程终止，然后该wait返回-1，并将errno设置为ECHILD。
2. 如果将SIGCLD设置为捕捉，则内核立即检查是否有子进程准备好等待，如果是这样，则调用SIGCLD处理程序；

## 可靠信号术语和语义

进程在信号递送给它之前仍可改变对该信号的动作，进程调用sigpending函数来判定哪些信号是设置为阻塞并处于未决状态。

每个进程都有一个信号屏蔽字，它规定了当前要阻塞递送到进程的信号集。

## 函数kill和raise

kill将信号发送给进程或进程组，raise则允许进程向自身发送信号。

```c
#include<signal.h>
int kill(pid_t pid, int signo);
int raise(int signo);
//成功返回0，出错返回-1
```

kill的pid参数有以下四种情况：

- pid>0：将信号发送给对应进程；
- pid==0：将信号发送给与发送进程属于同一进程组的所有进程，而且发送进程具有权限向这些进程发送信号。这里的所有进程不包含实现定义的系统进程集；
- pid<0：将信号发送给其进程组ID等于pid绝对值，而且发送进程具有发送权限的所有进程，如前所述，所有进程不包含系统进程集；
- pid==-1：将该信号发送给发送进程具有发送权限的所有进程，如前所述，所有进程不包含系统进程集；

超级用户可将信号发送给任一进程，非超级用户则发送者的实际用户ID或有效用户ID等于接收者的实际用户ID或有效用户ID。特例，如果被发送的信号是SIGCONT，则进程可将其发送给属于同一会话的任一其他进程。

POSIX.1将0定于为空信号。常常向一个并不存在的进程发送空信号，则kill返回-1，errno被设置为ESRCH。

## 函数alarm和pause

定时器超时产生SIGALRM信号，如果忽略或不捕捉此信号，则默认动作是终止调用该alarm函数的进程。

```c
#include <unistd.h>
unsigned int alarm(unsigned int seconds); 返回0或以前设置的闹钟时间的余留秒数
```

每个进程只能有一个闹钟时间。如果有以前注册的尚未超时的闹钟时间，而且本次调用的seconds为0，则取消以前的闹钟时间，其余留值作为返回值。

pause函数使调用进程挂起直至捕捉到一个信号：

```c
#include <unistd.h>
int pause(void); 返回-1,errno设置为EINTR
```

使用alarm和pause，进程可以使自己休眠一段指定的时间。

## 信号集

POSIX.1定义sigset_t以包含一个信号集，并定义了以下处理信号集的函数：

```c
#include <signal.h>
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
//成功返回0，否则返回-1
int sigismember(const sigset_t *set, int signo); 若真返回1，假返回0
```

所有程序在使用信号集之前都要调用sigemptyset和sigfillset一次，以进行初始化。

## 函数sigprocmask

该函数可以进行检测和更改进程的信号屏蔽字：

```c
#include <signal.h>
int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset); //成功返回0，出错返回-1
```

若oset是非空指针，那么进程的当前信号屏蔽字通过oset返回。若set是一个非空指针，则how指示如何修改当前信号屏蔽字，如下图所示：

![](..\image\Unix\3-10-3.png)

如果set是个空指针，则不改变该进程的信号屏蔽字，how的值也无意义。

## 函数sigpending

返回阻塞不能递送的信号集：

```c
#include <signal.h>
int sigpending(sigset_t *set); //成功返回0，否则为-1
```

## 函数sigaction

该函数检查或修改与指定信号关联的处理动作：

```c
#include <signal.h>
int sigaction(int signo, const struct sigaction *restrict act, struct sigaction *restrict oact); //成功返回0，否则返回-1
```

若act非空，则修改其动作，否则经由oact返回该信号的上一个动作。该函数使用以下结构：

![](..\image\Unix\3-10-4.png)

sa_flags的支持如下图所示：

![](..\image\Unix\3-10-16.png)

如果在sigaction结构中使用了SA_SIGINFO，则使用sa_sigaction信号处理程序。siginfo结构包含了信号产生的原因的有关信息。

## 函数sigsetjmp和siglongjmp

```c
#include <setjmp.h>
int sigsetjmp(sigjmp_buf env, int savemask); //若直接调用返回0，从siglongjmp调用返回非0；
int siglongjmp(sigjmp_buf env, int val);
```

如果savemask非0，则sigsetjmp在env中保存进程的当前信号屏蔽字，调用siglongjmp如果带非0 savemask的sigsetjmp调用已经保存了env，则siglongjmp从其中恢复保存的信号屏蔽字。

## 函数sigsuspend

在原子操作中设置信号屏蔽字，然后使得进程休眠：

```c
#include <signal.h>
int sigsuspend(const sigset_t *sigmask); 返回-1，并将errno设置为EINTR
```

## 函数abort

```c
#include<stdlib.h>
void abort(void);
```

将SIGABRT信号发送给调用进程。POSIX.1要求abort终止进程，它对所有打开标准IO流的效果应当与进程终止前对每个流调用fclose相同。

## 函数system

POSIX.1要求system忽略SIGINT和SIGQUIT，阻塞SIGCHLD。

## 函数sleep、nanosleep和clock_nanosleep

```c
#include <unistd.h>
#include <time.h>
unsigned int sleep(unsigned int seconds);
int nanosleep(const struct timespec *reqtp, struct timespec *remtp);
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *reqtp, struct timespec *remtp);
```

## 函数sigqueue

有些Unix系统开始增加对信号排队的支持。使用排队信号必须做以下几个操作：

1. 使用sigaction函数安装信号处理程序时指定SA_SIGINFO标志，如果没有给出这个标志，信号会延迟，但是否进入队列要取决于具体实现；

2. 在sigaction结构的sa_sigaction成员中提供信号处理程序，实现可能允许用户使用sa_handler字段，但不能获取sigqueue函数发送的额外信息；

3. 使用sigqueue发送信号：

   ```c
   #include <signal.h>
   int sigqueue(pid_t pid, int signo, const union sigval value); 成功返回0，出错返回-1；
   ```

此函数只能把信号发送给单个进程。信号不能被无限排队。

# 线程

## 线程概念

每个线程都包含表示执行环境必需的信息，其中包括进程中标识线程的线程ID、一组寄存器值、栈、调度优先级以及策略、信号屏蔽字、errno变量以及线程私有数据。一个进程的所有信息对该进程的所有线程都是共享的，包括可执行程序的代码、程序的全局内存和堆内存、栈以及文件描述符。

POSIX线程的功能测试宏是_POSIX_THREADS。应用程序可以将这个宏用于#ifdef测试，从而在编译时确定是否支持线程，也可以把\_SC_THREADS常数用于调用sysconf函数，进而在运行时确定是否支持线程。

## 线程标识

进程ID在整个系统是唯一的，但线程ID只有在它所属的进程上下文才有意义。线程id用pthread_t数据类型来表示，可以用以下函数来进行比较：

```c
#include <pthread.h>
int pthread_equal(pthread_t tid1, pthread_t tid2); //相等返回非0，否则返回0
```

线程可以通过pthread_self函数来获得自身的线程ID：

```c
#include <pthread.h>
pthread_t pthread_self(void); //返回线程的线程ID
```

## 线程创建

```c
#include <pthread.h>
int pthread_create(pthread_t *restrict tidp, const pthread_attr_t * attr, void *(*start_rtn)(void *), void *restrict arg); //成功返回0，否则返回错误编号
```

成功返回时，新的线程ID会被设置成tidp指向的内存单元。新创建的线程从start_rtn函数的地址开始运行。

线程创建时并不能保证是新创建的线程先运行还是调用线程先运行。新创建的线程可以访问进程的地址空间，并继承调用线程的浮点环境和信号屏蔽字，但该线程的挂起信号集会被清除。

## 线程终止

单个线程可以通过三种方式退出：

1. 线程可以简单的从启动例程返回，返回值是线程的退出码；
2. 线程可以被同一进程的其他线程取消；
3. 线程可以调用pthread_exit；

```c
#include <pthread.h>
void pthread_exit(void *rval_ptr);
```

进程中的其他线程可以通过调用pthread_join来访问rval_ptr这个指针：

```c
#include <pthread.h>
int pthread_join(pthread_t thread, void **rval_ptr); //成功返回0，否则返回错误编号
```

调用线程将一直阻塞，直到指定线程退出。如果线程简单的从它的启动例程返回，rval_ptr就包含返回码，如果线程被取消，由rval_ptr指定的内存单元就设置为PTHREAD_CANCELED。

线程可以调用pthread_cancel来请求取消同一进程的其他线程：

```c
#include <pthread.h>
int pthread_cancel(pthread_t tid); //成功返回0，否则返回错误编号
```

此函数并不等待线程终止，仅仅是提出请求。

线程可以安排它退出时需要调用的函数，如同进程退出时可以用atexit函数。一个线程可以建立多个清理处理程序，处理程序记录在栈中，即执行顺序与注册顺序相反：

```c
#include <pthread.h>
void pthread_cleanup_push(void (*rtn)(void *), void *arg);
void pthread_cleanup_pop(int execute);
```

当线程执行以下动作时，清理函数rtn是由pthread_cleanup_push函数调度的，调用时只有一个参数arg：

- 调用pthread_exit时；
- 响应取消请求时；
- 用非零execute参数调用pthread_cleanup_pop时；

如果execute设置为0，则清理函数不再调用。不管发生上述哪种情况，pthread_cleanup_pop都将删除上次pthread_cleanup_push调用建立的清理处理程序。

进程和线程之间相似关系如下图所示：

![](..\image\Unix\3-11-6.png)

除了pthread_join等待线程，还可以分离线程：

```c
#include <pthread.h>
int pthread_detach(pthread_t tid); //成功返回0，否则返回错误编号
```

## 线程同步

互斥变量用pthread_mutex_t数据类型表示，在使用之前必需先进行初始化，可以将其设置为常量PTHREAD_MUTEX_INITIALIZER（只适用于静态分配的互斥量），也可以使用pthread_mutex_init函数初始化。如果动态分配互斥量，在释放之前需要调用pthread_mutex_destroy。

```c
#include <pthread.h>
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
//成功返回0，否则返回错误编号
```

对互斥量进行加锁需要调用pthread_mutex_lock，解锁调用pthread_mutex_unlock。

```c
#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
//成功返回0，否则返回错误编号
```

如果线程不希望被阻塞，可以使用pthread_mutex_trylock，如果此时互斥量未锁住，则锁住互斥量不会阻塞直接返回0，否则pthread_mutex_trylock就会失败，返回EBUSY。

`pthread_mutex_timedlock` 用于尝试在指定时间内加锁一个 `pthread_mutex_t` 互斥锁，如果超时仍未获得锁则返回 `ETIMEDOUT`。

```c
#include <pthread.h>
#include <time.h>
int pthread_mutex_timedlock(pthread_mutex_t *restrict mutex, const struct timespec *restrict tsptr); //成功返回0，否则返回错误编号
```

对于读写锁来说，一次只有一个线程可以占有写模式的读写锁，但多个线程可以同时占有读模式的读写锁。与互斥量相比，读写锁在使用之前必须初始化，在释放它们底层的内存之前必须销毁：

```c
#include <pthread.h>
int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
//成功返回0，否则返回错误编号
```

要在读模式下锁定读写锁，需要调用pthread_rwlock_rdlock，要在写模式下锁住读写锁，需要调用pthread_rwlock_wrlock。解锁都可以使用pthread_rwlock_unlock解锁。

```c
#include <pthread.h>
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
//成功返回0，否则返回错误编号
```

Single UNIX Specification还定义了读写锁原语的条件：

```c
#include <pthread.h>
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
//成功返回0，否则返回错误编号
```

可以获取锁时，返回0，否则返回EBUSY。

与互斥量一样，Single UNIX Specification提供了带有超时的读写锁加锁函数：

```c
#include <pthread.h>
#include <time.h>
int pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict tsptr); 
int pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict tsptr);
//成功返回0，否则返回错误编号
```

条件变量本身是由互斥量保护的，线程在改变自身状态之前必须首先锁住互斥量。在使用条件变量之前，必须先进行初始化，可以把常量PTHREAD_COND_INITIALIZER赋值给静态分配的条件变量，或者使用pthread_cond_inid对动态分配的条件变量初始化。在释放条件变量底层的内存空间之前，可以使用pthread_cond_destroy对条件变量反初始化。

```c
#include <pthread.h>
int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
int pthread_cond_destroy(pthread_cond_t *cond);
//成功返回0，否则返回错误编号
```

使用pthread_cond_wait等待条件变量变为真。如果给定的时间内条件不能满足，会生成一个错误码。

```c
#include <pthread.h>
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
int pthread_cond_timewait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict tsptr);
//成功返回0，否则返回错误编号
```

当将锁住的互斥量传递给pthread_cond_wait时，互斥量解锁，当其返回时，互斥量会再次锁住。注意，等待时间是绝对值。

pthread_cond_signal至少唤醒一个等待该条件的线程，而pthread_cond_broadcast唤醒等待该条件的所有线程。

```c
#include <pthread.h>
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
//成功返回0，否则返回错误编号
```

自旋锁与互斥量类似，但它不是通过休眠使进程阻塞，而是在获取锁之前一直忙等。自旋锁可以用于以下场景：锁被持有的时间短，而且线程不希望在重新调度上花费太多成本。自旋锁通常作为底层原语实现其他类型的锁。在用户层，自旋锁并不是非常有用，当时间片到期或其他高优先级线程变成就绪可运行时，用户线程都可以被取消调度。

可以用pthread_spin_init对自旋锁进行初始化，用pthread_spin_destroy进行反初始化：

```c
#include <pthread.h>
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
//成功返回0，否则返回错误编号
```

pshared属性只在支持线程进程共享同步的平台上才用得到，如果设为PTHREAD_PROCESS_SHARED，则自旋锁能被可以访问锁底层内存的线程所获取，即使那些线程属于不同的进程，如果设置为PTHREAD_PROCESS_PRIVATE，则只能被初始化该锁的进程内部的线程所访问。

可以用pthread_spin_lock和pthread_spin_trylock进行加锁，前者在获取锁之前一直自旋，后者如果不能获取锁就立即返回EBUSY错误。不论以何种方式加锁，都可以用pthread_spin_unlock解锁。

```c
#include <pthread.h>
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);
//成功返回0，否则返回错误编号
```

如果线程已经加锁，则调用pthread_spin_lock的结果是未定义的，可能会返回EDEADLK，或者调用可能会永久自旋。试图对没有加锁的自旋锁解锁，结果也是未定义的。

屏障是用户协调多个线程并行工作的机制，它允许每个线程等待，直到所有的合作线程都到达某一点，然后从该点继续执行。屏障对象允许任意数量线程等待，直到所有线程完成处理工作。

```c
#include <pthread.h>
int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
//成功返回0，否则返回错误编号
```

初始化时可以用count指定在允许所有线程继续运行之前，必须到达屏障的线程数目。

可以用pthread_barrier_wait表明线程已经完成工作，准备等其他线程赶上来：

```c
#include <pthread.h>
int pthread_barrier_wait(pthread_barrier_t *barrier);
//成功返回0或者PTHREAD_BARRIER_SERIAL_THREAD，否则返回错误编号
```

一旦满足屏障计数，所有线程都被唤醒。返回PTHREAD_BARRIER_SERIAL_THREAD表明当前线程是最后一个到达屏障的线程，它可以被选为主线程来执行额外任务。

# 线程控制

## 线程限制

可以使用sysconf查询线程的有关限制，如下：

![](..\image\Unix\3-12-1.png)

## 线程属性

1. 每个对象与它自己类型的属性对象进行关联，一个属性对象可以代表多个属性，属性对象对程序来说是不透明的；
2. 有一个初始化函数，把属性设置为默认值；
3. 还有一个销毁属性对象的函数；
4. 每个属性都有一个从属性对象中获取属性值的函数；
5. 每个属性都有一个设置属性值的函数；

可以使用pthread_attr_init函数初始化pthread_attr_t结构。

```c
#include <pthread.h>
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
//成功返回0，否则返回错误编号
```

以下是各个操作系统平台对每个线程属性的支持情况：

![](..\image\Unix\3-12-3.png)

如果在线程一开始的时候就知道不需要了解线程的终止状态，就可以修改pthread_attr_t结构中的detachstate线程属性，让线程一开始就处于分离状态。可以使用pthread_attr_setdetachstate函数把线程属性设置为以下两个合法值之一：PTHREAD_CREATE_DETACHED以分离状态启动线程，或PTHREAD_CREATE_JOINABLE正常启动线程。应用程序可以获取线程的终止状态：

```c
#include <pthread.h>
int pthread_attr_getdetachstate(const pthread_attr_t *restrict attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int *detachstate);
//成功返回0，否则返回错误编号
```

对于支持线程栈属性的系统，可以使用以下方法管理线程栈属性：

```c
#include <pthread.h>
int pthread_attr_getstack(const pthread_attr_t *restrict attr, void **restrict stackaddr, size_t *restrict stacksize);
int pthread_attr_setstack(pthread_attr_t *addr, void *stackaddr, size_t stacksize);
//成功返回0，否则返回错误编号
```

如果线程栈的虚地址空间都用完了，可以用malloc和mmap来为可替代的栈分配空间，并用pthread_attr_setstack函数来改变新建线程的栈位置。

程序也可以通过pthread_attr_getstacksize和pthread_attr_setstacksize读取或设置线程属性stacksize。

```c
#include <pthread.h>
int pthread_attr_getstacksize(const pthread_attr_t *restrict attr, size_t *restrict stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
//成功返回0，否则返回错误编号
```

线程属性guardsize控制着线程栈末尾之后用以避免栈溢出的扩展内存大小。这个属性默认是由具体实现定义的，但常用值是系统页大小。可以将guardsize线程属性设置为0，不允许属性的这种特征行为发生：在这种情况下，不会提供警戒缓冲区。同样，如果修改了线程属性stackaddr，系统就认为我们将自己管理栈，进而使栈警戒缓冲区机制无效。

```c
#include <pthread.h>
int pthread_attr_getguardsize(const pthread_attr_t *restrict attr, size_t *restrict guardsize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
//成功返回0，否则返回错误编号
```

如果guardsize线程属性被修改，操作系统可能会把它取为页大小的整数倍。如果线程的栈指针溢出到警戒区域，应用程序就可能通过信号接收到出错信息。

## 同步属性

互斥量属性通过pthread_mutexattr_t结构表示。对于非默认属性，可以用pthread_mutexattr_init初始化，用pthread_mutexattr_destroy来反初始化。

```c
#include <pthread.h>
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
成功返回0，否则返回错误编号
```

pthread_mutexattr_init将用默认的互斥量属性初始化pthread_mutexattr_t结构，值得注意的三个属性是：进程共享属性、健壮属性以及类型属性。在POSIX.1中，进程共享属性是可选的，可以通过检查系统是否定义了\_POSIX_THREAD_PROCESS_SHARED符号来判断平台是否支持，也可以在运行时把\_SC_THREAD_PROCESS_SHARED参数传给sysconf函数检查。

在进程中，多个线程可以访问同一个同步对象，这是默认的行为，在这种情况下，进程共享互斥量属性需要设置为PTHREAD_PROCESS_PRIVATE。但也存在这样的机制：允许相互独立的多个进程把同一个内存数据块映射到各自独立的地址空间，多个进程访问共享数据通常也需要同步，如果进程共享互斥量属性设置为PTHREAD_PROCESS_SHARED，从多个进程彼此共享的内存数据块中分配的互斥量就可以用于这些进程的同步。

可以使用pthread_mutexattr_getpshared查询pthread_mutexattr_t结构得到它的进程共享属性，可以使用pthread_mutexattr_setpshared修改进程共享属性：

```c
#include <pthread>
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
//成功返回0，否则返回错误编号
```

互斥量健壮属性与多个进程共享的互斥量有关：

```c
#include <pthread>
int pthread_mutexattr_getrobust(const pthread_mutexattr_t *restrict attr, int *restrict robust);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);
//成功返回0，否则返回错误编号
```

健壮属性默认值为PTHREAD_MUTEX_STALLED，这意味着持有互斥量的进程终止时不需要采取特别的动作。这种情况下，使用互斥量后的行为是未定义的，等待互斥量解锁的应用程序会被拖住。另一个取值是PTHREAD_MUTEX_ROBUST，这将导致当线程调用pthread_mutex_lock时获取锁，而该锁被另一个进程持有，但它终止时未对该锁解锁时，此时线程会阻塞，从pthread_mutex_lock返回的值是EOWNERDEAD而不是0。

如果应用状态无法恢复，在线程对互斥量解锁后，该互斥量将处于永久不可用状态，为了避免这样的问题，可以调用pthread_mutex_consistent，指明与该互斥量相关的状态在互斥量解锁之前是一致的。

```c
#include <pthread.h>
int pthread_mutex_consistent(pthread_mutex_t *mutex);
//成功返回0，否则返回错误编号
```

如果线程没有先调用该函数就对互斥量解锁，那么其他试图获取该互斥量的阻塞线程就会获得错误码ENOTRECOVERABLE，如果这种情况互斥量就不再可用。

类型互斥量属性控制互斥量的锁定特性，有四种类型：

- PTHREAD_MUTEX_NORMAL：标准互斥量类型，不做特殊的错误检查或死锁检测；
- PTHREAD_MUTEX_ERRORCHECK：提供错误检查；
- PTHREAD_MUTEX_RECURSIVE：允许同一线程多次加锁，维护锁的计数，在加锁和解锁次数不相同的情况下不会释放锁。
- PTHREAD_MUTEX_DEFAULT：提供默认特性和行为；

有以下操作互斥量类型属性的方法：

```c
#include <pthread>
int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr, int *restrict type);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
//成功返回0，否则返回错误编号
```

读写锁属性通过以下方法初始化：

```c
#include <pthread>
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
//成功返回0，否则返回错误编号
```

读写锁支持的唯一属性是进程共享属性：

```c
#include <pthread>
int pthread_rwlockattr_getpshared(const pthread_rwlockxattr_t *restrict attr, int *restrict pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
//成功返回0，否则返回错误编号
```

Single UNIX Specification定义了条件变量的两个属性：进程共享属性和时钟属性。

```c
#include <pthread>
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_getpshared(const pthread_condattr_t *restrict attr, int *restrict pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
//成功返回0，否则返回错误编号
```

时钟属性控制pthread_cond_timedwait的超时参数采用的是哪个时钟。

```c
#include <pthread>
int pthread_condattr_getclock(const pthread_condattr_t *restrict attr, clockid_t *restrict clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);
//成功返回0，否则返回错误编号
```

屏障也有属性，目前只支持进程共享属性：

```c
#include <pthread>
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *restrict attr, int *restrict pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);
//成功返回0，否则返回错误编号
```

## 重入

支持线程安全的操作系统会在<unistd.h>中定义符号\_POSIX_THREAD_SAFE_FUNCTIONS，程序也可在sysconf函数传入\_SC_THREAD_SAFE_FUNCTIONS在运行时检查是否支持线程安全函数。

如果一个函数对多个线程来说是可重入的，就说这个函数是线程安全的，但不能说明对信号处理程序来说该函数也是可重入的。如果函数对异步信号处理函数的重入是安全的，那么可以说函数是异步信号安全的。

POSIX提供了以线程安全的方式管理FILE对象的方法。可以使用flockfile和ftrylockfile获取给定FILE对象关联的锁，这个锁是递归的。

```c
#include <stdio.h>
int ftrylockfile(FILE *fp); //成功返回0，若不能获取锁返回非0数值；
void flockfile(FILE *fp);
void funlockfile(FILE *fp);
```

如果标准I/O例程都获取它们各自的锁，那么一次一个字符的I/O时就会出现严重的性能下降，为了避免这种开销，出现了不加锁版本的基于字符的标准I/O例程：

```c
#include <stdio.h>
int getchar_unlocked(void);
int getc_unlocked(FILE *fp); //成功则返回下一个字符，如果遇到文件尾或出错，返回EOF
int putchar_unlocked(int c);
int putc_unlocked(int c, FILE *fp); //成功返回c，否则返回EOF
```

除非被flockfile和funlockfile包围，否则尽量不要调用这四个函数，因为它们会导致不可预期的结果。

## 线程私有数据

线程私有数据是存储和查询某个特定线程相关数据的一种机制。注意，即使是线程私有数据，也无法阻止同一进程的其他线程访问，虽然底层的实现无法阻止，但管理线程特定数据的函数可以提高线程间的数据独立性，使得线程不太容易访问到其他线程的线程特定数据。

在分配线程特定数据之前，需要创建与该数据关连的键：

```c
#include <pthread.h>
int pthread_key_create(pthread_key_t *keyp, void (*destructor)(void *));
//成功返回0，否则返回错误编号
```

创建的键存储在keyp指向的内存单元中，这个键可以被进程中的所有线程使用，但每个线程把这个键与不同的线程特定数据地址进行关联。创建新键时，每个线程的数据地址设为空值。可以为该键关联一个可选择的析构函数，当线程退出时，如果数据地址已经被置为非空值，那么析构函数就会被调用，它唯一的参数就是该数据地址。当线程调用pthread_exit或者线程执行返回时析构函数才会调用，如果线程调用了exit、\_exit、\_Exit或abort，或者其他非正常退出，就不会调用析构函数。

线程通常使用malloc为线程特定数据分配内存。线程也可以为线程特定数据分配多个键，每个键都可以有一个析构函数与它关联。每个操作系统实现可以对进程可分配的键的数量进行限制。

对所有的线程，都可以调用pthread_key_delete来取消键与特定线程数据之间的关联：

```c
#include <pthread.h>
int pthread_key_delete(pthread_key_t key); //成功返回0，否则返回错误编号
```

要保证无论多少线程，只有首次调用有效，可以使用pthread_once：

```c
#include <pthread.h>
pthread_once_t initflag = PTHREAD_ONCE_INIT;
int pthread_once(pthread_once_t *initflag, void (*initfn)(void));
//成功返回0，否则返回错误编号
```

initflag必须是一个非本地变量（如全局变量或者静态变量），而且必须初始化为PTHREAD_ONCE_INIT。

键一旦创建就可以通过pthread_setspecific把键和线程特定数据关联起来，可以通过pthread_getspecific函数获得线程特定数据的地址：

```c
#include <pthread.h>
void *pthread_getspecific(pthread_key_t key); //返回线程特定数据值，若没有值与该键关联，返回NULL
int pthread_setspecific(pthread_key_t key, const void *value); //成功返回0，否则返回错误编号；
```

如果没有线程特定数据值与键关联，pthread_getspecific将返回一个空指针，我们可以用这个返回值来确定是否需要调用pthread_setspecific。

## 取消选项

可取消状态和可取消类型没有在pthread_attr_t 结构中，这两个属性影响线程在响应pthread_cancel函数时所呈现从行为。

可取消状态可以是PTHREAD_CANCEL_ENABLE，也可以是PTHREAD_CANCEL_DISABLE。线程可以通过调用pthread_setcancelstate修改它的可取消状态。

```c
#include <pthread.h>
int pthread_setcancelstate(int state, int *oldstate); //成功返回0，否则返回错误编号
```

其设置state，并将就的状态存储在oldstate中，这是一个原子操作。

前文所述，pthread_cancel调用并不等待线程终止，在默认情况下，线程在取消请求发出之后仍旧运行，直到线程到达某个取消点。取消点是线程检查它是否被取消的一个位置，POSIX.1保证线程调用下列任何函数时，取消点都会出现：

![](..\image\Unix\3-12-14.png)

线程启动时默认的可取消状态是PTHREAD_CANCEL_ENABLE，当状态设为PTHREAD_CANCEL_DISABLE时，对pthread_cancel的调用并不会杀死线程，取消请求会是挂起状态，直到取消状态再次变为PTHREAD_CANCEL_ENABLE，线程将在下一个取消点对所有挂起的取消请求进行处理。

也可以调用pthread_testcancel函数在函数中添加自己的取消点：

```c
#include <pthread.h>
void pthread_testcancel(void);
```

POSIX.1定义的可选取消点如下：

![](..\image\Unix\3-12-15.png)

以上描述的默认的取消类型被称为推迟取消，调用pthread_cancel在线程到达取消点之前并不会真正取消，可以调用pthread_setcanceltype来修改取消类型：

```c
#include <pthread.h>
void pthread_setcanceltype(int type, int *oldtype); //成功返回0，否则返回错误编号
```

type可以是PTHREADCANCEL_DEFERRED，也可以是PTHREAD_CANCEL_ASYNCHRONOUS，原来的取消类型返回到oldtype指向的内存单元。使用异步取消时，线程可以在任意时间撤销。

## 线程和信号

进程的信号是递送到单个线程的，如果一个信号与硬件故障相关，该信号一般会被发送到引起该事件的线程中，而其他信号则被发送到任一线程。

sigpromask的行为在多线程的进程中并没有定义，线程使用pthread_sigmask来阻止信号发送。

```c
#include <signal.h>
int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset); //成功返回0，否则返回错误编号
```

set包含线程用于修改信号屏蔽字的信号集，how参数可以取以下三个值之一：SIG_BLOCK，把信号集添加到线程信号屏蔽字中，SIG_SETMASK，用信号集替换线程的信号屏蔽字，SIG_UNBLOCK，从线程信号屏蔽字中移除信号集。如果oset不为空，线程之前的信号屏蔽字就存在它指向的内存单元。线程可以通过把set设置为NULL，并且把oset参数设置为sigset_t结构的地址来获取当前的信号屏蔽字，这种情况下how参数会被忽略。

线程可以通过sigwait等待一个或多个信号出现：

```c
#include <signal.h>
int sigwait(const sigset_t *restrict set, int *restrict signop); //成功返回0，否则返回错误编号
```

set指定了等待的信号集，返回时，signop指向的整数包含发送信号的数量。

如果信号集中的某个信号在sigwait被调用时处于挂起状态，那么sigwait将无阻塞返回，在返回之前，sigwait将从进程中移除那些处于挂起等待状态的信号。如果具体实现支持排队信号，并且信号的多个实例被挂起，那么sigwait将会移除该信号的一个实例，其他实例继续排队。

为了避免错误行为发生，线程在调用sigwait之前必须阻塞那些它正在等待的信号。sigwait会原子的取消信号集的阻塞状态，直到有新的信号被递送，在返回之前，sigwait将恢复线程的信号屏蔽字。如果一个信号被捕获（例如通过sigaction），而且一个线程正在sigwait等待同一信号，那么这时将由操作系统决定以何种方式递送信号，既可以sigwait返回，也可以激活信号处理程序，但这两者不会同时发生。

将信号发送给进程可以用kill，将信号发送给线程可以使用pthread_kill：

```c
#include <signal.h>
int pthread_kill(pthread_t thread, int signo); //成功返回0，否则返回错误编号
```

可以传一个0值的signo来检查线程是否存在。如果信号的默认处理动作是终止进程，那么将信号传递给某个线程仍旧会杀死整个进程。

## 线程和fork

fork之后，子进程通过继承整个地址空间的副本，还继承了每个互斥量、读写锁和条件变量的状态。如果父进程包含一个以上的线程，子进程在fork返回之后，如果紧接着不是马上调用exec的话就需要清理锁状态。

在子进程内部，只存在一个线程，它由父进程调用fork的线程的副本构成的，如果父进程中的线程占有锁，子进程将同样占有锁，但子进程并不包含占有锁的线程的副本，所以子进程没办法知道它占有了哪些锁、需要释放哪些锁。

在多线程的进程中，为了避免不一致的问题，POSIX.1声明在fork返回和调用其中一个exec函数之间，子进程只能调用异步信号安全的函数。这限制了在调用exec之前子进程能做什么，但不涉及子进程中的锁状态的问题。要清除锁状态，可以调用pthread_atfork函数建立fork处理程序。

```c
#include <pthread.h>
int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));  //成功返回0，否则返回错误编号
```

其最多安装三个可以帮助清理锁的函数。prepare由父进程在fork创建子进程之前调用。这个fork处理程序是获取父进程定义的所有锁。parent是在fork创建子进程之后返回之前在父进程上下文中调用，这个fork处理程序是对prepare获取的所有锁进行解锁。child在fork返回之前在子进程上下文中调用，与parent一样，也必须释放prepare获取的所有锁。

可以多次调用pthread_atfork从而设置多套fork处理程序，如果不需要使用某个处理程序，可以给特定的处理程序参数传入空指针，它就不会起任何作用。使用多个fork处理程序时，parent和child处理程序按它们注册的顺序进行调用，prepare则与它们注册的顺序相反进行调用。

## 线程和I/O

pread和pwrite用来解决并发线程对同一文件的读写操作问题。

# 守护进程

守护进程是生存期长的一种进程，常常在系统引导装入时启动，仅在系统关闭时才终止，因为它们没有控制终端，所以说它们在后台运行。

## 守护进程的特征

父进程ID为0的各进程通常是内核进程，它们作为系统导引装入过程的一部分而启动（init是个例外，它是一个由内核在引导装入时启动的用户层次的命令）。内核进程是特殊的，通常存在于整个生命周期，以超级用户特权运行，无控制终端，无命令行。

对于需要在进程上下文执行工作却不被用户层进程上下文调用的每个内核组件，通常有他自己的内核守护进程，例如在linux中：

- kswapd守护进程也叫做内存换页守护进程，支持虚拟内存子系统在经过一段时间后将脏页面慢慢的写回磁盘来回收这些页面；
- flush守护进程可以在可用内存达到设置的最小阈值时将脏页面冲洗至磁盘，它也定期的将脏页面冲洗回磁盘来减少在系统出现故障时发生的数据丢失；
- sync_supers守护进程定期将文件系统元数据冲洗至磁盘；
- jbd守护进程帮助实现了ext4文件系统中的日志功能；

进程1通常是init（Mac OS X中是launchd），除了其他工作外，主要负责启动各运行层次特定的系统服务。rpcbind守护进程提供将远程工程调用程序号映射为网络端口号的服务，rsyslogd守护进程可以被由管理员启动的将系统消息记入日志的任何程序使用。inetd守护进程侦听网络接口，以便取得来自网络的对各种网络服务进程的请求。cron守护进程在定期安排的日期和时间执行命令。atd守护进程与cron类似，但每个任务它只执行一次。

## 编程规则

1. 首先要做的是调用umask将文件模式创建屏蔽字设置为一个已知值（通常是0）；
2. 调用fork，然后使父进程exit。这样做实现了以下几点，第一，如果该守护进程是一条简单的shell命令启动，那么父进程终止会让shell认为这条命令已经执行完毕，第二，虽然子进程继承了父进程的进程组ID，但获得了一个新的进程ID，这就保证了子进程不是一个进程组的组长进程，这是下面要进行的setsid的先决条件；
3. 调用setsid创建一个新会话，然后执行9.5节中的三个步骤，使得调用进程：1，成为新会话的首进程，2，成为一个新进程组的组长进程，3，没有控制终端；
4. 将当前工作目录更改为根目录；
5. 关闭不需要的文件描述符；
6. 某些守护进程打开/dev/null，使其具有文件描述符0，1和2；

## 出错记录

syslog的详细组织结构：

![](..\image\Unix\3-13-2.png)

有三种可以产生日志消息的方法：

1. 内核例程可以调用log函数；
2. 大多数用户进程（守护进程）调用syslog(3)来产生日志消息；
3. 用户进程可通过UDP端口514发送日志消息；

通常，syslogd守护进程读取所有三种格式的日志消息。此守护进程在启动时读取一个配置文件，一般为/etc/syslog.conf，该文件决定了不同种类的消息应该送向何处。该设施接口是syslog函数：

```c
#include <syslog.h>
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);
int setlogmask(int maskpri); //返回前日志记录优先级屏蔽字值
```

调用openlog使我们可以指定一个ident，此后，此ident将被加至每则日志消息中。

## 单实例守护进程

如果每一个守护进程创建一个有固定名字的文件，并在该文件的整体上加一把写锁，那么只允许创建一把这样的锁，之后创建写锁的尝试都会失败，这向后续守护进程副本指明已有一个副本正在运行。文件和记录锁提供了一种方便的互斥机制，如果守护进程在一个文件的整体上得到一把写锁，那么在该守护进程终止时，这把锁将被自动删除。

## 守护进程的惯例

在UNIX系统，守护进程遵循以下通用惯例：

- 若守护进程使用锁文件，那么该文件通常存储在/var/run目录中；
- 若守护进程支持配置选项，那么配置文件通常存放在/etc目录中；
- 守护进程可用命令行启动，但通常它们是由系统初始化脚本之一启动；
- 如果一个守护进程有一个配置文件，那么该守护进程启动时会读该文件，但在此之后一般就不会查看它。

# 高级I/O

## 非阻塞I/O

上文曾将系统调用分成两类：低速系统调用和其他，低速系统调用是可能会使进程永远阻塞的一类系统调用，包括：

- 如果某些文件类型（如读管道、终端设备和网络设备）的数据并不存在，读操作可能会使调用者永远阻塞；
- 如果数据不能被相同文件类型立即接受（如管道中无空间、网络流控制），写操作可能会使调用者永远阻塞；
- 在某种条件发生之前打开某些文件类型会发生阻塞（如要打开一个终端设备，需要先等待与之连接的调制解调器应答，又如若以只写模式打开FIFO，那么在没有其他进程已用读模式打开该FIFO时也要等待）；
- 对已经加上强制性记录锁的文件进行读写；
- 某些ioctl操作；
- 某些进程间通信函数；

非阻塞I/O使我们可以发出open、read和write这样的I/O操作，并使得这些操作永远不会阻塞。

对一个给定的描述符，有两种为其指定非阻塞I/O的方法：

1. 如果open获得描述符，则可指定O_NONBLOCK标志；
2. 对于一个已经打开的描述符，可调用fcntl，打开O_NONBLOCK标志；

POSIX.1要求对于一个非阻塞的描述符如果无数据可读，则read返回-1，errno被设置为EAGAIN。

## 记录锁

POSIX.1标准的基础是fcntl方法，下图列出了各种系统提供的不同形式的记录锁：

![](..\image\Unix\3-14-2.png)

fcntl对于记录锁，参数cmd有F_GETLK、F_SETLK或F_SETLKW。第三个参数指向flock结构的指针：

```c
struct flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
};
```

- l_type代表希望的锁类型：F_RDLCK（共享读锁）、F_WRLCK（独占性写锁）或F_UNLCK（解锁一个区域）；
- 要加锁或解锁区域的起始字节偏移量（l_start和l_whence）；
- 区域的字节长度（l_len）；
- 进程的ID（l_pid）持有的锁能阻塞当前进程；

关于加锁或解锁区域的说明还要注意以下几项规则：

- 指定区域起始偏移量的两个元素与lseek函数中最后两个参数类似。l_whence可选用的值是SEEK_SET、SEEK_CUR或SEEK_END；
- 锁可以在当前文件尾端开始或越过尾端开始，但不能在文件起始位置之前开始；
- 如果l_len为0，表示锁的范围可以扩展到最大可能偏移量；
- 为了对整个文件加锁，我们设置l_start和l_whence指向文件起始位置，并且指定l_len为0。

有以下的兼容性关系：

![](..\image\Unix\3-14-3.png)

如果一个进程对一个文件区间已经有了一把锁，后来该进程又企图在同一文件区间再加一把锁，那么新锁会替代已有锁。

加读锁时，该描述符必须是读打开的，加写锁时必须是写打开的。

以下介绍fcntl的三种命令：

- F_GETLK：判断由参数flockptr所描述的锁是否会被另外一把锁排斥（阻塞）。如果存在一把锁，它阻止创建由flockptr描述的锁，则该现有锁的信息将重写flockptr指向的信息，如果不存在这种情况，则除了将l_type设置为F_UNLCK之外，flockptr指向的其他信息不变；
- F_SETLK：设置由flockptr描述的锁。如果试图获得一把读锁或写锁，而兼容性规则阻止系统给这把锁，那么fcntl会立即出错返回，errno设置为EACEES或EAGAIN；
- F_SETLKW：此命令也用来清除由flockptr指定的锁，是F_SETLK的阻塞版本。如果请求的锁因要控制的部分进行了加锁而不能授予，则调用进程会陷入休眠，如果请求的锁可用或休眠由信号终端，那么该进程被唤醒；

注意，用F_GETLK测试能否建立一把锁，之后用F_SETLK或F_SETLKW企图建立锁，这两者不是原子操作，因此不能保证这中间被其他进程插入一把相同的锁。另外，F_GETLK命令决不会报告调用进程自己持有的锁。

关于记录锁的自动继承和释放有三条规则：

1. 锁与进程和文件两者相关联，有两重含义，一是当一个进程终止时它建立的锁全部释放，二是无论一个描述符何时关闭，该进程通过这一描述符引用的文件上的任何一把锁都会释放；
2. 由fork产生的子进程不继承父进程所设置的锁。对于继承过来的描述符，子进程需要调用fcntl才能获得他自己的锁。
3. 在执行exec之后，新程序可以继承原执行程序的锁；

建议性锁不能阻止对数据库文件有写权限的其他进程写这个数据库文件，而强制性锁会让内核检查每一个open、read和write，验证调用进程是否违反了正在访问文件上的某一把锁。对一个特定文件打开其设置组ID位、关闭其组执行位便开启了对该文件的强制性锁机制。

如果一个进程试图读或写一个强制性锁起作用的文件，而欲读或写的部分又由其他进程加上了锁，此时会发生什么，有以下可能性：

![](..\image\Unix\3-14-11.png)

通常，即使正在打开的文件具有强制性锁，该open也会成功，随后的read和write依从上图规则，但是如果欲打开的文件具有强制性锁，而且open调用中的标志指定为O_TRUNC或O_CREAT，则不论是否指定O_NONBLOCK，open都立即出错返回，errno设置为EAGAIN。

## I/O多路转接

这种技术是先构造一张感兴趣的描述符列表，然后调用一个函数，直到这些描述符中的一个已准备好I/O时该函数才返回。poll、pselect和select都可以进行I/O多路转接。

```c
#include <sys/select.h>
int select(int maxfdpl, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict tvptr); //返回就绪的描述符数目，超时返回0，出错返回-1
```

tvptr指定愿意等待的时间长度，单位是秒和微秒，有以下三种情况：

- tvptr == NULL：永远等待。如果捕捉到一个信号则中断无限等待。当指定的描述符中的一个已准备好或者捕捉到一个信号则返回。如果捕捉到一个信号，则select返回-1，errno设置为EINTR。
- tvptr->tv_sec == 0 && tvptr->tv_usec == 0：根本不等待。测试所有指定的描述符并立即返回；
- tvptr->tv_sec != 0 || tvptr->tv_usec != 0：等待指定的秒数和微秒数，当指定的描述符之一已经准备好或者指定的时间值已经超过时立即返回。如果超时到期还没有一个描述符准备好，则返回值是0.

对于fd_set数据类型，唯一可以进行的处理是：分配一个这种类型的变量，将这种类型的一个变量值赋给同类型的另一个变量，或对这种类型的变量使用以下四个函数中的一个：

```c
#include <sys/select.h>
int FD_ISSET(int fd, fd_set *fdset); //若fd在描述符集中返回非零值，否则返回零
void FD_CLR(int fd, fd_set *fdset); 
void FD_SET(int fd, fd_set *fdset); 
void FD_ZERO(fd_set *fdset); 
```

中间三个参数甚至可以全部是空指针，则select提供了比sleep更精确的定时器。maxfdpl是最大描述符编号+1，也可直接设置为FD_SETSIZE，它指定最大描述符数。

如果一个描述符上碰到了文件尾端，则select会认为该描述符是可读的，然后调用read返回0，这时Unix指示到达文件尾端的方法。

POSIX.1定义了一个select的变体：

```c
#include <sys/select.h>
int pselect(int maxfdpl, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, const struct timespec *restrict tsptr,
const sigset_t *restrict sigmask); //返回就绪的描述符数目，超时返回0，出错返回-1
```

pselect可以使用信号屏蔽字，在调用时安装该屏蔽字，返回时以前的恢复信号屏蔽字。

poll定义如下：

```c
#include <poll.h>
int poll(struct pollfd fdarray[], nfds_t nfds, int timeout); //返回就绪的描述符数目，超时返回0，出错返回-1
```

其构造了一个pollfd结构的数组，每个数组元素指定一个描述符编号以及对该描述符感兴趣的条件：

```c
struct pollfd {
	int fd;
	short events;
	short revents;
};
```

fdarray数组中的元素由nfds指定。返回时，revents成员由内核设置，用于说明每个描述符发生了哪些事件。events可设置的标志有：

![](..\image\Unix\3-14-17.png)

当一个描述符被挂断（POLLHUP）之后，就不能写该描述符，但仍有可能从该描述符读到数据。

timeout有如下情形：

- timeout == -1：永远等待，当指定的描述符中的一个准备好或捕捉到一个信号时返回。如果捕捉到一个信号，poll返回-1，errno设置为EINTR。
- timeout == 0：不等待。
- timeout > 0：等待timeout毫秒。

select和poll的可中断性。中断的系统调用的自动重启是4.2BSD引入的，但当时select是不重启的，这种特性在大多数系统中一直延续了下来，即使指定了SA_RESTART选项也是如此。但是在SVR4上，如果指定了SA_RESTART，那么select和poll也是自动重启的，为了将软件移植到SVR4派生的系统上阻止这一点，如果信号有可能中断select和poll，就要使用signal_intr函数。

## 异步I/O

在System V中，异步I/O是STREAMS系统的一部分，它只对STREAMS设备和STREAMS管道起作用，异步I/O信号是SIGPOLL。为了对一个STREAMS设备启动异步I/O，需要调用ioctl，将它的第二个参数设置为I_SETSIG。与STREAMS机制相关的接口在SUSv4中已被标记为弃用，所以不再讨论其细节。除了调用ioctl指定产生的SIGPOLL信号的条件之外，还应该建立信号处理程序。产生SIGPOLL的条件如下：

![](..\image\Unix\3-14-8.png)

而在BSD派生的系统中，异步I/O是信号SIGIO和SIGURG的组合。SIGIO是通用异步I/O，SIGURG则只用来通知进程网络山的带外数据已到达。为了接受SIGIO信号，需要执行以下步骤：

1. 调用signal或sigaction为SIGIO建立信号处理程序；
2. 以命令F_SETOWN调用fcntl来设置进程ID或组ID，用于接收对于该描述符的信号；
3. 以命令F_SETFL调用fcntl设置O_ASYNC文件状态标志，使在该描述符上可以进行异步I/O；

第三步仅能对指向终端或网络的描述符执行。对于SIGURG信号，只需执行第一步和第二步。

POSIX异步I/O使用AIO控制块来描述I/O接口，aiocb结构定义了AIO控制块：

```c
struct aiocb {
	int aio_fildes;
	off_t aio_offset;
	volatile void *aio_buf;
	size_t aio_nbytes;
	int aio_reqprio;
	struct sigevent aio_sigevent;
	int aio_lio_opcode;
};
```

aio_fildes字段表示打开的文件描述符，读或写操作从aio_offset偏移开始，读写操作都从缓冲区aio_buf开始，aio_nbytes包含了要读或写的字节数。注意，如果使用异步I/O接口向一个以追加模式打开的文件中写入数据，AIO控制块中的aio_offset字段会被忽略。应用程序使用aio_reqprio字段为异步I/O请求提示顺序，但不一定遵循该提示，aio_lio_opcode只能用于基于列表的异步I/O。aio_sigevent控制在I/O事件完成后如何通知应用程序。

在进行异步I/O之前需要初始化AIO控制块，调用aio_read或aio_write来进行异步读写操作：

```c
#include <aio.h>
int aio_read(struct aiocb *aiocb);
int aio_write(struct aiocb *aiocb);
//成功返回0，否则返回-1
```

要想强制所有等待中的异步操作不等待而写入持久化的存储中（相对于内存中的缓存来说），可以设立一个AIO控制块并调用aio_fsync函数：

```c
#include <aio.h>
int aio_fsync(int op, struct aiocb *aiocb); //成功返回0，出错返回-1
```

如果op设定为O_DSYNC，那么操作执行就像调用了fdatasync一样，如果设定为O_SYNC那么执行就像调用了fsync一样。

为了获知一个异步读写或者同步操作的完成状态，需要调用aio_error函数：

```c
#include <aio.h>
int aio_error(const struct aiocb *aiocb);
```

返回值是以下的四种情况：

- 0：异步操作完成，需要调用aio_return获取操作返回值；
- -1：对ai_error的调用失败；
- EINPROGRESS：异步读写或同步操作仍在等待；
- 其他情况：失败返回的错误码；

```c
#include <aio.h>
ssize_t aio_return(const struct aiocb *aiocb);
```

直到异步操作完成之前，都要小心不要调用此函数，操作完成之前的结果是未定义的。还需要小心对每个异步操作只调用一次，因为一旦调用了该函数，操作系统就可以释放掉包含I/O操作返回值的记录。如果aio_return本身失败，会返回-1并设置errno。

执行I/O操作时，如果还有其他事务处理不想被I/O操作阻塞，可以使用异步I/O，然后，如果在完成事务之后还有异步操作未完成，可以使用aio_suspend来阻塞进程直到操作完成：

```c
#include <aio.h>
int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout); //成功返回0，出错返回-1
```

该函数可能会返回下列三种情况的一种。如果被一个信号中断，它会返回-1并将errno设置为EINTR。如果在没有任何I/O完成的情况下超时，那么会返回-1将errno设置为EAGAIN。如果有I/O操作完成，那么返回0。如果在调用aio_suspend操作时所有操作I/O都已经返回，那么该函数将不阻塞直接返回。

list指向一个AIO控制块数组的指针，nent表明数组中的条目数。数组中的空指针会被跳过，其他条目都必须指向已用于初始化异步I/O操作的AIO控制块。

当不想等待异步I/O操作时，可以使用下列函数来取消：

```c
#include <aio.h>
int aio_cancel(int fd, struct aiocb *aiocb);
```

fd指定了未完成I/O操作的文件描述符。如果aiocb为NULL，系统将会尝试取消该文件上所有未完成的异步I/O操作。其他情况将尝试取消由aiocb描述的单个异步I/O操作。aio_cancel可能会返回以下四个值的一个：AIO_ALLDONE，所有操作在尝试取消它们之前已经完成；AIO_CANCELED，所有要求的操作已取消；AIO_NOTCANCELED，至少有一个要求的操作没有被取消；-1，对aio_cancel的调用失败，错误码存储在errno中。

如果异步I/O操作被成功取消，相应的AIO控制块调用aio_error将会返回错误ECANCELED。如果操作不能被取消，那么相应的AIO控制块不会因为对aio_cancel的调用而修改。

lio_listio可以以同步的方式使用，也能以异步的方式使用：

```c
#include <aio.h>
int lio_listio(int mode, struct aiocb *restrict const list[restrict], int nent, struct sigevent *restrict sigev); //成功返回0，出错返回-1
```

该函数提交一系列由AIO控制块列表描述符的IO请求。mode决定是否异步，如果设定为LIO_WAIT，该函数将会在列表指定的I/O操作完成之后返回，这种情况sigev参数将被忽略。如果mode设定为LIO_NOWAIT，那么该函数将立即返回。进程将在所有I/O完成后通过sigev指定的被异步通知，如果不想被通知，可以将sigev设定为NULL。

list指定AIO控制块列表，指定了要运行的I/O操作。nent指定了数组中的元素个数。AIO控制块可以包含NULL指针，这些条目将被忽略。

在每个AIO控制块中，aio_lio_opcode指定了该操作是一个读操作（LIO_READ）、写操作（LIO_WRITE），还是被忽略的空操作（LIO_NOP）。读操作会按照对应的AIO控制块被传给aio_read来处理，相应的写操作被传给aio_write来处理。

可以通过sysconf将name参数设置为\_SC_IO_LISTIO_MAX来设定AIO_LISTIO_MAX的值，类似，可以通过sysconf把name设置为\_SC_AIO_MAX来设定AIO_MAX的值。通过sysconf将name设置为\_SC_AIO_PRIO_DELTA_MAX来设定AIO_PRIO_DELTA_MAX的值。

![](..\image\Unix\3-14-19.png)

## 函数readv和writev

用于在一次函数调用中读写多个非连续缓冲区：

```c
#include <sys/uio.h>
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
//返回已读或已写的字节数，出错则返回-1
```

iovec结构如下：

```c
struct iovec {
	void *iov_base;
	size_t iov_len;
};
```

iov数组元素由iovcnt指定，最大值受限于IOV_MAX。

## 函数readn和writen

管道、FIFO和某些设备（特别是终端和网络），有以下两种性质：

1. 一次read操作返回的数据可能少于要求的数据，这不是一个错误，继续读即可；
2. 一次write操作的返回值也可能少于指定输出的字节数，这可能是内核输出缓冲区满了造成的，也不是个错误，继续写即可。

readn和writen是为了自动处理这两种情况，不属于规范，是自己定义的函数，见书中定义。

## 存储映射I/O

其能将一个磁盘文件映射到存储空间的一个缓冲区上。

```c
#include <sys/mman.h>
void *mmap(void *addr, size_t len, int prot, int flag, int fd, off_t off);
// 成功返回映射区的起始地址，出错返回MAP_FAILED
```

addr用于指定映射存储区的起始地址，通常设置为0，表示由系统选择。返回值是该映射区的起始地址。fd是指定要映射的文件描述符，在映射之前，必须先打开该文件，len是映射的字节数，off是要映射内容在文件的起始偏移量，prot指定了映射存储区的保护要求：

![](..\image\Unix\3-14-25.png)

注意，保护要求不得超过文件open时的访问权限。

映射存储区位于堆栈之间，这属于实现细节，各种实现之间可能不同。以下是flag影响映射存储区的多种属性：

- MAP_FIXED：返回值必须等于addr，因为不利于移植，所以不鼓励使用。如果未指定此标志，即使设置了addr，则内核则只是将其当作建议；
- MAP_SHARED：指定存储操作修改映射文件，必须指定本标志或MAP_PRIVATE，但不能同时指定两者；
- MAP_PRIVATE：存储操作会创建映射文件的一个私有副本。

off和addr通常被要求是系统虚拟存储页长度的倍数，虚拟存储页长可用带参数\_SC_PAGESIZE或\_SC_PAGE_SIZE的sysconf函数得到。注意，如果文件太短，而虚拟存储区又太大，这种情况在超出文件长度的区域进行修改不会反映到文件中，必须先加长文件。

与映射区相关的信号是SIGSEGV和SIGBUS。SIGSEGV通常用于指示进程试图访问对它不可用的存储区。如果存储区被mmap指定成了只读的，那么进程试图将数据存入这个存储区的时候也会产生此信号。如果映射区的某个部分在访问时已不存在，则产生SIGBUS信号。

子进程能通过fork继承存储映射区，新程序不能通过exec继承存储映射区。

mprotect可以更改一个现有映射的权限：

```c
#include <sys/mman.h>
int mprotect(void *addr, size_t len, int prot); //成功返回0，出错为-1
```

prot的合法值与mmap中的prot一样，addr必须是系统页长的整数倍。如果修改的页是通过MAP_SHARED映射到地址空间，那么修改并不会立即写回到文件，何时写回是由内核的守护进程决定的。

如果共享映射中的页已修改，可以调用msync将该页冲洗到被映射的文件中，msync类似fsync但作用于存储映射区。

```c
#include <sys/mman.h>
int msync(void *addr, size_t len, int flags); //成功返回0，否则为-1
```

如果映射是私有的，那么不修改映射文件。flags参数指定对冲洗存储区有某种程度的控制。可以指定MS_ASYNC来简单调试要写的页，如果希望在返回之前等待写操作完成，可指定MS_SYNC标志。MS_INVALLDATE是一个可选标志，允许我们通知系统丢弃那些与底层存储器没有同步的页。

进程终止时会自动解除映射，也可直接调用munmap函数，关闭映射使用的文件描述符并不解除映射：

```c
#include <sys/mman.h>
int munmap(void *addr, size_t len); //成功返回0，出错返回-1
```

调用munmap不会使映射区的内容写到磁盘上，对于MAP_SHARED区磁盘的更新，会在我们写入到映射区后的某个时刻，由内核自动进行。在存储区解除映射后，对MAP_PRIVATE存储区的修改会被丢弃。

# 进程间通信

## 管道

管道是UNIX系统IPC最古老的形式，所有UNIX系统都提供此种机制，但也有以下两种局限性：

1. 历史上是半双工的，现在某些系统提供全双工管道；
2. 管道只能在具有公共祖先的两个进程之间使用；

管道通过pipe函数创建：

```c
#include <unistd.h>
int pipe(int fd[2]); //成功返回0，出错返回-1
```

经由参数fd返回两个文件描述符，fd[0]为读而打开，fd[1]为写而打开。

fstat函数对管道的每一端都返回一个FIFO类型的文件描述符，可以用S_ISFIFO宏来测试管道。

通常，进程会先调用pipe，接着调用fork，从而创建父进程到子进程的IPC通道，反之亦然。

当管道的一端被关闭时，以下两种规则起作用：

1. 当读一个写端已被关闭的管道时，在所有数据都被读取后，read返回0，表示文件结束；
2. 如果写一个读端被关闭的管道，则产生信号SIGPIPE，如果忽略该信号或者捕捉该信号并从处理程序返回，则write返回-1，errno设置为EPIPE。

在写管道时，常量PIPE_BUF规定了内核的管道缓冲区大小，如果对管道调用write，并且写的字节数小于PIPE_BUF，则此操作不会与其他进程对同一管道（或FIFO）的write交叉进行，但是如果有多个进程同时写一个管道（或FIFO），并且我们要求写的字节数超过PIPE_BUF，那么写的数据有可能与其他进程所写的数据相互交叉。用pathconf或fpathconf可以确定PIPE_BUF的值。

## 函数popen和pclose

这两个函数实现的操作是：创建一个管道，fork一个子进程，关闭未使用的管道，执行一个shell命令，然后等待命令终止：

```c
#include <stdio.h>
FILE *popen(const char *cmdstring, const char *type)； //成功返回文件指针，出错返回NULL
int pclose(FILE *fp); //成功返回cmdstring的终止状态，出错返回-1
```

popen先执行fork，然后调用exec执行cmdstring，返回一个标准I/O文件指针，如果type是r，则文件指针连接到cmdstring的标准输出，如果是w，则文件指针连接到cmdstring的标准输入。pclose关闭标准I/O流。

## 协同进程

当一个过滤程序既产生某个过滤程序的输入，又读取该过滤程序的输出时，它就变成了协同进程。

## FIFO

又被称为命名管道，可以用来在不想关的进程之间交换数据。创建FIFO类似于创建文件，其路径名存在于文件系统：

```c
#include <sys/stat.h>
int mkfifo(const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
//成功返回0，否则返回-1
```

mode的说明与open函数中的mode相同。mkfifoat可以被用来在fd表示的目录相关的位置创建一个FIFO，这里有三种情形：

1. 如果path是绝对路径，则fd会被忽略；
2. 如果path是相对路径，则fd是一个打开目录的有效文件描述符，路径名和目录有关；
3. 如果path是相对路径，并且fd有一个特殊值AT_FDCWD，则路径名以当前目录开始；

当用mkfifo和mkfifoat创建FIFO时，要用open来打开它。当open一个FIFO时，非阻塞标志会产生下列影响：

- 一般情况下（没有指定O_NONBLOCK），只读open要阻塞到某个进程为写而打开这个FIFO为止，反之亦然；
- 如果指定了O_NONBLOCK，则只读open立即返回。如果没有进程为读打开一个FIFO，那么只写open将返回-1，并将errno设置为ENXIO。

类似管道，若write一个尚无进程为读而打开的FIFO，则产生信号SIGPIPE。若某个FIFO的最后一个写进程关闭了该FIFO，则将为该FIFO的读进程产生一个文件结束标志。

一个给定的FIFO有多个写进程是很常见的，这意味者如果不希望多个进程写的数据交叉，必须考虑原子写操作，和管道一样，常量PIPE_BUF说明了可被原子写到FIFO的最大数据量。

FIFO有以下两种用途：1）shell利用FIFO将数据从一条管道传送到另一条时，无需创建中间临时文件；2）客户进程-服务器进程程序中，FIFO用作汇聚点，在客户进程和服务器进程之间传递数据。

## XSI IPC

有三种称作XSI IPC的IPC：消息队列、信号量以及共享存储器。

每个内核中的IPC结构（消息队列、信号量以及共享存储段）都用一个非负整数的标识符加以引用。例如，向一个消息队列发送消息或者读取消息，只需要知道其队列标识符。与文件标识符不同，IPC标识符不是小的整数。当一个IPC结构被创建然后又被删除时，与这种结构相关的标识符连续加1，直到达到一个整数型的最大正值，然后又回转到0。

标识符是IPC对象的内部名，为了在外部能够访问，IPC对象都与一个键关联，将这个键作为该对象的外部名。无论何时创建一个IPC结构，都应该制定一个键。这个键的数据类型是key_t，通常在\<sys/types.h\>中被定义成长整型。

有多种方法可以使客户进程和服务器进程在同一IPC结构上汇聚：

1. 服务器进程可以制定键IPC_PRIVATE创建一个新的IPC结构，将返回的标识符存在某处（如一个文件）攻客户进程使用。键IPC_PRIVATE保证服务器创建一个新的IPC结构，缺点是操作系统需要服务器进程将整型标识符写道文件中，客户进程又需要读取这个文件取得此标识符；IPC_PRIVATE也可以用于父进程子关系，父进程指定IPC_PRIVATE创建一个新的IPC结构，返回的标识符可供fork后的子进程使用，接着子进程又可以将此标识作为exec函数的一个参数传给一个新程序。
2. 可以在一个公用头文件定义一个客户进程和服务器进程都认可的键，然后服务器进程指定此键创建一个新的IPC结构。这种方法的问题是该键可能已经与一个IPC结构结合，此种情况get函数（msgget、semget、shmget）出错返回，服务器进程必须处理这种错误，删除已经存在的IPC结构，然后再试着创建它。
3. 客户进程和服务器进程认可同一个路径名和项目ID(项目ID是0-255之间的字符值)，接着，调用ftok将这两个值变换为一个键：

```c
#include <sys/ipc.h>
key_t ftok(const char *path, int id); //成功返回键，出错返回(key_t)-1
```

path必须引用一个现有的文件，当产生键时，只使用id的低8位。

ftok创建键通常用以下方式构成：按给定的路径名取得其stat结构中的st_dev和st_ino字段，然后再将它们与项目ID组合。如果两个路径引用的是两个不同的文件，那么ftok通常为这两个路径名返回不同的键，但是因为i节点编号和键通常都存放在长整型中，所以创建键时可能会丢失信息，这意味着对于不同文件的两个路径名，如果使用同一id，那么可能产生相同的键。

三个get函数（msgget、semget和shmget）都有两个类似的参数：一个key和一个整型flag。在创建新的IPC结构时，如果key是IPC_PRIVATE或和当前某种类型的IPC结构无关，则需要指明flag的IPC_CREAT标志位。为了引用一个现有队列，key必须等于队列创建时指明的key的值，并且IPC_CREAT必须不被指明。

如果希望创建一个新的IPC结构，而且要确保没有引用具有同一标识符的一个现有IPC结构，那么必须在flag中同时指定IPC_CREAT和IPC_EXCL位，这样做了之后如果IPC结构已经存在就会出错，返回EEXIST。

XSI IPC为每一个IPC结构关联了一个ipc_perm结构。该结构规定了权限和所有者：

```c
struct ipc_perm {
	uid_t uid;
	gid_t gid;
	uid_t cuid;
	gid_t cgid;
	mode_t mode;
	...
};
```

如果想要了解完整定义，参见\<sys/ipc.h\>。在创建IPC结构时，对所有字段都赋初值，以后可以调用msgctl、semctl或shmctl修改uid、gid和mode字段。为了修改这些值，进程必须是IPC结构的创建者或者是超级用户。修改这些字段类似对文件调用chown和chmod。

mode字段对任何IPC结构都不存在执行权限，以下是权限说明：

![](..\image\Unix\3-15-24.png)

XSI IPC的一个基本问题是：IPC结构在整个系统范围内起作用，没有引用计数。如果某个创建了消息队列的进程放入消息后直接终止，那么消息队列及其内容不会被删除。它们会一直留存直到发生下列动作为止：某个进程调用msgrcv或msgctl读消息或删除消息队列；或某个进程执行ipcrm命令删除消息队列；或正在自举的系统删除消息队列。与管道相比，最后一个引用管道的进程终止时管道就被完全的删除了；对于FIFO来说，最后一个引用FIFO的进程终止时，虽然FIFO的名字仍保留在系统中直至被显式删除，但留在FIFO中的数据已经被删除了。

XSI IPC的另一个问题是这些IPC结构在文件系统中没有名字。不好对其进行操作。

不同形式的IPC比较如下：

![](..\image\Unix\3-15-25.png)

无连接指的是无需先调用某种形式的打开函数就能发送消息的能力。

## 消息队列

消息队列是消息的链接表，存储在内核中。msgget用于创建一个新队列或打开一个现有的队列。msgsnd将消息添加到队列尾端。每个消息包含一个正的长整型的字段、一个非负的长度以及实际数据字节数。msgrcv用于从队列中取消息。我们不一定要以先进先出的次序取消息，也可以按照消息类型取消息。

每个队列都有一个msqid_ds结构与其关联：

```c
struct msqid_ds {
	struct ipc_perm msg_perm;
	msgqnum_t msg_qnum;
	msqlen_t msg_qbytes;
	pid_t msg_lspid;
	pid_t msg_lrpid;
	time_t msg_stime;
	time_t msg_rtime;
	time_t msg_ctime;
	...
};
```

```c
#include <sys/msg.h>
int msgget(key_t key, int flag); //成功返回消息队列ID，出错返回-1
int msgctl(int msqid, int cmd, struct msqid_ds *buf); //成功返回0，出错返回-1
```

cmd指定对msqid指定的队列要执行的命令：

- IPC_STAT：取此队列的msqid_ds结构，将其存放在buf指向的结构中；
- IPC_SET：将字段msg_perm.uid、msg_perm.gid、msg_perm.mode和msg_qbytes从buf指向的结构复制到与这个队列相关的msgid_ds结构中。此命令只能由以下两种进程执行，一种是其有效用户ID等于msg_perm.cuid或msg_perm.uid，另一种是具有超级用户特权的进程。只有超级用户才能增加msg_qbytes的值；
- IPC_RMID：从系统中删除该消息队列以及仍在该队列的数据，这种操作立即生效，仍在使用这一消息队列的其他进程在它们下一次试图对此队列进行操作时将得到EIDRM错误。此命令只能由以下两种进程执行，一种是其有效用户ID等于msg_perm.cuid或msg_perm.uid，另一种是具有超级用户特权的进程。

调用msgsnd将数据放到消息队列中：

```c
#include <sys/msg.h>
int msgsnd(int msqid, const void *ptr, size_t nbytes, int flag); //成功返回0，否则返回-1
```

正如前面提到的，每个消息都由三部分组成：一个正的长整型类型的字段、一个非负的长度以及实际数据字节数。ptr参数指向一个长整数，包含了正的整型消息类型，紧接着消息数据，若发送的最长消息是512字节的，可定义以下结构：

```c
struct mymesg {
	long mtype;
	char mtext[512];
};
```

flag的值可以是IPC_NOWAIT，类似于IO的非阻塞标志，若消息队列已满，则指定此标志使得msgsnd立即出错返回EAGAIN。如果没有指定此标志，则进程会一直阻塞到：有空间可以容纳要发送的消息；或者从系统中删除了此队列；或者捕捉到一个信号，并从信号处理程序返回。在第二种情况下会返回EIDRM错误，最后一种情况则返回EINTR错误。

注意，对删除消息队列的处理不是很完善。在队列被删除后，仍在使用这一队列的进程在下次对队列进行操作时会出错返回，信号量机制也是如此。

msgrcv从队列中取消息：

```c
#include <sys/msg.h>
ssize_t msgrcv(int msqid, void *ptr, size_t nbytes, long type, int flag); //成功返回消息数据部分的长度，出错返回-1
```

ptr指向一个长整型数，其后跟随的是存储实际消息数据的缓存区，nbytes指定消息缓冲区的长度，若返回的消息长度大于nbytes，而且flag中设置了MSG_NOERROR位，则该消息会被截断。如果没有设置这一标志，而消息又太长，则出错返回E2BIG。参数type可以指定想要哪一种消息：

- type == 0：返回队列中的第一个消息；
- type > 0：返回队列中消息类型为type的第一个消息；
- type < 0：返回队列中消息类型值小于等于type绝对值的消息，如果这种消息有若干个，则取类型值最小的消息；

type值非0用于以非先进先出次序读消息。比如，如果程序对消息赋予优先权，那么type就可以是优先权值。如果一个消息队列由多个客户进程和一个服务器进程使用，那么type可以用来包含客户进程的进程ID。

也可以将flag指定为IPC_NOWAIT，使得操作不阻塞。这样如果没有指定类型的消息可用，则msgrcv返回-1，error设置为ENOMSG。如果没有指定则进程会一直阻塞到有指定类型的消息可用，或从系统中删除了此队列。

考虑到前文所述的问题，现在新的程序已经不再使用消息队列。

## 信号量

为了获得共享资源，进程需要执行以下操作：

1. 测试控制该资源的信号量；
2. 若此信号量的值为正，则进程可以使用该资源，这种情况下信号量减一；
3. 否则，若信号量的值为0，则进程进入休眠状态，直到信号量大于0，进程被唤醒返回步骤1；

当进程不再使用一个由信号量控制的共享资源时，信号量值增1。如果有进程正在休眠等待此信号量，则唤醒它们。

常用的信号量形式被称为二元信号量，它控制单个资源，其初始值为1，但一般而言，信号量的初值可以是任意一个正值，表示有多少个共享资源单位可供共享应用。然而，XSI信号量比这要复杂得多，以下三种特性造成了这种复杂性：

1. 信号量并非单个非负值，而是必须定义为一个或多个信号量值的集合，当创建信号量时，要制定集合中信号量值的数量；
2. 信号量的创建（semget）是独立于它的初始化（semctl）的，这是一个致命的缺点，因为不能原子的创建一个信号量集合，并且对该集合中的各个信号量值赋初值；
3. 即使没有进程正在使用各种形式的XSI IPC，它们仍然是存在的。有的程序在终止时没有释放已经分配给它的信号量。

内核为每个信号量集合维护着一个semid_ds结构：

```c
struct semid_ds {
	struct ipc_perm sem_perm;
	unsigned short sem_nsems;
	time_t sem_otime;
	time_t sem_ctime;
	...
};
```

每个信号量由一个无名结构表示，它至少包含以下成员：

```c
struct {
	unsigned short semval;
	pid_t sempid;
	unsigned short semncnt;
	unsigned short semzcnt;
	...
};
```

当想要使用XSI信号量时，首先需要调用函数semget来获取一个信号量ID：

```c
#include <sys/sem.h>
int semget(key_t key, int nsems, int flag);  //成功返回信号量ID，出错返回-1
```

nsems是该集合中的信号量数，如果是创建新集合，则必须制定nsems，如果是引用现有集合，则将nsems指定为0。

semctl包含了多种信号量操作：

```c
#include <sys/sem.h>
int semctl(int semid, int semnum, int cmd, ...); 
```

第四个参数可选，是否使用取决于cmd，如果使用该参数，其类型是semun。

semop自动执行信号量集合上的操作数组：

```c
#include <sys/sem.h>
int semop(int semid, struct sembuf semoparray[], size_t nops); //成功返回0，出错返回-1 
```

semoparray指向一个由sembuf结构表示的信号量操作数组。

如前文所述，如果进程终止时占用了经由信号量分配的资源，那么就会成为一个问题。无论何时只要为信号量操作指定了SEM_UNDO标志，然后分配资源，那么内核就会记住对于该特定信号量，分配给调用进程多少资源，当该进程终止时，不论自愿与否，内核都将检验该进程是否还有尚未处理的信号量调整值，如果有则按调整值对相应信号量值处理。

如果在多个进程间共享一个资源，可以使用信号量、记录锁和互斥量技术中的一种。若使用信号量，则先创建一个包含一个成员的信号量集合，然后将该信号量初始化为1，为了分配资源，以sem_op为-1调用semop。为了释放资源，以sem_op为+1调用semop。对于每个操作都指定SEM_UNDO，以处理在未释放资源条件下进程终止的情况。若使用记录锁，先创建一个空文件，并且用该文件的第一个字节（无需存在）作为锁字节，为了分配资源，先对该字节获得一个写锁，释放资源时则对该字节解锁，记录锁的性质保证了进程终止时内核会自动释放该锁。若使用互斥量，需要所有进程将相同文件映射到它们的地址空间，并使用PTHREAD_PROCESS_SHARED互斥量属性在文件的相同偏移处初始化互斥量。为了分配资源，对互斥量加锁，为了释放锁，解锁互斥量。这三种方式推荐使用记录锁。

## 共享存储

XSI共享存储与内存映射的文件不同之处在于，前者没有相关的文件，XSI共享存储段是内存的匿名段。内核为每个共享存储段维护一个结构，相关结构见书中说明。

调用的第一个函数通常是shmget，它获得一个共享存储标识符：

```c
#include <sys/shm.h>
int shmget(key_t key, size_t size, int flag); //成功返回共享存储ID，出错返回-1
```

参数size是该共享存储段的长度，以字节为单位，实现通常向上取整为系统页长的整数倍。但是如果应用指定的size并非系统页长的整数倍，那么最后一页余下的部分是不可使用的。如果正在创建一个新段，则必须指定其size，如果正在引用一个现存的段，则将size指定为0。当创建一个新段时，段内的内容初始化为0。

shmctl对共享存储段执行多种操作。

```c
#include <sys/sem.h>
int shmctl(int semid, int cmd, struct shmid_ds *buf); //成功返回0，否则返回-1 
```

详情见书中内容。

一但创建了一个共享存储段，进程就可以调用shmat将其连接到它的地址空间中：

```c
#include <sys/sem.h>
void *shmctl(int semid, const void *addr, int flag); //成功返回存储段的指针，否则返回-1 
```

连接到的地址与addr以及flag是否指定SHM_RND位有关：

- 如果addr为0，则此连接到由内核选择的第一个可用地址上，推荐此种；
- 如果addr非0，并且没有指定SHM_RND，则此段连接到addr指定的地址；
- 如果addr非0，并且指定了SHM_RND，则此段连接到addr-(addr mod SHMLBA)表示的地址，SHM_RND的意思是取整，SHMLBA的意思是低边界地址倍数，它总是2的乘方，该算式是将地址向下取最近一个SHMLBA的倍数。

如果flag指定了SHM_RDONLY位，则以只读方式连接此段，否则以读写方式连接。

shmat的返回值是该段连接的实际地址，如果出错返回-1，如果成功执行，那么内核将使得内核与该共享存储段相关的shmid_ds结构中的shm_nattch计数器值加1。

当操作结束时，则调用shmdt与该段分离。注意，这不从系统中删除其标识符以及相关的数据结构，直到某个进程带IPC_RMID命令的调用shmctl特地删除为止。

```c
#include <sys/shm.h>
int shmdt(const void *addr); //成功返回0，否则返回-1
```

如果进程是相关的，我们可以利用/dev/zero。先打开/dev/zero设备，然后指定长整型的长度调用mmap，映射成功即关闭此设备，然后进程创建一个子进程，子进程也利用这个映射区通信。

很多实现提供了一种类似/dev/zero的设施，成为匿名存储映射，为了使用这种功能，要在调用时指定MAP_ANON标志，并将文件描述符指定为-1。结果得到的区域是匿名的，并且创建了一个可与后代进程共享的存储区。

## POSIX信号量

POSIX信号量意在解决XSI信号量的几个缺陷：

- 相比于XSI接口，POSIX信号量考虑到了更高性能的实现；
- 使用更简单；
- 删除时表现更加完美；

POSIX信号量有两种形式：命名的和未命名的。它们的差异在创建和销毁上，但其他工作一样。未命名信号量只存在内存中，并要求能使用信号量的进程必须可以访问内存。这意味着它们只能在同一进程中不同线程中使用，或者不同进程已经映射相同内存内容到它们的地址空间中的线程。相关，命名信号量可以通过名字访问，因此任何知道它们名字的进程的线程都可以使用。

可以调用sem_open来创建一个新的命名信号量或者使用一个现有信号量：

```c
#include <semaphore.h>
sem_t *sem_open(const char *name, int oflag, ... /* mode_t mode, unsigned int value */); //成功返回信号量的指针，否则返回SEM_FAILED
```

当使用一个现有的命名信号量时，仅仅指定两个参数：信号量的名字和oflag的参数的0值。当oflag参数有O_CREAT标志集时，如果命名信号量不存在，则创建一个新的，如果已经存在，则会被使用，但不会有额外的初始化。当指定O_CREAT标志时，需要提供额外两个参数，mode指定谁可以访问信号量，value指定信号量的初值，取值是0-SEM_VALUE_MAX。如果想要确保创建信号量，可以设置oflag为O_CREAT|O_EXCL，如果信号量已经存在，会导致sem_open失败。

为了增加可移植性，信号量命名必须遵循一定的规则：

- 名字的第一个字符应是斜杠/；
- 名字不应包含其他斜杠以此避免实现定义的行为；
- 名字的最大长度是实现定义的，不应该长于\_POSIX_NAME_MAX个字符长度。

完成信号量操作后，使用如下函数来释放相关资源：

```c
#include <semaphore.h>
int sem_close(sem_t *sem); //成功返回0否则返回-1
```

如果进程没有首先调用该函数而退出，那么内核将自动关闭任何打开的信号量。

可以使用sem_unlink来销毁一个命名信号量：

```c
#include <semaphore.h>
int sem_unlink(const char *name); //成功返回0否则返回-1
```

该函数删除信号量的名字，如果没有打开的信号量引用，则该信号量会被销毁，否则，销毁将延迟到最后一个打开的引用关闭。

可以使用sem_wait和sem_trywait来实现信号量的减一操作。

```c
#include <semaphore.h>
int sem_trywait(sem_t *sem);
int sem_wait(sem_t *sem)
//成功返回0，出错返回-1
```

使用sem_wait时如果信号量计数是0就会阻塞，直到成功使得信号量减一或被信号中断时才返回。可以使用sem_trywait来避免阻塞，如果信号量是0则会返回-1并将errno置为EAGAIN。

第三个选择是阻塞一段时间：

```c
#include <semaphore.h>
#include <time.h>
int sem_timedwait(sem_t *restrict sem, const struct timespec *restrict tsptr);
//成功返回0，出错返回-1
```

如果信号量可以立即减1，那么超时值就不重要，如果超时到期信号量还是不能减1，将会返回-1并将errno设置为ETIMEDOUT。

可以调用sem_post使得信号量增1：

```c
#include <semaphore.h>
int sem_post(sem_t *sem); //成功返回0，否则为-1
```

调用该函数时如果sem_wait已经使进程阻塞，那么进程会被唤醒并且被sem_post增1的信号量计数会再次被sem_wait减一。

在单个进程使用时未命名信号量更容易，仅仅改变创建和销毁的方式，可以用sem_init来创建要给未命名信号量：

```c
#include <semaphore.h>
int sem_init(sem_t *sem, int pshared, unsigned int value); //成功返回0，否则为-1
```

pshared表明是否在多个进程中使用信号量。如果是将其设置成一个非0值。value指定了信号量的初值。需要声明一个sem_t类型的变量并把它的地址传递给sem_init来实现初始化，而不是像sem_open那样返回一个指向信号量的指针。如果想要在两个进程之间使用信号量，需要确保sem参数指向两个进程之间共享的内存范围。

对未命名信号量使用完成后，使用sem_destroy来丢弃它：

```c
#include <semaphore.h>
int sem_destroy(sem_t *sem); //成功返回0，否则为-1
```

sem_getvalue来检索信号量值：

```c
#include <semaphore.h>
int sem_getvalue(sem_t *restrict sem, int *restrict valp); //成功返回0，否则为-1
```

成功后valp返回的整数值将包含信号量值，但请注意，我们试图使用我们读取的这个值时，信号量的值可能已经变量，除非使用额外的同步机制来避免这种竞争。

# 网络IPC：套接字

## 套接字描述符

套接字描述符在UNIX系统被当作一种文件描述符。为创建一个套接字，调用socket函数：

```c
#include <sys/socket.h>
int socket(int domain, int type, int protocol); //成功返回文件描述符，出错返回-1
```

domain确定通信的特性，包括地址格式。各个域都有自己表示地址的格式，而表示各个域的常熟都以AF_开头，意指地址族。

大多数系统还定义了AF_LOCAL域，这是AF_UNIX的别名。如下：

![](..\image\Unix\3-16-1.png)

type确定套接字的类型，进一步确定通信特征，如下：

![](..\image\Unix\3-16-2.png)

protocol通常是0，表示为给定的域和套接字类型选择默认协议。如下：

![](..\image\Unix\3-16-3.png)

以下总结了大多数以文件描述符为参数的函数使用套接字描述符时的行为：

![](..\image\Unix\3-16-4.png)

套接字是双向的，可以通过shutdown来禁止一个套接字的I/O：

```c
#include <sys/socket.h>
int shutdown(int sockfd, int how); //成功返回0，出错返回-1
```

如果how是SHUT_RD，那么无法从套接字读取数据，如果是SHUT_WR，则无法使用套接字发送数据，如果是SHUT_RDWR，则无法读也无法写。

## 寻址

进程标识由两部分组成，一部分是计算机的网络地址，它可以帮助标识网络上我们想要通信的计算机，另一部分是该计算机上用端口号表示的服务，它可以帮助标识特定的进程。

字节序是一个处理器架构特性，用于指示像整数这样的大数据类型内部的字节如何排序。如果支持大端字节序，那么最大字节地址出现在最低有效字节，小端则相反。

网络协议指定了字节序。对于TCP/IP应用程序，以下四个函数用来帮助字节序和网络字节序的转换：

```c
#include <arpa/inet.h>
uint32_t htonl(uint32_t hostint32);  //返回以网络字节序表示的32位整数
uint16_t htons(uint16_t hostint16);  //返回以网络字节序表示的16位整数
uint32_t ntohl(uint32_t netint32);  //返回以主机字节序表示的32位整数
uint16_t htohs(uint16_t netint16);  //返回以主机字节序表示的16位整数
```

为使不同格式地址能够传入套接字函数，地址会被强制转换成一个通用的地址结构sockaddr：

```c
struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
	...
};
```

套接字实现可以自由添加额外的成员并且定义sa_data成员的大小。

因特网地址定义在<netinte/in.h>头文件中，在IPV4中套接字地址由sockaddr_in表示，IPV6中用sockaddr_in6表示。

有时需要打印人所理解的地址格式，BSD网络软件包含inet_addr和inet_ntoa，用于二进制地址格式与点分十进制字符表示之间的相互转换，但这些函数仅适用于IPV4地址，inet_ntop和inet_pton同时支持ipv4和ipv6：

```c
#include <arpa/inet.h>
const char *inet_ntop(int domain, const void *restrict addr, char *restrict str, socklen_t size);  //成功返回地址字符串指针，否则返回NULL；
int inet_ptop(int domain, const void *restrict str, void *restrict addr);  //成功返回1,格式无效返回0，出错返回-1
```

size指定了保存文本字符串的缓冲区大小，两个常数用于简化工作：INET_ADDRSTRLEN定义了足够大的空间来存放一个表示IPv4地址的文本字符串，INET6_ADDRSTRLEN定义了一个足够大的空间来存放一个表示IPv6地址的文本字符串。

通过gethostent可以找到给定计算机的主机信息：

```c
#include <netdb.h>
struct hostent *gethostent(void);  //成功返回指针，出错返回NULL
void sethostent(int stayopen);
void endhostent(void);
```

如果文件数据库文件没有打开，gethostent会打开它。gethostent会返回文件中的下一个条目。sethostent会打开文件，如果文件已经被打开，那么将其回绕。stayopen设置为非0时，调用gethostent之后文件仍然是打开的。endhostent可以关闭文件。

可以采用以下的接口来获得网络名字和编号：

```c
#include <netdb.h>
struct netent *getnetbyaddr(uint32_t net, int type);
struct netent *getnetbyname(const char *name);
struct netent *getnetent(void);
//成功返回指针，出错返回NULL
void setnetent(int stayopen);
void endnetent(void);
```

其他转换接口见书中内容。

bind来关联地址和套接字：

```c
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t len); //成功返回0，出错返回-1
```

对于因特网域，如果指定IP地址为INADDR_ANY，则套接字端点可以被绑定到所有的系统网络接口上。这意味着可以接收这个系统所安装的任何一个网卡的数据包。

可以使用getsockname来发现绑定到套接字上的地址：

```c
#include <sys/socket.h>
int getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict alenp); //成功返回0，否则返回-1
```

调用该函数之前，将alenp设置为一个指向整数的指针，该整数指定缓冲区sockaddr的长度，返回时，该整数会被设置为返回地址的大小。如果地址和提供缓冲区长度不匹配，地址会被自动截断而不报错。如果当前没有地址绑定到该套接字，则其结果是未定义的。

如果套接字已经和对等方连接，可以使用getpeername来找到对方的地址：

```c
#include <sys/socket.h>
int getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict alenp); //成功返回0，否则返回-1
```

## 建立连接

```c
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *addr, socklen_t len);
//成功返回0，否则为-1
```

addr是我们想要与之通信的地址，如果sockfd没有绑定到一个地址，connect会给调用者绑定一个默认地址。

由于历史原因，Single UNIX Specification警告，如果connect失败，套接字的状态会变成未定义的。

如果套接字处于非阻塞模式，在连接不能马上建立时，connect会返回-1并且将errno设置为EINPROGRESS。程序可以使用poll或select来判断文件描述符何时可写，如果可写，连接完成。

connect也能用于无连接的网络服务（SOCK_DGRAM），如果SOCK_DGRAM套接字调用connect，传送的报文的目标地址会设置为connect中指定的地址，这样每次传送报文就不需要再提供地址。另外，仅能接收指定地址的报文。

```c
#include <sys/socket.h>
int listen(int sockfd, int backlog);
//成功返回0，否则为-1
```

backlog提供了一个提示，提示系统该进程所要入队的未完成连接请求数量，其实际值由系统决定，但上限由<sys/socket.h>中的SOMAXCONN指定。一旦队列满，系统就会拒绝多余的连接请求。

一旦服务器调用了listen，所用的套接字就能接收连接请求。使用accept获得连接请求并建立连接：

```c
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict len);  //成功返回文件描述符，出错返回-1
```

返回的文件描述符连接到调用connect的客户端，这个新的套接字描述符具有和原始套接字相同的套接字类型和地址族。传给accept的原始套接字没有关联到这个连接，而是继续保持可用状态并接收其他连接请求。

如果不关心客户端表示，可以将addr和len设为NULL，否则在调用accept之前，将addr设为足够大的缓冲区来存放地址，并将len指向的整数设为这个缓冲区大小。返回时，accept会在缓冲区填充客户端的地址，并且更新len的整数来反应该地址的大小。

如果没有连接请求在等待，accpet会阻塞直到一个请求到来，如果sockfd处于非阻塞模式，accept会返回-1并将errno设置为EAGAIN或者EWOULDBLOCK。

服务器可以使用poll或select来等待一个请求到来，这种情况下，一个带有连接请求的套接字会以可读的方式出现。

## 数据传输

send和write很像，但可以指定标志来改变处理传输数据的方式：

```c
#include <sys/socket.h>
ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags); //成功返回发送的字节数，出错返回-1
```

类似write，使用之前套接字必须已经连接。flags的作用如下所示：

![](..\image\Unix\3-16-13.png)

即使send成功返回，也不表示对端一定接收了数据，我们保证的只是数据已经被无错误的发送到网络驱动程序上。

对于支持报文边界的协议，如果发送的单个报文长度超过协议支持的最大长度，那么send会失败，并将errno设置为EMSGSIZE。对于字节流协议，send会阻塞直到整个数据传输完成。sendto跟send类似，但可以在无连接的套接字指定一个目标地址：

```c
#include <sys/socket.h>
ssize_t sendto(int sockfd, const void *buf, size_t nbytes, int flags,
const struct sockaddr *destaddr, socklen_t destlen); //成功返回发送的字节数，出错返回-1
```

通过套接字发送数据时还有一个选择，可以调用带有msghdr结构的sendmsg来指定多重缓冲区传输数据，这和writev很相似：

```c
#include <sys/socket.h>
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags); //成功返回发送的字节数，出错返回-1
```

recv和read很相似，但recv可以指定标志来控制如何接收数据：

```c
#include <sys/socket.h>
ssize_t recv(int sockfd, const void *buf, size_t nbytes, int flags); //成功返回数据的字节数，如果无可用数据或对等方已经按序结束返回0，出错返回-1
```

flags总结如下：

![](..\image\Unix\3-16-14.png)

当指定MSG_PEEK标志可以查看下一个要读取的数据而不取走它，当再次调用read或其中一个recv函数时会返回刚才查看的数据。对于SOCK_STREAM套接字，接收的数据可以比预期少，MSG_WAITALL标志会阻止这种行为，直到请求的数据全部返回recv函数才会返回。对于SOCK_DGRAM和SOCK_SEQPACKET，MSG_WAITALL没有改变什么行为，因为基于这些报文的套接字类型一次读取就返回整个报文。如果发送者已经调用shutdown来结束传输或者网络协议支持按默认的顺序关闭并且发送端已经关闭，那么当所有数据接收完毕后recv会返回0。

如果有兴趣定位发送者，可以使用recvfrom来得到数据发送者的源地址：

```c
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *restrict buf, size_t len, int flags, struct sockaddr *restrict addr, socklen_t *restrict addrlen); //返回数据的字节长度，若无可用数据或对方已经按序结束，返回0，出错返回-1
```

如果addr非空它将包含发送者的套接字端点地址，在调用recvfrom之前，需要设置addrlen指向一个整数，该整数包含addr指向的套接字缓冲区的字节长度，返回时该整数设置为该地址的实际字节长度。

为了将接收到的数据送入多个缓冲区，类似于readv或者想接收辅助数据，可以使用recvmsg：

```c
#include <sys/socket.h>
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags); //返回数据的字节长度，若无可用数据或对方已经按序结束，返回0，出错返回-1
```

## 套接字选项

套接字提供了两个套接字选项来控制套接字行为，一个接口用来设置选项，一个用来查询选项的状态。可以获取和设置以下三种选项：

1. 通用选项，工作在所有套接字类型上；
2. 在套接字层次管理的选项，但依赖下层选项的支持；
3. 特定于某协议的选项，每个协议独有的；

可以使用setsockopt来设置套接字选项：

```c
#include <sys/socket.h>
int setsockopt(int sockfd, int level, int option, const void *val, socklen_t len);  //成功返回0，出错返回-1
```

level标识了选项应用的协议。如果是通用的套接字层次选项，则level设置为SOL_SOCKET，否则level设置为控制这个选项的协议编号。对于TCP，level是IPPROTO_TCP，对于IP，level是IPPROTO_IP，以下是Single UNIX Specification定义的通用套接字层次选项：

![](..\image\Unix\3-16-21.png)

val根据选项的不同指向一个数据结构或整数。如果整数非0则启用选项，如果为0则禁止选项，len指定val指向对象的大小。

可以使用getsockopt来查看选项的当前值：

```c
#include <sys/socket.h>
int getsockopt(int sockfd, int level, int option, void *restrict val, socklen_t *restrict lenp);  //成功返回0，出错返回-1
```

lenp是一个指向整数的指针，在调用该函数之前设置为复制选项缓冲区的长度，如果选项的实际 长度大于此值，则选项会被截断，如果小于，则此值更新为实际长度。

## 带外数据

带外数据是一些通信协议支持的可选功能，与普通数据相比，它允许更高优先级的数据传输。带外数据先行传输，即使传输队列已有数据。TCP支持带外数据，但UDP不支持。

TCP将带外数据称为紧急数据。TCP仅支持产生一个字节的紧急数据，但允许紧急数据在普通数据传输机制之外传输。为了产生紧急数据，可以在三个send函数中任一个指定MSG_OOB标志，当发送的字节数超过一个时，最有一个字节被视为紧急数据字节。

如果通过套接字安排了信号的产生，那么紧急数据被接收时会发送SIGURG信号。在fcntl中使用F_SETOWN来设置一个套接字所有权，如果fcntl第三个参数为正，那么它指定的就是进程ID，如果是非-1的负值，那么它代表的就是进程组ID。

TCP支持紧急标记概念，即在普通数据流中紧急数据所在的位置。如果采用套接字选项SO_OOBINLINE，那么可以在普通数据中接收紧急数据。为帮助判断是否已经到达紧急标记，可以使用sockatmark：

```c
#include <sys/socket.h>
int sockatmark(int sockfd);  //若在标记处，返回1，没在标记处，返回0，出错返回-1
```

当带外数据出现在套接字读取队列时，select会返回一个文件描述符并有一个待处理的异常条件。如果在接收当前的紧急数据之前又有新的紧急数据到来，那么已有的字节会被丢弃。

## 非阻塞和异步I/O

在套接字非阻塞模式下，在没有数据可用或输出空间没有足够空间来发送消息时不会阻塞而是会失败，并将errno设置为EWOULDBLOCK或EAGAIN。当这种情况发生时，可以使用poll或select来判断能否接收或传输数据。

Single UNIX Specification包含通用异步I/O机制的支持，套接字机制有其自己的处理异步I/O方式，但没有标准化。

在基于套接字的异步I/O中，当从套接字读取数据或者空间变得可用时，可以安排要发送的信号SIGIO，启用异步I/O是一个两步骤的过程：

1. 建立套接字所有权，这样信号可以被传递到合适的进程；
2. 通知套接字当I/O操作不会阻塞时发送信号；

可以用以下三种方式完成第一个步骤：在fcntl中使用F_SETOWN，在ioctl使用FIOSETOWN，在ioctl使用SIOCSPGRP。

要完成第二个步骤，有以下两个选择：在fcntl中使用F_SETFL命令并启用文件标志O_ASYNC，在ioctl中使用FIOASYNC命令。

下图是相关平台对这些选项的支持情况：

![](..\image\Unix\3-16-23.png)

# 高级进程间通信

## UNIX域套接字

Unix域套接字用于在同一计算机上运行的进程之间的通信。虽然套接字也可以做到，但UNIX域套接字的效率更高。UNIX域套接字仅仅复制数据，不执行协议处理，不需要添加或删除网络报头，无需计算校验和，不要产生顺序号，无需发送确认报文。

UNIX域套接字提供流和数据报两种接口。UNIX域数据报服务是可靠的，既不会丢失报文也不会传递出错。UNIX套接字就像是套接字和管道的混合。可以使用它们面向网络的域套接字接口或者使用socketpair来创建一对无命名的、相互连接的UNIX域套接字。

```c
#include <sys/socket.h>
int socketpair(int domain, int type, int protocol, int sockfd[2]); //成功返回0，出错返回-1
```

一般来说，操作系统仅对UNIX域提供支持。一对相互连接的UNIX域套接字可以起到全双工管道的作用：两端对读写开放，我们称其为fd管道，以便与普通半双工管道区分。

UNIX域套接字地址由sockaddr_un结构表示。可使用bind绑定地址，如下所示：

![](..\image\Unix\3-17-5.png)

Linux中还可以使用抽象命名空间。抽象命名空间是 Unix 域套接字中以 `\0`（null 字符）开头的套接字路径名，用于在内核中命名套接字，而不创建真正的文件。示例如下：

```c
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

int main() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // 抽象命名空间名以 '\0' 开头，注意：这是非空字符串但第一个字节是 0
    const char *abstract_name = "\0my_socket";
    memcpy(addr.sun_path, abstract_name, strlen(abstract_name));

    bind(sockfd, (struct sockaddr*)&addr, sizeof(sa_family_t) + strlen(abstract_name));
    listen(sockfd, 5);

    std::cout << "Server listening..." << std::endl;

    int client = accept(sockfd, NULL, NULL);
    write(client, "Hello", 5);
    close(client);
    close(sockfd);
}

```

## 唯一连接

服务器进程可以使用标准bind、listen和accept函数为客户进程安排一个唯一UNIX域连接。客户进程使用connect与服务器进程联系。

服务器进程在接受客户进程连接时，还可以进行以下校验，确认套接字属于客户进程：

![](D:\project\study\image\Unix\3-17-9.png)

可以使用UNIX域套接字来传送文件描述符。

getopt可以帮助开发者以一致的方式处理命令行选项：

```c
#include <unistd.h>
int getopt(int argc, char *const argv[], const char *options);
extern int optind, opterr, optopt;
extern char *optarg;
//若所有选项被处理完返回-1，否则返回下一个字符；
```

如果options传入了一个iu:z字符串，说明iu选项可以不需要参数，z必须要参数。该函数一般用在循环体内，循环直到返回-1为止。

# 终端I/O

## 综述

终端I/O有两种不同的工作模式：

1. 规范模式输入处理，这种模式下终端输入以行为单位进行处理，对于每个读请求，终端驱动程序最多返回一行；
2. 非规范模式输入处理，输入字符不装配成行；

## 获得和设置终端属性

可以调用tcgetattr和tcsetattr来检测和修改各种终端选项标志和特殊字符：

```c
#include <termios.h>
int tcgetattr(int fd, struct termios *termptr);
int tcsetattr(int fd, int opt, const struct termios *termptr);
//成功返回0，否则返回-1
```

因为这两个函数只对终端设备操作，如果fd没有引用终端设备，则出错返回-1，errno设置为ENOTTY。

tcsetattr的opt可以指定什么时候新的终端属性才起作用，有以下选项：

- TCSANOW：更改立即发生；
- TCSADRAIN：发送了所有输出后更改才发生。若更改输出参数则应使用此选项；
- TCSAFLUSH：发送了所有输出后更改才发生，更进一步，在更改发生时未读的所有输入数据都被丢弃；

注意，tcsetattr即使未能执行要求的动作也返回OK，因此在调用该函数设置属性后，要调用tcgetattr将实际属性与希望的属性比较，以检测是否有区别。

## stty命令

在命令行中用stty来检查更改属性。

## 波特率函数

多数终端设备对输入输出使用同一波特率，但只要硬件许可，可以将其设置为两个不同的值。

```c
#include <termios.h>
speed_t cfgetispeed(const struct termios *termptr);
speed_t cfgetospeed(const struct termios *termptr);
int cfsetispeed(struct termios *termptr, speed_t speed);
int cfsetospeed(struct termios *termptr, speed_t speed); //成功返回0，出错返回-1
```

两个cfget函数的返回值以及两个cfset函数的speed参数都是以下常量之一：B50、B75、B110、B134、B150、B200、B300、B600、B1200、B1800、B2400、B4800、B9600、B19200、B38400。B0表示挂断。

## 行控制函数

以下函数提供了终端设备的行控制能力，以上函数都要求fd引用一个终端设备，否则出错返回-1，errno设置为ENOTTY。

```c
#include <termios.h>
int tcdrain(int fd);
int tcflow(int fd, int action);
int tcflush(int fd, int queue);
int tcsendbreak(int fd, int duration);
```

## 终端标识

在大多数UNIX系统中，控制终端的名字一直是/dev/tty。POSIX.1提供了一个运行时函数来确定控制终端的名字：

```c
#include <stdio.h>
char *ctermid(char *ptr); //成功返回指向控制终端名的指针，出错返回指向空字符串的指针。
```

## 终端窗口大小

大多数UNIX系统提供了一种跟踪当前终端窗口大小的方法，当窗口大小变化时，内核通知前台进程组。内核为每个终端和伪终端都维护了一个winsize结构。

ioctl可以操作此结构。

# 伪终端

## 概述

使用伪终端时进程的典型行为如下：

- 通常，一个进程打开伪终端主设备，然后fork，子进程回建立一个新的会话，打开一个伪终端从设备，将文件描述符复制到标准输入标准输出以及标准错误，然后调用exec，伪终端从设备成为子进程的控制终端；
- 对于伪终端从设备的用户进程来说，其标准输入标准输出标准错误都是终端设备。通过这些描述符，用户可以处理所有终端I/O函数，但由于伪终端不是真正的终端设备，所以无意义的函数调用，如改变波特率、发送中断符等将被忽略；
- 任何写到伪终端主设备的都会作为从设备的输入，反之亦然。

伪终端可以用来做网络登录服务器、窗口系统终端模拟等、

# 数据库函数库

以下是操作系统常用的数据库函数库：

![](..\image\Unix\3-20-1.png)

# 与网络打印机通信

内容见书，一般用不到。