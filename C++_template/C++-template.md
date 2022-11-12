# C++模板

C++模板主要是为了泛型存在的，避免了重复造轮子。模板是为了一种或多种未明确定义的类型而定义的函数或类，在使用模板时，需要显式或隐式的指定模板参数。由于模板是C++语言特性，因此支持类型和作用域检查。

## 第一章 函数模板

函数模板是被参数化的函数，因此是具有相似功能的函数族。

以下是一个模板的示例：

```c++
template<typename T>
T max(T a, T b)
{
	return b < a ? a : b;
}
```

注意，在本例中T必须支持小于运算符，另外为了支持返回值，T还应该是可拷贝的。由于历史原因，这里也可以用class来代替typename，但应优先用typename。

在编译阶段，模板并不是编译成一个实体，而是每一个用于该模板的类型都会产生一个独立的实体，由具体参数取代模板类型参数的过程叫做实例化。

模板在编译时是分两步进行编译，在模板定义阶段，编译时的检查不包含类型参数的检查，只包含以下几个方面：

- 语法检查，例如少了分号；
- 使用了未定义的不依赖于模板参数的名称；
- 未使用模板参数的static assertions；

在模板实例化阶段，编译时会检查那些依赖类型参数的部分。

两阶段的编译检查给模板的处理带来了一个问题：当实例化一个模板时，编译器需要（一定程度）看到模板的完整定义，这不同于函数编译和链接分离的思想，函数在编译阶段只声明就够了。之后会讨论如何处理这个问题，这里先采用最简单的办法，将模板的实现写在头文件里。

在类型推断时的自动转换是有限制的：

- 如果调用参数是按照引用传递，则任何类型转换都不允许；
- 如果调用参数是按值传递，那么只有退化这样简单的转换才被允许：const和volatile限制符会被忽略。

解决参数推断时的错误，我们可以对参数进行类型转换，也可以显式指出类型参数的类型，如`max<double>(4, 7.2)`，也可以指明参数可以有不同的类型（多个模板参数）。

注意，类型推断并不适用于默认调用参数：

```c++
template<typename T>
void f(T="");
...
f(1); //OK
f(); //ERROR，无法推断T的类型
```

为了应付这种情况，也需要给模板类型参数也声明一个默认参数，如下：

```c++
template<typename T = std::string>
void f(T = "");
...
f();
```

对于多个模板参数的情况，容易出现以下的问题：

```c++
template<typename T1, typename T2>
T1 max(T1 a, T2 b)
{
	return b < a ? a : b;
}
...
auto m = ::max(4, 7.2);
```

返回的类型跟调用顺序有关，要解决这个问题，有以下的方法：

- 引入第三个模板参数作为返回类型；
- 让编译器找到返回类型；
- 将返回类型定义为两个参数类型的公共类型；

引入第三个模板参数作为返回值类型可行，但模板本身不会推断，因此必须显式指明模板参数的类型，像这样：

```c++
template<typename T1, typename T2, typename RT>
RT max(T1 a, T2 b);
...
::max<int, double, double>(4, 7.2);
```

这样太繁琐，我们指定的参数只要满足模板推断的需要就可以了，例如：

```c++
::max<double>(4, 7.2) //OK: 返回类型是double，T1 和T2 根据调用参数推断
```

也可让编译器来做这件事，从C++14开始这成为可能，不过要声明返回类型为auto：

```c++
template<typename T1, typename T2>
auto max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

如果是C++11，就需要借助尾置返回类型，如下：

```c++
template<typename T1, typename T2>
auto max(T1 a, T2 b) -> decltype(b<a?a:b)
```

编译器会根据实际结果来决定实际返回类型，实际上用true作为?:的条件就足够了：

```c++
template<typename T1, typename T2>
auto max(T1 a, T2 b) -> decltype(true?a:b)
```

但在某些情况下会有严重的问题，由于T可能是引用类型，返回类型也可能被推断为引用类型，因此返回的应该是decay后的T，如下：

```c++
#include <type_traits>
template<typename T1, typename T2>
audo max(T1 a, T2 b) -> typename std::decay<decltype(true?a:b)>::type
{
	return b < a ? a : b;
}
```

这里使用了类型萃取，由于其type成员是一个类型，为了获取其结果，需要用typename修饰这个表达式。注意，初始化auto变量跟返回类型是auto的时候（与上述矛盾，需要后续验证），其类型总是退化了后的类型。

定义公共类型需要使用std::common_type<>::type，产生的类型是模板参数的公共类型，如下：

```c++
#include <type_traits>
template<typename T1, typename T2>
std::common_type_t<T1,T2> max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

