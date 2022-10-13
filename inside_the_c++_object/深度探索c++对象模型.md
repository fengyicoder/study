# P深度探索C++对象模型

## 关于对象

将c程序转换成C++之后，一般并不会增加成本，成员变量自不必说，对于非内联成员函数，虽然含在class的声明中，但不出现在object中，每一个非内联成员函数只会诞生一个函数实体，对于内联函数则会在每一个使用者（模块）上产生一个函数实体。因此，C++的封装并未在空间或执行期带来不良影响，其在布局以及存取时间上的额外负担主要由于virtual引起的，包括：

- 虚函数：支持一个有效率的执行期绑定；
- 虚基类：用以实现多次出现在继承体系中的basic class有一个单一而被共享的实体；

此外，还有一些多重继承下的额外负担，发生在一个派生类和它第二或后继基类的转换。因此，并没有理由说C++一定比C庞大而迟缓。

在C++中有静态和非静态两种数据类型，有静态、非静态、虚三种成员函数类型。

类在内存中的模型可以是一个简单对象模型，即为了解决成员类型不同，类对象只存放执行各个成员的指针。还可以是表格驱动模型，对象只存放两个指针，一个指向所有的数据成员，一个指向所有的成员函数。真正的C++对象模型则是从简单对象模型派生而来，非静态成员存在类对象之内，而静态成员则在类对象之外，静态和非静态成员函数也在所有的类对象之外，虚函数则需要以下两个步骤支持：

1. 每一个类产生一堆执行虚函数的指针，放在虚表中；
2. 每个类对象被添加一个指针，指向这个虚表，虚表的设定、重置都由每一个类的构造、析构和拷贝赋值运算符自动完成。每一个类所关联的type_info对象（用以支持runtime type identification, RTTI）也由虚表指出，通常放在虚表的第一个slot处。

如图所示：

![](..\image\inside_c++\c++_memory.png)

C++最初采用的继承模型并不运用任何间接性，base class subobject的数据成员被直接置放在derived class object中，自C++2.0起导入virtual base class，其原始模型是在类对象中为每一个有关联的virtual base class加上一个指针。

C语言有以下的技巧，把单一元素的数组放在一个struct的尾端，于是一个struct objects可以拥有可变大小的数组：

```c++
struct mumble {
	char pc[1];
};
struct mumble *pmumbl = (struct mumble*)malloc(sizeof(struct mumble) + strlen(string) + 1);
strcpy(pmumbl->pc, string);
```

但在C++中以上的伎俩或许有效，但要保证protected data members放在private data members的前面才行，否则其data members的布局的顺序不确定：

```c++
class stumble {
public:
	...
protected:
	...
private:
	char pc[1];
};
```

如果确实需要一个相当复杂的C++ class的某部分数据像C声明的那样，那么这一部分最好抽取出来成为一个单独的struct声明，将C与C++组合一起的做法是从C struct中派生C++部分：

```c++
struct C_point {...};
class Point: public C_point {...};
```

但这种方法现在由于某些编译器对虚函数的支持已不再推荐，使用组合而非继承才是结合C和C++的唯一可行方法：

```c++
struct C_point{...};
class Point {
public:
	....
private:
	C_point _c_point;
};
```

这样通过struct将数据封装起来，可以保证拥有与C兼容的空间的布局，可以传递到某个C函数中。

在OO中，处于继承体系之中的对象的真实类型在每一个特定执行点之前，是无法解析的，在C++中，只有通过指针和引用的操作才能完成，如下：

```c++
//描述objects：不确定类型
Librar_materials *px = retrieve_some_material();
Librar_materials &rx = *px;

//描述已知物：不可能有其他结果产生
Librar_materials dx = *px;
```

C++以以下方法支持多态：

1. 经由一组隐含的转化操作，例如把一个derived class指针转换为一个指向public base type的指针：`shape *ps = new circle();`
2. 经由virtual function机制：`ps->rotate();`
3. 经由dynamic_cast和typeid运算符：`if(circle *pc = dynamic_cast\<circle\*\>(ps)) ...`

## 构造函数语意学

explicit之所以被导入C++，就是为了防止单一参数的构造函数被当作一个conversion运算符。众所周知，在必要的情况下会合成默认构造函数。这个默认的构造函数又分为必要（有用的）的和不必要（无用的）的，不必要的构造函数不进行任何操作（例如声明default的），所有与C语言兼容的数据类型（POD类型）都有无用的构造函数，实际上无用的构造函数根本不会合成。如果一个类没有任何构造函数，但它内含一个成员对象，其有默认构造函数，那么此时就是必要的情况，编译器需要为这个类合成一个默认构造函数，但这个合成操作只有在构造函数真正需要时才被调用。这个时候就出现了一个问题，如何避免合成多个默认构造函数，解决方法是把default constructor、copy constructor、destructor、assignment copy operator都以inline方式完成，一个inline函数有静态链接，不会被文件以外者看到。如果函数太复杂，则会合成出一个explicit non-inline static实体。有以下的例子：

```c++
class Foo {public: Foo(), Foo(int)...};
class Bar {public: Foo foo; char *str;};
void foo_bar()
{
	Bar bar;
	if (str) {c}
}
```

这就符合上述情况，合成的默认构造函数会初始化Bar之中的foo，但str不会，str的初始化是程序员的责任。如果我们添加了如下的构造函数：

```c++
Bar::Bar()
{
	str = 0;
}
```

为了满足编译器的要求，其会扩张已存在的constructors，使得可以调用必要的默认构造函数。如果有多个类成员对象都要求构造函数初始化操作，则会按照声明顺序来调用各个构造函数。

类似的道理，如果一个没有构造函数的类派生自一个带有默认构造函数的基类，那么合成默认构造函数也是有必要的。

另有两种情况，也需要合成默认构造函数：

- class声明（或继承）一个虚函数；
- class派生自一个继承链，其中有一个或多个虚基类；

看以下的例子：

```c++
class Widget {
public:
	virtual void flip() = 0;
}

void flip(const Widget& widget) { widget.flip();}
void foo()
{
	Bell b;
	Whistle w;
	flip(b);
	flip(w);
}
```

有以下两个扩张行动会发生：1. 一个虚函数表会被产生，用来存放虚函数地址；2. 虚表指针被产生放在类对象中。对虚函数的调用，实质如下：

```c++
widget.flip() -> (*widget.vptr[1])(&widget)
```

因此，需要合成一个默认构造函数，来正确初始化这样一个虚表指针。

虚基类的实现不同编译器之间有很大差异，对应虚函数的执行我们必须使其延迟到执行期确定下来，原来cfront的做法是靠在派生类对象插入一个指向虚基类的指针来决定，所有经由引用或指针来存取一个虚基类的操作都可以经由相关指针完成，这个插入的指针就是在类对象的构造期间完成的。对于class所定义的每一个构造函数，编译器会插入这样一个指针，如果没有声明构造函数，那么编译器必须合成一个。

