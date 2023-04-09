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