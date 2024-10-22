# Linux多线程服务端编程：使用muduo C++网络库

## 线程安全的对象生命期管理

### 当析构函数遇到多线程

即析构时面临的线程安全问题，一个线程安全的类应当满足以下条件：

- 多个线程同时访问行为正确；
- 无论系统如何调度这些线程，调用端代码无须额外的同步或其他协调动作；

依据这个定义，标准库里大多数类都不是线程安全的，比如std::string、std::vector、std::map等。

临界区在windows上是struct critical_section，是可重入的，在linux下是pthread_mutex_t，默认是不可重入的。

### 对象的创建很简单

对象构造要做到线程安全，唯一的要求是构造时不要泄露this指针，即：

- 不要在构造函数中注册任何回调；
- 不要在构造函数中把this传给跨线程的对象，即使在构造函数最后一行也不行；

这时因为此时对象还没有完全初始化，此时如果被使用后果不堪设想。至于为什么最后一行抛出对象还不行是因为对象有可能是个基类，此时可能执行派生类的构造函数，那么此时对象还是处于未完全初始化。

### 销毁太难

在析构函数里使用mutex不是办法，因为mutex成员对象的生命周期最多和对象一样长。另外一个函数如果要锁住相同类型的多个对象，为了保证按相同顺序加锁，可以比较mutex对象的地址，始终先加锁地址较小的mutex。

### 线程安全的observer有多难

在OOP中，对象的关系主要就是组合、关联和聚合这三种，对于组合来说，对象x的生命周期由拥有者控制，因此比较好处理，后两种比较难办，处理不好会造成内存泄漏或重复释放。为了解决这种问题，一种简单的方案是只创建不销毁，使用对象池来暂存用过的对象，下次申请新对象时如果对象池有存货就重复利用现有的对象，否则新建一个，对象用完了不是直接释放而是放回池子里。这种方案的问题是：

- 对象池的线程安全，如何安全、完整的将对象放回池子里，防止出现部分放回的竟态（线程A认为对象x已经放回，B则认为对象x还活着）；
- 全局共享数据引发的lock contention，集中化的对象池会不会把多线程并发的操作串行化；
- 如果共享对象的类型不止一种，那么重复利用对象池还是使用类模板；
- 会不会造成内存泄漏与分片？因为对象池占用的内存只增不减，而且多个对象池不能共享内存；

我们可能会构造出这样的observer代码：

```c++
class Observer
{
    // 同前
    void observe(Observable* s) {
        s->register_(this);
        subject_ = s;
    }

    virtual ~Observer() {
    	subject_->unregister(this);
    }

    Observable* subject_;
};
```

然而，在析构的时候如何知道subject\_还是活的？就算subject\_活着，那么也有可能一个线程刚析构，另一个线程开始调用notify。

### 原始指针有何不妥

以上的分析在于裸指针没有检测自己是否活着的能力，那么考虑使用智能指针。

空悬指针是由于两个指针指向了一个内存，其中一个指针释放，就造成了空悬指针。一个解决方案是引入一层间接性，让p1和p2所指的对象永久有效，即：

![muduo-1-2](..\image\Network\muduo-1-2.png)

销毁object之后proxy对象依旧存在，不过值变为0。但线程安全的情况依旧存在，比如p2看到proxy非零正要调用object的成员函数，期间object被p1销毁。为了安全的释放proxy，引入引用计数，只有两个指针都释放引用计数才为0，此时可以安全的释放proxy和object，这正是引用计数型的智能指针。

### 神器share_ptr和weak_ptr

- shared_ptr：只要有一个对象绑定shared_ptr，该对象就不会被析构，除非最后一个shared_ptr析构或者reset的时候，对象x会被销毁；
- weak_ptr不控制对象的生命周期，它知道对象是否活着，如果活着，它可以提升为shared_ptr，如果死了，提升会失败返回一个空的shared_ptr，提升/lock行为是线程安全的；
- shared_ptr/weak_ptr的计数在主流平台是原子操作，没有用锁；
- shared_ptr/weak_ptr的线程安全级别与string和stl容器一样；

### 插曲：系统的避免各种指针错误

1. 缓冲区溢出：用std::vector\<char\>/std::string或自己编写的Buffer class来管理缓冲区，自动记住使用缓冲区的长度，并通过成员函数而不是裸指针来修改缓冲区；
2. 空悬指针/野指针：用shared_ptr或者weak_ptr；
3. 重复释放：用scoped_ptr，只在对象析构时释放一次；
4. 内存泄漏：用scoped_ptr，对象析构的时候自动释放内存；
5. 不配对的new[]/delete：把new[]替换成std::vector/scoped_array；

注意，如果这几种智能指针是对象x的数据成员，而它的模板参数T是个incomplete类型，那么x的析构函数不能是默认的或是内联的，必须在cpp文件里显式定义，否则会有错误。

### 应用到Observer上

使用weak_ptr来改善观察者，因为其能探查对象的生死：

```c++
class Observable // not 100% thread safe!
{
public:
    void register_(weak_ptr<Observer> x); // 参数类型可用 const weak_ptr<Observer>&
    // void unregister(weak_ptr<Observer> x); // 不需要它
    void notifyObservers();

private:
    mutable MutexLock mutex_;
    std::vector<weak_ptr<Observer> > observers_;
    typedef std::vector<weak_ptr<Observer> >::iterator Iterator;
};

void Observable::notifyObservers()
{
    MutexLockGuard lock(mutex_);
    Iterator it = observers_.begin(); // Iterator 的定义见第 49 行
    while (it != observers_.end())
    {
        shared_ptr<Observer> obj(it->lock()); // 尝试提升，这一步是线程安全的
        if (obj)
        {
            // 提升成功，现在引用计数值几乎总是大于等于 2。（想想为什么？）
            obj->update(); // 没有竞态条件，因为 obj 在栈上，对象不可能在本作用域内销毁
            ++it;
        }
        else
        {
            // 对象已经销毁，从容器中拿掉 weak_ptr
            it = observers_.erase(it);
        }
    }
}

```

### 再论shared_ptr的线程安全

shared_ptr的引用计数本身安全无锁，但对象的读写则不是，因为shared_ptr有两个数据成员，读写操作不能原子化，其的线程安全级别和内建类型、标准库容器、std::string一样，即：

- 一个shared_ptr对象实体可以被多个线程同时读取；
- 两个shared_ptr对象实体可以被两个线程同时写入；
- 如果要从多个线程读写同一个shared_ptr对象，需要加锁；

正确保护的代码如下，并且尽量使得临界区最短：

```c++
void read()
{
    shared_ptr<Foo> localPtr;
    {
        MutexLockGuard lock(mutex);
        localPtr = globalPtr; // read globalPtr
    }
    // use localPtr since here，读写 localPtr 也无须加锁
    doit(localPtr);
}

void write()
{
    shared_ptr<Foo> newPtr(new Foo); // 注意，对象的创建在临界区之外
    {
        MutexLockGuard lock(mutex);
        globalPtr = newPtr; // write to globalPtr
    }
    // use newPtr since here，读写 newPtr 无须加锁
    doit(newPtr);
}

```

### shared_ptr技术与陷阱

有可能意外延长对象的生命周期，shared_ptr是强引用，如果不小心遗留了一个拷贝，那么对象永远不会析构，比如把observes_的类型改为vector\<shared_ptr\<Observe\>\>，那么除非手动调用unregister，否则Observer对象会跟Observable绑定，直到Observable析构Observer才会析构，这违背了观察者模式的本意，而使用weak_ptr则没这种顾虑，如果Observer存在就会发送通知，没有就不发。

另一个出错的可能是boost::bind，其会把实参拷贝一份，如果参数是个shared_ptr，那么对象的生命周期就不会短于boost::function对象。

### 对象池

示例如下：

```c++
// version 1: questionable code
class StockFactory : boost::noncopyable
{
public:
	shared_ptr<Stock> get(const string& key);
private:
    mutable MutexLock mutex_;
    std::map<string, weak_ptr<Stock> > stocks_;
};

// version 2: 数据成员修改为 std::map<string, weak_ptr<Stock> > stocks_;
shared_ptr<Stock> StockFactory::get(const string& key)
{
    shared_ptr<Stock> pStock;
    MutexLockGuard lock(mutex_);
    weak_ptr<Stock>& wkStock = stocks_[key]; // 如果 key 不存在，会默认构造一个
    pStock = wkStock.lock(); // 尝试把“棉线”提升为“铁丝”
    if (!pStock) {
        pStock.reset(new Stock(key));
        wkStock = pStock; // 这里更新了 stocks_[key]，注意 wkStock 是个引用
    }
    return pStock;
}
```

但这样写会造成stocks_大小只增不减，因此还是要利用shared_ptr的定制析构的功能：

```c++
// version 3
class StockFactory : boost::noncopyable
{
// 在 get() 中，将 pStock.reset(new Stock(key)); 改为：
// pStock.reset(new Stock(key),
// boost::bind(&StockFactory::deleteStock, this, _1)); // ***
private:
    void deleteStock(Stock* stock)
    {
        if (stock) {
            MutexLockGuard lock(mutex_);
            stocks_.erase(stock->key());
        }
        delete stock; // sorry, I lied
	}
// assuming StockFactory lives longer than all Stock's ...
// ...

```

