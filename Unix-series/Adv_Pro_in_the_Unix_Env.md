# 信号

## 信号概念

信号是软件中断。每个信号的名字都以SIG开头。比如SIGABRT是夭折信号，进程调用abort函数产生这种信号，SIGALRM是闹钟信号，由alarm函数设置定时器超时产生。在头文件signal.h中，信号都被定义成正整数常量。不存在编号为0的信号，POSIX.1将此种信号编号称为空信号。

信号相关的动作如下：

1. 忽略此信号，大多数信号都可采用这种方式处理，但两种信号除外：SIGKILL和SIGSTOP。其不能被忽略的原因是他们向内核和超级用户提供了使得进程终止或停止的可靠方法。
2. 捕捉信号。
3. 执行系统默认动作。

在图示中，SUS列中，点表示此种信号定义为基本POSIX.1规范部分，XSI表示该信号定义在XSI扩展部分。在系统默认动作列，终止+core表示在进程当前工作目录的core文件中复制了该进程的内存映像。

![](..\image\Unix\3-10-1.png)