拷贝复制构造主要发生在以下三种情况：显式的拷贝，以参数的形式传入，以及作为返回值传出。如果一个类没有声明显式的拷贝构造函数，那么在发生拷贝时，会把每一个成员的值拷贝到对应的地方，但不会拷贝其中的类成员对象，而是会递归的拷贝类对象的值。对于编译器要不要合成一个默认拷贝构造函数，要看类是否具有位逐次拷贝的语义，如下：

```c++
class Word {
public:
	Word(const *);
	~Word() { delete [] str;}
private:
	int cnt;
	char *str;
}
```

这段声明展现了逐次位拷贝的语义，因此就不需要合成默认拷贝构造。而对于以下的示例：

```c++
class Word {
public:
	Word(const *);
	~Word() { delete [] str;}
private:
	int cnt;
	String str;
}

class String {
public:
	String(const char*);
	String(const String&);
}
```

这种情况就必须合成出一个拷贝构造函数，以便调用String成员的拷贝构造函数。

一个类不展现逐次位拷贝语义有以下四种情况：

1. 一个类内含一个成员对象，后者声明有一个拷贝构造；
2. 类继承一个基类，而基类存在一个拷贝构造；
3. 类声明了一个或多个虚函数时；
4. 类派生自一个继承链，其中有一个或多个虚基类；

前两种情况必须把所含对象或基类的拷贝构造插入到合成的默认拷贝构造中。下面来讨论后面的情况，在编译期间，如果不能合适的设置vptr的初始值，就会导致可怕的后果，这也是不展现逐次位拷贝语义的原因。对于一个含有虚函数的类，如果是逐次位拷贝，那么很可能在将派生类对象拷贝到基类对象时发生vptr的值错误。虚基类的情况类似，不过是因为类切割的问题。

值传递临时对象的生成首先是对参数的拷贝，之后函数直接使用这个拷贝对象，令人讨厌的一点是这样操作函数的声明也要转换，从一个对象改变为对该对象的引用，另一种实现方法是该临时对象直接建构在其应该在的位置，视情况记录在堆栈中，函数返回之前临时对象被析构。

返回值临时对象的构建分为以下两步，首先参数引入一个对类对象的引用，用以建构返回值，之后在return指令之前调用拷贝构造完成值的传递。比如以下的转换：

```c++
X xx = bar();
//转换为
X xx; //不必进行默认构造
bar(xx);
```

在使用者层面对上述的问题可以做一定的优化，例如：

```c++
X bar(const T &y, const T &z)
{
	return X(y, x);
}
```

这样省去了拷贝构造。

编译器层面也可能自己做优化，方法是以result参数取代named return value（NRV）。示例如下：

```c++
X bar()
{
	X xx;
	return xx;
}
//未优化
void bar(X& __result)
{
	X xx;
	xx.X::X();
	...
	__result.X::X(xx);
	return;
}
//优化后
void bar(X& __result)
{

	__result.X::X();
	return;
}
```

那么，有了NRV之后，拷贝构造还需不需要。当类成员需要大量的memberwise初始化操作，例如已传值的方式传回object，这种情况提供一个显式内联的拷贝构造是非常合理的。当然，在拷贝构造时，使用memcpy也非常有效率，例如：

```c++
Point3d::Point3D(const Point3d &rhs) {
	memcpy(this, &rhs, sizeof(Point3d));
}
```

但这种情况必须在类不含有任何由编译器产生的内部成员的情况下才可以。如果类声明一个或以上的虚函数或内含一个虚基类，那么就不能这么做，否则会导致由编译器添加的成员例如虚表指针被改写。

下面再来讨论类成员的初始化，以下的情况必须使用初始化列表：

1. 初始化一个引用成员时；
2. 初始化一个const成员时；
3. 调用一个基类的构造函数，而它拥有一组参数时；
4. 调用一个成员类的构造函数，而它拥有一组参数时；

列表初始化在任何explicit user code之前，当然，初始化顺序按照声明顺序。另外，如果使用一个成员初始化另一个成员，建议放在构造函数内。

有以下的代码：

```c++
X::X(int val)
	: i(xfoo(val)), j(val) {}
```

此时使用成员函数xfoo合法吗？答案是合法，因为此时this指针已经构建完毕。另外还是建议将xfoo放在构造函数中，这样哪一个成员在xfoo执行时被设立初值就不会混淆。

## Data语意学

一个空的class：

```c++
//sizeof X == 1
class X {}
```

事实上，一个空类的大小并不是0，而是有一个隐藏的1 byte大小，这是编译器插入的一个char，这使得一个class的两个object有不同的地址。

对于以下的继承情况：

![Diamond_inheritance](..\image\inside_c++\Diamond_inheritance.png)

Y跟Z的大小都是8，受到以下几个因素影响：

1. 虚基类会带来一定的负担，是某种形式的指针，或者指向虚基类子对象，或者指向一个相关子表格，表格中存放的不是虚基类子对象的地址，就是其偏移位置，在这台机器上，指针是4 byte；
2. 虚基类的子对象也会出现在派生类中，占用1 byte，被放在固定部分的尾端；
3. 目前Y和Z的大小为5 byte，在大部分的机器上，聚合的结构体大小会受到alignment的限制，使其更有效率的在内存中被存取，在本机中alignment为4 byte，因此需要补齐到8 byte；

一些编译器会对虚基类进行优化，其在派生类中被视为最开始的一部分，因此没有1 byte的占用，也就不需要进行字节对齐，因此这种情况下Y和Z都是4 byte。

而A的大小主要有下面几点决定：

- 被大家共享的唯一的Z的实例，大小为1 byte；
- Y跟Z的大小，减去因虚基类配置的大小，分别为4，那么总共为8；
- 字节对齐，为12；

如果发生了前述的优化，那么A中没有那1 byte，也就不需要字节对齐，那么大小就是8。

下面考虑数据成员的绑定问题，有以下的代码：

```c++
extern float x;
class Point3d 
{
public:
	Point3d(float, float, float);
	float X() const { return x;}
	void X(float new_x) const { x = new_x}
private:
	float x, y, z;
}
```

如果调用X()，我们毫无疑问直到此时返回的是对象内部的x，而不是global的x，但在以前的编译器中，如果在实例中对x做出取用操作，该操作就会指向global x。这就导出早期C++的防御性设计风格：

1. 将所有的数据成员放在class的声明起始处，以确保正确的绑定：

   ```c++
   class Point3d 
   {
   	float x, y, z;
   public:
   	...
   }
   ```

2. 将所有的内联函数都放在class声明之外：

   ```c++
   class point3d
   {
   ...
   public:
   	float X() const;
   }
   inline float
   Point3d::X() const
   {
   	return x;
   }
   ```