get方法把原始指针this保存到了function中，如果StockFactory的生命周期比Stock短，那么Stock析构时回调deleteStock就会core dump，此时应该使用shared_ptr，但是get本身是个成员函数，如何获得一个指向当前对象的shared_ptr\<StockFactory\>对象呢？可以使用enable_shared_from_this，这是一个以其派生类为模板类型实参的基类模板，继承它之后this指针就能变身为shared_ptr：

```c++
class StockFactory : public boost::enable_shared_from_this<StockFactory>,
boost::noncopyable
{ /* ... */ };
```

为了使用它，StockFactory不能是栈对象，必须是堆对象且由shared_ptr管理生命周期：

```c++
shared_ptr<StockFactory> stockFactory(new StockFactory);

// version 4
shared_ptr<Stock> StockFactory::get(const string& key)
{
    // change
    // pStock.reset(new Stock(key),
    // boost::bind(&StockFactory::deleteStock, this, _1));
    // to
    pStock.reset(new Stock(key),
    boost::bind(&StockFactory::deleteStock,
    shared_from_this(),
    _1));
// ...

```

注意一点，shared_from_this不能在构造函数里调用，因为在构造时它还没有被shared_ptr接管。

将shared_ptr绑定到function里，那么回调的时候StockFactory始终存在，是安全的，但也延长了对象的生命周期，使之不短于绑定的function对象。有时我们需要如果对象活着就调用它的成员函数，否则忽略之的语义，就像Observable::notifyObservers()那样，也可称之为弱回调，可用weak_ptr实现，即将weak_ptr绑定到function，这样对象的生命周期不会被延长，然后在回调的时候再尝试提升为shared_ptr，成功则执行回调，失败则忽略。代码如下：

```c++
class StockFactory : public boost::enable_shared_from_this<StockFactory>,
boost::noncopyable
{
public:
    shared_ptr<Stock> get(const string& key)
    {
        shared_ptr<Stock> pStock;
        MutexLockGuard lock(mutex_);
        weak_ptr<Stock>& wkStock = stocks_[key]; // 注意 wkStock 是引用
        pStock = wkStock.lock();
        if (!pStock)
        {
            pStock.reset(new Stock(key),
            boost::bind(&StockFactory::weakDeleteCallback,
            boost::weak_ptr<StockFactory>(shared_from_this()),
            _1));
            // 上面必须强制把 shared_from_this() 转型为 weak_ptr，才不会延长生命期，
            // 因为 boost::bind 拷贝的是实参类型，不是形参类型
            wkStock = pStock;
        }
        return pStock;
    }
private:
    static void weakDeleteCallback(const boost::weak_ptr<StockFactory>& wkFactory,
    Stock* stock)
    {
        shared_ptr<StockFactory> factory(wkFactory.lock()); // 尝试提升
        if (factory) // 如果 factory 还在，那就清理 stocks_
        {
        	factory->removeStock(stock);
        }
        delete stock; // sorry, I lied
	}
    void removeStock(Stock* stock)
    {
        if (stock)
        {
            MutexLockGuard lock(mutex_);
            stocks_.erase(stock->key());
        }
    }
private:
    mutable MutexLock mutex_;
    std::map<string, weak_ptr<Stock> > stocks_;
};

```

这样无论Stock和StockFactory谁先挂掉都不会影响程序的正常运行，借助了shared_ptr和weak_ptr完美解决了两个对象互相引用的问题。

### Observer之谬

观察者是基类，带来了非常强的耦合性，这种耦合不仅限制了成员函数的名字、参数、返回值等，还限制了成员函数所属于的类型。比如想要观察两个事件，可能就需要继承两个基类。C++里为了替换Observer，可以用信号/槽机制，在11中，借用可变模板，可以实现最简单的一对多回调：

```c++
template<typename Signature>
class SignalTrivial;
// NOT thread safe !!!
template <typename RET, typename... ARGS>
class SignalTrivial<RET(ARGS...)>
{
public:
    typedef std::function<void (ARGS...)> Functor;
    void connect(Functor&& func)
    {
    	functors_.push_back(std::forward<Functor>(func));
    }
    void call(ARGS&&... args)
    {
        for (const Functor& f: functors_)
        {
        	f(args...);
        }
    }
private:
	std::vector<Functor> functors_;
};

```

## 线程同步精要

线程同步的四项原则：

- 首要原则是尽量最低限度共享对象，减少同步的场合，一个对象能不暴露给其他线程就不暴露，如果要暴露优先考虑immutable对象，实在不行才可暴露可修改的对象，并用同步措施保护；
- 其次是使用高级的并发编程构建，如TaskQueue、Producer-Consumer Queue、CountDownLatch等等；
- 不得已使用底层同步原语时只用非递归的互斥器和条件变量，慎用读写锁，不要用信号量；
- 除了使用atomic整数之外，不自己编写无锁代码，不用内核级同步原语，不凭空猜测哪种性能更好，比如spin lock vs mutex;

### 互斥器

互斥器作者推荐的原则是：

- 用RAII手法封装mutex的创建、销毁、加锁、解锁四个操作；
- 只用非递归的mutex（即不可重入的mutex）；
- 不手工调用lock和unlock函数，一切交给栈上的Guard对象的构造和析构函数负责，Guard对象的生命期正好等于临界区，这样保证始终在同一个函数同一个scope里对某个mutex加锁和解锁，这种做法叫做Scoped Locking；
- 在每次构造Guard对象时候，思考调用栈上已经持有的锁，防止因加锁顺序不同出现死锁。

次要原则有：

- 不适用跨进程的mutex，进程间通信只用TCP sockets；
- 加锁解锁在同一个线程；
- 别忘了解锁；
- 不重复解锁；
- 必要的时候可以用PTHREAD_MUTEX_ERRORCHECK来排错；

mutex分为递归和非递归两种，区别在于：同一个线程可以重复对recursive mutex加锁，但不能重复对non-recursive mutex加锁。使用非递归的锁主要是为了体现设计意图，因为两者性能差别不大，仅仅非递归锁因为少用了一个计数器，略快一点。

### 条件变量

条件变量只有一种正确使用的方式，对于wait端：

1. 必须与mutex一起使用，该布尔表达式的读写需要受此mutex保护；
2. 在mutex已上锁的时候才能调用wait；
3. 把判断布尔条件和wait放到while循环中；

示例：

```c++
muduo::MutexLock mutex;
muduo::Condition cond(mutex);
std::deque<int> queue;
int dequeue()
{
    MutexLockGuard lock(mutex);
    while (queue.empty()) // 必须用循环；必须在判断之后再 wait()
    {
        cond.wait(); // 这一步会原子地 unlock mutex 并进入等待，不会与 enqueue 死锁
        // wait() 执行完毕时会自动重新加锁
    }
    assert(!queue.empty());
    int top = queue.front();
    queue.pop_front();
    return top;
}

```

条件变量是非常底层的同步原语，很少直接使用，以下是一个CountDownLatch的例子：

```c++
class CountDownLatch : boost::noncopyable
{
public:
    explicit CountDownLatch(int count); // 倒数几次
    void wait(); // 等待计数值变为 0
    void countDown(); // 计数减一
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
    	condition_.wait();
}
void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
    	condition_.notifyAll();
}

```

### 不要用读写锁和信号量

读写锁不一定是个好的选择：

- 从正确性来讲，有可能在持有read lock的时候修改了共享数据；
- 从性能来说，读写锁不见得比普通mutex更高效，无论如何read lock加锁的开销不会比mutex lock小，因为它要更新当前reader的数目，如果临界区很小，锁竞争不激烈，mutex往往更快；
- reader lock可能允许提升为write lock，也可能不允许提升；如果在允许提升的情况下，假设有对vector的post和traver操作，当traver操作时误调用了post，此时相当于可重入锁，可能会由于push_back操作导致偶尔的crash，相比较而言，还不如不允许提升，出现的死锁方便调试；
- 通常读锁是可重入的，写锁是不可重入的，但是为了防止写锁饥饿，写锁通常会阻塞后来的读锁，因此读锁在重入的时候可能死锁，另外，在追求低延迟读取的场合也不适合读写锁。



封装MutexLock、MutexLockGuard、Condition以及构造线程安全的单例实现已过时，此处不予讨论。

### sleep(3)不是同步原语

生产代码中线程的等待可分成两种：一种是等待资源可用（要么等在select/poll/epoll_wait上，要么等在条件变量上）；一种是等待进入临界区（mutex)上以便读写数据。后者一般等待时间极短。

在程序的正常执行中，如果需要等待一段时间，应该往event loop中注册一个timer，然后在timer的回调函数里接着干活，因为线程是个珍贵的共享资源，如果等待某个时间发生，应该采用条件变量或者IO事件回调，不能用sleep轮询。

### 借shared_ptr实现copy-on-write

原理是借shared_ptr管理共享数据，原理如下：

- shared_ptr是引用计数智能指针，如果当前只有一个观察者，那么引用计数为1；
- 对于写端，如果引用计数为1，这时可以安全修改共享对象，不必担心有人正在读取；
- 对于读端，读之前将引用计数加一，读完减一，保证在读的期间引用计数大于一，阻止并发写；

示例如下：

