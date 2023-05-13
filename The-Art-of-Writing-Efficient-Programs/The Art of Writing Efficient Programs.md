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

标准表示，编译器可以对程序进行更改，只要这些更改不会改变可观测对象的行为即可，标准对于什么构成了可观察行为也非常具体：

1. 对易变（volatile）对象的访问（读和写）需要严格按照它们的表达式语义进行，特别是对于同一线程上其他易变对象的访问，编译器不能重排；
2. 程序终止时，写入文件的数据与写入程序执行时的数据一样；
3. 发送到交互设备的提示文本，将在程序等待输入之前显示出来，简单来说，输入和输出操作不能省略或重排；

可以使用谷歌的benchmark库来测试。

## CPU架构、资源和性能

#### 微基准测试性能

当全局安装benchmark后，只需要运行以下命令：

```shell
g++ -g -O3 -mavx2 -Wall -pedantic test.cpp -lbenchmark -pthread -lrt -lm -o example
```

其中-g代表生成带有调试信息的可执行文件，-O3开启最高级别优化，-mavx2启用AVX2指令集，-Wall开启所有警告。-pedantic开启严格的ANSI/ISO C++标准模式，-lbenchmark链接benchmark库，-pthread链接pthread库，-lrt使用real-time库，-lm使用math库。

在示例中，运行如下代码：

```c++
for (size_t i = 0; i < N; ++i) {
    a1 += p1[i] + p2[i];
    a2 += p1[i] * p2[i];
    a3 += p1[i] << 2;
    a4 += p2[i] – p1[i];
    a5 += (p2[i] << 1)*p2[i];
    a6 += (p2[i] - 3)*p1[i];
}
```

发现处理器一次可以执行多个操作，这就是所谓的指令级并行。

我们可以通过LLVM-MCA来确认，第一步是用分析器标注代码，选择要分析的代码段：

```c++
#define MCA_START __asm volatile("# LLVM-MCA-BEGIN");
#define MCA_END __asm volatile("# LLVM-MCA-END");

        for (size_t i = 0; i < N; ++i) {
MCA_START
            a1 += b1[i] ? p1[i] : p2[i];
MCA_END
        }
```

使用clang编译：

```shell
clang test.cpp -g -O3 -mavx2 -mllvm -x86-asm-syntax=intel  -S -o - | llvm-mca -mcpu=btver2 -timeline
```

使用C++编译为：

```shell
g++ test.cpp -g -O3 -mavx2 -masm=intel -S -o - | llvm-mca -mcpu=btver2 -timeline
```

在一次迭代中执行一次操作与两次操作的结果如下：

![3-12](..\image\the-art-of-writing-effective-program\3-12.png)

![3-13](..\image\the-art-of-writing-effective-program\3-13.png)

第一次实验每次迭代执行3条指令，用时55个周期，第二次实验每次迭代执行6条指令，但用时只有56个周期，说明处理器在同样长的时间执行了两倍的指令。

这说明当在寄存器中有了一些值之后，在相同的值上加法计算可能不会降低任何性能，除非程序已经非常高效，将硬件压到了极致。当然，这种经验实际可能并没有什么用处。

### 数据依赖和流水线

对CPU的分析表明，只要操作数在寄存器上，处理器就可以执行多个操作，它只依赖于两个值，所需的时间与将这些值相加所需的时间一样多，但依赖于两个值的限定非常严重。比如以下的代码：

```c++
 for (size_t i = 0; i < N; ++i) {
    a1 += (p1[i] + p2[i])*(p1[i] - p2[i]);
}
```

因为存在数据依赖，最终的结果依赖加减运算的结果。硬件设计者想出了流水线的解决方案：

![3-15](..\image\the-art-of-writing-effective-program\3-15.png)

这样可以实现加法与乘法交错执行，即将复杂的表达式分解成多个阶段，在流水线中执行。这样完全消除了由数据依赖引起的性能损失。

