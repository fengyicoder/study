注意：本书出版的年代有些早，因此有些内容已经过时，因此只是起到参考的作用。

## 条款1：仔细选择你的容器

stl的指导方案：默认可以使用vector，当很频繁的在序列中部进行插入和删除时应该使用list，当大部分插入和删除发生在序列的头或尾时可以选择deque。

标准的连续内存容器是vector、string和deque，非标准的rope也是连续内存容器。表现为链表的容器例如list和slist是基于节点的，所有的标准关联容器也是（它们的典型实现是平衡树）。有以下的指导建议：

- 是否需要在容器的任意位置插入新元素的能力，若是，选择序列容器，关联容器做不到；
- 关心元素在容器的顺序吗？若不关心，散列容器可选择，否则要避免使用；
- 必须使用标准C++容器吗？若是除去散列容器、slist和rope；
- 需要哪类的迭代器，若是随机访问迭代器只能选择vector、deque和string，也可考虑rope，若是双向迭代器，那就用不了slist和散列容器的一般实现；
- 插入或删除数据时是否在意容器内现有元素的移动，若是，就必须放弃连续内存容器；
- 容器中的数据内存布局是否要兼容C，若是就只能用vector；
- 查找速度重要吗？若是应该按以下的顺序选择，散列容器、排序的vector和标准关联容器；
- 介意容器使用引用计数吗？若是则避免使用string和rope；
- 需要插入和删除的事务性语义吗？即能够回退插入和删除，若是就应该使用基于节点的容器，如果需要多元素插入的事务性语义，应该选择list；
- 需要把迭代器、指针和引用的失效次数减少到最少吗？若是应该使用基于节点的容器；
- 需要可以使用随机访问迭代器，只要没有删除且插入只发生在容器结尾，指针和引入的数据不会失效的容器，那么选择deque（插入发生在容器结尾时，deque的迭代器也可能失效，但迭代器失效不会使它的指针和引用失效）。

## 条款2：小心对”容器无关代码“的幻想

stl是建立在泛化的基础之上，数组泛化为容器，参数化了所包含的对象的类型，函数泛化为算法，参数化了所有迭代器的类型，指针泛化成了迭代器，参数化了所指向的对象的类型；独立的容器类型泛化为序列或关联容器，类似的容器都拥有类似的功能，例如标准的内存相邻容器都提供随机访问迭代器，标准的基于节点的容器都提供双向迭代器，序列容器支持push_front或push_back，但关联容器不支持，关联容器提供对数时间复杂度的Lower_bound、upper_bound和equal_range成员函数但序列容器没有。

虽然泛化很方便，但不建议脱离容器去泛化，比如现在可以用vector，但以后可以用deque或list等，因为每种容器的实现机制都不一样。虽然我们一次次的改变容器，但仍旧有简化的方法，其中最简单的是对容器和迭代器类型使用typedef，不要这么写：

```c++
class Widget{...};
vector<Widget> vw;
Widget bestWidget;
...
vector<Widget>::iterator i = find(vw.begin(), vw.end(), bestWidget);
```

而可以这样修改：

```c++
class Widget{...};
typedef vector<Widget> WidgetContainer;
typedef WidgetContainer::iterator WCIterator;
WidgetContainer cw;
Widget bestWidget;
...
WCIterator i = find(cw.begin(), cw.end(), bestWidget);
```

现在我们想要改变容器类型就简单许多。

typedef只是其他类型的同义字，所有其提供的封装是纯的语法（不要#define是在预编译阶段进行替换）。如果不想暴露所使用的容器类型，可以使用class增加私密性。比如我们建立一个CustomerList类，将list隐藏在它的private区域：

```c++
class CustomerList {
private:
    typedef list<Customer> CustomerContainer;
    typedef CustomerContainer::iterator CCIterator;
    CustomerContainer customers;
public: // 通过这个接口
    ... // 限制list特殊信息的可见性
};
```

而且这样操作带来的好处就是封装性很好，假设我们需要更换容器，那么客户基本没啥影响。

## 条款3：使容器里对象的拷贝操作轻量而正确

我们在容器里的存取操作，其实是一种拷进来拷出去的操作。然而，如果元素非常大，那么拷贝就会浪费性能，这还是其次，如果涉及到继承，例如把派生类对象插入基类对象的容器中，很可能会出错。

一种避免以上问题的方法是容器中存储指针，但指针的容器也有它们自己STL相关的头疼问题，因此最好选用只智能指针。

## 条款4：用empty来代替检查size()是否为0

事实上empty的典型实现是一个返回size是否为0的内联函数，那么为什么推荐使用empty呢，这是因为对于所有的标准容器，empty是一个常数时间的操作，但对于一些list实现，size花费线性时间。注意，对于所有标准容器，只有list提供了不用拷贝数据就能把元素从一个地方接合到另一个地方的能力。

## 条款5：尽量使用区间成员函数代替它们的单元素兄弟

无论何时当必须完全代替一个容器的内容时都应该想到赋值，不要忘记assign。例如如下的实现就较为的高效：