```c++
typedef std::vector<Foo> FooList;
typedef boost::shared_ptr<FooList> FooListPtr;
MutexLock mutex;
FooListPtr g_foos;

1 void traverse()
2 {
3 	FooListPtr foos;
4 	{
5 		MutexLockGuard lock(mutex);
6 		foos = g_foos;
7 		assert(!g_foos.unique());
8 	}
9
10 // assert(!foos.unique()); 这个断言不成立
11 	for (std::vector<Foo>::const_iterator it = foos->begin();
12 	it != foos->end(); ++it)
13 	{
14 		it->doit();
15 	}
16 }

17 void post(const Foo& f)
18 {
19 	printf("post\n");
20 	MutexLockGuard lock(mutex);
21 	if (!g_foos.unique())
22 	{
23 		g_foos.reset(new FooList(＊g_foos));
24 		printf("copy the whole list\n"); // 练习：将这句话移出临界区
25 	}
26 	assert(g_foos.unique());
27 	g_foos->push_back(f);
28 }
```

## 多线程服务器的适用场合与常用编程模型

### 进程与线程

本书的进程指的是linux操作系统通过fork系统调用产生的东西，或者windows下createProcess的产物，每个进程有自己独立的地址空间。线程的特点是共享地址空间，从而可以高效的共享数据。

### 单线程服务器的常用编程模型

最广泛的是non-blocking IO + IO multiplexing这种模型，即Reactor模式，在这种模型中，程序的基本结构是一个事件循环，以事件驱动和事件回调的方式实现业务逻辑：

```c++
// 代码仅为示意，没有完整考虑各种情况
while (!done)
{
    int timeout_ms = max(1000, getNextTimedCallback());
    int retval = ::poll(fds, nfds, timeout_ms);
    if (retval < 0) {
    	处理错误，回调用户的 error handler
    } else {
    	处理到期的 timers，回调用户的 timer handler
        if (retval > 0) {
        	处理 IO 事件，回调用户的 IO event handler
        }
    }
}
```

Reactor模型的优点很明显，编程不难，效率也不错，不仅可以用于读写socket连接的建立，甚至DNS解析都可以以非阻塞的方式进行，以提高并发度和吞吐量，对于IO密集的应用是个不错的选择。但这种模型也有其本质的缺点，要求事件回调函数必须是非阻塞的，对于设计网络IO的请求响应式协议，容易割裂业务逻辑，使其散布于多个回调之中。

### 多线程服务器的常用编程模型

大概有如下几种：

1. 每个请求创建一个线程，使用阻塞式IO操作;
2. 使用线程池，同样使用阻塞式IO操作，比第一种性能高；
3. 使用non-blocking IO + IO multiplexing，即Java NIO方式；
4. Leader/Fellower等高级模式；

默认情况下，作者使用第三种方式来编写多线程C++网络服务程序。

在这种模型之下，程序里的每个IO线程有一个event loop（或者叫做Reactor），用于处理读写和定时事件（无论是周期性的还是单次的），这种方式的好处是：

- 线程数目基本固定，可以在程序启动的时候设置，不会频繁创建销毁；
- 可以很方便的在线程间调配负载；
- IO事件发生的线程固定，同一个TCP连接不必考虑事件并发；

Event loop代表线程的主循环，需要哪个线程干活就把timer或IO channel（如TCP连接）注册到哪个线程的loop中即可。对实时性有要求的connection可以单独用一个线程，数据量大的connection可以独占一个线程，并把数据处理任务分摊到另外几个计算线程（用线程池）；其他次要的辅助性connects可以共享一个线程。

对于non-trivial的服务端程序，一般采用non-blocking IO + IO multiplexing，每个connection/accptor都会注册到某个event loop中，程序中有多个event loop，每个线程至多有一个event loop.

多线程程序要求event loop线程安全。

对于没有IO光有计算任务的线程，使用event loop有点浪费，作者使用了blocking queue实现的任务队列：

```c++
typedef boost::function<void()> Functor;
BlockingQueue<Functor> taskQueue; // 线程安全的阻塞队列
void workerThread()
{
    while (running) // running 变量是个全局标志
    {
        Functor task = taskQueue.take(); // this blocks
        task(); // 在产品代码中需要考虑异常处理
    }
}
```

用这种方式实现线程池较为容易：

```c++
int N = num_of_computing_threads;
for (int i = 0; i < N; ++i)
{
create_thread(&workerThread); // 伪代码：启动线程
}

```

使用也比较简单：

```c++
Foo foo; // Foo 有 calc() 成员函数
boost::function<void()> task = boost::bind(&Foo::calc, &foo);
taskQueue.post(task);
```

总结来说，作者推荐C++多线程服务端编程模式为：one(event) loop per thread + thread pool：

- event loop（IO loop）用作IO multiplexing，配合non-blocking IO和定时器；
- thread pool用作计算，具体可以是任务队列或者生产者消费者队列；

### 进程间通信只用TCP

Linux下进程间通信方式有很多：匿名管道（pipe）、具名管道（FIFO）、Posix消息队列、共享内存、信号、sockets等。同步原语又有很多，如互斥器、条件变量、读写锁、文件锁、信号量等。作者推荐进程间通信首选Sockets，其最大的好处在于：可以跨主机，具有伸缩性。

在编程上，TCP sockets和pipe都是操作文件描述符，来收发字节流，都可以read、write、fcntl、select和poll等，不同的是TCP是双向的，linux的pipe是单向的，双向通信还得开两个文件描述符，而且需要进程间有父子关系。在收发字节流这一通信模型下，没有比Sockets/TCP更自然的IPC了，当然pipe也有一个经典应用场景，那就是写Reactor/event loop时用来异步唤醒select（或等价的poll/epoll_wait）调用。

TCP port由一个进程独占，且操作系统会自动回收。两个进程通过TCP通信，如果一个崩溃，操作系统会关闭连接，另一个进程几乎可以立刻感知，可以快速failover。当然应用层的心跳也是必不可少的。

与其他IPC相比，TCP的一个好处就是可记录可重现。tcpdump和wireshark是解决两个进程间协议和状态争端的好帮手，也是性能分析的利器。另外，如果网络库带连接重试功能的话，我们可以不要求系统里的进程以特定的顺序启动，任何一个进程都能单独重启。

使用TCP这种字节流方式通信，会有marshal/unmarshal的开销，要求选用合适的消息格式，准确说是wire format，目前作者推荐protocol buffers。

分布式系统的软件设计和功能划分一般以进程为单位，从宏观看，一个分布式系统由运行在多台机器上的多个进程组成的，进程之间采用TCP长连接通信。本章讨论分布式系统中单个服务进程的设计方法。使用TCP长连接的好处有两点：一是容易定位分布式系统中的服务之间的依赖关系，只要在机器上运行netstat -tpna | grep :port就能立刻列出用到某服务的客户端地址，然后在客户端的机器上用netstat或lsof命令找到是哪个进程发起的连接，TCP短链接和UDP则不具备这一特性。二是通过接收和发送队列的长度也比较容易定位网络或者程序故障。在正常运行时netstat打印的Recv-Q和Send-Q都应该接近0，或者在0附近摆动，如果Recv-Q保持不变或持续增加，则通常意味着服务进程的处理速度变慢，可能发生了死锁或阻塞。如果Send-Q保持不变或持续增加，有可能是对方服务器太忙、来不及处理，也有可能是网络中间某个路由器或交换机故障造成丢包或者对方服务器掉线。

### 多线程服务器的适用场合

指的是跑在多核机器上的linux用户态的没有用户界面的长期运行的网络应用程序，通常是分布式系统的组成部件。

开发服务端程序的一个基本任务是处理并发连接，现在服务端网络编程处理并发连接主要有两种方式：

- 当线程很廉价时，一台机器上可以创建远高于CPU数目的线程，这时一个线程只处理一个TCP连接，通常使用阻塞IO。
- 当线程很宝贵时，一台机器只创建与CPU数目相当的线程，这时一个线程要处理多个TCP连接上的IO，通常使用非阻塞IO和IO multiplexing（IO多路复用）；

在处理并发连接时也要充分发挥硬件资源的作用，不能用CPU闲置。本节主要讨论后一种方式，这些线程应该属于一个进程（以下模式2），还是分属于多个进程（模式3），与前文相同，本节的进程指的是fork系统调用的产物，线程指的是pthread_create的产物，因此是宝贵的原生线程。

一个多台机器组成的分布式系统必然是多进程的，针对一台机器来说，如果要提供一种服务或执行一个任务，可用的模式有：

1. 运行一个单线程的进程；
2. 运行一个多线程的进程；
3. 运行多个单线程的进程；
4. 运行多个多线程的进程；

总结如下，模式1不可伸缩，不能发挥多核的计算能力；模式3是目前公认的主流模式，有以下两种子模式，简单的把模式1种进程运行多份，主进程加worker进程，如果必须绑定到一个TCP port；模式2被鄙视，多线程难写而且与模式3相比没什么优点；模式4更是没啥优点，返回结合了2和3的缺点。本文主要想讨论的是模式2和3b的优劣，即什么时候一个服务器程序应该是多线程的。

必须用单线程的场合，作者知道的有两种：

1. 程序可能会fork(2)；
2. 限制程序的CPU占用率；

对于1来说只有单线程程序能fork，因为多线程程序fork的话会遇到很多麻烦，一个程序fork(2)之后一般有两种行为：

1. 立即执行exec()，变身为另一个程序，例如shell和inetd，或者集群中运行在计算节点上负责启动job的守护进程（即看门狗进程）；
2. 不调用exec，继续执行当前程序。要么通过共享的文件描述符与父进程通信，协同完成任务，要么接过父进程传过来的文件描述符独立完成工作；

