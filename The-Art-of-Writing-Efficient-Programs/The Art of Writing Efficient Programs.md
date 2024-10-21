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
 g++ substring_sort.c -Wl,--no-as-needed,-lprofiler,--as-needed -g -o example
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

使用perf进行收据收集，必须使用调试信息进行编译，并使用perf record -g(记录符号信息)命令运行分析器。perf可以使用三种方式收集调用堆栈，第一种栈指针，perf record --call-graph fp，要求使用--fnoomit-frame-pointer构建二进制文件，第二种DWARF调试信息，perf record --call-graph dwarf，要求-g构建二进制，第三种使用英特尔最后分支记录硬件功能，perf record --call-graph lbr，通过解析LBR堆栈获取调用堆栈。可以指定计数器或一组计数器，也可以指定采样频率。收集完信息后可以使用perf report将收集的数据可视化。通过instructions/cycles，可以得到IPC（每周期指令数，越高越好）；通过指定事件，uops_issued.any,uops_executed.thread,uops_retired.slots，可以查看微操作的数量，通常，用于一条指令的微操作越少越好，意味着硬件对其支持越好，并且可能具有更低的延迟和更高的吞吐。事件ref-cycles统计的周期数指的是没有频繁缩放的情况下；指定事件mem_load_retired.fb_hit,mem_load_retired.l1_miss,
  mem_load_retired.l1_hit,mem_inst_retired.all_loads收集L1缓存失效的数量，加载操作可能命中已分配的填充缓冲区（fb_hit），或者命中L1缓存（l1_hit），或者两者都未命中（l1_miss），因此 all_loads = fb_hit + l1_hit + l1_miss，还可进一步分析L1数据缺失L2缓存行为， perf stat -e mem_load_retired.l1_miss,
  mem_load_retired.l2_hit,mem_load_retired.l2_miss -- a.exe；通过事件branches,branch-misses来检查分支预测错误的数量；以上的事件只是常用性能计数器的映射，还可以通过指定Event和Umask十六进制值来访问它们，例如$ perf stat -e cpu/event=0xc4,umask=0x0,name=BR_INST_RETIRED.ALL_BRANCHES/ -- ./a.exe;

对于这行命令，perf stat --topdown -a -- taskset -c 0 ./benchmark.exe，perf stat命令中--topdown选项用来打印TMA第一级指标，即“前端受限”、“后端受限”、“退休”、“错误猜测”，-a用于分析整个系统，taskset -c 0将基准测试固定在核心0上。由于其只支持一级TMA指标，要访问2、3以及更高级，可以使用toplev工具（pmu-tools的一部分），要使用toplev需要启用特定的linux内核设置。~/pmu-tools/toplev.py --core S0-C0 -l2 -v --no-desc taskset -c 0 ./benchmark.exe，-l2告诉工具收集level 2指标，--no-desc禁用每个指标描述。找到了热点事件就可以利用perf进行事件采样找到代码位置。从zen4开始，AMD也支持一级二级TMA分析，它被称为管道利用率分析，比如perf stat -M PipelineL1,PipelineL2 -- ./cryptest.exe b1 10，或者在单个运行中收集一组指标，perf stat -M frontend_bound,backend_bound；

perf采样数据，示例：perf record -o ./perf.data --call-graph dwarf,8192 --event cycles --aio --sample-cpu --pid 874174。对于arm处理器，还可以使用arm工程师开发的topdown-tool: https://learn.arm.com/install-guides/topdown-tool/2 这个工具，比如 topdown-tool --all-cpus -m Topdown_L1 -- python -c "from ai_benchmark import AIBenchmark; results = AIBenchmark(use_CPU=True).run()"，其中 --all-cpus 选项启用所有 CPU 的系统级收集，而 -m Topdown_L1 则收集 Topdown 1 级指标。 -- 后面的所有内容都是运行 AI Benchmark Alpha 套件的命令行。

在intel平台，如果需要堆栈数据可以使用perf record -b -e cycles ./benchmark.exe，虽然perf record --call-graph lbr也可，但信息量比前者少，不会收集分支预测和周期数据。用户可以导出原始LBR堆栈自定义分析，

perf script -F brstack &> dump.txt。从 Linux 内核 6.1 开始，Linux “perf” 在 AMD Zen4 处理器上支持分支分析用例，除非另有明确说明。Linux perf 命令收集 AMD LBR 使用相同的 -b 和 -j 选项。示例： perf record --call-graph lbr -- ./a.exe；perf report -n --stdio

    99.96%  99.94%    65447    a.exe    a.exe  [.] bar
            |          
             --99.94%--main
                       |          
                       |--90.86%--foo
                       |          |          
                       |           --90.86%--bar
                       |          
                        --9.08%--zoo
                                  bar

查看热门分支的预测错误率：perf record -e cycles -b -- ./7zip.exe b；perf report -n --sort symbol_from,symbol_to -F +mispredict,srcline_from,srcline_to --stdio

    46.12%   303391   N   dec.c:36   dec.c:40  LzmaDec     LzmaDec   
    22.33%   146900   N   enc.c:25   enc.c:26  LzmaFind    LzmaFind  
     6.70%    44074   N   lz.c:13    lz.c:27   LzmaEnc     LzmaEnc   
     6.33%    41665   Y   dec.c:36   dec.c:40  LzmaDec     LzmaDec

性能分析工具，除了perf之外，intel平台可以使用Vtune，AMD使用uProf，Apple使用Instruments。

如果想要查看精确事件，可使用pp，如$ perf record -e cycles:pp -- ./a.exe