这也是一个类型萃取，定义在<type_traits>中，它返回一个结构体，结构体的type成员被用作目标类型，应用场景如下：

```c++
typename std::common_type<T1, T2>::type //since C++11
```

从C++14开始，可以简化用法，在后面加个_t就可以省掉typename和::type。注意，其结果也是退化的。

也可以给模板参数指定默认值，这些默认值甚至可以根据前面的参数来决定自己的类型，比如给返回类型引入一个模板参数RT，并将其声明为其他参数的公共类型，比如：

```c++
template<typename T1, typename T2, typename RT = std::decay_t<decltype(true ? T1() : T2())>>
RT max(T1 a, T2 b)
{
	return b < a ? a : b;
}
```

注意，这种实现方式要求我们能够调用模板参数的默认构造函数，还有另一种方式std::declval，后续再讨论。

也可以利用类型萃取std::common_type<>作为返回类型的默认值：

```c++
#include <type_traits>
template<typename T1, typename T2, typename RT =
std::common_type_t<T1,T2>>
RT max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

也可以采用如下的方式：

```c++
template<typename RT = long, typename T1, typename T2>
RT max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

基于这个定义，可以如下的调用：

```c++
int i; long l;
…
max(i, l); // 返回值类型是long (RT 的默认值)
max<int>(4, 42); //返回int，因为其被显式指定
```

这样其实也过于麻烦，最简单的办法就是让编译器来推断返回类型。

函数模板也可以重载，一个非模板函数可以与一个同名函数共存。有如下示例：

```c++
int max(int a, int b)
{
	return b < a ? a : b;
}
template<typename T>
T max(T a, T b)
{
	return b < a ? a : b;
}
```

在所有其他因素相同的情况下，模板解析过程优先选择非模板函数，而不是从模板实例化出来的函数，如果模板可以实例化出一个更匹配的函数，那么会选择这个模板。也可以显式指定一个空的模板参数，这表明它会被解析成一个模板调用，如下：

```c++
::max<>(7, 42);
```

由于模板参数推断时不允许自动类型转换，而常规函数是允许的，因此以下的调用会选择非模板参数：

```c++
::max("a", 42.7);
```

一个有趣的例子是我们可以为max实现一个显式指定返回值类型的模板：