在这些行为中，作者认为只有看门狗进程必须坚持单线程其他的从功能讲均可替换为多线程程序。

单线程程序能限制程序的CPU占用率。因此对于一些辅助类的程序，如果其必须和主要服务进程运行在同一台机器的话，做成单线程能避免过分抢夺系统的计算资源。

Event loop有一个明显的缺点，就是它是非抢占的，假设事件a优先级高于b，处理事件a需要1ms，b需要10ms，但现在b稍早于a发生，当a到来时候，程序已经离开了poll(2)调用，并开始处理事件b，那么a就需要等待10ms才有事件处理，这等于发生了优先级反转，这个缺点可以用多线程来克服。

前文所述，无论是IO bund还是CPU bound，多线程都没有什么绝对意义上的性能优势，这句话是说如果用很少的CPU负载就能让IO跑满，或者用很少的IO流量就能让CPU跑满，那么多线程没啥用处。举例：

- 对于静态web服务器，或者ftp服务器，cpu的负载较轻，主要瓶颈在于磁盘IO和网络IO方面，这时候一个单线程的程序（模式1）就可以撑满IO，此时多线程并没有增益，因为其不能提高吞吐量；
- CPU跑满的情况比较少见，作者虚构了一个例子，假设有一个服务，输入是n个整数，问能够从中选出m个使其和为0，对于这样一个服务，哪怕很小的n值也会让CPU算死，对于这种应用，模式3a最适合，能发挥多核的优势，程序也简单；

总结一下，只要任何一方早早的先到达瓶颈，多线程程序都没啥优势。

多线程适用以下的场景：提高响应速度，让IO和计算相互重叠，降低latency。一个程序要做成多线程，大致要满足：

- 有多个CPU可用，单核机器上单线程没有性能优势；
- 线程间有共享数据；
- 共享的数据可以修改；
- 提供非均质的服务，即事件的响应有优先级的差异，我们可以用专门的线程来处理优先级高的事件，防止优先级反转；
- latency和throughput同样重要，不是逻辑简单的IO bound或者CPU bound的程序，换言之，程序要有相当的计算量；
- 利用异步操作，比如logging，不论向磁盘写log file还是往log server发消息都不应该阻塞critical path；
- 能scale up，一个好的多线程程序应该能享受增加CPU带来的好处；
- 具有可预测的性能，随着负载增加，性能缓慢下降，超过某个临界点之后会极速下降，线程数目一般不随着负载变化；
- 多线程能有效的划分责任和功能；

一个多线程服务程序中的线程大致可分为三类：

1. IO线程，主循环是IO multiplexing，阻塞的等在select/poll/epoll_wait系统调用上，这类线程也处理定时事件，当然其功能不止IO，一些简单的计算也可以放入其中；
2. 计算线程，主循环是blocking queue，阻塞的等在条件变量上，这类线程一般位于线程池中；
3. 第三方库所用的线程，比如logging；

服务器程序一般不会频繁的启动和终止线程。

### 多线程服务器的适用场合例释和答疑

1. 一个linux能同时启动多少个线程：对于32位linux来说，地址空间是4G，用户态能访问3G左右，一个线程的默认栈大小是10M，则一个进程大约能同时启动300个线程；
2. 多线程能提高并发度吗：不能，如果采用thread per connection模型，并发连接数至多300，这远远低于Reactor模式中的IO multiplexing event loop；
3. 多线程能提高吞吐量吗：对于计算密集型服务，不能；
4. 多线程能降低响应时间吗：设计合理，充分利用多核资源的话可以；
5. 多线程如何让IO和计算相互重叠，降低latency：将IO操作通过BlockingQueue交给别的线程处理，自己不必等待；
6. 什么是线程池大小的阻抗匹配原则：假设池中线程执行任务时密集计算所占比重为P，而系统一共有C个CPU，则为了让CPU跑满，线程池大小应为C/P，根据经验，T的最佳值可以上下浮动50%。
7. 除了Reactor+thread pool，还有别的non-trivial多线程编程模型吗：还有Proactor，如果一次请求相应中要和别的进程打多次交道，那么Proactor模型往往能做到更高的并发度，代价是代码支离破碎。
8. 模式2和3a如何取舍：如果工作集（服务程序中响应一次请求所访问的内存大小）较大就用多线程，避免CPU缓存换入换出，否则就用单线程多进程。

## C++多线程系统编程精要

### 基本线程原语的选用

11个最基本的pthread函数为：

- 2个：线程的创建和等待结束；
- 4个：mutex的创建、销毁、加锁、解锁；
- 5个：条件变量的创建、销毁、等待、通知、广播；

可以酌情使用的有：

- pthread_once，封装为muduo::Singleton<T>，不如直接用全局变量；
- pthread_key*，封装为muduo::ThreadLocal<T>，可以考虑用__thread替换；

不建议使用：

- pthread_rwlock，读写锁慎用；
- sem_*：避免用信号量，功能与条件变量重合，容易出错；
- pthread_{cancel, kill}，出现它们通常意味着设计出了问题；

### Linux上的线程标识

posix threads提供了pthread_self函数用于返回当前进程的标识符，其类型为pthread_t，pthread_t不一定是一个数值类型（整数或指针），还有可能是一个结构体，因此pthreads提供了pthread_equal来用于对比两个线程标识符是否相等，这会带来一系列的问题：

- 无法打印输出pthread_t；
- 无法比较pthread_t的大小或计算其hash值，因此无法用于关联容器的key；
- 无法定义一个非法的pthread_t的值，用来表示绝对不存在的线程id；
- pthread_t值只在进程内有意义，与操作系统的任务调度之间无法建立有效的关联，比如想在/proc文件系统里找对应的task；

不仅如此，glibc实际把pthread_t用作一个结构体指针，指向一块动态分配的内存，而这块内存经常被使用，这很容易会造成两个pthread_t的值相同。综上，pthread_t并不适合用于在程序中对线程的标识符，在linux上建议使用gettid(2)系统调用的返回值用作线程id，好处有：

- 类型是pid_t，其值通常是一个小整数，方便在日志中输出；
- 在现代linux中，直接表示内核的任务调度id，因此在/proc里可以找到，比如/proc/tid或/prod/pid/task/tid；
- 在其他系统工具中也容易定位，比如top(1)；
- 任何时候都是全局唯一的；
- 0是非法值，因为操作系统第一个进程init的pid是1；

muduo::CurrentThread::tid()封装是用__thread变量来缓存gettid(2)的返回值，这样只有在本线程第一次调用时才进行系统调用，以后都是直接从thread local缓存的线程id中拿到结果。

### 线程创建与销毁的守则

线程的创建要遵循几条简单的原则：

- 程序库不应该在未提前告知的情况下创建自己的背景线程；
- 尽量用相同方式创建线程，例如muduo::Thread；
- 在进入main函数之前不应该启动线程；
- 线程的创建最好在初始化阶段全部完成；

线程是稀缺资源，一个进程可以创建的并发线程数目受限于地址空间的大小和内核参数，一台机器可以同时并行运行的线程数目受限于CPU的数目，因此要精心规划线程的数目，为关键任务保留足够的计算资源，如果程序库背地里使用了额外的线程，资源规划就漏算了。还有一个重要的原因，一旦程序中不止有一个线程，就很难安全的fork。使用相同方式创建线程方便我们管理线程。main函数之前不应该启动线程，是因为会影响全局对象的安全构造。不要为了每个计算任务去请求创建线程是为了尽量避免thrashing，导致机器失去响应，这样我们不能探查机器在失去响应之后究竟在做什么，在程序运行期间不再创建和销毁线程，使得我们更容易把计算任务和IO任务分配给已有的线程，代价只有新建线程的几分之一。

线程的销毁有几种方式：

- 自然死亡，从线程主函数返回，线程正常退出；
- 非正常死亡，从线程主函数抛出异常或线程触发段错误等非法操作；
- 自杀，在线程中调用pthread_exit立刻退出线程；
- 他杀，其他线程调用pthread_cancel来强制终止某个线程；

只有第一种方式是合理的设计。如果确实需要强行终止一个耗时很长的计算任务，又不想在计算期间周期的检查某个全局退出标志，可以考虑把这部分代码fork为新的进程，这样杀一个进程比杀进程内线程要安全的多。当然fork的新进程与本进程的通信方式也要慎重，最好用文件描述符（pipe(2)/socketpair(2)/TCP socket）来收发数据，而不要用共享内存和跨进程的互斥器等IPC，因为这样仍然有死锁的可能。

posix threads有取消点这个概念，意思是线程执行到这里有可能被终止（如果别的线程对它调用了pthread_cancel的话）；在C++中，cancellation point实现与C不同，线程不是执行到此函数立刻终止，还是会抛出异常，这样有机会执行stack unwind，析构栈上对象。

exit(3)在C++中作用除了终止进程，还会析构全局对象和已经构造完的函数静态对象，这有潜在死锁的可能。比如：

```c++
void someFunctionMayCallExit()
{
	exit(1);
}
class GlobalObject // : boost::noncopyable
{
public:
    void doit()
    {
        MutexLockGuard lock(mutex_);
        someFunctionMayCallExit();
    }
    ～GlobalObject()
    {
        printf("GlobalObject:～GlobalObject\n");
        MutexLockGuard lock(mutex_); // 此处发生死锁
        // clean up
        printf("GlobalObject:～GlobalObject cleanning\n");
    }
private:
	MutexLock mutex_;
};

GlobalObject g_obj;
int main()
{
	g_obj.doit();
}
```