```c++
v1.assign(v2.begin() + v2.size() / 2, v2.end());
```

另外，几乎所有目标区间是通过插入迭代器指定的copy的使用都可以通过调用区间的成员函数来替代，例如：

```c++
v1.clear();
copy(v2.begin() + v2.size() / 2, v2.end(), back_inserter(v1));
//insert替代
v1.insert(v1.end(), v2.begin()+v2.size()/2, v2.end());
```

至于本条准则的原因，有以下的描述：

- 一般来说使用区间成员函数可以输入更少的代码；
- 区间成员函数会导致代码更清晰直接；

另外，还有效率的考量，比如当处理标准序列容器时，应用单元素成员函数比完成同样目的的成员函数需要更多的内存分配，更频繁的拷贝，或造成多余的操作。

一般的成员区间函数有区间构造，区间插入insert，区间删除erase，区间赋值assign。

## 条款6：警惕C++最令人恼怒的解析

例如这样的声明：

```
list<int> data(istream_iterator<int>(dataFile), istream_iteration<int>());

```

一般来说，参数名左右的括号被忽略，但单独的括号指出存在一个参数列表，例如上面第一个参数就是dataFile，而第二个参数就是一个输入参数为空，输出类型为istream_iteration<int>的函数指针。显然第一个参数并不是我们想要的。我们知道，用括号包围一个实参的声明是不合法的，但用括号包围一个函数调用是合法的，因此可以如下修改：

```c++
list<int> data((istream_iterator<int>(dataFile)), // 注意在list构造函数
istream_iterator<int>()); // 的第一个实参左右的
// 新括号
```

这样会强迫编译器将其理解成函数调用，也就达到了我们的目的，但并非所有的编译器都吃这一套，所以我们可以如下的修改：

```c++
ifstream dataFile("ints.dat");
istream_iterator<int> dataBegin(dataFile);
istream_iterator<int> dataEnd;
list<int> data(dataBegin, dataEnd);
```

## 条款7：当使用new得指针的容器时，记得在销毁容器前delete那些指针

当容器存储了元素的指针时，直接释放容器必然会导致内存泄漏，因为指针的“析构函数”是误操作的，其必然不会调用delete。可以如下操作：

```c++
template<typename T>
struct DeleteObject : // 条款40描述了为什么
public unary_function<const T*, void> { // 这里有这个继承
    void operator()(const T* ptr) const
    {
    	delete ptr;
    }
};
```

现在可以如下处理：

```c++
void doSomething()
{
... // 同上
    for_each(vwp.begin(), vwp.end(), DeleteObject<Widget>);
}
```

但这样暴露了我们要删除的是widget类型，一旦弄错了就会出现bug。例如我们知道，所有标准STL容器都确实虚析构函数，而从没有虚析构函数的类公有继承是C++的一大禁忌，如果我们有需要声明了SpecialString继承了string，那么下面的代码就会有问题：

```c++
void doSomething()
{
    deque<SpecialString*> dssp;
    ...
    for_each(dssp.begin(), dssp.end(), // 行为未定义！通过没有
    DeleteObject<string>()); // 虚析构函数的基类
} // 指针来删除派生对象
```

我们可以通过编译器推断传给DeleteObject::operator()的指针的类型来消除这个错误，例如：

```c++
struct DeleteObject { // 删除这里的
    // 模板化和基类
    template<typename T> // 模板化加在这里
    void operator()(const T* ptr) const
    {
    	delete ptr;
    }
}
```

但这仍旧不是异常安全的，如果在new之后和for_each之前发生异常，就会内存泄漏，一个解决方法是使用智能指针。

## 条款8：永不建立auto_ptr的容器

auto_ptr已废弃

## 条款9：在删除选项中仔细选择

我们需要根据容器选择删除的方法。比如一个连续内存容器，最好的方法是erase-remove方法，我们现在要删除c中所有值为1936的对象：

```c++
c.erase(remove(c.begin(), c.end(), 1936), c.end()); //当c是vector、string或deque时，erase-remove是惯用删除特定值的方法
```

这种方法也适合list，但list的成员函数remove更高效：

```c++
c.remove(1936);
```

当c是关联容器时就不能使用remove，因为没有，我们可以使用erase，如下：

```c++
c.erase(1936); //当c是标准关联容器时，erase成员函数是去除特定值元素的最佳方法。
```

如果在这种情况下还需要满足某种条件，那对于标准序列容器可以使用remove_if，例如：

```c++
c.erase(remove_if(c.begin(), c.end(), badValue), c.end());
```

对标准关联容器，有以下两种办法，一个更容易编码，一个更高效，容易编码的方案是用remove_copy_if把我们需要的值拷贝到新的容器，然后把原容器的内容与之做交换，如下：

```c++
AssocContainer<int> c; // c现在是一种
... // 标准关联容器
AssocContainer<int> goodValues; // 用于容纳不删除
// 的值的临时容器
remove_copy_if(c.begin(), c.end(), // 从c拷贝不删除
inserter(goodValues, // 的值到
goodValues.end()), // goodValues
badValue);
c.swap(goodValues); // 交换c和goodValues
```