```c++
template<typename T1, typename T2>
auto max (T1 a, T2 b)
{
	return b < a ? a : b;
}
template<typename RT, typename T1, typename T2>
RT max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

有以下的调用例子：

```c++
auto a = ::max(4, 7.2); // uses first template，因为不满足三个参数的模板
auto b = ::max<long double>(7.2, 4); // uses second template，因为不满足第一个模板的参数条件
auto c = ::max<int>(4, 7.2); // ERROR: both function templates match
```

以上，所以我们要保证重载模板时对任意一个调用，都只会有一个模板匹配。注意在重载模板时，我们要尽可能的少做改动，应该仅仅是改动模板参数的个数或者显式的指定某些模板参数，否则可能会遇到意想不到的问题，如下：

```c++
#include <cstring>
// maximum of two values of any type (call-by-reference)
template<typenameT> T const& max (T const& a, T const& b)
{
	return b < a ? a : b;
}/
/ maximum of two C-strings (call-by-value)
char const* max (char const* a, char const* b)
{
	return std::strcmp(b,a) < 0 ? a : b;
}
// maximum of three values of any type (call-by-reference)
template<typename T>
T const& max (T const& a, T const& b, T const& c)
{
	return max (max(a,b), c); // error if max(a,b) uses call-by-value
}
int main ()
{
    auto m1 = ::max(7, 42, 68); // OK
    char const* s1 = "frederic";
    char const* s2 = "anica";
    char const* s3 = "lucas";
    auto m2 = ::max(s1, s2, s3); //run-time ERROR
}
```

这里先发生的是调用按值传递的max模板，之后的max调用时返回一个引用，由于这个引用指向一个返回的临时变量，因此会释放掉发生段错误。

另外，需要保证函数模板在调用时，其已经在前方定义，例如：

```c++
#include <iostream>
// maximum of two values of any type:
template<typename T>
T max (T a, T b)
{
    std::cout << "max<T>() \n";
    return b < a ? a : b;
}
// maximum of three values of any type:
template<typename T>
T max (T a, T b, T c)
{
	return max (max(a,b), c); // uses the template version even for ints
} //because the following declaration comes
// too late:
// maximum of two int values:
int max (int a, int b)
{
    std::cout << "max(int,int) \n";
    return b < a ? a : b;
}
int main()
{
	::max(47,11,33); // OOPS: uses max<T>() instead of max(int,int)
}
```

通常而言，建议将按引用传递用于除简单类型以外的类型，这样可免除不必要的拷贝成本，不过由于以下原因，按值传递更好一些：

- 语法简单；
- 编译器能够更好的优化；
- 移动语义通常使拷贝成本较低；
- 某些情况下甚至没有拷贝或者移动；

另外，对于模板还有一些特殊情况：

- 模板既可以用于复杂类型，也可用于简单类型，如果默认复杂可能对简单类型产生不利影响；
- 可自己选择使用std::ref()和std::cref()来按引用传递参数；
- 虽然按值传递string literal和raw array经常遇到问题，但按照引用传递通常问题更大；

通常而言，函数模板不需要被声明成inline，因为其也可以定义在头文件中被多个编译单元include，唯一一个例外，是模板对某些类型的全特化，留待下文进行讨论。

## 第二章 类模板

在类的内部，使用类型时不带<T>代表类型和模板类的参数类型相同，如下：

```c++
template<typename T>
class Stack {
	...
	Stack(Stack const&);
	Stack& operator=(Stack const&);
	...
};
```

但如果是在类外，则需要如下表示：

```c++
template<typename T>
bool operator==(Stack<T> const& lhs, Stack<T> const& rhs);
```

注意在只需要类的名字而不是类型的地方，可以只用Stack，这和声明构造函数和析构函数的情况相同。

**注意**，不同于非模板类，不可以在函数内部或块作用域内声明和定义模板，通常模板只能定义在global或namespace作用域，或者其他类的声明里面。

直到C++17，使用模板仍需要显式指明模板参数，如下：

```c++
Stack<int> intStack;
```

注意，模板函数和模板成员函数只有在被调用时候才会实例化。如果一个类模板有static成员，对每一个用到这个模板的类型，相应的静态成员也只会被实例化一次。

模板参数可以是任意类型，唯一的要求是要支持模板中用到的各种操作。

可以部分的使用类模板，因为模板参数只需要提供哪些会被用到的操作，对于已经定义的操作而我们显式指定的参数却不支持，只要我们不执行这些特定的操作就不会报错。这带来了一个问题，我们如何知道参数满足实例化一个模板需要哪些限制？例如C++标准库是基于这样一些concepts：可随机进入的迭代器和可默认构造。从11开始，我们至少可以通过关键字static_assert和其他一些预定义的类型萃取来做一些简单的检查，如：

```c++
template<typename T>
class C
{
	static_assert(std::is_default_constructible<T>::value, "Class C requires default-constructible elements");
	...
}
```

如果想要打印stack的内容，可以重载stack的operator<<运算符，其应该被实现为非成员函数，如下：

```c++
template<typename T>
class Stack {
    …
    void printOn() (std::ostream& strm) const {
    	…
    }
    friend std::ostream& operator<< (std::ostream& strm, Stack<T>
    const& s) {
        s.printOn(strm);
        return strm;
    }
};
```

这里operator<<并不是一个函数模板，而是在需要的时候随类模板实例化的一个常规函数（友元是已经针对特定类型实例化的函数）。如果我们选择先声明一个友元函数再去定义它，情况会变得复杂，有以下两种选择：

1. 可以隐式声明一个新的函数模板，但必须使用一个不同于类模板的模板参数，如：

   ```c++
   template<typename T>
   class Stack {
   	template<typename U>
       friend std::ostream& operator<< (std::ostream&, Stack<U>
       const&);
   };
   ```

2. 也可以先将Stack<T>的operator<<声明为一个模板，这要求先对Stack<T>进行声明：

   ```c++
   template<typename T>
   class Stack;
   template<typename T>
   std::ostream& operator<< (std::ostream&, Stack<T> const&);
   template<typename T>
   class Stack {
       …
       friend std::ostream& operator<< <T> (std::ostream&, Stack<T>
       const&);
   }
   ```

和函数模板的重载类似，类模板的特化允许对某一特定类型做优化，或者去修正类模板针对某一特定类型实例化之后的行为。虽然允许只特例化模板类的一个成员函数，但一旦这样做就无法再去特化那些未被特化的部分。特化示例如下：

```c++
template<>
class Stack<std::string> {}
```

类模板可以只被部分特例化，这样就可以为某些特殊的情况提供特殊的实现，不过使用者还是要定义一部分模板参数，例如：

```c++
#include "stack1.hpp"
// partial specialization of class Stack<> for pointers:
template<typename T>
class Stack<T*> {
Private:
	std::vector<T*> elems; // elements
public:
    void push(T*); // push element
    T* pop(); // pop element
    T* top() const; // return top element
    bool empty() const { // return whether the stack is empty
        return elems.empty();
    }
};
template<typename T>
void Stack<T*>::push (T* elem)
{
	elems.push_back(elem); // append copy of passed elem
}
template<typename T>
T* Stack<T*>::pop ()
{
    assert(!elems.empty());
    T* p = elems.back();
    elems.pop_back(); // remove last element
    return p; // and return it (unlike in the general case)
}
template<typename T>
T* Stack<T*>::top () const
{
    assert(!elems.empty());
    return elems.back(); // return copy of last element
}
```

通过以下代码来实现一个依然被类型T参数化，但被用来特化处理指针的类模板：

```c++
template<typename T>
class Stack<T*> {};
```

多模板参数也可以部分特例化，如下：

```c++
template<typename T1, typename T2>
class MyClass {
	…
};
// partial specialization: both template parameters have same type
template<typename T>
class MyClass<T,T> {
    …
};
// partial specialization: second type is int
template<typename T>
class MyClass<T,int> {
    …
};
// partial specialization: both template parameters are pointer types
template<typename T1, typename T2>
class MyClass<T1*,T2*> {
    …
};
```

如果有不止一个特例化的版本可以匹配，说明定义是有歧义的，为了消除这种歧义，可以提供一个单独的特例化版本来处理相同类型的指针。

和函数模板一样，也可以为类模板提供默认参数。如下：

```c++
template<typename T, typename Cont = std::vector<T>>
class Stack {
	...
}
```

给模板定义一个类型别名有两种方法可用，一种是使用typedef，例如：

```c++
typedef Stack<int> IntStack;
```

第二种是使用using（从11开始）：

```c++
using IntStack = Stack<int>
```

以上两种给一个已经存在的类型定义一个新名字的方式称为type alias declaration，新的名字称为type alias。不同于typedef，alias declaration也可以被模板化，这一特性从11开始，称为alias templates。例如：

```c++
template<typename T>
using DequeStack = Stack<T, std::deque<T>>;
```

成员的别名模板可以使我们很方便的为成员类型定义一个快捷方式，如下：

```c++
template<typename T>
using MyTypeIterator = typename MyType<T>::iterator;
```

从14开始，利用上面的技术，标准库所有返回一个类型的type trait定义了快捷方式，比如：

```c++
std::add_const_t<T> 
//而不是
typename std::add_const<T>::type
```

直到C++17，使用类模板都必须显式指出所有的模板参数的类型（除非有默认值），从17开始这一要求不再那么严格，如果构造函数能够推断出所有模板参数的类型，就不再需要显式指明模板参数的类型。例如以下关于拷贝构造函数的例子：

```c++
Stack< int> intStack1; // stack of strings
Stack< int> intStack2 = intStack1; // OK in all versions
Stack intStack3 = intStack1; // OK since C++17
```

通过提供一个接受初始化参数的构造函数，就可以推断出Stack的元素类型，如下：

```c++
template<typename T>
class Stack {
private:
	std::vector<T> elems; // elements
public:
    Stack () = default;
    Stack (T const& elem) // initialize stack with one element
    : elems({elem}) {
    }
    …
};