如果确实需要主动结束线程，可以考虑用_exit(2)系统调用，它不会试图析构全局对象，但是也不会执行其他任何清理工作，比如flush标准输出。

### 善用__thread关键字

__thread是gcc内置的线程局部存储设施，其实现非常高效，其存取效率可与全局变量相比。\_\_thread只能修饰POD类型（简单的旧数据，一个POD类型没有用户定义的构造、拷贝构造、析构和赋值运算符，没有虚函数和虚基类，所有的非静态成员都是POD类型或者是一个数组，没有类类型的非静态成员有默认构造函数），不能修饰class类型，因为无法自动调用构造和析构函数，可以修饰全局变量、函数内的静态变量，但是不能修饰函数的局部变量或者class的普通成员变量，另外，\_\_thread变量的初始化只能用编译期常量，例如：

```c++
__thread string t_obj1("Chen Shuo"); // 错误，不能调用对象的构造函数
__thread string* t_obj2 = new string; // 错误，初始化必须用编译期常量
__thread string* t_obj3 = NULL; // 正确，但是需要手工初始化并销毁对象

```

\_\_thread变量每个线程有一份独立实体，各个线程的变量值互不打扰，除此之外，其还可以修饰哪些“值可能会变，带有全局性，但又不值得用全局锁保护”的变量。

### 多线程与IO

可以用多线程处理同一个socket用以提高效率吗？首先，操作文件描述符的系统调用本身是线程安全的，不必担心多个线程同时操作造成进程崩溃或内核崩溃，但是多个线程操作同一个socket文件确实很麻烦，作者认为得不偿失。

作者推荐的原则是：每个文件描述符只由一个线程操作，从而轻松解决消息收发的顺序问题，也避免了关闭文件描述符的各种race condition。一个线程可以操作多个文件描述符，但一个线程不能操作别的线程拥有的文件描述符。

epoll也遵循相同的原则，同一个epoll fd的操作应该都放在同一个线程执行。

这条规则有两个例外：对于磁盘文件，必要的时候多个线程可以同时调用pread(2)/pwrite(2)来读写同一个文件；对于UDP，由于协议本身保证消息的原子性，在适当的条件下可以多个线程同时读写同一个UDP描述符。

### 用RAII包装文件描述符

linux中关于文件描述符，0是标准输入，1是标准输出，2是标准错误，如果新打开一个文件，其文件描述符会是3，因为posix标准要求每次新打开文件的时候必须使用当前最小可用的文件描述符号码，这可能造成串话，比如正在使用某个文件描述符，结果这个文件描述符突然关闭，另一个task打开了新的文件描述符，很可能当前操作使用了这个新的文件描述符。解决方法就是RAII，用Socket对象包装文件描述符。只要Socket对象还活着，就不会有其他Socket对象跟它有一样的文件描述符。

为了防止访问失效的对象或者发生网络串话，muduo使用shared_ptr来管理TcpConnection的生命周期。

### RAII与fork

我们可以用对象包装资源，把资源管理与对象生命周期管理统一起来，但是如果程序fork之后，这个假设就会被破坏。fork之后，子进程继承了父进程的几乎全部资源，但也有少数例外。子进程会继承地址空间和文件描述符，但不会继承：

- 父进程的内存锁，mlock(2)、mlockall(2)；
- 父进程的文件锁，fcntl(2)；
- 父进程的某些定时器，settimer(2)、alarm(2)、timer_create(2)等；
- 其他，见man 2 fork；

通常我们会用RAII手法管理以上种类的资源，但在fork出来的子进程中不一定正常工作，因为资源在fork时已经被释放。因此在编写服务端程序的时候是否允许fork是在一开始就应该慎重考虑的。

### 多线程与fork

fork一般不能在多线程程序中调用，因为linux的fork只克隆当前线程的thread of control，不克隆其他线程，fork之后除了当前线程之外其他线程都消失了。这样会造成一个危险的局面，其他线程正好位于临界区，持有某个锁，而它突然死亡，再也没机会解锁，如果子进程再试图对同一个mutex加锁就会立即死锁。在fork之后，子进程相当于处于signal handler中，不能调用线程安全的函数（除非是可重入的），而只能调用异步信号安全的函数，比如fork之后，子进程不能调用：

- malloc(3)，因为malloc在全局访问的状态下肯定会加锁；
- 任何可能分配或释放内存的函数，包括new、map::insert()、snprintf……
- 任何Pthreads函数，不能用pthread_cond_signal去通知父进程，只能通过pipe(2)来同步；
- printf系列函数，因为其他线程可能恰好持有stdout/stderr的锁；
- 除了man 7 signal中明确列出的signal安全函数之外的任何函数；

照此看来唯一安全的做法是fork之后立即调用exec执行另一个程序，彻底隔断父子进程的联系。不得不说，同样是创建进程，windows的CreateProcess函数的顾虑要少得多，因为它创建的进程跟当前进程关联较少。

### 多线程与signal

单线程时代，编写信号处理函数是一件棘手的事情，由于signal打断了正在运行的thread of control，在signal handler中只能调用async-signal-safe的函数，即所谓的可重入函数。还有一点，如果signal handler中需要修改全局数据，那么被修改的变量必须是sig_atomic_t类型的，否则被打断的函数在恢复执行后很可能不能立即看到signale handler改动后的数据，因为编译器有可能假定这个变量不会被他处修改从而优化了内存访问。

多线程时代，signal语义更加复杂，信号分为两类：发送给某一线程（SIGSEGV），发送给进程中的任一线程（SIGTERM），还有考虑掩码对信号的屏蔽。特别在signal handler中不能调用任何pthreads函数，不能通过condition variable来通知其他线程。在多线程程序中，使用signal的第一原则是不要使用signal，包括：

- 不要用signal作为IPC的手段，包括不要SIGUSR1等来触发服务端的行为，如果确实需要，可以用9.5介绍的增加监听端口的方式来实现双向的、可远程访问的进程控制；
- 也不要使用基于signal实现的定时函数，包括alarm/ualarm/setitimer/timer_create/sleep/usleep等；
- 不主动处理各种异常信号（SIGTERM、SIGINT等等），只用默认语义：结束进程。有一个例外：SIGPIPE，服务器程序常用的做法是忽略此信号，否则如果对方断开连接，而本机继续write的话会导致程序意外终止；
- 在没有别的替代方法的情况下（比如说处理SIGCHLD信号），把异步信号转换为同步的文件描述符时间，传统的做法是在signal handler里往一个特定的pipe(2)写一个字节，在主程序的从这个pipe读取，从而纳入统一的IO事件处理框架中去。现代linux的做法是采用signalfd(2)把信号直接转换为文件描述符事件，从而从根本上避免使用signal handler。

### linux新增系统调用的启示

大致从linux内核2.6.27起，凡是会创建文件描述符的syscall一般都增加了额外的flag参数，可以直接指定O_NONBLOCK和FD_CLOEXEC，例如accept4-2.6.28、eventfd2-2.6.27、inotify_init1-2.6.27、pipe2-2.6.27、signalfd4-2.6.27、timerfd_create-2.6.25。O_NONBLOCK的功能是开启非阻塞IO，而文件描述符默认是阻塞的。FD_CLOEXEC是让程序exec时，进程会自动关闭这个文件描述符，以防止该文件描述符被子进程继承。

## 高效的多线程日志

对于关键进程，日志通常要记录：

1. 收到的每条内部消息的id；
2. 收到的每条消息的全文；
3. 发出的每条消息的全文，每条消息都有全局唯一的id；
4. 关键内部状态的变更等等；

一个日志库大体可分为前端和后端，前端是供程序使用的接口，生成日志消息，后端则负责把日志消息写到目的地，这两部分的接口有可能简单到只有一个回调函数：`void output(const char* message, int len);`   其中message是一条完整的日志消息，包含日志级别、时间戳、源文件位置、线程id等基本字段以及消息的具体内容。在多线程程序中，写日志是个典型的多生产者单消费者问题，对生产者（前端）来说，要尽量做到低延迟、低cpu开销、无阻塞，对消费者（后端）而言，要做到足够大的吞吐量，并占用较少资源。对C++而言，最好整个程序都使用相同的日志库，从这个意义来说，日志库是个单例，C++日志库的前端大体有两种API风格：

- C/Java的printf(fmt, ...)风格；
- C++的stream<<风格；

muduo日志库是C++ stream风格，其不必费心保持格式字符串与参数类型的一致性，随用随写，而且类型安全。另一个好处是当输出日志的基本高于语句的日志级别时，打印日志是个空操作。

往文件写日志的一个常见问题是万一程序崩溃，最后若干条日志往往丢失，因为日志库不能每条消息都flush硬盘，更不能每条消息都open/close文件，这样性能开销太大，muduo日志库用两个方法来应对，其一是定期（默认3s）将缓冲区消息flush到硬盘，其二是每条内存中的日志消息都带有cookie，其值为某个函数的地址，这样通过来core dump文件中查找cookie就能找到尚未来得及写入到磁盘的消息。

日志消息格式有几个要点：