然而，看起来这是不可能实现的，因为下一个迭代必须等待上一个迭代停止，使用它的寄存器。硬件解决这个矛盾方法是使用寄存器重命名的计数，即程序中看到的寄存器名，例如rsi，其实不是真正的寄存器，可以由CPU映射到真实的物理寄存器上，所以同名rsi可以映射到具有相同大小和功能的不同寄存器上。

将循环转换成线性代码称为循环展开。

### 流水线和分支

流水线也有前提条件：为了从循环迭代中交叉执行代码，必须知道将要执行什么代码。因此，条件执行或分支也是流水线的障碍。

### 投机执行

比如如下的代码：

```c++
int f(int* p) {
    if (p) {
    	return *p;
    } else {
    	return 0;
    }
}
```

通过假设循环的结束条件不会在当前的迭代之后发生，可以将下一次迭代的指令与当前迭代的指令交叉，这样就有更多的指令可以并行执行。无论如何，预测都会出错，所以需要做的就是放弃一些不应该计算的结果，看起来像是从未计算过一样。但对于上面的代码，如果在为null的时候走了为真的分支，就会造成崩溃，所以处理器是知道推测执行是否应该执行。

### 复杂条件的优化

在复杂条件下，以上的投机执行不怎么奏效，因为有不可预测的分支，所以需要手动优化。对于以下代码：

```c++
for (size_i i = 0; i < N; ++i) {
	if ((c1[i] && c2[i]) || c3[i]) { … } else { … }
}

```

如果预先计算所有的条件表达式，并存储在一个新数组中，大多数编译器不会干掉这个临时数组：

```c++
for (size_i i = 0; i < N; ++i) {
	c[i] = (c1[i] && c2[i]) || c3[i];
}
…
for (size_i i = 0; i < N; ++i) {
	if (c[i]) { … } else { … }
}

```

当然，现在用于初始化c[i]的布尔表达式面临着分支预测错误的问题，因此只有当第二个循环执行的次数比初始化循环多很多时，转换才有用。通常有效的优化方法是用加法或乘法，或按位&和|操作替代逻辑&&和||操作，这样做之前，必须确定&&和||操作的参数是布尔值而不是整数。

### 无分支计算

循环展开是一个非常特殊的优化，编译器会做这件事情。将有分支转换为无分支可能带来惊人的性能收益，比如：

```c++
unsigned long* p1 = ...; // Data
bool* b1 = ...; // Unpredictable condition
unsigned long a1 = 0, a2 = 0;
for (size_t i = 0; i < N; ++i) {
    if (b1[i]) {
    	a1 += p1[i];
    } else {
    	a2 += p1[i];
    }
}
```

可以如下使用如下做法消除分支：

```c++
unsigned long* p1 = ...; // Data
bool* b1 = ...; // Unpredictable condition
unsigned long a1 = 0, a2 = 0;
unsigned long* a[2] = { &a2, &a1 };
for (size_t i = 0; i < N; ++i) {
    a[b1[i]] += p1[i];
}
```

注意，以下的做法是不合理的：

```c++
decltype(f1)* f[] = { f1, f2 };
for (size_t i = 0; i < N; ++i) {
	a += f[b1[i]](p1[i], p2[i]);
}
```

因为函数调用本身就破坏了流水。如果必须基于一个条件从多个函数中进行选择，函数指针表比链式if-else语句更有效。

## 内存架构和性能

### 测试访问内存速度

比如使用以下的基准测试：

