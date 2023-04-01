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