- 尽量每条日志占一行，容易用awk、sed、grep等命令行等工具快速联机分析日志；
- 时间戳精确到微秒，每条消息通过gettimeofday(2)获取当前时间不会有什么性能损失，因为在linux中gettimeofday(2)不是系统调用不会陷入内核；
- 始终使用GMT时区，省去了时区的转换；
- 打印线程id；
- 打印日志级别；
- 打印源文件名和行号；

### 性能需求

日志库只有高效，才能让程序员敢于在代码中输出足够多的诊断信息，减小运维难度，提高效率，高效性体现在以下几个方面：

- 每秒写几千上万条日志的时候没有明显的性能损失；
- 能应对一个进程产生大量日志的场景；
- 不阻塞正常的执行流程；
- 在多线程中，不造成争用；

作者列了一些具体的性能指标，考虑往普通7200rpm SATA硬盘写日志文件的情况：

- 磁盘带宽约为110MB/s，日志库应该能瞬间写完带宽；
- 假如每条消息平均长度为110字节，意味着1s要写100万条日志；

muduo日志库的优化措施比较突出的是：

- 时间戳字符串中的日期和时间两部分是缓存的，一秒之内多条日志只需要重新格式化微秒部分；
- 日志消息的前4个字段是定长的，可以避免在运行期求字符串长度；
- 线程id是预先格式化为字符串，在输出日志消息的时候只需要简单拷贝几个字节；
- 每行日志消息的源文件名部分采用了编译期计算来获得basename；

### 多线程异步日志

作者认为多线程程序最好只写一个日志文件，用一个背景线程负责收集日志消息并写入文件，其他业务线程只管往这个日志线程发送日志消息，这称为异步日志。需要一个队列将日志前端的数据传送到后端，但这个队列不必是现成的BlockingQueue\<std::string\>，因为不用每产生一条日志消息都通知接收方。

muduo日志库采用的是双缓冲技术，即准备两块bufferA和B，前端负责往A里面填数据，后端负责将B中的数据写入到文件，当A写满后交换A和B的数据，让后端将A中的数据写入，这样的好处是批处理数据，减少了线程唤醒的频率降低了开销。另外为了及时将消息写入文件，即使A未满，日志库也会每3s执行一次上述交换写入操作。

## muduo网络库简介

### 使用教程

作者认为TCP网络编程最本质的是处理三个半事件：

1. 连接的建立，包括服务端接收新连接（accept）和客户端成功发起连接（connect）；
2. 连接的断开，包括主动断开（close、shutdown）和被动断开（read(2)返回0）；
3. 消息到达，文件描述符可读，这是最为重要的一个事件，对它的处理方式决定了网络编程的风格（阻塞还是非阻塞，如何处理分包，应用层的缓冲如何设计等待）；
4. 消息发送完毕，这算半个，对于低流量的服务可以不必关心这个事件，另外这里的发送完毕指的是将数据写入操作系统的缓冲区，将由TCP协议栈负责数据的发送和重传，不代表对方已经收到数据；

## muduo编程示例

### 五个简单的TCP示例

本节介绍五个简单的TCP网络服务程序，包括echo、discard、chargen、daytime、time这五个协议，以及time协议的客户端，各程序的协议简介如下：

- discard：丢失所有收到的数据；
- daytime：服务端连接后，以字符串形式发送当前时间，然后主动断开连接；
- time：服务端accept连接之后，以二进制的形式发送当前时间（从Epoch到现在的秒数），然后主动断开连接，我们需要一个客户程序把收到的时间转换为字符串；
- echo：回显服务，把收到的数据发回客户端；
- chargen：服务端accept连接之后不停发送测试数据；

这五个协议使用不同的端口，可以放在同一个进程中实现，且不必使用多线程。

discard可以算最简单的长连接TCP应用层协议，只需要关注消息/数据到达时间，事件处理函数如下：

```c++
void DiscardServer::onMessage(const TcpConnectionPtr& conn,
Buffer* buf,
Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
    << " bytes received at " << time.toString();
}
```

daytime是短连接协议，在发送完当前时间后由服务端主动断开连接：

```c++
void DaytimeServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "DaytimeServer - " << conn->peerAddress().toIpPort() << " -> "
    << conn->localAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected())
    {
        conn->send(Timestamp::now().toFormattedString() + "\n");
        conn->shutdown();
    }
}

```

time与daytime类似，但它返回的是一个32bit的整数：

```c++
void TimeServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    LOG_INFO << "TimeServer - " << conn->peerAddress().toIpPort() << " -> "
    << conn->localAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected())
    {
        time_t now = ::time(NULL);
        int32_t be32 = sockets::hostToNetwork32(static_cast<int32_t>(now));
        conn->send(&be32, sizeof be32);
        conn->shutdown();
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer＊ buf, Timestamp receiveTime)
{
    if (buf->readableBytes() >= sizeof(int32_t))
    {
        const void＊ data = buf->peek();
        int32_t be32 = ＊static_cast<const int32_t＊>(data);
        buf->retrieve(sizeof(int32_t));
        time_t time = sockets::networkToHost32(be32);
        Timestamp ts(time ＊ Timestamp::kMicroSecondsPerSecond);
        LOG_INFO << "Server time = " << time << ", " << ts.toFormattedString();
    }
    else
    {
        LOG_INFO << conn->name() << " no enough data " << buf->readableBytes()
        << " at " << receiveTime.toFormattedString();
    }
}
```

echo是一个双向的协议：

```c++
void EchoServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
muduo::net::Buffer＊ buf,
muduo::Timestamp time)
{
    muduo::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
    << "data received at " << time.toString();
    conn->send(msg);
}

```

这里有一点数据就发送，是为了避免客户端恶意不发送换行符，导致服务端一直缓存数据导致内存暴涨。但这里其实还有一个漏洞，那就是客户端故意不断发送数据但从不接收，会导致服务端的发送缓冲区一直堆积。解决方法可以参考chargen协议，或者在发送缓冲区堆积到一定大小时主动断开连接。

charge协议只发送数据不接收数据，且发送数据的速度不能快过客户端接收的速度：

```c++
void ChargenServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "ChargenServer - " << conn->peerAddress().toIpPort() << " -> "
    << conn->localAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected())
    {
        conn->setTcpNoDelay(true);
        conn->send(message_);
    }
}

void ChargenServer::onMessage(const TcpConnectionPtr& conn,
Buffer＊ buf,
Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
    << " bytes received at " << time.toString();
}

void ChargenServer::onWriteComplete(const TcpConnectionPtr& conn)
{
    transferred_ += message_.size();
    conn->send(message_);
}
```

### Boost.Asio的聊天服务器

分包指的是在发送一个消息或一帧数据时通过一定的处理，让接收方能从字节流中识别并截取出一个个消息。对于短连接的TCP服务，分包不是一个问题，只要发送方主动关闭连接，就表示一条消息发送完毕，接收方read返回0从而知道消息的结尾。对于长连接的TCP服务，分包有四种方法：

1. 消息长度固定，比如muduo的roundtrip示例就采用了固定的16字节消息；
2. 使用特殊的字符或字符串作为消息的边界，例如http协议的headers以及\r\n为字段的分隔符；
3. 在每条消息的头部加一个长度字段，这恐怕是最常见的做法；
4. 利用消息本身的格式来分包，例如XML格式中的<root>...</root>配对；

本节介绍的聊天服务的消息格式非常简单，消息本身是一个字符串，每条消息有一个4字节的头部，以网络序存放字符串的长度，消息之间没有间隙，字符串也不要求以"\0"结尾。打包的代码如下：

```c++
void send(muduo::net::TcpConnection＊ conn,
const muduo::StringPiece& message)
{
    muduo::net::Buffer buf;
    buf.append(message.data(), message.size());
    int32_t len = static_cast<int32_t>(message.size());
    int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    buf.prepend(&be32, sizeof be32);
    conn->send(&buf);
}
```

分包的代码如下所示：

```c++
void onMessage(const muduo::net::TcpConnectionPtr& conn,
muduo::net::Buffer* buf,
muduo::Timestamp receiveTime)
{
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
        // FIXME: use Buffer::peekInt32()
        const void* data = buf->peek();
        int32_t be32 = *static_cast<const int32_t＊>(data); // SIGBUS
        const int32_t len = muduo::net::sockets::networkToHost32(be32);
        if (len > 65536 || len < 0)
        {
            LOG_ERROR << "Invalid length " << len;
            conn->shutdown(); // FIXME: disable reading
            break;
        }
        else if (buf->readableBytes() >= len + kHeaderLen)
        {
            buf->retrieve(kHeaderLen);
            muduo::string message(buf->peek(), len);
            messageCallback_(conn, message, receiveTime);
            buf->retrieve(len);
        }
        else
        {
        	break;
        }
    }
}
```

服务端注册回调：