```c++
#define REPEAT2(x) x x
#define REPEAT4(x) REPEAT2(x) REPEAT2(x)
#define REPEAT8(x) REPEAT4(x) REPEAT4(x)
#define REPEAT16(x) REPEAT8(x) REPEAT8(x)
#define REPEAT32(x) REPEAT16(x) REPEAT16(x)
#define REPEAT(x) REPEAT32(x)

template <class Word>
void BM_read_seq(benchmark::State& state) {
    const size_t size = state.range(0);
    void* memory = ::malloc(size);
    void* const end = static_cast<char*>(memory) + size;
    volatile Word* const p0 = static_cast<Word*>(memory);
    Word* const p1 = static_cast<Word*>(end);
    for (auto _ : state) {
        for (volatile Word* p = p0; p != p1; ) {
            REPEAT(benchmark::DoNotOptimize(*p++);)
        }
        benchmark::ClobberMemory();
    }
    ::free(memory);
    state.SetBytesProcessed(size*state.iterations());
    state.SetItemsProcessed((p1 - p0)*state.iterations());
}

```

### 内存速度：数字

通过运行基准测试，发现数据量与读取速度成反比，32kb的L1缓存速度很快，数据量都能装入L1缓存，读取速度就不依赖数据量，超过32kb数据，读取速度就开始下降，数据现在适合L2缓存，它更大，但速度更慢，依次类推，直到数据大小超过8M，开始真正从主存读取数据。

### 优化内存性能

如果数据相对较大，顺序存取访问内存的速度要比随机存取快，因为大多数数据在缓存中。如果能够知道数据大小，可以预先分配一个数组。但如果不能知道数据的大小，直接使用vector可能不是一个好的选择，因为增长vector的效率非常低，这时候就需要一种更智能的数据结构，将vector的内存和list的调整大小的效率结合起来，如图所示：

![4-11](..\image\the-art-of-writing-effective-program\4-11.png)

这种数据结构以固定数量的块分配内存，通常小到足以装到L1缓存（通常在2kb到16kb之间），每个块都用作数组，块本身在一个链表中。在STL中，有这样的数据结构std::deque。

如果算法中发生了数据访问模式的改变，那么改变的数据结构通常对算法有利，即使要付出一些内存复制的代价。

可使用如下指令来查看L1缓存的使用效率：

```shell
$ perf stat -e \
cycles,instructions,L1-dcache-load-misses,L1-dcache-loads \
./program
```

### 机器里的幽灵

Spectre攻击的示意图如下：

![4-17](..\image\the-art-of-writing-effective-program\4-17.png)

即使用投机执行过程中获得的a[i]来索引另一个数组t，之后，数组中的一个元素t[a[i]]将加载到缓存中，t的其余部分从未访问仍然在内存中，当投机执行结束后会检测到错误预测，且投机操作的所有后果都将回滚，包括潜在的内存访问错误，然而t[a[i]]的值仍然在缓存中，这会有一个可观察的结果，t的一个元素的访问速度要比其他元素快得多。

## 线程、内存和并发

### 线程和并发

对于适合L1和L2缓存的小数据集，每个线程的内存速度几乎保持不变，即使是16个线程，然而进入L3缓存或超过它的大小，增加线程能提高内存速度，但文中示例中超过4个线程之后速度就会下降，从8个增加到16个性能改进非常小，系统中没有足够的带宽以足够快的速度将数据写入内存。

如果程序在单线程的情况下内存受限，那么性能就会受到在主存中移动数据速度的限制，当期望从并发中获得性能增益时就会受到严格的限制。对于多线程程序，更重要的是避免内存限制，这里使用的技术是分割计算，在更小的数据集上完成更多的工作，这些数据集适合存放在L1或L2缓存，再重新安排计算，当然可能会以重复一些计算为代价。优化内存访问模式，顺序访问而不是随机访问。如果还不行可能就需要更合适的并发算法。

### 内存同步的代价

在这一节的测试中，证明了锁定互斥锁是一个相当耗时的操作，即使在一个线程上，互斥锁需要23ns，而原子变量需要7ns，随着线程数量的增加，性能下降的更快。

### 共享数据很贵

为什么数据共享很贵，假设有一个原子的全局数组，每个线程访问数组中自己的元素，明明互不影响，但仍有比较高的代价，这是由于伪共享的存在。数据在处理器和内存之间移动的方式见下图：