这个语言规则是member rewriting rule，值得是一个内联函数实体，在整个class声明未被完全看见之前，是不会被评估求值的。当然，这个规则的必要性在C++2.0之后就消失了，就算在函数声明时定义了一个内联函数，其分析还是在整个class声明完全出现后才进行。然而，这种规则对于成员函数的参数列表不适用，例如以下的例子：

```c++
typedef int length;
class Point3d 
{
public:
	//length被错误决议为int
	void mumble(length val) { _val = val;}
private:
	typedef float length;
	length _val;
}
```

这种情况仍旧需要防御性代码风格，将nested type声明放在class起始处。

下面讨论以下数据成员的布局，有以下的代码：

```c++
class Point3d 
{
public:
	...
private:
	float x;
	static List<Point3d*> *freeList;
	float y;
	static const int chunkSize = 250;
	float z;
}
```

x、y、z在对象顺序中与其声明一致，静态变量不在对象中，放在数据段中。标准要求，在同一个access中的成员排列只需要晚出现的成员处于较高的地址即可，也就是说各成员之间不一定是连续排列的，有时需要字节对齐，有时编译器也会合成一些必要的成员，例如vptr，传统上其被放在所有显式声明的成员之后，但现在也有编译器将其放在类对象的最前端。

标准也允许编译器将多个access section之中的数据成员自由排列，不必在乎它们在class中的声明顺序，但现在的编译器都是把一个以上的access section连锁在一起，依照声明的顺序成为一个连续的区块。对于静态成员变量来说，通过指针和通过对象来存取成员变量的成本相同，因为静态变量不在类对象之中，由于其唯一性，即便是从复杂的继承之中继承而来也如此。如果是经由函数调用或其他某些语法来进行存取，虽然标准要求函数调用必须被求值，但从结果来看，并没有什么差别。如果有个类，每一个都有一个静态成员freeList，就会导致名称冲突，编译器的解决方法是对每一个静态成员暗中编码，获得一个独一无二的程序识别代码。

非静态数据成员存放在类中，要想存取它们，必须显式或隐式的经过类对象。比如针对以下的代码：

```c++
Point3d
Point3d::translate(const Point3d &pt) {
	x += pt.x;
	y += pt.y;
	z += pt.z;
}
```

其事实上是经过一个隐式类对象（由this指针表达）来完成的：

```c++
Point3d
Point3d::translate(Point3d *const this, const Point3d &pt) {
	this->x += pt.x;
	this->y += pt.y;
	this->z += pt.z;
}
```

其进行存取操作，需要通过使用起始地址加上偏移位置，比如地址`&origin._y`的地址为：

```c++
&origin + (&Point3d::_y - 1)
```

注意这个-1操作，指向数据成员的指针，其偏移量总是被加上1，这样可以使得编译系统区分出“一个指向数据成员的指针，用以指出类的第一个成员”和“一个指向数据成员的指针，没有指出任何成员”的情况。

对于虚继承，例如：

```c++
Point3d *pt3d;
pt3d->_x = 0.0
```

执行效率在_x是一个结构体成员、类成员、单一继承以及多重继承的情况下都完全相同，但如果\_x是一个虚基类的成员，存取速度会稍慢。回忆以前的例子：

```c++
origin.x = 0.0;
pt->x = 0.0;
```

如果类是一个虚基类的派生类，那么pt的类型必须要延迟至执行期才能确定，而origin的类型编译器就可以确定，显然，两者速度有差异。

对一个派生类来说，派生类成员和基类的成员的排列顺序理论上编译器可以自由安排，但对于大部分编译期，基类成员总是先出现，但属于虚基类的除外。

对于单一继承且没有虚函数时的数据布局，有以下的示例：

```c++
class Concrete {
public:
	...
private:
	int val;
	char c1;
	char c2;
	char c3;
};
```

这份内存布局中，加上字节对齐占用内存为8 byte，可一旦拆开，如：

```c++
class Concrete {
public:
	...
private:
	int val;
	char c1;
};
class Concrete2： public Concrete {
public:
	...
private:
	char c2;
};
class Concrete3: public Concrete2{
public:
	...
private:
	char c3;
};
```

对于Concrete2，其内存布局为基类字节对齐后的8 byte加上字节的1 byte，再经过字节对齐，为12 byte，同理，Concrete3为16 byte。这样是为了防止复制时发生错误，例如派生类复制给基类，没有这样的规则那么复制会出错。

如果一旦加上多态，会有很多的负担，例如虚函数表，为了多态编译器也会插入许多代码。另外，vptr的位置也是许多讨论的热点，最初的时候vptr放在对象的尾端，为了跟c的内存情况保持一致，这样前面都是数据成员。但从C++2.0之后，某些编译器开始将vptr放在对象的开头，这样的好处是多重继承时调用虚函数方便了许多，代价就是c的兼容性。

多重继承的情况下，有时只需要指定地址即可，有时却需要修改地址。例如内存布局如下：

![Vertex3d](..\image\inside_c++\Vertex3d.png)

对于如下的代码：

```c++
Vertex3d *pv3d;
Vertex *pv;
pv = pv3d;
```

此时的转换就不能只是简单的`pv = (Vertex*)((char\*)pv3d) + sizeof(Point3d)`；还要考虑pv3d为0，因此需要做一个条件测试`pv = pv3d ? (Vertex*)((char\*)pv3d) + sizeof(Point3d) : 0`。至于引用，则不需要进行防卫，因为引用不可能为空。

对于虚继承，如果一个类含有一个或多个虚基类子对象，那么类将被分割成一个不变区域和共享区域，不变区域中的数据拥有固定的偏移，所有这部分数据可以被直接存取，共享区域表现的则是虚基类子对象，这一部分数据其位置会因为每次的派生操作有变化，所以只可以被间接存取。一般来说，先安排好不变部分再建立其共享部分，编译器会在每一个派生类中安插指针，每个指针指向一个虚基类，要存取继承而来的虚基类成员，可通过相关指针间接完成。这样有两个缺点：

1. 每个对象必须针对其每一个虚基类背负一个额外的指针；
2. 继承链一旦加长，则间接存取层次会增加，时间成本也会增加；

针对第二个问题，可以拷贝取得所有的嵌套虚基类指针，放在派生类中，不过继承链多长都可以有固定的存取时间，模型如下：

![metaware](..\image\inside_c++\metaware.png)

对于第一个问题，一般有两种解决方法，第一种是microsoft编译器引入了虚基类表，表里存放了一个或多个虚基类；第二种是在虚函数表中放置虚基类的偏移，内存模型如下：

![Sun](..\image\inside_c++\Sun.png)

下面讨论指向类成员的指针：

```c++
&Point3d::z; //取成员地址的偏移并应该-1；
&origin.z //取成员真正的地址
```

至于偏移值为何总是多1，则是为了区分一个没有指向任何成员的指针和一个指向第一个数据成员的指针。

## Function 语意学

C++设计的准则之一就是非静态成员函数至少必须和普通的非成员函数具有相同的效率。实际上，成员函数被内化为非成员的形式，要经过以下的几步：