perf如果需要生成火焰图，需要下载FlameGraph工具，首先采样数据sudo perf record -F 99 -p PID -g -- sleep 30或者sudo perf record -g ./main2，之后将采集到的数据转化为火焰图需要的中间格式sudo perf script > out.perf
或
sudo perf script -i perf.data> out.perf，之后生成火焰图./stackcollapse-perf.pl < ../out.perf | ./flamegraph.pl > out.svg。

如果想要分析特定的代码，可以使用专业的性能分析器，比如Tracy。

有些时候，还可能需要持续性能分析，一种始终处于系统级、基于采样的性能分析器，但采样率较低，以尽量减少运行时影响，此时可以使用parca。

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

流水线也有前提条件：为了从循环迭代中交叉执行代码，必须知道将要执行什么代码。因此，条件执行或分支也是流水线的障碍。一般来说Bad Speculation指标在百分之五到十的范围内是正常的，一旦超过百分之十就需要关注。

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

### 优化计算

当应用TMA方法时，低效计算通常反应在Core Bound类别中，以及一定程度上反应在Retiring类别中。Core Bound类别代表CPU乱序执行引擎内所有非内存问题引起的停顿，主要包括两个类别：

- 软件指令之间的数据依赖性；
- 硬件计算资源短缺；

内联可以消除调用函数的开销，如果确实有必要内联，对于 GCC 和 Clang 编译器，可以使用 C++11 的 `[[gnu::always_inline]]` 属性作为内联 `foo` 的提示，如下面的代码示例所示。对于较早的 C++ 标准，可以使用 `__attribute__((always_inline))`。对于 MSVC 编译器，可以使用 `__forceinline` 关键字。

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

在TMA方法论中，Memory Bound估算了由于加载或存储指令的需求导致CPU管道可能停滞的插槽的比例，因此，需要找到导致高Memory Bound指标的内存访问。而`前端瓶颈(Front-End Bound)` 指标捕捉 FE 性能问题。它代表了 CPU FE 无法向 BE 提供指令的周期百分比，而它本可以接受它们。大多数实际应用程序都会经历非零的`Front-End Bound`指标，这意味着一定百分比的运行时间将因次优的指令获取和解码而损失。低于10%是常态。如果你看到"Front-End Bound"指标超过20%，那么绝对值得花时间解决它。

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

如果性能瓶颈出现在来回传输的内存，还可以尝试压缩来使得内存更紧凑，比如：

```c++
struct S {
  unsigned a:4;
  unsigned b:2;
  unsigned c:2;
}; // S is only 1 byte
```

而且将结构体的成员按其大小进行声明可以减小其大小：

```c++
struct S1 {
  bool b;
  int i;
  short s;
}; // S1 is `sizeof(int) * 3` bytes

struct S2 {
  int i;
  short s;  
  bool b;
}; // S2 is `sizeof(int) * 2` bytes
```

为了避免使用多个缓存行，还可以利用alignas关键字进行缓存对齐。

未能在缓存中解析的内存访问通常代价高昂，现代CPU有两种解决这个问题的机制：硬件预取和ooo执行。我们有时候可以使用显式软件预取来避免缓存未命中，比如使用gcc的\_\_builtin_prefetch，x86平台利用显式软件预取的另一种选择是编译器内部函数\_mm_prefetch。

减少DTLB未命中可以使用大页面，主要有以下几种。显式大页面（EHP），是系统内存的一部分，作为大页面文件系统hugetlbfs暴露，应在启动时或运行时预留，比如：

```c++
void ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
if (ptr == MAP_FAILED)
  throw std::bad_alloc{};                
...
munmap(ptr, size);
```

透明大页面(THP)又有两种操作模式，系统范围和每个进程。启用系统范围的THP，将自动将大页面用于常规内存分配，无需程序明确要求。使用每个进程选项时，实例如下：

```c++
void ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
if (ptr == MAP_FAILED)
  throw std::bad_alloc{};
madvise(ptr, size, MADV_HUGEPAGE);
// use the memory region `ptr`
munmap(ptr, size);
```

改善基本块布局，举例：

```c++
// 热路径
if (cond) [[unlikely]]
  coldFunc();
// 再次热路径
```

在上面代码中，`[[unlikely]]` 提示将指示编译器 `cond` 不太可能为真，因此编译器应相应地调整代码布局。在 C++20 之前，开发人员可以使用 `__builtin_expect`:

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

高性能的一个原则，不做总是比做要快，我们现在要使用多个线程来搜索共享数据，如果找到一个，则原子计数加一，这其实性能也有些差，最好在每个线程上保持本地计数，并且只增加共享计数一次：

```c++
unsigned long count;
std::mutex M; // Guards count
…
// On each thread
unsigned long local_count = 0;
for ( … counting loop … ) {
    … search …
    if (… found …) ++local_count;
}
std::lock_guard<std::mutex> L(M);
count += local_count;
```

当然，最好的线程安全是不需要多个线程访问共享数据。

### 线程安全的堆栈

所有的C++容器都提供了弱线程安全，多个线程可以安全的访问只读容器，只要没有线程调用非const函数。

我们可以用自己的类来包装堆栈类，只要别忘记使用互斥锁保护临界区。我们可以使用继承而不是封装，这样会使得我们的包装类更简单，但使用公共继承会公开基类stack的每个成员函数，如果忘记包装一个，代码会有风险。私有（或受保护）的继承避免了这个问题，但会带来其他风险，有些构造函数需要重新实现，比如移动构造函数需要锁定正在移动的堆栈，所以需要自定义实现。