```c++
class ChatServer : boost::noncopyable
{
public:
    ChatServer(EventLoop＊ loop,
    const InetAddress& listenAddr)
    : loop_(loop),
    server_(loop, listenAddr, "ChatServer"),
    codec_(boost::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
    {
        server_.setConnectionCallback(
        boost::bind(&ChatServer::onConnection, this, _1));
        server_.setMessageCallback(
        boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }

    void start()
    {
    	server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr& conn)
     {
         LOG_INFO << conn->localAddress().toIpPort() << " -> "
         << conn->peerAddress().toIpPort() << " is "
         << (conn->connected() ? "UP" : "DOWN");

         if (conn->connected())
         {
         	connections_.insert(conn);
         }
         else
         {
         	connections_.erase(conn);
         }
    }
    void onStringMessage(const TcpConnectionPtr&,
         const string& message,
         Timestamp)
     {
         for (ConnectionList::iterator it = connections_.begin();
         it != connections_.end();
         ++it)
         {
         	codec_.send(get_pointer(＊it), message);
         }
     }
     typedef std::set<TcpConnectionPtr> ConnectionList;
     EventLoop＊ loop_;
     TcpServer server_;
     LengthHeaderCodec codec_;
     ConnectionList connections_;
 };
```

### muduo Buffer类的设计和使用

event loop是non-blocking网络编程的核心，在现实生活中，non-blocking几乎总是和IO multiplexing一起使用，原因有两点：

- 没人会真的用轮询来检查某个non-blocking IO操作是否完成，那样太浪费CPU；
- IO multiplexing一般不能和blocking IO用在一起，因为blocking IO中的read/write/accept/connect都有可能阻塞当前线程。

non-blocking IO的核心思想是避免阻塞在read或write或其他IO系统调用上，这样可以最大限度复用thread-of-control，让一个线程可以服务于多个socket连接。IO线程只能阻塞在IO multiplexing函数上，如select/poll/epoll_wait，这样一来，应用层的缓冲是必需的，每个TCP socket都要有stateful的input buffer和output buffer。首先TcpConncetion必须要有output buffer，这是因为数据可能不能一下子发完，比如想要发送100k数据，但在write系统调用中系统只接受了80k（受TCP advertised window控制），那么我们不想要等待，就可以将剩余的数据保存在output buffer中，然后注册POLLOUT事件，一旦socket变得可写，就立即发送数据。如果写完了这剩余的20k，网络库应该立即停止关注POLLOUT，以免造成busy loop。如果程序又写入了50k，这时候output buffer中还有待发送的20k数据，那么网络库不应该直接调用write，而应该把这50k数据append在那20k数据之后，等socket变得可写时候一并写入。如果output buffer还有数据，但程序又想关闭连接，那么必须要等到数据发送完毕。

TcpConnection必须要有input buffer，这是因为TCP是一个无边界的字节流协议，接收方必须要对消息进行分包处理，而网络库在处理socket可读事件的时候必须一次性把socket里的数据读完，否则会反复触发POLLIN事件，造成busy-loop。所以为了应付网络不完整的情况，收到的数据通常先放到input buffer，等构成一条完整的消息再通知程序的业务逻辑。

muduo Buffer的设计要点如下：

- 对外表现为一块连续的内存以方便客户代码的编写；
- size自动增长以适应不同长度的消息；
- 内部以vector<char>来保存数据，并提供相应的访问程序；

Buffer其实像一个queue，从末尾写入数据，从头部读出数据。

在非阻塞网络编程中，如何设计并使用缓冲区，muduo的做法是在栈上准备一个65536字节的extrabuf，然后利用readv来读取数据，iovec有两块，第一块指向muduo buffer中的writable字节，另一块指向栈上的extrabuf，这样如果读取的数据不多那么就全部读取到buffer中，如果长度超过buffer的writable字节数，就会读到栈上的extrabuf里，然后再把extrabuf里的数据append到buffer中。

Buffer的内部是一个std::vector<char>，其又两个数据成员指向该vector中的元素，这两个index的类型是int，目的是应对迭代器失效。buffer的结构如图所示：

![buffer](..\image\Network\buffer.png)

Buffer里面有两个常数kCheapPrepend和kInitialSize，定义了prependable的初始大小和writable的初始大小，readable的初始大小为0。

有时候经过若干次读写，readIndex移动的位置靠后，此时我们想要写入300字节可能writable不够，muduo在这种情况下不会分配新的内存，而是把已有的数据移到前面挪出空间。

prependable空间是为了让程序以很低的代价在数据前面添加几个字节。

如果不想用vector，可以自己管理内存，如下：

![muduo-7-23](..\image\Network\muduo-7-23.png)

如果对性能有着极高的要求，受不了copy和resize，可以考虑实现分段连续的zero copy buffer再配合gather scatter IO：

![muduo-7-24](..\image\Network\muduo-7-24.png)

### 限制服务器的最大并发连接数

为什么要限制并发连接数，一方面是不希望服务程序超载，另一方面是因为file descriptor是稀缺资源。

对于可能出现文件描述符耗尽这种情况，muduo的acceptor的做法是准备一个空闲的文件描述符，遇到耗尽时先关闭这个空闲文件获得一个文件描述符，accept拿到新socket连接的描述符，随后立刻close它，这样就优雅的断开了客户端连接，最后重新打开一个空闲文件把坑占住。另外还有一种比较简单的办法就是施加一个限制，比如说当前进程最大文件描述符是1024个，那么我们可以在连接数达到1000的时候进入拒绝新连接状态。

### 定时器

linux的计时函数，可用于获取当前时间：

- time(2)/timt_t(s)；
- ftime(3)/struct timeb(ms);
- gettimeofday(2)/struct timeval(us);
- clock_gettime(2)/struct timespec(ns);

还有gmtime/localtime/timegm/mktime/strftime/struct tm等与当前时间无关的时间格式转换函数；

定时函数有以下的几种：sleep(3)、alarm(2)、usleep(3)、nanosleep(2)、clock_nanosleep(2)、getitimer(2)/setitimer(2)、 timer_create(2) / timer_settime(2) / timer_gettime(2) / timer_delete(2)、timerfd_create(2) / timerfd_gettime(2) / timerfd_settime(2)；

作者建议计时只使用gettimeofday(2)来获取当前时间，定时只使用timerfd_*系列函数来处理定时任务。主要是因为time(2)的精度太低，ftime(3)已经被废弃，clock_gettime(2)精度最高，但其系统调用开销比gettimeofday(2)大，另外在x86-64平台上，gettimeofday(2)不是系统调用，而是在用户态实现，因此没有上下文切换和陷入内核的开销，gettimeofday(2)的分辨率是1us，足以满足需要。定时函数入选的原因是sleep(3)/alarm(2)/usleep(3)在实现时有可能用了SIGALRM信号，在多线程中处理信号相当麻烦，nanosleep(2)和clock_nanosleep(2)是线程安全的，但是在非阻塞网络编程中，不能让线程挂起来等待一段时间，这样会失去响应，正确的做法是注册一个时间回调函数，getitiemr(2)和timer_create(2)也是用信号来deliver超时，虽然timer_create可以指定信号的接收方是进程还是线程，但信号处理函数能做的事情还是很有限。而timerfd_create(2)把时间变成了一个文件描述符，该文件在定时器超时的那一刻变的可读，可以很方便的融入到select(2)/poll(2)框架中，传统的reactor使用select/poll/epoll的timeout来实现定时功能，但poll和epoll_wait的定时精度只有ms，远低于timerfd_settime的定时精度。

### 测量两台机器的网络延迟和时间差

两台机器之间的网络延迟，即往返时间（RTT）。

### 用timing wheel踢掉空闲连接

在严肃的网络程序中，应用层的心跳协议必不可少，应该用心跳消息来判断对方进程能否正常工作。如果一个连接连续几秒内没有收到数据，就把它断开，为此有两种简单粗暴的做法：

- 每个连接保存最后收到数据的时间lastReceiveTime，然后用一个定时器每秒遍历一遍所有连接，断开那些大于8s的。这种做法全局只有一个repeated timer，不过每次timeout都有检查全部连接，如果连接数目比较多，则比较费时；
- 每个连接设置一个one-shot timer，超时定为8s，超时就断开连接，每次收到数据则更新timer，这种做法则需要很多个one-shot timer，会频繁更新timer；

使用timing wheel能避免以上的缺点，这里使用了一个简单的结构：8个桶组成的循环队列，第一个桶放1s后将要超时的连接，第二个桶放2s后将要超时的连接。每个连接一收到数据就把自己放到第8个桶，然后在每秒的timer里把第一个桶里的连接断开，把这个空桶挪到队尾。

timing wheel中每个格子是个hash set，可以容纳不止一个连接。

## 分布式系统工程实践

### 分布式系统的可靠性浅说

如何优雅的重启：

- 主动停止一个服务进程的心跳，对于短连接，关闭listen port，不会有新请求到达，对于长连接，客户会主动failover到备用地址或其他或者的服务端；
- 等一段时间，直到该服务进程没有活动的请求；
- kill并重启进程；
- 检查新进程的服务正常与否；
- 依次重启服务端剩余进程，可避免终端服务；

### 分布式系统中心跳协议的设计

使用TCP连接作为分布式系统进程间通信的唯一方式，其好处之一是任何一方进程意外退出的时候对方能及时得到连接断开的通知，因为操作系统会关闭进程中使用的TCP socket，会往对方发送FIN分节。尽管如此，应用层心跳还是必不可少的，原因有：

- 如果操作系统崩溃导致机器重启，没有机会发送FIN分节；
- 服务器硬件故障导致机器重启，也没有机会发送FIN分节；
- 并发连接数很高时，系统或进程如果重启，很可能没有机会断开全部连接，即FIN分节可能出现丢包，此时没机会重试；
- 网络故障，连接双方得知这一情况的唯一方案是检测心跳超时；