Stack intStack = 0; // Stack<int> deduced since C++17
```

通过0初始化这个stack时模板参数被推断为int，但要注意以下的细节：

- 由于定义了int作为参数的构造函数，要记得要编译器要求生成默认构造函数及其全部默认行为，这是因为默认构造函数只有在没有定义其他构造函数的情况下才会默认生成；
- 初始化elems时采用了初始化列表，相当于用{elem}来初始化elems，这是因为vector没有接受一个参数的构造函数；

当通过字符串常量来初始化Stack时，会带来一堆的问题，如：

```c++
Stack stringStack = "bottom";
```

由于参数是按照T的引用进行传递，那么参数类型不会退化，相当于初始化了一个这样的Stack：

```c++
Stack<char const[7]>
```

这样我们就不能继续向Stack追加一个不同维度的字符串常量了。如果参数是按值传递的，参数类型就会被退化，裸数组将退化为裸指针，这样参数类型会是char const*，基于以上原因，有必要将构造函数声明改为值传递方式，如下：

```c++
template<typename T>
class Stack {
private:
	std::vector<T> elems; // elements
public:
    Stack () = default;
    Stack (T elem) // initialize stack with one element by value
    : elems({elem}) {
    }
    …
};
```

针对以上的问题，除了将构造函数改为按值传递，还有一个方案：由于在容器中处理裸指针容易导致很多问题，对于容器一类的类，不应该将类型推断为字符的裸指针，可以通过提供”推断指引“来提供额外的模板参数推断规则，或者修正已有的模板参数推断规则。比如我们可以定义，当传递一个字符串常量或者C类型的字符串时，应该用std::string实例化Stack模板类：

```c++
Stack(char const*) -> Stack<std::string>;
```

这个指引语句必须出现在和模板类的定义相同的作用域或命名空间内，它通常紧跟着模板类的定义，->后面的类型被称为推断指引的"guided type"。现在有以下的结果：

```c++
Stack stringStack{"bottom"}; //OK
Stack stringStack = "bottom"; //fail
```

模板参数推断的结果也是可以拷贝的，在将stringStack声明为Stack\<std::string\>之后，下面初始化的结果也是正确的：

```c++
Stack stack2{stringStack};
```

聚合类（这样一类class或者struct：没有用户定义的显式的或继承来的构造函数，没有private或者protected的非静态成员，没有虚函数，没有virtual，private或者protected的基类）也可以是模板。比如：

```c++
template<typename T>
struct ValueWithComment {
	T value;
	std::string comment;
};
```

从C++17开始，对于聚合类的模板甚至可以使用类型推断指引，如下：

```c++
ValueWithComment(char const*, char const*) -> ValueWithComment<std::string>;
ValueWithComment vc2 = {"hello", "initial value"};
```

标准库的std::array<>类也是一个聚合类。

## 非类型模板参数

对于非类型模板参数，待定的不再是类型，而是某个数值，在使用这种模板时需要显式的指出待定数值的具体值，之后代码会被实例化。

有如下的代码：

```c++
#include <array>
#include <cassert>
template<typename T, std::size_t Maxsize>
class Stack {
	private:
		std::array<T, Maxsize> elems;
		std::size_t numElems;
	public:
		Stack();
		void push(T const& elem);
		T const& top() const;
		bool empty() const {
			return numElems == 0;
		}
		std::size_t size() const {
			return numElems;
		}
};
template<typename T, std::size_t Maxsize>
Stack<T, Maxsize>::Stack(): numElems(0)
{}
template<typename T, std::size_t MaxSize>
void Stack<T, Maxsize>::push(T const& elem)
{
    assert(numElems < Maxsize);
    elems[numElems] = elem;
    ++numElems;
}
template<typename T, std::size_t Maxsize>
void Stack<T, Maxsize>::pop()
{
    assert(!elems.empty());
    --numElems;
}
template<typename T, std::size_t Maxsize>
T const& Stack<T, Maxsize>::top() const
{
    assert(!elems.empty());
    return elems[numElems-1];
}
```

对非类型模板参数，也可以指定默认值：

```c++
template<typename T=int, std::size_t Maxsize=100>
class Stack {
	...
};
```

同样，也可以为函数模板定义非类型模板参数，如：

```c++
template<int Val, typename T>
T addValue(T x)
{
	return x + Val;
}
```

同样的，也可以基于前面的模板参数推断出当前模板参数的类型，比如可以通过传入的非类型模板参数推断出返回类型：

```c++
template<auto Val, typename T = decltype(Val)>
T foo();
```

或者可以通过以下的方式确保传入的非类型模板参数的类型和类型参数的类型一致

```c++
template<typename T, T val = T{}>
T bar();
```

使用非类型模板参数是有限制的，通常它们只能是整型常量（包含枚举），指向object/functions/members的指针，objects或者functions的左值引用，或者是std::nullptr_t。浮点型数值和class类型的对象都不能作为非类型模板参数使用：

```c++
template<double VAT> // ERROR: floating-point values are not
double process (double v) // allowed as template parameters
{
return v * VAT;
}t
emplate<std::string name> // ERROR: class-type objects are not
class MyClass { // allowed as template parameters
…
};
```

当传递对象的指针或者引用作为模板参数时，对象不能是字符串常量，临时变量或者数据成员以及其他子对象，另外还有一些针对版本的限制：

- C++11，对象必须要有外部链接；
- C++14，对象必须是外部链接或者内部链接；

```c++
extern char const s03[] = "hi"; // external linkage
char const s11[] = "hi"; // internal linkage
int main()
{
    MyClass<s03> m03; // OK (all versions)
    MyClass<s11> m11; // OK since C++11
    static char const s17[] = "hi"; // no linkage
    MyClass<s17> m17; // OK since C++17
}
```

非类型模板参数可以是任何编译器表达式，如下：

```c++
template<int I, bool B>
class C;
...
C<sizeof(int)+4, sizeof(int)==4> c; 
```

如果在表达式中使用了operator>，就必须放在括号里面，防止>作为模板参数列表的末尾，从而截断列表，应采用如下写法：

```c++
C<42, (sizeof(int)>4)> c;
```

从C++17开始，可以不指定非类型模板参数的具体类型，可用auto来代替，比如有以下的示例：

```c++
#include <array>
#include <cassert>
template<typename T, auto Maxsize>
class Stack {
public:
	using size_type = decltype(Maxsize);
private:
    std::array<T,Maxsize> elems; // elements
    size_type numElems; // current number of elements
public:
    Stack(); // constructor
    void push(T const& elem); // push element
    void pop(); // pop element
    T const& top() const; // return top element
    bool empty() const { //return whether the stack isempty
    return numElems == 0;
    }
    size_type size() const { //return current number of elements
        return numElems;
    }
};
// constructor
template<typename T, auto Maxsize>
Stack<T,Maxsize>::Stack ()
: numElems(0) //start with no elements
{
	// nothing else to do
}
template<typename T, auto Maxsize>
void Stack<T,Maxsize>::push (T const& elem)
{
    assert(numElems < Maxsize);
    elems[numElems] = elem; // append element
    ++numElems; // increment number of elements
}
template<typename T, auto Maxsize>
void Stack<T,Maxsize>::pop ()
{
    assert(!elems.empty());
}