```c++
template <typename T> void reset(T& s) { T().swap(s); }

template <typename T> class mt_stack
{
    std::deque<T> s_;
    mutable spinlock l_;
    int cap_ = 0;
    struct counts_t { 
        int pc_ = 0; // Producer count
        int cc_ = 0; // Consumer count
        bool equal(std::atomic<counts_t>& n) {
            if (pc_ == cc_) return true;
            *this = n.load(std::memory_order_relaxed);
            return false;
        }
    };
    mutable std::atomic<counts_t> n_;
    public:
    mt_stack(size_t n = 100000000) : s_(n), cap_(n) {}
    void push(const T& v) {
        // Reserve the slot for the new object by advancing the producer count.
        counts_t n = n_.load(std::memory_order_relaxed);
        if (n.pc_ == cap_) abort();
        int i = 0;
        while (!n.equal(n_) || !n_.compare_exchange_weak(n, {n.pc_ + 1, n.cc_}, std::memory_order_acquire, std::memory_order_relaxed)) {
            if (n.pc_ == cap_) abort();
            nanosleep(i);
        };
        // Producer count advanced, slot n.pc_ + 1 is ours.
        ++n.pc_;
        new (&s_[n.pc_]) T(v);

        // Advance the consumer count to match.
        if (!n_.compare_exchange_strong(n, {n.pc_, n.cc_ + 1}, std::memory_order_release, std::memory_order_relaxed)) abort();
    }
    std::optional<T> pop() {
        // Decrement the consumer count.
        counts_t n = n_.load(std::memory_order_relaxed);
        if (n.cc_ == 0) return std::optional<T>(std::nullopt);
        int i = 0;
        while (!n.equal(n_) || !n_.compare_exchange_weak(n, {n.pc_, n.cc_ - 1}, std::memory_order_acquire, std::memory_order_relaxed)) { 
            if (n.cc_ == 0) return std::optional<T>(std::nullopt);
            nanosleep(i);
        };
        // Consumer count decremented, slot n.cc_ - 1 is ours.
        --n.cc_;
        std::optional<T> res(std::move(s_[n.pc_]));
        s_[n.pc_].~T();

        // Decrement the producer count to match.
        if (!n_.compare_exchange_strong(n, {n.pc_ - 1, n.cc_}, std::memory_order_release, std::memory_order_relaxed)) abort();
        return res;
    }
    std::optional<T> top() const {
        // Decrement the consumer count.
        counts_t n = n_.load(std::memory_order_relaxed);
        if (n.cc_ == 0) return std::optional<T>(std::nullopt);
        int i = 0;
        while (!n.equal(n_) || !n_.compare_exchange_weak(n, {n.pc_, n.cc_ - 1}, std::memory_order_acquire, std::memory_order_relaxed)) { 
            if (n.cc_ == 0) return std::optional<T>(std::nullopt);
            nanosleep(i);
        };
        // Consumer count decremented, slot n.cc_ - 1 is ours.
        --n.cc_;
        std::optional<T> res(std::move(s_[n.pc_]));

        // Restore the consumer count.
        if (!n_.compare_exchange_strong(n, {n.pc_, n.cc_ + 1}, std::memory_order_release, std::memory_order_relaxed)) abort();
        return res;
    }
    void reset() { ::reset(s_); s_.resize(cap_); }
};
```

注意，我们没有使用std::stack，因为它在线程之间共享，对它的push和pop没有锁定就意味着不是线程安全的，为了简单，这里使用了deque，并构造了足够多的元素。

### 线程安全的队列

队列跟堆栈不一样，只要队列非空，生产者消费者就不会交互，push不需要知道头部的索引，pop不关心队尾的索引。我们首先考虑将这些角色分配给不同线程的场景，进一步简化使用每一种线程的方式，我们现在考虑一个生产者和一个消费者。因为只有一个生产者，不需要为这个索引使用原子整数，只有队列为空时两个线程才会交互，为此需要一个原子变量表示队列的大小，消费者与之相反。

在内存的处理上，对上文的堆栈我们假设意志最大容量，并且不会超过，对于队列来说就不行了，因为前后的索引都会移动，这里的解决方法是将数组视为循环缓冲区，并对数组下标使用取模运算：

```c++
struct LS {
    long x[1024];
    LS(char i) { for (size_t k = 0; k < 1024; ++k) x[k] = i; }
};

template <typename T> class pc_queue {
  public:
  explicit pc_queue(size_t capacity) : 
    capacity_(capacity), data_(static_cast<T*>(::malloc(sizeof(T)*capacity_))) {}
  ~pc_queue() { ::free(data_); }
  bool push(const T& v) {
    if (size_.load(std::memory_order_relaxed) == capacity_) return false;
    new (data_ + (back_ % capacity_)) T(v);
    ++back_;
    size_.fetch_add(1, std::memory_order_release);
    return true;
  }
  std::optional<T> pop() {
    if (size_.load(std::memory_order_acquire) == 0) {
      return std::optional<T>(std::nullopt);
    } else {
      std::optional<T> res(std::move(data_[front_ % capacity_]));
      data_[front_ % capacity_].~T();
      ++front_;
      size_.fetch_sub(1, std::memory_order_relaxed);
      return res;
    }
  }

  void reset() { size_ = front_ = back_ = 0; }
  private:
  const size_t capacity_;
  T* const data_;
  size_t front_ = 0;
  size_t back_ = 0;
  std::atomic<size_t> size_;
};
```

如果对于多个生产者和消费者，那么通用的方案使用与堆栈相同的技术，将front和back索引打包到一个64位原子类型中，并以比较-交换原子方式访问。