1. 改写函数的signature以安插一个额外的参数到成员函数中，用以提供一个存取管道，使得类对象可以被函数调用，这个额外的参数就是this指针。
2. 将每一个对非静态数据成员的存取操作改为由this执行；
3. 将成员函数重写成一个外部函数，函数名经过mangling处理，成为程序中独一无二的词汇；

一般而言，成员的名称前面会被加上class的名称，形成独一无二的命令，这主要是为了在继承过程中的识别。为了识别重载这种情况，参数链表也会被加上，但返回类型没有被考虑在内。

如果调用的是虚函数：

```c++
ptr->normalize();
```

其在内部可能转换为：

```c++
(*ptr->vptr[1])(ptr);
```

如果在normalize之中再调用一个成员函数，也是虚函数，此时虚拟机制已经决议妥当，因此适合显式的调用Point3d实例，并压制虚拟机制防止产生不必要的重复调用：

```c++
register float mag = Point3d::magnitude();
```

如果magnitude声明为内联，会更有效率。使用显式调用一个虚函数，其决议方式会和非静态成员一样。

如果是静态成员函数，其将被转换为一般的非成员函数调用。其实如果没有发生对成员的存取操作，是没有必要使用一个对象的，但如果静态成员是非公有的，那么就需要提供成员函数来存取这个数据，也就需要绑定对象进行操作。但有时候，独立于对象之外的操作也非常重要，当我们这么希望时，在程序方法上的解决方法是将0强制转换为一个class指针，因而提供出一个this实例，而在语言层面上的解决之道，就是引入了静态成员方法，其没有this指针，并具有以下的特性：

- 不能直接存取class中的非静态成员；
- 不能声明为const、volatile或virtual；
- 不需要经由class object才被调用；

如果对一个静态成员函数取地址，会取得真实的地址，由于静态成员函数没有this指针，所以其地址类型不是一个指向class member function的指针，而是一个非成员函数指针。这也提供了一个意想不到的好处，那就是它可以成员一个回调，使得可以在C++和C之间进行通信。

为了支持虚函数的机制，必须首先能够对多态对象有某种形式的"执行期类型判断法"，也就是说`ptr->z()`这样的操作将需要ptr在执行期间的信息，其中最直接的但成本最高的方法是把必要的信息加在ptr上。在这种策略下，一个指针或引用持有两项信息：

- 它所参考到的对象的地址；
- 对象类型的某种编码或是某个结构（内含某些信息，用以正确决议出z()函数实例）的地址；

这样会有两个问题：增加了空间的负担；不再于C兼容，因此此法不可取。在C++中，多态表示以一个public base class的指针或引用寻址出一个derived class object的意思。最终所采用的方法是看是否有任何的虚函数，只要有虚函数，那么就需要额外的信息来支持多态。这个额外的信息哪些是必要的呢：

- ptr所指向对象的真实类型，以便选择正确的z()实例；
- z()实例的位置，以便能够调用；

在实现上，可以为每一个多态的class对象上增加两个成员：一个字符串或数字，代表class的类型；一个指针，指向某种表格，表格中持有虚函数的执行期地址。这个表格可以在编译期进行建构，但如何找到哪些地址呢：

- 为了找到该表格，每个对象被安插了一个编译器内部产生的指针，指向该表格；
- 为了找到函数地址，每个虚函数被指派一个表格索引值；

一个类只会有一个虚表，这个虚表中的函数有：这一类定义的函数实例，其改写了继承的虚函数；继承基类的函数实例；一个pure_virtual_called()函数实例，其既可以扮演Pure virtual function的空间保卫者角色，也可以当作执行期异常处理函数。在编译期，是可以将上述的函数调用转换为`(*ptr->vptr[4])(ptr)`，唯一一个在执行期才能知道的是slot4到底指的是哪一个z()函数的实例。

如果是多重继承，情况就复杂了许多，主要在于第二个以及后继的base class身上以及必须在执行期调整this这点。有以下的例子：

```c++
class Base1 {
public:
	Base1();
	virtual ~Base1();
	virtual void speakClearly();
	virtual Base1 *clone() const;
protected:
	float data_Base1;
};
class Base2 {
public:
	Base2();
	virtual ~Base2();
	virtual void mumble();
	virtual Base2 *clone() const;
protected:
	float data_Derived;
};
```

对这个例子来说，有三个问题需要解决：1) virtual destructor；2) 被继承下来的Base2::mumble()；3) 一组clone()实例。这些问题依次解决，考虑这样一个语句`Base2 *pbase2 = new Derived;`编译器会产生如下的代码：

```c++
Derived *temp = new Derived;
Base2 *pbase2 = temp ? temp + sizeof(Base1) : 0;
```

然而当要删除pbase2指向的对象时`delete pbase2`。指针必须再次被调整，以求再次指向Derived对象的起始处，但却不能在编译期确定，因为pbase2的真正对象只有在执行期才能确定。一般规则是经由指向第二或后继基类的指针来调用派生类虚函数，其连带的this指针调整，必须在执行期完成。也就是说，offset的大小以及把offset加到指针上的那一小段代码必须由编译器在某个地方插入，cfront编译器的方法是将虚表加大，每一个slot不再只是一个指针，而是一个集合体，包含可能的offset和地址，于是虚函数的调用由：

```c++
(*pbase2->vptr[1])(pbase2)
```

变为：

```c++
(*pbase2->vptr[1].faddr)(pbase2 + pbase2->vptr[1].offset);
```

其中faddr内含虚函数的地址，offset内含this指针调整值，这样的坏处就是为那些并不需要调整的虚函数增加了负担。比较有效率的解决方法是thunk。所谓的thunk，是一小段assembly代码，用来1） 以适当的offset值调整this指针2）跳到虚函数中。例如，由一个base2指针调用派生类析构函数，其thunk可能是如下的样子：

```c++
pbase2_dtor_thunk:
	this += sizeof(base1);
	Derived::~Derived(this);
```

有了thunk，slot中的指针可以指向虚函数，也可以指向一个相关的thunk。

调整this指针的第二个额外负担是，由于两种不同的可能：经由derived（或第一个基类）调用；经由第二个（或其后继）base class调用，同一函数在虚表中可能需要多笔对应的slot：

```c++
Base1 *pbase1 = new Derived;
Base2 *pbase2 = new Derived;
delete pbase1;
delete pbase2;
```

虽然两个delete操作会使用相同的派生类析构函数，但它们需要两个不同的虚表slots：

1. pbase1不需要调整this指针，其虚表slot放置真正的析构函数；
2. pbase2需要调整this指针，其虚表slot放置thunk地址；

在多重继承之下，一个派生类内含n-1个额外的虚表，n表示上一层基类的个数。对于本例而言，派生类会有两个虚表：

1. 一个主要实例，与Base1共享；
2. 一个次要实例，与Base2有关；