![5-7](..\image\the-art-of-writing-effective-program\5-7.png)

数据是通过内存总线从主存复制到高速缓存，内存总线宽的多，可以从内存复制到高速缓存并返回的最小数据量称为缓存行，在所有X86 CPU上，一条缓存行的大小为64字节，当CPU需要为一个原子事务锁定内存位置时，比如一个原子增量，CPU可能在写一个数据，并锁定整个缓存行。这就解释了伪共享，即使相邻的数组元素在线程之间并没有真正共享，但确实占用了相同的缓存行。

分析表明，访问共享数据的高成本的真正原因是，必须维护对缓存行的独占访问，并确保所有 CPU 的缓存中有一致的数据。当一个 CPU 获得了独占访问权，并且更新了缓存行上的 1 位后，所 有其他 CPU 的缓存行副本就过期了。在其他 CPU 访问同一缓存行的数据之前，必须从主存中获 取更新后的内容，这将花费较长的时间。

### 并发和内存序

内存序最不受限制的是自由序，两个线程看到的操作可能完全不一样。对于获取-释放内存序，当原子操作按照这个顺序执行时， 可以保证访问内存，并在原子操作之前执行的操作，在另一个线程对同一原子变量执行原子操作之前是可见的。类似地，在原子操作之后执行的操作只有在对同一变量进行原子操作之后才可见。对于获取-释放内存序，首先顺序是相对于两个线程在同一个原子变量上执行的操作定义的，第二，在访问原子变量时两个线程都必须使用获取-释放内存序，最后，所有的顺序都是根据对原子变量的操作前后给出的。这种操作顺序可以用如下的图表示：

![5-10](..\image\the-art-of-writing-effective-program\5-10.png)

有时候我们可能需要不那么严格的内存序，比如释放内存序，所有在使用栅栏的原子操作之前执行的操作，必须在其他线程执行相应的原子操作之前可见：

![5-11](..\image\the-art-of-writing-effective-program\5-11.png)

获取内存序则是保证栅栏后执行的所有操作对栅栏后的其他线程可见：

![5-12](..\image\the-art-of-writing-effective-program\5-12.png)

获取和释放内存栅栏总是成对使用的：如果一个线程使用一个原子操作来释放内存，那么另一个线程必须在同一个原子变量上使用获取内存序。

### C++的内存模型

我们已经看到了自由、获取、释放、获取-释放等内存序，C++还有一个更严格的内存顺序——顺序一致，这是默认的内存序，要求程序的行为都按照一个全局顺序执行。

## 并发性和性能

### 如何高效的使用并发

使用并发性来提升性能无非就两种方法：为并发线程和进程提供足够的工作，使它们始终处于忙碌的状态，第二个是减少共享数据的使用。但往往很难做到。我们可能都听过Amdahl法则，对于一个具有并行部分和单线程部分的程序，最大可能的加速如下：
$$
{s_0} = \frac{{{s_0}}}{{{s_0}(1 - p) + p}}
$$

### 锁的替代方案以及性能

无等待可以以原子增量操作举例，虽然硬件本身必须锁定对共享数据的访问以确保操作的原子性，但从开发者角度来看，代码本身没有等待没有尝试和再尝试；无锁可以以比较-交换的程序举例，线程无法更新共享计数的唯一原因是其他线程先进行了更新，因为在所有试图同时增加计数的线程中至少有一个是成功的。

操作系统提供的互斥锁对于非常短的操作通常不是特别有效，这种情况最简单有效的锁就是自旋锁，比如：

```c++
class Spinlock {
public:
    void lock() {
   		while (flag_.exchange(1, std::memory_order_acquire)) {}
    }
    void unlock() { flag_.store(0, std::memory_order_release); }
private:
    std::atomic<unsigned int> flag_;
};
```