如果放弃队列的顺序一致性（即入队的顺序和出队的顺序相同），可以开发出一种全新的方法：可以有几个单线程子队列，而非单个线程安全队列，每个线程必须以原子的方式获得子队列中的所有权。最简单的方法是使用指向子队列的原子指针数组。为了获得该队列的所有权同时防止其他线程访问该队列，需要将子队列指针自动交换为空。

![7-22](..\image\the-art-of-writing-effective-program\7-22.png)
需要访问队列的线程必须获得一个子队列，可以从指针数组的任何元素开始。如果为空则该子队列当前处于繁忙状态，然后尝试下一个元素。操作完成后线程原子性的将子队列指针写回数组，将子队列的所有权返回给队列：

```c++
// Circular queue of a fixed size.
// This class is not thread-safe, it is supposed to be manipulated by one
// thread at a time.
template <typename T> class subqueue {
    public:
    explicit subqueue(size_t capacity) : capacity_(capacity), begin_(0), size_(0) {}
    size_t capacity() const { return capacity_; }
    size_t size() const { return size_; }
    bool push(const T& x) {
        if (size_ == capacity_) return false;
        size_t end = begin_ + size_++;
        if (end >= capacity_) end -= capacity_;
        data_[end] = x;
        return true;
    }
    bool pop(std::optional<T>& x) {
        if (size_ == 0) return false;
        --size_;
        size_t pos = begin_;
        if (++begin_ == capacity_) begin_ -= capacity_;
        x.emplace(std::move(data_[pos]));
        data_[pos].~T();
        return true;
    }
    static size_t memsize(size_t capacity) {
        return sizeof(subqueue) + capacity*sizeof(T);
    }
    static subqueue* construct(size_t capacity) {
        return new(::malloc(subqueue::memsize(capacity))) subqueue(capacity);
    }
    static void destroy(subqueue* queue) {
        queue->~subqueue();
        ::free(queue);
    }
    void reset() { 
        begin_ = size_ = 0;
    }

    private:
    const size_t capacity_;
    size_t begin_;
    size_t size_;
    T data_[1]; // Actually [capacity_]
};

// Collection of several subqueues, for optimizing of concurrent access.
// Each queue pointer is on a separate cache line.
template <typename Q> struct subqueue_ptr {
  subqueue_ptr() : queue() {}
  std::atomic<Q*> queue;
  char padding[64 - sizeof(queue)]; // Padding to cache line
};

template <typename T> class concurrent_queue {
  typedef subqueue<T> subqueue_t;
  typedef subqueue_ptr<subqueue_t> subqueue_ptr_t;
  public:
    explicit concurrent_queue(size_t capacity = 1UL << 15) {
      for (int i = 0; i < QUEUE_COUNT; ++i) {
        queues_[i].queue.store(subqueue_t::construct(capacity), std::memory_order_relaxed);
      }
    }
    ~concurrent_queue() {
      for (int i = 0; i < QUEUE_COUNT; ++i) {
        subqueue_t* queue = queues_[i].queue.exchange(nullptr, std::memory_order_relaxed);
        subqueue_t::destroy(queue);
      }
    }

    // How many entries are in the queue?
    size_t size() const { return count_.load(std::memory_order_acquire); }

    // Is the queue empty?
    bool empty() const { return size() == 0; }

    // Get an element from the queue.
    // This method blocks until either the queue is empty (size() == 0) or an
    // element is returned from one of the subqueues.
    std::optional<T> pop() {
      std::optional<T> res;
      if (count_.load(std::memory_order_acquire) == 0) {
        return res;
      }
      // Take ownership of a subqueue. The subqueue pointer is reset to NULL
      // while the calling thread owns the subqueue. When done, relinquish
      // the ownership by restoring the pointer.  The subqueue we got may be
      // empty, but this does not mean that we have no entries: we must check
      // other queues. We can exit the loop when we got an element or the element
      // count shows that we have no entries.
      // Note that decrementing the count is not atomic with dequeueing the
      // entries, so we might spin on empty queues for a little while until the
      // count catches up.
      subqueue_t* queue = NULL;
      bool success = false;
      for (size_t i = 0; ;)
      {
        i = dequeue_slot_.fetch_add(1, std::memory_order_relaxed) & (QUEUE_COUNT - 1);
        queue = queues_[i].queue.exchange(nullptr, std::memory_order_acquire);  // Take ownership of the subqueue
        if (queue) {
          success = queue->pop(res);                                            // Dequeue element while we own the queue
          queues_[i].queue.store(queue, std::memory_order_release);             // Relinquish ownership
          if (success) break;                                                   // We have an element
          if (count_.load(std::memory_order_acquire) == 0) goto EMPTY;          // No element, and no more left
        } else {                                                                // queue is NULL, nothing to relinquish
          if (count_.load(std::memory_order_acquire) == 0) goto EMPTY;          // We failed to get a queue but there are no more entries left
        }
        if (success) break;
        if (count_.load(std::memory_order_acquire) == 0) goto EMPTY;
        static const struct timespec ns = { 0, 1 };
        nanosleep(&ns, NULL);
      };
      // If we have an element, decrement the queued element count.
      count_.fetch_add(-1);
EMPTY:
      return success;
    }

    // Add an element to the queue.
    // This method blocks until either the queue is full or an element is added to
    // one of the subqueues. Note that the "queue is full" condition cannot be
    // checked atomically for all subqueues, so it's approximate, we try
    // several subqueues, if they are all full we give up.
    bool push(const T& v) {
      // Preemptively increment the element count: get() will spin on subqueues
      // as long as it thinks there is an element to dequeue, but it will exit as
      // soon as the count is zero, so we want to avoid the situation when we
      // added an element to a subqueue, have not incremented the count yet, but
      // get() exited with no element. If this were to happen, the pool could be
      // deleted with entries still in the queue.
      count_.fetch_add(1);
      // Take ownership of a subqueue. The subqueue pointer is reset to NULL
      // while the calling thread owns the subqueue. When done, relinquish
      // the ownership by restoring the pointer.  The subqueue we got may be
      // full, in which case we try another subqueue, but don't loop forever
      // if all subqueues keep coming up full.
      subqueue_t* queue = NULL;
      bool success = false;
      int full_count = 0;                                             // How many subqueues we tried and found full
      //for (size_t enqueue_slot = 0, i = 0; ;)
      for (size_t i = 0; ;)
      {
        //i = ++enqueue_slot & (QUEUE_COUNT - 1);
        i = enqueue_slot_.fetch_add(1, std::memory_order_relaxed) & (QUEUE_COUNT - 1);
        queue = queues_[i].queue.exchange(nullptr, std::memory_order_acquire);    // Take ownership of the subqueue
        if (queue) {
          success = queue->push(v);                                               // Enqueue element while we own the queue
          queues_[i].queue.store(queue, std::memory_order_release);               // Relinquish ownership
          if (success) return success;                                            // We added the element
          if (++full_count == QUEUE_COUNT) break;                                 // We tried hard enough, probably queue is full
        }
        static const struct timespec ns = { 0, 1 };
        nanosleep(&ns, NULL);
      };
      // If we added the element, the count is already incremented. Otherwise,
      // we must decrement the count now.
      count_.fetch_add(-1);
      return success;
    }

  void reset() { 
    count_ = enqueue_slot_ = dequeue_slot_ = 0;
    for (int i = 0; i < QUEUE_COUNT; ++i) {
      subqueue_t* queue = queues_[i].queue;
      queue->reset();
    }
  }

  private:
    enum { QUEUE_COUNT = 16 };
    subqueue_ptr_t queues_[QUEUE_COUNT];
    std::atomic<int> count_;
    std::atomic<size_t> enqueue_slot_;
    std::atomic<size_t> dequeue_slot_;
};

template <typename T> class concurrent_std_queue {
  using subqueue_t = std::queue<T>;
  struct subqueue_ptr_t {
    subqueue_ptr_t() : queue() {}
    std::atomic<subqueue_t*> queue;
    char padding[64 - sizeof(queue)]; // Padding to cache line
  };

  public:
    explicit concurrent_std_queue() {
      for (int i = 0; i < QUEUE_COUNT; ++i) {
        queues_[i].queue.store(new subqueue_t, std::memory_order_relaxed);
      }
    }
    ~concurrent_std_queue() {
      for (int i = 0; i < QUEUE_COUNT; ++i) {
        subqueue_t* queue = queues_[i].queue.exchange(nullptr, std::memory_order_relaxed);
        delete queue;
      }
    }

    // How many entries are in the queue?
    size_t size() const { return count_.load(std::memory_order_acquire); }

    // Is the queue empty?
    bool empty() const { return size() == 0; }

    // Get an element from the queue.
    // This method blocks until either the queue is empty (size() == 0) or an
    // element is returned from one of the subqueues.
    std::optional<T> pop() {
      std::optional<T> res;
      if (count_.load(std::memory_order_acquire) == 0) {
        return res;
      }
      // Take ownership of a subqueue. The subqueue pointer is reset to NULL
      // while the calling thread owns the subqueue. When done, relinquish
      // the ownership by restoring the pointer.  The subqueue we got may be
      // empty, but this does not mean that we have no entries: we must check
      // other queues. We can exit the loop when we got a element or the element
      // count shows that we have no elements.
      // Note that decrementing the count is not atomic with dequeueing the
      // entries, so we might spin on empty queues for a little while until the
      // count catches up.
      subqueue_t* queue = NULL;
      bool success = false;
      for (size_t i = 0; ;)
      {
        i = dequeue_slot_.fetch_add(1, std::memory_order_relaxed) & (QUEUE_COUNT - 1);
        queue = queues_[i].queue.exchange(nullptr, std::memory_order_acquire);  // Take ownership of the subqueue
        if (queue) {
          if (!queue->empty()) {                                                // Dequeue element while we own the queue
            success = true;
            res.emplace(std::move(queue->front()));
            queue->pop();
            queues_[i].queue.store(queue, std::memory_order_release);           // Relinquish ownership
            break;                                                              // We have an element
          } else {                                                              // Subqueue is empty, try the next one
            queues_[i].queue.store(queue, std::memory_order_release);           // Relinquish ownership
            if (count_.load(std::memory_order_acquire) == 0) goto EMPTY;        // No element, and no more left
          }
        } else {                                                                // queue is NULL, nothing to relinquish
          if (count_.load(std::memory_order_acquire) == 0) goto EMPTY;          // We failed to get a queue but there are no more entries left
        }
        if (success) break;
        if (count_.load(std::memory_order_acquire) == 0) break; 
        static const struct timespec ns = { 0, 1 };
        nanosleep(&ns, NULL);
      };
      // If we have an element, decrement the queued element count.
      count_.fetch_add(-1);
EMPTY:
      return res;
  }

  // Add an element to the queue.
  // This method blocks until either the queue is full or an element is added to
  // one of the subqueues. Note that the "queue is full" condition cannot be
  // checked atomically for all subqueues, so it's approximate, we try
  // several subqueues, if they are all full we give up.
  bool push(const T& v) {
    // Preemptively increment the element count: get() will spin on subqueues
    // as long as it thinks there is an element to dequeue, but it will exit as
    // soon as the count is zero, so we want to avoid the situation when we
    // added an element to a subqueue, have not incremented the count yet, but
    // get() exited with no element. If this were to happen, the pool could be
    // deleted with entries still in the queue.
    count_.fetch_add(1);
    // Take ownership of a subqueue. The subqueue pointer is reset to NULL
    // while the calling thread owns the subqueue. When done, relinquish
    // the ownership by restoring the pointer.  The subqueue we got may be
    // full, in which case we try another subqueue, but don't loop forever
    // if all subqueues keep coming up full.
    subqueue_t* queue = NULL;
    bool success = false;
    for (size_t i = 0; ;)
    {
      i = enqueue_slot_.fetch_add(1, std::memory_order_relaxed) & (QUEUE_COUNT - 1);
      queue = queues_[i].queue.exchange(nullptr, std::memory_order_acquire);    // Take ownership of the subqueue 
      if (queue) {
        success = true;
        queue->push(v);                                                         // Enqueue element while we own the queue 
        queues_[i].queue.store(queue, std::memory_order_release);               // Relinquish ownership
        return success;
      }
      static const struct timespec ns = { 0, 1 };
      nanosleep(&ns, NULL);
    };
    return success;
  }

  void reset() { 
    count_ = enqueue_slot_ = dequeue_slot_ = 0;
    for (int i = 0; i < QUEUE_COUNT; ++i) {
      subqueue_t* queue = queues_[i].queue;
      subqueue_t().swap(*queue);
    }
  }

  private:
  enum { QUEUE_COUNT = 16 };
  subqueue_ptr_t queues_[QUEUE_COUNT];
  std::atomic<int> count_;
  std::atomic<size_t> enqueue_slot_;
  std::atomic<size_t> dequeue_slot_;
};
```