用以支持一个类拥有多个虚表的传统方法是将每一个表格用外部对象的形式产生出来，并给与独一无二的名称，例如Derived关联的两个表可能是vtb1\_\_Derived、vtb1\_\_Base2\_\_Derived。所以将Derived对象指定给一个Base1指针或Derived指针时，被处理的是主要表格vtb1\_\_Derived，而指定给Base2指针时，处理的是次要表格vtb1\_\_Base2__Derived。示意图如下：

![vt](..\image\inside_c++\vt.png)

前文所述，有三种情况，第二或后继基类会影响对虚函数的支持。第一种情况是通过指向第二个基类的指针调用派生类虚函数，这样会导致this指针调整到派生类的起始处。第二种情况是通过一个派生类的指针调用第二个基类中一个继承而来的虚函数，在此情况，this必须再次调整，指向第二个基类的子对象；第三种情况是允许一个虚函数的返回值类型有所变化，比如：

```c++
Base2 *pb1 = new Derived;
Base2 *pb2 = pb1->clone();
```

当调用时，pb1会调整指向Derived对象的起始地址，于是clone的Derived版会被调用；它会传回一个指针，指向一个新的Derived对象；该对象在被指向pb2之前先经过调整，以指向Base2的子对象。

当函数足够小时，sun编译器会提供一个所谓的split functions技术：以相同的算法产生两个函数，其中第二个在返回之前为指针加上必须的offset。这样可以根据实际的指向调用不同的函数。如果函数并不小，split function策略会给予此函数种的多个进入点的一个。如果函数支持多重入点，就不必有许多thunks。像IBM就是函数一开始先调整this指针，然后执行函数代码，如果无需调整则直接执行函数代码。微软则使用address points来取代thunk策略。

如果是虚继承，那么派生类跟基类的对象也不会相符，两者之间的转换也需要调整this指针，其内存布局如下：

![vvt](..\image\inside_c++\vvt.png)

建议，不要在虚基类中声明非静态成员，不然其对虚基类的支持会变得非常复杂。

## 构造、析构、拷贝语意学

考虑以下的代码：

```c++
class Abstract_base {
public:
	virtual ~Abstract_base()=0;
	virtual void interface() const = 0;
    virtual const char* mumble() const { return _mumble; }
protected:
	char *_mumble;
};
```

虽然这是个抽象基类，但仍需要显式的构造函数以初始化_mumble，否则其派生类的局部性对象\_mumble将无法决定初值，例如：

```c++
class Concrete_derived: public Abstract_base {
public:
	Concrete_derived();
};

void foo() {
	Concrete_derived trouble;
}
```

因此，类的数据成员应该被初始化，并且只在构造函数或其他成员函数中指定初值。

纯虚函数可以被定义和调用，但只能被静态的调用，比如：

```c++
inline void 
Abstract_base::interface() const {
...
}
inline void
Concrete_derived::interface() const
{
	Abstract_base::interface();
}
```

要不要这样做，全由设计者决定，但唯一例外的是纯虚析构，一定要定义它，因为派生类的析构函数要进行扩张，就必然以静态方法调用基类的析构函数，缺少任何一个定义都会导致链接失败。

把mumble设计为一个虚函数是一个糟糕的选择，因为其派生类不太可能改变其类型，并且内联函数的效率也较高。const的存在麻烦在如果派生类突然想要修改成员，因此最好不要使用const，重新修改声明如下：

```c++
class Abstract_base {
public:
	virtual ~Abstract_base();
	virtual void interface()  = 0;
    const char* mumble() const { return _mumble; }
protected:
	Abstract_base(char *pc = 0 );
	char *_mumble;
};
```

有以下的程序片段：

```c++
Point global;
Point foobar() {
	Point local;
	Point *heap = new Point;
	*heap = local;
	delete heap;
	return local;
}
```

将Point声明为Plain Old Data形式：

```c++
typedef struct {
	float x, y, z;
} Point;
```

理论上，对global非必要的构造和析构都会被调用，然后，这些非必要的成员要不是没定义就是没调用，程序的行为在c中的表现一样。在C中，global被视为一个临时性的定义，因为其没有显式的初始化操作，一个临时性定义可以发生多次，这些实例会被链接器折叠起来，只留下一个单独的实例，被放在数据段中的特别保留为未初始化的全局对象使用的空间，即BBS，但在C++中，其不支持临时性的定义，因此global被视为完全定义的，其被视为初始化过的数据来对待。对于local，同样也是既没有被构造也没有被析构，而heap的定义会导致new的调用，但并没有默认构造施加于new产生的指针。接下来有一个对heap的赋值操作，由于local没有被初始化，因此会产生编译警告，并触发非必要的拷贝赋值操作，但实际上该对象是一个Plain O1' Data，因此赋值操作只是像C那样纯粹的位搬移操作。delete操作理论上会触发非必要的析构，但如前文讨论的，析构不是没产生就是没调用。最后返回local，实际上的return也没有触发非必要的拷贝构造，只是一个简单的位拷贝操作。

如果对Point的声明如下：

```c++
class Point {
public:
	Point(float x=0.0, float y=0.0, float z=0.0):
		_x(x), _y(y), _z(z) {}
private:
	float _x, _y, _z;
};
```

这是一个经过封装的类，其大小与前面相比没有什么变化。对于一个global，现在有了默认构造作用在上面，其初始化操作将延迟到程序启动才开始。如果要将类中所有成员都设定常量初值，那么给予一个显式初始化列表比较有效率。比如：

```c++
void mumble()
{
	Point local1 = {1.0, 1.0, 1.0};
	Point local2;
	local2._x = 1.0;
	local2._y = 1.0;
	local2._z = 1.0;
}
```

local1会比较有效率些，因为当函数的activation record被放进程序堆栈时，上述初始化列表中的常量就可以被放进local1的内存中了。但显式初始化列表也带来了三个缺点：

1. 只有当类成员都是public时，此法才奏效；
2. 只能指定常量，因为它们在编译期间就可以被评估求值；
3. 由于编译器没有自动施行，因此初始化行为的失败可能性会高一些；

在编译器层面会有一个优化机制来识别内联构造，这个内联构造只简单提供一个member-by-member的常量指定操作，编译器会抽取出这些值并像是显式初始化列表那样，而不会把构造函数扩展成一系列的赋值指令。观念上，按照foobar里的操作，类有一个相关的默认拷贝构造、拷贝赋值和析构，但实际上，它们都是不必要的，编译器实际并没有产生。

现在为继承做准备：

```c++
class Point {
public:
	Point(float x=0.0, float y=0.0, float z=0.0):
		_x(x), _y(y), _z(z) {}
	virtual float z();
private:
	float _x, _y, _z;
};
```

由于虚函数的存在，每个类会附加一个vptr，除了其空间占用之外，虚函数的导入也导致类产生膨胀作用：

- 构造函数被附加了一些代码，以便将vptr初始化；
- 合成一个拷贝构造和拷贝赋值操作符，而且其操作不再是非必要的了（但隐式析构仍旧是非必要的）。