这里省略了默认构造函数和不可复制的声明，注意两个内存栅栏：锁定伴随着获取栅栏，解锁伴随着打开栅栏。这个自旋锁的性能很差，还需要做一些优化。如果标志的值是1则完全不需要用1来替换它，因为交换是一个读改写的操作，即使它将旧值更新为相同的值，也需要独占访问包含该标志的缓存行。者在以下的场景中非常重要，一个锁锁住了，拥有锁的线程没有更改它，但是其他的线程都在检查锁，并等待锁的值更改为0，如果线程不尝试写入标志，那么缓存行就需要在CPU之间切换，线程在缓冲中有相同的副本，并且这个副本是当前的不需要发送数据到其他地方，之后当其中一个线程修改了值时硬件才需要将内存中的新内容发送给所有CPU，以下是刚刚描述的优化：

```c++
class Spinlock {
    void lock() {
        while (flag_.load(std::memory_order_relaxed) ||
        flag_.exchange(1, std::memory_order_acquire)) {}
    }
}
```

这里的优化是首先读取该标志，知道看到0，然后才与1交换。

优化之后性能仍然很差，原因与操作系统倾向于优先处理线程的方式有关，当一个线程正在做一些有用的事情，那么会获得更多的CPU时间。在我们的例子中，计算量最大的线程正在试图获得锁并将CPU分配给它，而另一个线程希望解锁，但在一段时间内没有调度执行。解决方法是等待的线程在多次尝试后放弃CPU，这样其他的线程就可以运行并且可以完成自己的工作并解锁。目前没有一个通用的好方法，大多数都是通过系统调用完成，在linux，通过调用nanosleep在很短的时间内sleep，可能会产生最好的结果，优于调用sched_yield，sched_yield是另一个提供CPU访问的系统函数，与硬件指令相比，所有系统调用的开销都很大，所以最好不要频繁使用：

```c++
class Spinlock {
    void lock() {
        for (int i=0; flag_.load(std::memory_order_relaxed) ||
        flag_.exchange(1, std::memory_order_acquire); ++i) {
            if (i == 8) {
                lock_sleep();
                i = 0;
            }
        }
    }
    void lock_sleep() {
        static const timespec ns = { 0, 1 }; // 1 nanosecond
        nanosleep(&ns, NULL);
    }
}
```

自旋锁的缺点是等待的线程不断使用CPU或忙等待，但如果线程仅占用几个周期，那么忙等也不错，比让线程进入睡眠状态快多了，然而一旦锁定的计算包含多个指令，那么等待自旋的线程将浪费大量CPU时间，并剥夺其他线程访问所需硬件资源的机会。总的来说，如果只需要锁定增量，那么可以使用自旋锁，如果锁定大量的计算，还是用其他的方案，比如系统的锁。

即便没有死锁和活锁的问题，基于锁的程序也会遇到其他的问题，比如协同，可以在一个锁中发生也可能在多个锁中发生，情景如下，线程A拥有一个锁正在对共享数据进行操作，其他线程等待，这项大的任务需要多个线程一起操作，每个线程操作的时候都会独占数据。现在A完成一个任务释放锁然后切换到下一个任务，然而其他线程还没完全醒，A在CPU上又没有睡，因此A会再次获得锁，其他线程什么也做不了。

锁的另一个问题是没有优先级的概念，持有锁的低优先级线程可以抢占高的，因此有时候高优先级线程必须等待低优先级线程，这种情况有时叫做优先级倒置。

无锁不会有这种问题，因为至少有一个线程不被阻塞，那么就不会出现死锁或者活锁。由于所有线程都忙于计算通向原子操作（CAS）的路径，高优先级更容易到达那里，并提交结果，而低优先级更有可能使CAS失败。类似的，提交结果的一次成功并不会使获胜的线程比其他线程有任何优势，准备先尝试执行CAS的线程就是成功的线程，这自然消除了出现协同的可能。