```

C++14开始，也可以使用auto，让编译器推断出具体返回类型。

从C++17开始，可以通过标准类型萃取std::is_same和decltype来验证两个类型是否一致：

```c++
if (!std::is_same<decltype(size1), decltype(size2)>::value) {
	std::cout << "size types differ" << "\n";
}
```

从17开始，对返回类型的类型萃取，可以通过使用下标_v省略掉::value，因此可以这样简写：

```c++
if (!std::is_same_v<decltype(size1), decltype(size2)>) {
	std::cout << "size types differ" << "\n";
}
```

也可以使用template\<decltype(auto)\>，将N实例化引用类型：

```c++
template<decltype(auto) N>
class C {
…
};
int i;
C<(i)> x; // N is int&
```

## 变参模板

即可以将参数定义成能接受多个模板参数的情况，比如：

```c++
#include <iostream>
void print() {}
template<typename T, typename... Types>
void print(T firstArg, Types... args) {
	std::cout << firstArg << "\n";
	print(args...);
}
```

也可以这样写：

```c++
#include <iostream>
template<typename T>
void print (T arg)
{
	std::cout << arg << ’\n’; //print passed argument
}
template<typename T, typename… Types>
void print (T firstArg, Types… args)
{
    print(firstArg); // call print() for the first argument
    print(args…); // call print() for remainingarguments
}
```

当两个模板的区别只在于尾部的参数包的时候，会优先选择没有尾部参数包的那个模板。

C++11为变参模板引入了新的sizeof运算符：sizeof...。它被扩展为参数包包含的参数数目。使用如下：

```c++
template<typename T, typename… Types>
void print (T firstArg, Types… args)
{
    std::cout << firstArg << ’\n’; //print first argument
    std::cout << sizeof…(Types) << ’\n’; //print number of remaining
    types
    std::cout << sizeof…(args) << ’\n’; //print number of remaining
    args
    …
}
```

如果我们不定义不接受参数的print函数，那么以下的代码就是错误的：

```c++
template<typename T, typename… Types>
void print (T firstArg, Types… args)
{
    std::cout << firstArg << ’\n’;
    if (sizeof…(args) > 0) { //error if sizeof…(args)==0
    	print(args…); // and no print() for no arguments declared
    }
}
```

这是因为是否使用实例化的代码是在运行期间决定的，而是否实例化代码确是在编译期决定的，因此如果在只有一个参数的时候调用该模板，if语句中的print也会被实例化，会出现报错。

从17开始，出现了折叠表达式，可以用来计算参数包（可以有初始值）中所有参数运算结果的二元运算符，比如有如下的代码：

```c++
template<typename… T>
auto foldSum (T… s) {
	return (… + s); // ((s1 + s2) + s3) …
}
```

如果参数包是空的，这个表达式将不合规范（不过此时对于运算符&&，结果会是true，对于||，结果会是false，对于逗号运算符，结果会是void()）。以下是可能的折叠表达式：

![fold_expression](..\image\template\fold_expression.png)

几乎所有的二元表达式都可以用于折叠表达式。

变参模块的应用很广泛，经常使用移动语义对象对参数进行完美转发，通常像如下声明：

```c++
namespace std {
template<typename T, typename… Args> shared_ptr<T>
make_shared(Args&&… args);
class thread {
public:
    template<typename F, typename… Args>
    explicit thread(F&& f, Args&&… args);
    …
};
template<typename T, typename Allocator = allocator<T>>
class vector {
public:
    template<typename… Args>
    reference emplace_back(Args&&… args);
    …
};
}
```

注意常规模板参数的规则同样适用于变参模板参数，比如参数是按值传递，参数会被拷贝，类型也会退化，如果引用传递，那么参数会是实参的引用且类型不会退化。

除了转发所有参数之外，还可以做些别的事情，比如计算它们的值：

```c++
template<typename... T>
void printDouble(T const&... args) {
	print(args + args...);
}
```

先翻倍，再调用print，如果是向每个参数加1，则可以如下做：

```c++
template<typename… T>
void addOne (T const&… args)
{
    print (args + 1…); // ERROR: 1… is a literal with too many decimal points
    print (args + 1 …); // OK
	print ((args + 1)…); // OK
}
```

编译阶段的表达式同样可以这样做：

```c++
template<typename T1, typename… TN>
constexpr bool isHomogeneous (T1, TN…)
{
	return (std::is_same<T1,TN>::value && …); // since C++17
}
```

还可以通过变参下标来访问相应的元素，比如：

```c++
template<typename C, typename... Idx>
void printElems(C const& coll, Idx... idx)
{
	print(coll[idx]...);
}
std::vector<std::string> coll = {"good", "times", "say", "bye"};
printElems(coll,2,0,3);
```

也可以将非类型模板参数声明成参数包：

```c++
template<std::size_t... Idx, typename C>
void printIdx(C const& coll)
{
	print(coll[Idx]...);
}
std::vector<std::string> coll = {"good", "times", "say", "bye"};
printIdx<2,0,3>(coll);
```

类模板也可以是变参的，一个例子是通过任意多个模板参数指定了class相应数据成员的类型，如：

```c++
template<typename… Elements>class Tuple;
Tuple<int, std::string, char> t; // t can hold integer, string, and
character
```

另一个例子是指定对象可能包含的类型：

```c++
template<typename… Types>
class Variant;
Variant<int, std::string, char> v; // v can hold integer, string, or character
```

还可以将class定义成一组下标的类型，如下使用：

```c++
// type for arbitrary number of indices:
template<std::size_t…>
struct Indices {
};
template<typename T, std::size_t… Idx>
void printByIdx(T t, Indices<Idx…>)
{
	print(std::get<Idx>(t)…);
}
std::array<std::string, 5> arr = {"Hello", "my", "new", "!", "World"};
printByIdx(arr, Indices<0, 4, 3>());
//或者如下使用
auto t = std::make_tuple(12, "monkeys", 2.0);
printByIdx(t, Indices<0, 1, 2>());
```

推断指引也可以是变参的，比如C++中，为std::array定义了如下的推断指引：

```c++
namespace std {
	template<typename T, typename... U> array(T, U...)
	->array<enable_if_t<(is_same_V<T, U>&& ...), T>, (1+sizeof...(U))>;
}
```

针对这样的初始化`std::array a{42, 45, 77}`，会将指引中的T推断为array的首元素的类型，而U...会被推断为剩余元素的类型。其中有一个折叠表达式，可以展开为：

```c++
is_same_v<T, U1> && is_same_v<T, U2> && is_same_v<T, U3> ...
```

还可以用来定义变参基类，示例如下：

```c++
#include <string>
#include <unordered_set>
class Customer
{
private:
	std::string name;
public:
    Customer(std::string const& n) : name(n) { }
    std::string getName() const { return name; }
};
struct CustomerEq {
    bool operator() (Customer const& c1, Customer const& c2) const {
    return c1.getName() == c2.getName();
}
};
struct CustomerHash {
    std::size_t operator() (Customer const& c) const {
    return std::hash<std::string>()(c.getName());
}
};
// define class that combines operator() for variadic base classes:
template<typename… Bases>
struct Overloader : Bases…
{
	using Bases::operator()…; // OK since C++17
};
int main()
{
    // combine hasher and equality for customers in one type:
    using CustomerOP = Overloader<CustomerHash,CustomerEq>;
    std::unordered_set<Customer,CustomerHash,CustomerEq> coll1;
    std::unordered_set<Customer,CustomerOP,CustomerOP> coll2;
    …
}
```

## 基础技巧

typename用来澄清模板内部的一个标识符代表的是某种类型，而不是数据成员。