一般来说，如果有许多函数都需要以传值方式传回一个local class object，那么提供一个拷贝构造就比较合理，其存在会触发NRV优化。

下面讨论继承体系下的对象构造，当定义一个对象时，如`T object;`。如果T有一个构造函数时，其构造函数会被调用，编译器会有扩充操作，如下：

1. 记录在成员初始化列表中的成员初始化会在构造函数内进行操作，并以成员的声明顺序进行初始化；
2. 如果一个成员没有出现在成员初始化列表，但它有默认构造函数，那么该默认初始化函数将会被调用；
3. 在那之前，如果存在vptr，那么vptr会被设定初值，指向vpt；
4. 在那之前，所有上一层的基类构造必须被调用，以基类声明顺序为顺序：如果基类被列于初始化列表中，那么任何显式的参数都会传递进去；如果基类没有列于成员初始化列表，而有默认构造函数，则调用之；如果基类是多重继承下的第二或后继基类，则this指针必须有所调整；
5. 在那之前，所有虚基类的构造函数必须被调用，从左至右，从最深到最浅：如果类被列于初始化列表中，那么有任何的显式参数都应该传递过去，若没有列于list之中，而类有一个默认构造也应该调用；类的每一个虚基类子对象的偏移位置必须在执行期可被存取；如果对象是最底层的类其构造函数可能被调用，某些用于支持这一行为的机制必须被放进来。

有如下的代码：

```c++
class Point {
public:
	Point(float x=0.0, float y = 0.0);
	Point(const Point&);
	Point& operator=(const Point&);
	virtual ~Point();
	virtual float z() {return 0.0;}
    
protected:
	float _x, _y;
};
```

如果这样使用：

```c++
class　Line {
	Point _begin, _end;
public:
	Line(float = 0.0, float = 0.0, float = 0.0, float = 0.0);
	Line(const Point&, const Point&);
	draw();
};
Line::Line(const Point &begin, const Point &end):
	_end(end), _begin(begin) {}
```

由于类成员声明了拷贝构造、拷贝赋值以及析构，因此Line的隐式拷贝构造、拷贝赋值以及析构都将是重要的，注意，如果Line是派生自Point的话其析构合成出来将是虚的，但现在只是重要的。

现在考虑虚拟继承的情况：

```c++
class　Point3d: public virtual Point {
public:
	Point3d(float x=0.0, float y=0.0, float z=0.0): Point(x, y), _z(z) {}
	Point3d(const Point3d& rhs): Point(rhs), _z(rhs._z) {}
	~Point3d();
    Point3d& operator=(const Point3d&);
    virtual float z() {return _z;}
protected:
    float _z;
};
```

此时传统的构造函数扩充并没有发生，也就是说以下的伪码是错误的：

```c++
Point3d* Point3d::Point3d(Point3d *this, float x, float y, float z)
{
	this->Point::Point(x, y);
    //虚继承下两个虚表的设定
	this_>__vptr_Point3d = __vtb1_Point3d;
	this->__vptr_Point3d_Point = __vtb1_Point3d__Point;
	this->_z = rhs._z;
	return ;
}
```

原因在于这样的情况：

```c++
class Vertex: virtual public Point {...};
class Vertex3d: public Point3d, public Vertex {...};
class PVertex: public Vertex3d {...};
```

显然，Point3d和Vertex同为Vertex3d的子对象，它们对Point的构造函数的调用操作一定不可以发生，相关的调用应该由PVertex执行。现在Point3d不应该执行对虚基类的构造操作，则扩充内容会更加复杂：

```c++
Point3d* Point3d::Point3d(Point3d *this, bool __most_derived, float x, float y, float z)
{
	if (__most_derived!=false) this->Point::Point(x, y);
	this_>__vptr_Point3d = __vtb1_Point3d;
	this->__vptr_Point3d_Point = __vtb1_Point3d__Point;
	this->_z = rhs._z;
	return ;
}
```

在更深层的继承情况下，比如Vertex3d调用Point3d和Vertex的构造函数时会将__most_derived设为false，压制其对Point构造的调用操作。总结一下，显然只有完整的对象被定义出来虚基类的构造才会被构造，而如果只是子对象，那么就不会调用虚基类的构造。

有这样一个问题，在构造函数中如果调用自己的成员函数，而该成员函数又调用了虚函数该如何确定，答案是通过vt，而vt又是通过vptr来控制的，因此只需要控制vptr的初始化和设定操作。实际中，vptr的初始化在基类初始化之后，而在用户代码或者成员初始化列表中的成员初始化之前。构造函数的执行算法通常如下：

1. 在派生类构造函数中，所有虚基类以及上一层基类的构造函数被调用；
2. 上述完成后对象的vptr被初始化，指向相应的vt；
3. 如果有成员列表初始化则在构造函数中扩展开来；
4. 最后执行程序员提供的代码；

例如有如下的代码：

```c++
PVertex::PVertex(float x, float y, float z)
	: _next(0), Vertex3d(x, y, z),
	Point(x, y)
{
	...
}
```

其可能会被扩展为：

```c++
PVertex::PVertex(PVertex* this, bool __most__derived, float x, float y, float z)
{
    if (__most__derived != false) this->Point::Point(x, y);
    this->Vertex3d::Vertex3d(x, y, z);
	this->__vptr_PVertex = __vtbl_PVertex;
	this->__vptr_Point__PVertex = __vtb1_Point__PVertex;
	...
	return this;
}
```

vptr在以下两种情况下必须被设定：当一个完整的对象被构建，例如声明一个Point对象，则Point对象的构造函数必须设定其vptr；当一个子对象构造函数调用了一个虚函数（不论是直接调用还是间接调用）。现在假设设定如下的构造函数：

```c++
Point::Point(float x, float y): _x(x), _y(y) {}
Point3d::Point3d(float x, float y, float z): Point(x, y), _z(z) {}
```

显然其vptr将不再需要在每一个基类构造函数中设定。解决方法是将构造函数分离为一个完整的对象实例和一个子对象实例，子对象实例中的vptr的设定可以省略。

接下来讨论对象复制的语意，还是以Point为例：

```c++
class Point {
public:
	Point(float x=0.0, float y = 0.0);
    
protected:
	float _x, _y;
};
```

如果要拷贝一个Point的话，显然默认行为已经足够，只有在默认行为导致的语意不安全或不正确的时候我们才需要设计一个拷贝复制操作符。一个类对于默认的拷贝复制操作，在以下的情况下不会表现出bitwise语意：

1. 当类内含一个成员对象，而其类内有拷贝复制操作；
2. 当一个类的基类有一个拷贝复制操作；
3. 当一个类声明了任何虚函数时（一定不能拷贝右端的vptr地址，因为其可能是派生类对象）；
4. 当类继承自一个虚基类时（不论虚基类有无拷贝操作）；