对于并发数据结构的内存管理，当需要无锁数据结构时，可以估计其最大容量并预分配内存，如果需要增加内存，最理想的情况是添加内存但不复制现有数据结构，就像deque一样，当需要更多内存时会分配另一个内存块，通常会有一些指针地址的修改。为了防止多个线程在设置等待标志之前同时检测内存不足的情况，这通常会导致竞争，可以使用双重检查锁，它同时使用互斥锁和原子标志，如果标志没有设置，则一切正常，如果设置标志，则必须获取锁，并再次检查该标志：

```c++
std::atomic<int> wait;  // 1 if managing memory
std::mutex lock;
while (wait == 1) {};  // Memory allocation in progress
if ( … out of memory … ) {
    std::lock_guard g(lock);
    if (… out of memory …) { // We got here first!
    wait = 1;
    … allocate more memory …
    wait = 0;
    }
}
… do the operation normally …
```

### 线程安全的链表

有一种非常常见的故障A-B-A问题，如下所示：

![7-27](..\image\the-art-of-writing-effective-program\7-27.png)

假设线程A要删除第一个节点，所以其保存了第二个节点作为头节点，现在线程B开始运行，删除了前两个节点又新增了一个节点，这个时候这个新节点大概率重用了原来第一个节点的地址，现在A又开始运行，进行比较和交换操作，发现第一个节点地址不变因此操作成功，现在头节点指向了T2以前使用的内存，出错了。这种问题的根源在于，如果内存回收和分配，那么内存中的指针或地址就不能作为数据的唯一标识。所以必须确保在读取一个将会比较-交换使用的指针时，在比较交换完成之前该地址的内存不能释放。这都属于延迟内存回收，一种是垃圾收集，另一种是使用风险指针。本书介绍的则是一种使用原子共享指针的方法。

