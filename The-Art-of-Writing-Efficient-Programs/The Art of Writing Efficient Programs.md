# The Art of Writing Efficient Programs

## 性能和并发性

#### 性能是什么

性能有以下几个指标：吞吐量，指运行时间；功耗，指计算过程消耗的功率；实时性，则用尾部延迟作为指标，延迟指数据准备好和处理完成之间的延迟，尾部延迟指的是最慢的请求或操作的延迟时间，通常以百分位数的形式给出，比如95th百分位数指的是在所有请求中，最慢的5%的请求的延迟时间，如果一个请求的延迟时间为t，平均计算时间为t0，则该请求的尾部延迟为(t-t0)/t0。

## 性能测试

### 性能测试示例

文中给出的例子为：

```shell
clang++-11 -g -03 -mavx2 -Wall -pedantic compare.C example.C -lprofiler -o example
CPUPROFILE=prof.data ./example
```

最重要的是编译时链接profiler，但这段示例并不能生成prof.data，原因在于程序中没有真正使用profiler，所以编译器根本没有链接profiler，一种方式在程序中直接调用ProfilerStart(filename)和ProfilerStop()，强制使用这个库，另一种方法是强制链接器不剔除未引用的库，修改如下：

```shell
 g++ substring_sort.c -Wl,--no-as-needed,-lprofiler -o example
 CPUPROFILE=prof.data ./example
```

其中-Wl就是将指定的选项传递给链接器ld。

当然还可以强制引入分析机制，这样不再需要链接profiler，如下：

```shell
LD_PRELOAD=.../libprofiler.so
```

最后使用google-pprof列出每个函数花费的时间：

```shell
google-pprof --text ./example prof.data
```

### 性能基准测试

一般我们测量计时信息，可以简单写出如下的代码：

```c++
#include <chrono>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
…
auto t0 = system_clock::now();
… do some work …
auto t1 = system_clock::now();
auto delta_t = duration_cast<milliseconds>(t1 – t0);
cout << "Time: " << delta_t.count() << endl;
```

这测量的是实时时间，更详细的分析则通常需要测量CPU时间，即CPU的工作时间和空闲时间。

为了测量CPU时间，必须使用特定于操作系统的系统格式，在linux和其他posix兼容的系统上，可以使用clock_gettime来使用硬件的高精度计时器：

```c++
timespec t0, t1;
clockid_t clock_id = …; // Specific clock
clock_gettime(clock_id, &t0);
… do some work …
clock_gettime(clock_id, &t1);
double delta_t = t1.tv_sec – t0.tv_sec + 1e-9*(t1.tv_nsec – t0.tv_nsec);
```

硬件计时器可以通过clock_id的值选择，其中实时时钟的ID为CLOCK_REALTIME，CPU计时器的两个是CLOCK_PROCESS_CPUTIME_ID，测量当前程序使用CPU时间的计时器，CLOCK_THREAD_CPUTIME_ID是测量线程调用使用时间的计时器。如果CPU时间与实际时间不匹配，可能是机器过载（许多其他进程正在争夺CPU资源）或者程序耗尽内存（如果程序使用的内存超过机器的物理内存，将使用慢的多的磁盘进行数据交换，当程序等待从磁盘调入内存时，CPU无法执行任何工作）。如果没有太多的计算而是等待用户输入，或者从网络接收数据，或者做一些其他不占用CPU资源的工作，则cpu时间小得多，观察这种行为最简单的方法是调用sleep。

### 性能分析

perf是基于硬件性能计数器和基于时间采样的分析器，运行这个分析器最简单的方法是收集整个程序的计数器值，可以通过perf stat命令完成，不需要额外的编译选项：

```shell
perf stat ./example
```

分析器本身可以列出所有已知的事件，并可以计数：

```shell
perf list
```

当然也可以自己指定其他的计数器：

```shell
perf stat -e cycles,instructions,branchs ./example
```

使用perf进行收据收集，必须使用调试信息进行编译，并使用perf record -g(记录符号信息)命令运行分析器。也可以指定计数器或一组计数器，但可以指定采样频率。收集完信息后可以使用perf report将收集的数据可视化。

谷歌的分析器需要链接profiler库，各选项通过环境变量控制。

### 微基准测试

即运行一小块代码，测试其性能。