对于这样的赋值操作：

```c++
Point a, b;
a = b;
```

由bitwise copy完成，其间并没有拷贝赋值操作被调用。注意，还是提供一个拷贝构造比较好，为的是把NRV优化打开，但拷贝构造的出现不代表一定要提供拷贝赋值操作符。

现在引入拷贝赋值操作：

```c++
inline Point& Point::operator=(const Point &p) {
	_x = p._x;
	_y = p._y;
	reture *this;
}
```

现在派生一个Point3d类：

```c++
class Point3d: virtual public Point {
public:
	Point3d(float x=0.0, float y=0.0, float z=0.0);
protected:
	float _z;
};
```

如果没有为Point3d定义一个拷贝赋值操作，编译器就必须合成一个：

```c++
inline Point3d& Point3d::operator=(Point3d* const this, const Point3d &p) {
	this->Point::operator=(p);
	_z = p._z;
	return *this;
}
```

注意，拷贝赋值操作没有类似初始化列表的东西，也就没有办法压抑上一层基类的拷贝赋值操作。那么对于前文所述的继承关系，以下是Vetex3d的拷贝赋值操作：

```c++
inline Vertex3d& 
Vertex3d::operator=(const Vertex3d &v) {
	this->Point::operator=(v);
	this->Point3d::operator=(v);
	this->Vertex::operator=(v);
	...
}
```

显然需要在Point3d和Vertex中压抑Point的拷贝赋值操作，但又不能像在构造函数那样的解决方案（附上额外的参数），因为和构造和析构不同的是，取拷贝赋值操作符的地址完全合法，这意味着以下代码合法：

```c++
typedef Point3d& (Point3d::*pmfPoint3d)(const Point3d&);
pmfPoint3d pmf = &Point3d::operator=;
(x.*pmf(x));
```

然而我们无法支持，因为仍然需要根据继承体系安插任何可能个参数给拷贝构造操作符。

另一个方法是为拷贝赋值操作符产生分化函数，以支持这个类成为中间的类或最终的派生类。

事实上，拷贝赋值操作在虚继承的情况下行为不佳，容易出现在每一个中间的拷贝赋值操作中调用每一个基类的实例。建议不要允许虚基类的拷贝操作，甚至虚基类不要提供任何数据。

再来讨论一下析构语意学，如果类没有定义析构，那么只有在类内含的成员对象（或基类）拥有析构的情况下，编译器才会自动合成一个出来，否则被视为不需要，也就不需要合成。一个程序员定义的析构被扩展的方式类似构造函数被扩展的方式，但顺序相反：

1. 执行虚构的函数本体；
2. 如果类拥有类成员对象，后者拥有析构函数，那么会以其声明相反的顺序进行析构；
3. 如果类内含一个vptr，首先重设相关的虚表；
4. 如果有任何直接的非虚基类拥有析构，它们会以其声明顺序相反的顺序析构；
5. 如果有任何虚基类拥有析构，而此时所说的类处于最尾端，那么它们会以其原来的构造顺序的相反顺序被调用；

像构造函数一样，对于析构的一种最佳实现策略是维护两份析构实例：

1. 一个完整对象实例，总是设定好vptr，并调用虚基类的析构；
2. 一个基类子对象实例，除非在析构函数中调用一个虚函数，否则它绝不会调用虚基类析构并设定vptr。

## 执行期语意学

有以下的式子`if (yy == xx.getValue()) ...`。其中xx与yy都是类，则其可能扩张为：

```c++
{
	X temp1 = xx.getValue();
	Y temp2 = temp1.operator Y();
	int temp3 = yy.operator==(temp2);
	if (temp3) ...
	temp2.Y::~Y();
	temp1.X::~X();
}
```

一般而言，会把对象尽可能的放在使用它的那个程序区段附近，这样做可以节省非必要的对象产生操作和摧毁操作。

但对于全局变量，如下：

```c++
Matrix identity;
main() {
	Matrix m1 = identity;
	...
	return 0;
}
```

C++保证在main函数第一次用到identity之前把identity构造出来，而在main函数结束之前把identity摧毁掉。像identity这样所谓的全局对象如果有构造和析构的话需要静态的初始化操作和内存释放操作。最早的静态初始化以及内存释放方法是munch策略：

1. 为每一个需要静态初始化的文件产生一个__sti()函数，内含必要的构造操作或inline expansions；
2. 类似的，为每一个需要静态的内存释放操作产生一个__std()函数，内含必要的析构操作或者inline expansions；
3. 提供一组runtime library "munch"函数：一个_main()函数（用以调用可执行文件中的所有\_\_sti()函数），以及一个exit()函数（以类似方式调用所有的\_\_std()函数）；

![5-1](..\image\inside_c++\5-1.png)

最后一个需要解决的问题是如何收集一个程序中各个目标文件的\_\_sti()函数和\_\_std()函数，解决方法是使用nm命令，其会dump出目标文件的符号表格项目。之后munch程序会搜寻\_\_sti和\_\_std开头的名称，然后把函数名称加到一个sti和std函数的跳离表格，接下来把这个表格写到一个小的program text文件中，CC命令将被重新激活，将这个内含表格的文件加以编译，整个可执行文件被重新链接。_main()和exit()于是在各个表格上走访一遍，轮流调用每一个项目。对于虚基类，也会需要编译器提供必要的支持，以支持类对象的静态初始化。

对于局部静态变量，如：

```c++
const Matrix& identity() {
	static Matrix mat_identity;
	...
	return mat_identity;
}
```

对于局部静态变量来说：

1. mat_identity的构造函数只能施行一次；
2. mat_identity的析构函数必须只能施行一次；

cfront中的做法是导入一个临时性对象来保护mat_identity的初始化操作，第一次处理identity时，临时对象被评估为false，调用构造函数，之后临时对象改为true，析构同理。但仍然有一个困难，那就是cfront产生c码，mat_identity对于函数来说仍然是局部的，因此没有办法在静态的内存释放函数中存取它，解决方法是取出局部对象的地址。

对于对象数组，一般是建立储存空间，之后对每一个元素施加构造函数，在cfront中是使用了一个vec_new函数，如下：

```c++
void*
vec_new(
	void *array;        //数组起始地址
	size_t elem_size;   //每一个类对象的大小
	int elem_count;     //数组中元素个数
	void (*constructor)(void*),
	void (*destructor)(void*, char)
)
```

array持有的若不是具名数组的地址，就是0，若是0，那么数组将经由new，被动态的配置于heap中。相对应的，析构时会使用一个vec_delete函数：

```c++
void*
vec_delete(
	void *array;        //数组起始地址
	size_t elem_size;   //每一个类对象的大小
	int elem_count;     //数组中元素个数
	void (*destructor)(void*, char)
)
```

如果赋予了一些初值，则这些初值不会调用vec_new，而未赋值的则会调用。原来支持声明一个带参构造函数的类对象的数组是不允许的，为了改进这点，cfront会产生一个内部的stub constructor，没有参数，在这个函数内调用用户提供的构造函数。