## C++中的并发

### C++17的并发支持

除了添加了std::scoped_lock和std::shared_mutex之外，还允许决定L1缓存的缓存行大小，std::hardware_destructive_interference_size和std::hardware_constructive_interference_size。另外还有了一些并行算法。

### C++20的并发支持

协程具有中断和恢复功能的线程。协同程序有两种风格：有栈和无栈。

假设在f()里面调用了g()，则其栈帧如下所示：

![8-5](..\image\the-art-of-writing-effective-program\8-5.png)

无栈协程的状态存储在堆上，这种分配称为活帧，活帧与协程句柄相关联，协程句柄是一个智能指针的对象，可以发出和返回函数的调用，只要句柄没有损害，活帧就一直存在。协程还需要堆栈空间调用其他函数，该空间在调用者的堆栈上分配。

20中协程的主要内容有：

- 协程是可以挂起自己的函数，且这个操作是由开发者显式完成的；
- 与栈帧相关联的常规函数不同，协程具有句柄对象，只要句柄处于活动状态，协程状态就会保持；
- 协程挂起后，控制权将返回给调用者，调用者可以继续以相同的方式运行；
- 协程可以从任何位置恢复，不一定是调用者本身，此外协程甚至可以从不同的线程恢复。

## 高性能C++

### 不必要的复制

在类的构造函数中有时候必须存储数据的副本，可以如下操作：

```c++
class C {
	std::vector<int> v_;
	C(const std::vector<int>& v) : v_(v) { … }
	C(std::vector<int>&& v) : v_(std::move(v)) { … }
};

```

缺点就是需要编写两个构造函数，另一种方法是按值传递所有参数并移动参数：

```c++
class C {
    std::vector<int> v_;
    C(std::vector<int> v) : v_(std::move(v))
    { … do not use v here!!! … }
};

```