无锁编程的缺点第一个就是优点的反面，即使CAS尝试失败线程也会保持忙碌，这解决了优先级问题，但代价很高，在高争用的情况下，大量的CPU时间会浪费在重复工作上，更糟糕的是这些为访问单个原子变量而竞争的线程会从一些不相关计算的其他线程那里夺取CPU资源。第二个缺点就是无锁程序有无限多种数据同步方案，实现过于复杂。

### 并发编程的构建块

线程安全的数据结构在多线程程序中也不是那么必要的，最常见的不需要线程安全的情况是一个对象仅有一个线程使用，这非常常见且可取，因为共享数据是并发程序效率低下的主要原因。但是，即使每个对象不会在线程之间共享，能确定在多线程中使用一个类或数据结构是安全的吗，不一定，因为接口层没有共享不代表实现层没有。另一方面，只要没有线程修改对象，在多线程代码中使用许多数据结构是安全的。

线程安全的最高级别称为强线程安全保证，提供这种保证的对象可以被多个线程并发使用，而不会引起数据竞争或其他未定义的行为；下一级称为弱线程安全保证，只要所有线程限制为只读访问，提供这种对象可以使用多个线程同时访问，其次任何对对象具有独占访问权的线程都可以对对象执行其他的操作（无论其他线程在做什么）。不提供任何此类保证的对象不能在多线程程序中使用，即使对象本身不共享，其内部实现也可能被其他线程修改。

线程安全始于设计阶段，必须明智的选择使用的数据结构和接口，以便表示适当的抽象级别以及发生线程交互级别上的正确事务。

最简单的线程安全对象是简单的计数器或累加器，这里需要强线程安全保证。

使用自旋锁访问单个变量或单个对象时可以进一步优化自旋锁，可以让锁成为保护对象的唯一引用而不是通用标志，原子变量是指针而不是整数，如下：

```c++
template <typename T>
class PtrSpinlock {
public:
    explicit PtrSpinlock(T* p) : p_(p) {}
    T* lock() {
        while (!(saved_p_ =
        p_.exchange(nullptr, std::memory_order_acquire))) {}
    }
    void unlock() {
    	p_.store(saved_p_, std::memory_order_release);
    }
private:
    std::atomic<T*> p_;
    T* saved_p_ = nullptr;
};
```

指针自旋锁的第一个优点提供了访问保护对象的唯一方式，这就不可能有条件竞争，并在没有锁的情况下访问共享数据，第二个优点，锁的性能通常略优于常规自旋锁，自旋锁是否优于原子操作也取决于硬件。

无论选择何种实现，线程安全的累加器或计数器都不应该公开而应该封装在类中，原因之一是为类的客户端提供稳定的接口，同时保留优化实现的自由度。第二个原因更微妙，是与计数器提供的保证有关，比如说当计数器的值从N-1更改为N之前，要保证数组前N个元素的更改对所有线程可见，做到这一点，其他线程读取这个数组不加锁也是安全的。

有以下的原子索引类和原子计数类：

```c++
class AtomicIndex {
	std::atomic<unsigned long> c_;
public:
    unsigned long incr() noexcept {
    	return 1 + c_.fetch_add(1, std::memory_order_release);
    }
    unsigned long get() const noexcept {
    	return c_.load(std::memory_order_acquire);
    }
};

class AtomicCount {
	std::atomic<unsigned long> c_;
public:
    unsigned long incr() noexcept {
    	return 1 + c_.fetch_add(1, std::memory_order_relaxed);
    }
    unsigned long get() const noexcept {
    	return c_.load(std::memory_order_relaxed);
    }
};
```

唯一的不同点在于索引在计数是有内存可见性保证的，计数不提供这种可见性保证。

我们试图解决的问题在并发中非常常见。线程正在创建新数据，程序的其余部分只能在数据准备好时看到这些数据，创建数据的线程通常称为写线程或生产者线程，其他线程都是读取线程或消费者线程。加锁当然最简单，但现在所有的使用者线程都在只读环境中操作，不需要任何锁，挑战在于保证读写之间的边界。这个无锁解决方案依赖非常特殊的协议，从而可以在生产者和消费者线程之间传递信息：