运算符new的使用，其实是分成了两个部分：

1. 通过new运算符函数实例，配置所需要的内存，`int *p = \_\_new(sizeof(int));`
2. 将配置得来的对象设置初值，`*pi = 5;`

合起来就是：`int *pi = new int(5);`

delete也类似，如`delete pi；`会转换为:

```c++
if (pi != 0) __delete(pi);
```

但注意，pi不会被清除为0。

一般来说，对于new的实现都比较简单明了，比如：

```c++
extern void* 
operator new(size_t size) {
	if (size == 0) size = 1;
	void *last_alloc;
	while(!(last_alloc = malloc(size))) {
		if (_new_handler) (*_new_handler)();
        else return 0;
	}
	return last_alloc;
}
```

注意，虽然`new T[0]`的写法合法，但语言要求每一次对new的调用都必须传回一个独一无二的指针，这也是为什么size被设为1的原因。

当这么写时：

```c++
int *p_array = new int[5];
```

vec_new不会被调用，因为其主要功能是把默认构造施加于数组的每个元素上，所有只有new运算符会调用：

```c++
int *p_array = (int*)__new(5*sizeof(int));
```

如果类定义了默认构造，某些版本的vec_new就会被调用。

在C++2.0版之前，需要将数组的大小提供给delete，在2.1的时候，我们只需要这样写就可以`delete [] p_array；`这是为了delete的效率，如果没有中括号，那我们默认只有一个元素需要删除。

对于继承来说，比如：

```c++
class Point3d: public Point {...}
Point *ptr = new Point3d[10];
delete [] ptr
```

这是错误的，因为以为删除的是Point类型的元素，但实际中存储的是Point3d类型的，因此，要避免以一个基类指针指向一个派生类对象所组成的数组。

有一个预先定义好的重载new操作符，称为placement operator new，它需要第二个参数，类型为void*，调用方式如下：

```c++
Point2w *ptw = new(arena) Point2w;
```

其中arena指向内存中的区块。

下面讨论临时对象，对于以下的形式：

```c++
T c = a + b;
```

其中加法运算符被定义为：

```c++
T operator+(const T&, const T&);
//或者如下的形式
T T::operator+(const T&);
```

那么实现时根本不会产生临时对象，但如果是以下的形式：

```c++
c = a + b;
```

那么就不能忽略临时性对象，会导致下面的结果：

```c++
T temp;
temp.operator+(a, b);
c.operator=(temp);
temp.T::~T();
```

另外，对于`a + b`，也会产生一个临时对象来放置运算后的结果。临时对象的生命规则有两个例外，一个例外是发生在表达式用来初始化一个对象的时候，此时应该留存到初始化完成之后才能被摧毁，第二个是临时性对象被绑定到引用，那么知道被引用的对象的生命结束才能被摧毁。

## 站在对象模型的顶端

对一个指针来说，比如`Point<float> *ptr = 0;`其并不会实例化一个对象，而如果是引用，既`const Point<float> &ref = 0;`其真的会实例化出来一个实例，会扩展如下：

```c++
Point<float> temporary(float(0));
const Point<float> &ref = temporary;
```

这是因为0被转化为了Point<float>对象。

注意，模板类中的成员函数只有被用到的时候才会被实例化。有关类型的错误，模板只会在实例化的时候才进行检查。

有如下的两种情况，一种是模板定义所在的范围，一种是模板实例所在的范围，如下：

```c++
extern double foo(double);
template <class type>
class ScopeRules
{
public:
	void invariant() {
		_member = foo(_val);
	}
	type type_dependent() {
		return foo(_member);
	}
private:
	int _val;
	type _member;
};

extern int foo(int);
ScopeRules<int> sr0;
sr0.invariant();
```

此时调用的会是哪个foo函数？答案是double foo(double)。模板之中，对于一个非模板成员名的决议结果，是根据这个名字是否跟实例化模板的参数有关而决定的，如果使用互不相干，则以模板定义所在的范围来决定名字，如果相关，则以模板实例所在的范围来决定名字。

当一个异常发生，编译系统必须完成以下的事情：

1. 检验发生throw操作的函数；
2. 决定throw操作是否发生在try区段；
3. 若是，编译系统必须把异常类型拿来和每一个catch子句进行比较；
4. 如果比较后吻合，流程控制交到catch子句；
5. 如果throw的发生不在throw区段，或没有一个catch子句吻合，则必须摧毁所有active local objects，从堆栈中将目前的函数unwind掉，进行到程序堆栈的下一个函数中，重复以上过程；

对于一个这样的catch子句：

```c++
catch(exPoint p) {
	...
	throw;
}
```

假设抛出的是exVertex，派生自exPoint，那么再次抛出的还是exVertex，任何对p的修改都会抛弃。

对于以下的继承关系：

```c++
class node {...};
class type: public node {...}
class fcn: public type { ... }
class gen: public type { ... }
```

当给出一个type*类型的成员，我们就需要判断其派生类到底是哪个。在2.0之前，除了析构函数之外唯一不能被重载的就是conversion运算符，因为它们不使用参数。当引进了const成员函数之后，情况有了变化，以下的声明是可能的：

```c++
class String {
public:
	operator char*();
	operator char*() const;
};
```

也就是说，在之前以一个explicit cast来存取派生对象总是安全的，比如：

```c++
typedef type *ptype;
typedef fct *pfct;
simplify_conv_op(ptype pt) {
	pfct pf = pfct(pt);
}
```

此时如果pt是一个gen指针，那么也是安全的，但2.0之后这是错误的。这种基类转换成派生类被称为向下转换。想要支持安全的向下转换，对象空间和执行空间都需要一些负担：

- 需要额外的空间存储类型信息，通常是一个指针，指向某个类型信息节点；
- 需要额外的时间决定执行期的类型；

C++的RTTI机制为了提供安全的向下转换，将于该类相关的RTTI对象放进虚表中（通常在第一个slot中）。dynamic_cast可以在执行期决定真正的类型，如果向下转换安全，则这个运算符会传回适当转换过的指针，如果向下转换不安全，这个运算符会传回0。dynamic_cast的真正成本是一个类型描述器会被编译器产生出来。

如果将dynamic_cast运算符运用于引用身上，会跟指针的行为不一样，因为不能给引用传回一个0，那意味着产生一个临时性对象，该临时对象的初值为0。取而代之会发生以下的事情：

- 如果引用真正参考到适当的派生类，则向下转换会安全的执行；
- 如果引用并不真正是一种派生类，那么会抛出bad_cast异常；

使用typeid运算符，就有可能以一个引用达到相同的执行期替代路线：

```c++
simplify_conv_op(const type &rt) {
	if(typeid(rt) == typeid(fct)) {
		fct &rf = static_cast<fct&>(rt);
	}
	else {...}
}
```