返回值优化（RVO）是一种特殊的优化。通常编译器可以对程序做任何事情，只要不改变可观察对象的行为，可观察行为包括输入、输出以及访问易失性存储器。但是RVO导致了一个可观察的变化，复制构造函数和匹配的析构函数的预期输出不见了，实际上这是一个例外：编译器允许消除复制或移动构造函数和相应析构函数的调用，即使这些函数有包含可观察行为的副作用，而且这个例外不局限于RVO。通常不能仅仅因为编写了一些似乎在进行复制的代码，就指望调用复制和移动构造函数，这就是所谓的忽略复制。另外，只是一种优化，代码必须先编译然后才能优化，因此，如果删除了复制和移动构造函数就可能编译报错。

```c++
 C makeC(int i) { return C(i); }
```

这段代码在17中可以很好的编译，原因在于未命名RVO自动17之后是强制性的，标准规定一开始就不要求复制或移动。注意返回一个显式的移动将禁用RVO，这个时候返回值如果显式的调用std::move，要么获得一个移动要么会进行复制。这种情况下如果删除了移动构造函数而没有删除复制构造函数会发生什么。需要注意的是，声明一个已删除的成员函数与不声明任何成员函数是不同的，如果编译器对移动构造函数执行重载解析，将找到一个移动构造函数（即使这个构造函数被删除了）。编译失败的原因在于重载解析选择一个已删除的函数作为最佳的重载。如果想强制使用复制构造函数，就不必声明任何移动构造函数。

当然我们也可以使用指针来避免复制，为了不用管理生命周期，最佳方案是返回智能指针，比如：

```c++
std::unique_ptr<C> makeC(int i) {
	return std::make_unique<C>(i);
}
```

这样的工厂函数应该返回唯一指针，即使想要共享指针来管理，从唯一指针移动到共享指针的成本也很低。

### 低效的内存管理

从基准测试可以看出，分配内存很消耗性能，因此如果预先知道最大内存，可以在开始预留内存。如果事先不知道内存大小，可以使用仅增长的缓存区来处理，比如：

```c++
class Buffer {
    size_t size_;
    std::unique_ptr<char[]> buf_;
public:
    explicit Buffer(size_t N) : size_(N), buf_(
    new char[N]) {}
    void resize(size_t N) {
        if (N <= size_) return;
        char* new_buf = new char[N];
        memcpy(new_buf, get(), size_);
        buf_.reset(new_buf);
        size_ = N;
    }
    char* get() { return &buf_[0]; }
};
```

只增长的缓冲区比固定大小的缓冲区慢，但比每次分配和回收内存都会快很多。

在并发程序中，内存分配更加低效，如果多个线程同时分配和释放内存，那么内部数据的管理必须由锁来保护，这是一个全局锁，因此如果经常调用分配器，会限制整个程序的扩展性。最常见的解决方案是使用带有线程局部缓存的分配器，比如流行的malloc替换库TCMalloc。这些分配器为每个线程预留了一定数量的内存，当一个线程需要分配内存时，首先从线程本地内存域中获取并分配，释放时内存会添加到线程特定域中，不需要任何锁。

对于内存碎片，一种解决方法是块分配器，比如所有内存都以固定大小的块（比如64kb）分配。

### 条件执行的优化

注意只有当分析器显示出较差的分支预测时才应该进行条件执行的优化。比如，手动优化这段代码几乎没什么用：

```c++
int f(int x) { return (x > 0) ? x : 0; }
```

原因在于大多数编译器不会使用条件跳转来实现这一行，在x86上，一些编译器使用CMOVE指令，根据条件，将值从两个源寄存器之一移动到目标寄存器。另一个不太可能优化的是条件函数调用：

```c++
if (condition) f1(… args …) else f2(… args …);
```

无分支实现可以使用函数指针数组：

```c++
using func_ptr = int(*)(… params …);
static const func_ptr f[2] = { &f1, &f2 };
(*f[condition])(… args …);
```

但如果函数最初是内联的，那么用间接函数调用替换会影响性能。编译期间跳转到另一个地址未知的函数，其效果类似于错误的预测分支，所以这段代码会使得CPU刷新流水线。

## C++中的编译器优化

### 翻译器优化代码

对于翻译器优化代码：

- 非内联函数会破坏大多数优化，因为编译器必须假设一个函数，但没有看到具体的代码，所以可以做任何合法的事情；
- 全局变量和共享变量对于优化有害；
- 编译器更可能优化短而简单的代码，而不是长而复杂的代码；

编译器的大多数优化都局限于基本代码块，这些块只有一个入口点和一个出口点，在程序的流控制图中充当节点。基本块之所以重要，是因为编译器可以看到块内发生的一切，因此可以对不改变输出的代码转换进行推理，所以内联的优点是它增加了基本块的大小。

对于内联函数，当使用函数体的副本替换函数调用时，将由编译器执行。为了实现这一点，内联函数的定义必须在调用代码的编译过程中可见，调用的函数必须在编译时已知。第一个要求在进行全程序优化的编译器中是宽松的，第二个要求排除虚函数调用和通过函数指针间接调用，并不是每个可以内联的函数最终都可以内联，编译器必须权衡代码膨胀和内联带来的好处。C++的inline关键字只是建议，编译器可以忽视。

函数调用内联最明显的好处是消除了函数调用的成本，大多数情况下函数调用的开销并不大。主要的好处是编译器在函数调用中可以做的优化非常受限。内联对优化影响的关键在于，内联允许编译器看到在本来神秘的函数中没有发生的事情，比如空的析构函数。内联还有另一个重要作用，创建内联函数体的唯一副本，可以使用调用者给出的特定输入进行优化。