心跳协议的基本形式是如果进程C依赖S，那么S应该按周期向C发送心跳，而C按固定周期检查心跳。心跳检查也比较简单，即比较当前时间与最后一次收到心跳消息的时间，如果超过某个timeout值则判断对方心跳失效。

通常Sender的发送周期与Reciever的检查周期相同，均为Tc，而timeout>Tc，为了避免误报，通常可取timeout为2Tc。心跳消息应该包含发送方的标识符，可按9.4节的方式确定分布式每个进程的唯一标识符，建议包含当前负载，以便客户端做负载均衡。如果Sender和Receiver之间有其他消息中转进程，那么还应该在心跳消息中加上Sender的发送时间，防止假心跳，判断规则改为如果最近的心跳消息的发送时间早有now-2Tc，心跳失效，使用这种方式，两台机器的时间都应该通过NTP协议与时间服务器同步。NTP协议如图：

![ntp](..\image\Network\ntp.png)

其中往返延迟时间为(t4-t1)-(t3-t2)，则客户端和服务端的时间差offset为((t2-t1)+(t3-t4))/2。

心跳协议还有两个实现上的关键点：

1. 要在工作线程发送，不要单独起一个心跳线程；
2. 与业务消息用同一个连接，不要单独用心跳连接；

第一点主要是为了防止工作线程死锁或阻塞时还在继续发心跳。第二点主要是由于心跳消息的作用之一就是验证网络畅通，不再同一连接就没有这个作用了。

### 分布式系统中的进程标识

ip:port或者host:pid都可能有进程重启的问题，正确做法是以四元组ip:port:start_time:pid作为分布式系统进程的gpid。

### 为系统演化做准备

采用可扩展的消息格式，建议使用某种中间语言来描述消息格式，然后生成不同语言的解析与打包代码，如果用文本格式可以考虑json或xml，如果用二进制可以考虑google protocol buffers。

## C++编译链接模型精要

### C语言编译模型及其成因

为了能在尽量减少内存使用的情况下实现分离编译，C语言采用了隐式函数声明的做法，代码在使用前文未定义的函数时，编译期不需要也不检查函数原型：既不检查参数个数，也不检查参数类型和返回值类型，编译期认为未声明的函数都返回int，并且能接受任意个数的int型参数。而且早期的C语言甚至不严格区分指针和int，认为二者可以相互赋值转换。如果C用了某个未定义的函数，那么实际造成的是链接错误，而不是编译错误。

由于不能将整个源文件的语法树保存在内存中，C语言其实是按单遍编译来设计的，编译器只能看到目前已经解析过的代码，看不到之后的代码，而且过眼即忘，这意味着：

- C语言要求结构体必须先定义才能访问其成员，否则编译器不知道结构体成员的类型和偏移量，无法立刻生成目标代码；
- 局部变量必须先定义再使用，如果把定义放在后面，编译器看到一个局部变量不知道它的类型和在stack中的位置，也就无法立刻生成代码；
- 为了方便编译器分配stack空间，C语言要求局部变量只能在语句块的开始处定义；
- 对于外部变量，编译器只需要知道它的类型和名字，不需要知道它的地址，因为需要先声明后使用，在生成的目标代码中，外部变量的地址是个空白，留给链接器去填上；
- 编译器看到一个函数调用时，按隐式函数声明规则，编译器可以立刻生成调用函数的汇编代码（参数入栈、调用、获取返回值），这里唯一不能确定的是函数的实际地址，编译器可以留下一个空白给链接器去填；

### C++的编译模型

C++为了和C兼容，继承了单遍编译，这特别影响了名字查找和函数重载决议。

对于类，有几种情况不需要看见其完整定义：

- 定义或声明Foo*或者Foo&，包括用于函数参数、返回类型、局部变量、类成员变量等等，这是因为C++的内存模型是flat的，Foo的定义无法改变Foo的指针或引用的含义；
- 声明一个以Foo为参数或返回类型的返回，但如果代码里调用这个函数就需要知道Foo的含义，因为编译器要使用Foo的拷贝构造或析构函数，因此至少要看到他们的声明；

muduo代码大量使用了前向声明来减少include，并且避免把内部class的定义暴露给用户代码。

### C++链接

函数重载时返回类型不参与名字改编，因此要注意这样的情景，一个源文件用到了重载函数，但它看到的函数原型的返回类型是错的。

现在C++编译器采用重复代码消除的方法来避免重复定义，如果编译器无法inline展开的话，每个编译单元都会生成inline函数的目标代码，然后链接器会从多份实现中任选择一份保留，其余的丢弃，如果能够展开inline函数，则不必单独为之生成目标代码（除非使用函数指针指向它，因为指针的指向只有运行时才能确定）。

如何判断一个C++可执行文件是debug build还是release build，即如何判断一个可执行文件是-O0编译还是-O2编译，可以通过看类模板的短成员函数有没有被inline展开。比如判断inline函数size，直接运行：

```shell
nm ./out | grep size | c++filt
```

如果出现了函数弱定义，则说明没有inline展开。

对于模板来说，一般都是把定义直接放在头文件，否则链接器会报错，那么有办法只在头文件放声明吗，其实可以，只要知道模板会有哪些具现化类型，并事先显式或隐式具现化出来。另外11新增了extern template特性，可以阻止隐式模板具现化。

### 工程项目中头文件的使用规则

这里介绍一个查找头文件包含路径的小技巧，假设一个程序包含了<iostream>却能使用string，想知道string如何被引入的，方法是在当前路径创建一个string文件，然后制造编译错误，步骤如下：

```c++
// hello.cc
#include <iostream>
int main()
{
std::string s = "muduo"; // 奇怪，明明没有包含 <string> 却能使用 std::string
}
$ cat > string // 创建一个只有一行内容的 string 文件
#error error
^D
$ g++ -M -I . hello.cc // 用 g++ 帮我们查出包含途径，原来是 locale_classes.h 干的
In file included from /usr/include/c++/4.4/bits/locale_classes.h:42,
from /usr/include/c++/4.4/bits/ios_base.h:43,
from /usr/include/c++/4.4/ios:43,
from /usr/include/c++/4.4/ostream:40,
from /usr/include/c++/4.4/iostream:40,
from hello.cc:1:
./string:1:2: error: #error error

```

-M选项告诉编译器输出依赖关系，-I告诉编译器在当前目录查找文件

## 反思C++面向对象与虚函数

### 程序的二进制兼容性

如果以shared library方式提供给函数库，那么头文件和库文件不能轻易修改，否则容易破坏已有的二进制可执行文件，或者其他用到这个库的库。本章所指的二进制兼容是在升级库文件的时候不必重新编译使用了这个库的可执行文件或其他库文件。C++编译器的ABI主要内容包括以下几个方面：

- 函数参数传递的方式；
- 虚函数的调用方式，通常是vptr/vtbl机制，然后用vtbl[offset]来调用；
- struct和class的内存布局，通过偏移量来访问数据成员；
- name mangling；
- RTTI和异常处理的实现；

哪些做法是安全的呢：

- 增加新的class；
- 增加non-virtual成员函数或static成员函数；
- 修改数据成员的名称；
- 等等；

解决办法我们可以使用静态链接，这里指完全从源码编译出可执行文件。也可以通过动态库的版本管理来控制兼容性。也可以用pimpl技法。

### 避免使用虚函数作为库的接口

使用虚函数作为库的接口会给保持二进制兼容性带来很大麻烦。作为一个库的作者，要考虑可扩展性和易用性，因此最好以动态库的形式发布。如果对外暴露的接口以虚函数的形式发布，可能会造成ABI的兼容性，为了防止出错，可以将新增的接口放在最下面，但有些丑陋，也可以通过链式继承来扩展接口，但也比较丑陋，还可以通过多重继承来扩展现有的接口，即让实现同时继承这两个接口类，但也很丑。

动态库的接口的推荐做法，如果动态库的使用范围比较窄，只要做好版本的管理就好了，如果使用范围非常广，那么推荐pimpl技法。首先暴露的接口里面不要有虚函数，要显式声明析构函数、构造函数，并且不能inline（一是防止在头文件暴露细节，二是改动会引起全部重新编译）。其次是在库的实现中把调用转发给实现。如果要加入新功能，可以原地修改。

### 以function和bind取代虚函数

### iostream的用途和局限

局限主要有以下方面：

- 输入方面，istream不适合输入带格式的数据；
- 输出方面，ostream的格式化输出非常繁琐，而且写死在代码里，不如stdio灵活，建议只做简单的无格式输出；
- log方面，由于ostream没有办法在多线程程序中保证一行输出的完整性，建议不要直接用来写log；
- In-memory格式化方面，由于ostringstream会动态分配内存，不适合性能要求较高的场合；
- 文件IO跟性能都不强；

### 值语义与数据抽象

值语义指的是对象的拷贝与原对象无关，对象语义指的是面向对象意义下的对象，对象拷贝是禁止的。值语义的生命周期好管理。

## C++经验谈

### 用异或来交换变量是错误的

比如如下的代码：

```c++
void reverse_by_xor(char＊ str, int n)
{
    // WARNING: BAD code
    char* begin = str;
    char* end = str + n - 1;
    while (begin < end)
    {
    *begin ^= *end;
    *end ^= *begin;
    *begin ^= *end;
    ++begin;
    --end;
    }
}

```

这种做法既丑又会劣化代码。