这种方法的缺点是拷贝了所有不删除的元素，存在开销。

我们还可以这样做：

```c++
AssocContainer<int> c;
...
for (AssocContainer<int>::iterator i = c.begin(); // 清晰，直截了当
i!= c.end(); // 而漏洞百出的用于
++i) { // 删除c中badValue返回真
	if (badValue(*i)) c.erase(i); // 的每个元素的代码
} // 不要这么做！
```

这样会造成迭代器失效，也不可取，修改如下：

```c++
AssocContainer<int> c;
...
for (AssocContainer<int>::iterator i = c.begin(); // for循环的第三部分
i != c.end(); // 是空的；i现在在下面
/*nothing*/ ){ // 自增
    if (badValue(*i)) c.erase(i++); // 对于坏的值，把当前的
    else ++i; // i传给erase，然后
} // 作为副作用增加i；
// 对于好的值，
// 只增加i
```

这样不适合序列容器，因此对vector、string和deque采取如下修改：

```c++
for (SeqContainer<int>::iterator i = c.begin();
i != c.end();){
	if (badValue(*i)){
        logFile << "Erasing " << *i << '\n';
        i = c.erase(i); // 通过把erase的返回值
    } // 赋给i来保持i有效
    else
    	++i;
}
```

经过以上讨论，有如下的结论：

- 去除一个容器中有特定值的所有对象：

  如果容器是vector、string和deque，使用erase-remove，如果是list，使用remove，如果是标准关联容器，使用erase成员函数；

- 去除一个容器中满足一个特定判定式的所有对象：如果容器是vector、string、deque，使用erase-remove_if惯常用法，是list则使用remove_if，标准关联容器则用remove_copy_if和swap，或写一个循环来遍历容器元素，记得将迭代器传给erase时后置递增；

- 在循环内做某些事，如果是标准序列，遍历时调用erase记得用返回值更新迭代器，若是标准关联容器，记得将迭代器传给erase时后置递增；

## 条款10：注意分配器的协定和约束

分配器最初被设想为抽象内存模型，这种情况下，分配器在它们定义的内存模型中提供指针和引用的typedef才有意义。在C++标准里，类型T的对象的默认分配器提供typedef allocate<T>::pointer和allocator<T>::reference，而且希望用户定义的分配器也提供这些typedef。然而，C++中没法捏造引用，因为这样做要求有能力重载operator.，而这是不允许的，另外建立行为像引用的对象是使用代理对象的例子，而代理对象会导致很多问题。实际上，标准明确允许库实现假设每个分配器的Pointer typedef是T*的同义词，每个分配器的reference typedef与T&相同，所以库实现忽视typedef并直接使用原始指针和引用。

另外，分配器是对象，那就表明它们可能有成员函数，内嵌的类型和typedef等，但标准允许STL实现认为所有相同类型的分配器对象都是等价且比较总是相等的。

考虑如下的代码：

```c++
template<typename T> // 一个用户定义的分配器
class SpecialAllocator {...}; // 模板
typedef SpecialAllocator<Widget> SAW; // SAW = “SpecialAllocator
// for Widgets”
list<Widget, SAW> L1;
list<Widget, SAW> L2;
...
L1.splice(L1.begin(), L2); // 把L2的节点移到
// L1前端
```

注意到list元素从一个list被接合到另一个时，没有拷贝什么，而是调整了一些指针。当L1被销毁时，它必须销毁它的所有节点，因为它现在包含最初是L2一部分的节点，L1的分配器必须回收最初由L2分配器分配的节点，所以标准允许STL实现认为同类型的分配器等价。这个标准非常严格，意味着可移植的分配器对象不能有状态，即不能有任何非静态数据成员，至少没有会影响它们行为的。

operator new跟allocator<T>::allocate还是有很大程度的不同，首先看函数声明：

```c++
void* operator new(size_t bytes);
pointer allocator<T>::allocate(size_type numObjects);
// 记住事实上“pointer”总是
// T*的typedef
```

operator new指定的是字节数，而分配器指定的是内存容纳多少个T对象，其次是返回对象，new返回void\*，而分配器返回一个T\*，虽然这里T还没有被构造。

考虑list容器，里面可能存储一些节点ListNode，当我们用分配器为它获取内存时，我们要的不是T的内存，而是ListNode的内存，可以这样写：

```c++
template<typename T> // 标准分配器像这样声明，
class allocator { // 但也可以是用户写的
public: // 分配器模板
    template<typename U>
    struct rebind{
    	typedef allocator<U> other;
    }
    ...
};
```

这样，ListNodes的对应分配器类型是Allocator::rebind<ListNode>::other；

下面总结一下内容：

- 将分配器做成模板，带有模板参数T，代表要分配内存的对象类型；
- 提供pointer和reference的typedef，但总是让pointer是T*，reference是T&；
- 绝不要给分配器每对象状态，一般不能有非静态成员；
- 传给分配器的是对象个数，返回的是T*指针；
- 一定要提供标准容器依赖的内嵌rebind模板；

## 条款11：理解自定义分配器的正确用法