```c++
bool pred(int i) { return i == 0; }
…
std::vector<int> v = … fill vector with data …;
auto it = std::find_if(v.begin(), v.end(), pred);
```

假设pred的定义和find_if的调用在同一个编译单元，对pred的调用是否内联？答案是可能，取决于find_if是否内联。现在find_if是模板，如果find_if没有内联，则会从模板中为特定类型生成一个函数，这个函数中第三个实参是一个函数指针，但这个指针在编译时未知，同一个find_if可以用许多不同的谓词来调用，这些谓词都不能是内联的。只有编译器为这个特定的调用生成唯一的find_if副本时，谓词函数才能内联，编译器有时会这么做，但在大多数情况下，内联谓词作为参数传入的其他内部函数的唯一方法是首先外联外部函数。

```c++
bool pred(int i) { return i == 0; }
…
std::vector<int> v = … fill vector with data …;
auto it = std::find_if(v.begin(), v.end(),
[&](int i) { return pred(i); });

```

这段代码中，pred反而更容易内联，因为这一次谓词的类型是唯一的。

有时候我们可以帮助编译器确定一些信息，比如：

```c++
//优化前
template <typename T>
int f(const std::vector<int>& v, const T& b) {
    int sum = 0;
    for (int a: v) {
    	if (b) sum += g(a);
    }
    return sum;
}

//优化后
template <typename T>
int f(const std::vector<int>& v, const T& t) {
    const bool b = bool(t);
    int sum = 0;
    for (int a: v) {
    	if (b) sum += g(a);
    }
    return sum;
}

```

通过创建b这个临时变量，我们告诉编译器，g的调用不会改变t的值。

另一个阻止编译器进行优化的情况是混叠的发生，比如：

```c++
void init(char* a, char* b, size_t N) {
    std::memset(a, '0', N);
    std::memset(b, '1', N);
}
```

编译器不会优化，因为编译器必须考虑a跟b是否指向同一个数组，或一个数组部分是否有重叠的可能性。编译器只关心一个问题，如何不改变代码的行为。同样的问题也发生在通过指针或引用接收多个形参的函数中：

```c++
void do_work(int& a, int& b, int& x) {
    if (x < 0) x = -x;
    a += x;
    b += x;
}
```

编译器会考虑是否有别名的存在，必须在a递增后从内存中读取x，防止自己假设x保持不变，因为a和x可能指向相同的值。如果确定不会发生混叠，可以使用C中的关键字restrict，告诉编译器一个特定的指针是访问当前函数范围内的唯一方法：

```c++
void init(char* restrict a, char* restrict b, size_t N);
```

对于奇异值（特别是引用）创建一个临时变量可以解决如下问题：

```c++
void do_work(int& a, int& b, int& x) {
    if (x < 0) x = -x;
    const int y = x;
    a += y;
    b += y;
}
```

以下是一个将运行时值转换为编译时值的例子：

```c++
//优化前
enum op_t { do_shrink, do_grow };
void process(std::vector<Shape>& v, op_t op) {
    for (Shape& s : v) {
        if (op == do_shrink) s.shrink();
        else s.grow();
    }
}

//优化后
template <op_t op>
void process(std::vector<Shape>& v) {
    for (Shape& s : v) {
        if (op == do_shrink) s.shrink();
        else s.grow();
    }
}
void process(std::vector<Shape>& v, op_t op) {
    if (op == do_shrink) process<do_shrink>(v);
    else process<do_grow>(v);
}

```

## 未定义的行为和性能

### 未定义行为

标准中说明如果行为没有定义，则标准对结果没有任何要求。

### 未定义的行为和C++优化

通过假设程序中的循环最终都会结束，编译器能够优化某些循环和包含这些循环的代码。优化器使用的逻辑都相同：首先假设程序不显示UB（未定义的行为），然后推导出必须为真的条件，使这个假设成立，并假设这些条件确实总是为真，最后，在这种假设下有效的优化都可以进行。

如果想使用const来进行优化，则：

- 如果值不改变，将其声明为const；
- 更好的优化是如果该值在编译时已知，将其声明为constexpr；
- 通过const引用传递形参给函数没有任何优化作用；
- 对于小类型，按值传递比按引用传递更有效；

### 使用未定义的行为进行高效的设计

文中举得例子可以分为两种，一种是像++k + k表达式这样的代码是错误的，因为其根本没有定义行为，第二种是像k+1这样的代码，其中k是有符号的整数，如果溢出结果是未定义的。也就是说，代码具有隐式的先决条件。即，如果输入符合某些限制，则保证结果是正确的，并且程序以一种定义良好的方式运行。

编译器有选项来启动UB杀灭器（UBSan）来找出UB。对于Clang和GCC，选项为：

```shell
clang++ --std=c++17 –O3 –fsanitize=undefined ub.C
```

## 为性能而设计

### 为性能设计

最小信息原则，即极可能少的交互信息，上下文在这里非常重要，建议组件尽可能少的暴露它如何处理特定请求的信息。在此类接口或交互中，作出和履行承诺的一方不应该提供额外的信息。即接口不应该暴露实现。

最大信息原则，不同于完成请求的组件应该避免暴露可能限制实现的信息，对发出请求的组件应该能够提供关于具体需要什么的特定信息。

### API的设计考虑

设计高效并发API的指导原则：

- 用于并发使用的接口应该是事务性的；
- 接口应该提供最小的必要线程安全保证；
- 对于既用作客户端可见API，又用作创建自己的、更复杂的事务，需要提供锁的高级组件构建块接口，通常有两个版本，一个具有强线程安全保证，另一个具有弱线程安全保证；