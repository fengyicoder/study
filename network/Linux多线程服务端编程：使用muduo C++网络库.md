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

shared_ptr的引用计数本身安全无锁，但对象的读写则不是，因为shared_ptr有两个数据成员，读写操作不能原子化。