- 生产者线程在内存中准备时其他线程无法访问数据。
- 所有的消费者线程必须使用共享指针来访问数据，称之为根指针，这个指针最初是空的，在生产线程构造数据时保持为空，所以从消费者线程来看，目前没有数据，更一般的说，指针不需要是一个实际的指针：任何类型的句柄或引用都可以，只要是允许访问内存位置，并且可以设置为一个无效值。如果所有的新对象都是在预先分配的数组中创建，那么指针可以是数组的索引，无效值可以是大于或等于数组长度的值；
- 协议关键在于消费者访问数据的唯一方式是通过根指针，在生产者准备显示或发布数据之前根指针保持为空。发布数据的过程非常简单，生产者必须在根指针中存储数据的正确内存位置，并且这个改变必须伴随着内存释放栅栏；
- 消费者线程必须以原子的方式读取根指针，并获取栅栏，如果读取非空值，可以通过根指针读取可访问的数据；

下一步介绍如何将协议的指针封装入类，发布指针可以如下所示：

```c++
template <typename T>
class ts_unique_ptr {
public:
    ts_unique_ptr() = default;
    explicit ts_unique_ptr(T* p) : p_(p) {}
    ts_unique_ptr(const ts_unique_ptr&) = delete;
    ts_unique_ptr& operator=(const ts_unique_ptr&) = delete;
    ~ts_unique_ptr() {
    	delete p_.load(std::memory_order_relaxed);
    }
    void publish(T* p) noexcept {
    	p_.store(p, std::memory_order_release);
    }
    const T* get() const noexcept {
    	return p_.load(std::memory_order_acquire);
    }
    const T& operator*() const noexcept { return *this->get(); }
    ts_unique_ptr& operator=(T* p) noexcept {
    	this->publish(p); return *this;
    }
private:
	std::atomic<T*> p_ { nullptr };
};
```

指针的构建过程中没有线程安全性，所以很重要的是在指针构建时没有线程可以访问它。另外指针没有为多个生产者线程提供同步，如果多个线程试图通过同一个指针发布数据，结果将是未定义的且存在数据竞争，最后虽然指针实现了线程安全的发布协议，但没有执行安全的取消发布和删除数据。

有时希望指针能够以线程安全的方式同时管理数据的创建和删除，所以需要一个线程安全的共享指针。多个线程共享一个共享指针无疑是不安全的，我们需要保证共享指针本身操作是原子的，20中允许写std::atomic\<std::shared_ptr\<T\>\>，另外也可以显式的处理：

```c++
//生产者线程
std::shared_ptr<T> p_;
T* data = new T;
… finish initializing the data …
std::atomic_store_explicit(
&p_, std::shared_ptr<T>(data), std::memory_order_release);
//消费者线程
std::shared_ptr<T> p_;
const T* data = std::atomic_load_explicit(
&p_, std::memory_order_acquire).get();
```

这种方法的缺点主要是无法避免非原子访问。

以上的方法虽然方便，但std::shared_ptr并不是一个特别有效的指针，并且原子访问使它变的很慢。

需要共享所有权的时候，使用有限的功能和实现设计自己的引用计数指针更好，一种常见的方法是使用侵入引用计数，侵入式共享指针将其引用计数存储在指向的对象中，比如：

```c++
template <typename T> struct Wrapper {
    T object;
    Wrapper(… arguments …) : object(…) {}
    ~Wrapper() = default;
    Wrapper (const Wrapper&) = delete;
    Wrapper& operator=(const Wrapper&) = delete;
    std::atomic<size_t> ref_cnt_ = 0;
    void AddRef() {
    	ref_cnt_.fetch_add(1, std::memory_order_acq_rel);
    }
    bool DelRef() { return
    	ref_cnt_.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }
};

```

## 用于并发的数据结构

### 线程安全的数据结构

