# C++模板

C++模板主要是为了泛型存在的，避免了重复造轮子。模板是为了一种或多种未明确定义的类型而定义的函数或类，在使用模板时，需要显式或隐式的指定模板参数。由于模板是C++语言特性，因此支持类型和作用域检查。

## 函数模板

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

## 类模板

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

### typename关键字

typename用来澄清模板内部的一个标识符代表的是某种类型，而不是数据成员。通常来说，一个类型依赖于模板参数，就必须用typename。使用typename的一种场景是用来声明泛型代码中标准容器的迭代器：

```c++
typename T::const_iterator pos;
typename T::const_iterator end(coll.end())
```

### 零初始化

对于基础类型，比如int、double以及指针类型，由于它们没有默认构造函数，因此不会被默认初始化一个有意义的值。比如任何未被初始化的局部变量都是未定义的：

```c++
void foo()
{
int x; // x has undefined value
int* ptr; // ptr points to anywhere (instead of nowhere)
}
```

因此在定义模板时，如果想要一个模板类型的变量被初始化成一个默认值，那么只是简单的定义是不够的，因为对内置类型，它们不会被初始化：

```c++
template<typename T>
void foo()
{
    T x; // x has undefined value if T is built-in type
}
```

因此，对于内置类型，最好显式调用其默认构造函数来将它们初始化为0，比如：

```c++
template<typename T>
void foo()
{
	T x{}; // x is zero (or false) if T is a built-in type
}
```

这种初始化的方法叫做值初始化，要么调用一个对象的已有的构造函数，要么就用0来初始化这个对象，即使它有显式的构造函数也是如此。

在11之前，确保一个对象得到显式初始化的方式是：

```c++
T x = T(); // x is zero (or false) if T is a built-in type
```

在17之前，只有在与拷贝初始化对应的构造函数没有被声明为explicit的时候，这一方法才有效，从17开始，由于强制拷贝省略（比如RVO和右值拷贝优化）的使用，这一限制被解除，因此17之后两种方法都有效，但使用花括号初始化的情况，如果没有可用的默认构造函数，还可以使用列表初始化构造函数。从11开始也可以使用这种方式对非静态成员进行默认初始化：

```c++
template<typename T>
class MyClass {
private:
	T x{};
}
```

但不可以对默认参数使用这一方式：

```c++
template<typename T>
void foo(T p{}) { //ERROR …
}
template<typename T>
void foo(T p = T{}) { //OK (must use T() before C++11) …
}
```

### 使用this->

对于类模板，如果其基类也是依赖于模板参数的，即使x是继承而来的，使用this->x和x也不一定等效，比如：

```c++
template<typename T>
class Base {
public:
	void bar();
};
template<typename T>
class Derived: Base<T> {
public:
	void foo() {
		bar(); //calls external bar() or error
	}
}
```

派生类中的bar()永远不会被解析成基类中的bar()，目前建议当使用定义于基类中的、依赖于模板参数的成员时，用this->或者Base<T>::来修饰。

### 使用裸数组或者字符串常量的模板

当向模板传递裸数组或者字符串常量时，需要格外注意以下的内容，第一如果参数是按引用传递，那么参数类型不会退化，也就是说，传递"hello"时模板类型会被推断为char const[6]，只有当按值传递时类型才会退化，会被推断为char const*。不过也可以定义如下专门处理裸数组或字符串的模板：

```c++
template<typename T, int T, int M>
bool less(T(&a)[N], T(&b)[M])
{
	for (int i = 0; i<N && i<M; ++i)
    {
    if (a[i]<b[i]) return true; if (b[i]<a[i]) return false;
    }
    return N < M;
}
```

当这样使用该模板时：

```c++
int x[] = {1, 2, 3};
int y[] = {1, 2, 3, 4, 5};
std::cout << less(x,y) << ’\n
```

less<>中的T会被实例化成int，N被实例化成3，M被实例化为5。

### 成员模板

一般来说，举例，即使两个stack的元素类型之家可以隐式转换，它们也不能互相赋值，除非将赋值运算符也定义成模板，如下：

```c++
template<typename T>
class Stack {
private:
	std::deque<T> elems;
public:
	void push(T const&);
	void pop();
	T const& top() const;
	bool empty() const {
		return elems.empty();
	}
	template<typename T2>
	Stack& operator=(Stack<T2> const&);
}
```

新的赋值运算符被定义成下面这样：

```c++
template<typename T>
template<typename T2>
Stack<T>& Stack<T>::operator=(Stack<T2> const& op2)
{
	Stack<T2> tmp(op2);
	elems.clear();
	while(!tmp.empty()) {
		elems.push_front(tmp.top());
		tmp.pop();
	}
	return *this;
}
```

为了访问op2的私有成员，可以将其他所有类型的stack模板的实例都定义成友元：

```c++
template<typename T>
class Stack {
private:
	std::deque<T> elems; // elements
public:
    Void push(T const&); // push element
    void pop(); // pop element
    T const& top() const; // return top element
    bool empty() const { // return whether the stack is empty
   		return elems.empty();
    }
    // assign stack of elements of type T2
    template<typename T2>
    Stack& operator= (Stack<T2> const&);
    // to get access to private members of Stack<T2> for any type T2:
    template<typename> friend class Stack;
};
```

可以将赋值运算符进一步省略：

```c++
template<typename T>
template<typename T2>
Stack<T>& Stack<T>::operator=(Stack<T2> const& op2)
{
	elems.clear();
	elems.insert(elems.begin(), op2.elems.begin(), op2.elems.end());
	return *this;
}
```

现在，以下的代码就是合法的：

```c++
Stack<int> intStack;
Stack<float> floatStack;
...
floatStack = intStack;
```

同样，也可以将内部的容器类型参数化，如下：

```c++
template<typename T, typename Cont = std::deque<T>>
class Stack {
private:
	Cont elems; // elements
public:
    void push(T const&); // push element
    void pop(); // pop element
    T const& top() const; // return top element
    bool empty() const { // return whether the stack is empty
   		return elems.empty();
    }
    // assign stack of elements of type T2
    template<typename T2, typename Cont2>
    Stack& operator= (Stack<T2,Cont2> const&);
    // to get access to private members of Stack<T2> for any type T2:
    template<typename, typename> friend class Stack;
};
```

此时的赋值运算符为：

```c++
template<typename T, typename Cont>
template<typename T2, typename Cont2>
Stack<T,Cont>& Stack<T,Cont>::operator= (Stack<T2,Cont2> const& op2)
{
    elems.clear(); // remove existing elements
    elems.insert(elems.begin(), // insert at the beginning
    op2.elems.begin(), // all elements from op2
    op2.elems.end());
    return *this;
}
```

成员函数模板也可以被全部或部分特例化，比如：

```c++
class BoolString {
private:
	std::string value;
public:
	BoolString(std::string const& s): value(s) {}
	template<typename T=std::string> T get() const {
		return value;
	}
};
```

可以像下面这样对成员函数模板get进行全特例化：

```c++
template<>
inline bool BoolString::get<bool>() const {
	return value == "true" || value == "1" || value == "on";
}
```

**注意**，我们不需要也不能对特例化的版本进行声明，只能定义它们。由于这是一个定义于头文件中的全实例化的版本（相当于常规函数），如果有多个编译单元include这个头文件，为避免重复定义的错误，必须将其定义为inline的。可以像如下使用：

```c++
std::cout << std::boolalpha;
BoolString s1("hello");
std::cout << s1.get() << ’\n’; //prints hello
std::cout << s1.get<bool>() << ’\n’; //prints false
BoolString s2("on");
std::cout << s2.get<bool>() << ’\n’; //prints true
```

如果能够通过特殊成员函数copy或者move对象，那么相应的特殊成员函数（copy构造函数以及move构造函数）也将可以被模板化，和前面的赋值运算符相似，构造函数也可以是模板，但构造函数模板或者赋值运算符模板不会取代预定义的构造函数和赋值运算符，比如在上面的例子中，如果相同类型的stack之间相互赋值，调用的依然是默认赋值运算符。

某些情况下，在调用成员模板的时候需要显式的指定其模板参数的类型，这时候就需要关键字tempalte来确保符号<会被理解成为模板参数列表的开始，而不是一个比较运算符，例如：

```c++
template<unsigned long N>
void printBitset (std::bitset<N> const& bs) {
	std::coout << bs.template to_string<char, std::char_traits<char>, std:;allocator<char>>();
}
```

如果没有.template的话，编译器会将to_string()后面的<符号理解成小于运算符，而不是模板参数列表的开始，这种情况只会在点号前面的对象依赖于模板参数的时候才会发生。.template标识符（->template和::template也类似）只能用于模板内部，并且它前面的对象应该依赖于模板参数。

14中引入的泛型lambdas，是一种成员模板的简化。比如以下的泛型lambda：

```c++
[] (auto x, auto y) {
	return x + y;
}
```

编译器会默认构造成下面这样一个类：

```c++
class SomeCompilerSpecificName {
public:
    SomeCompilerSpecificName(); // constructor only callable by
    compiler
    template<typename T1, typename T2>
    auto operator() (T1 x, T2 y) const {
    	return x + y;
    }
};
```

### 变量模板

从14开始，变量也可以被某种类型参数化，被称为变量模板，如下：

```c++
template<typename T>
constexpr T pi{3.1415926};
```

注意，和其他几种模板类似，这个定义最好不要出现在函数内部或者块作用域内部。在使用变量模板的时候，必须指明它们的类型：

```c++
std::cout << pi<double> << '\n';
std::cout << pi<float> << '\n'; 
```

变量模板也可以用于不同编译单元：

```c++
template<typename T> T val{}; // zero initialized value//== translation
unit 1:
#include "header.hpp"
int main()
{
    val<long> = 42;
    print();
}
//== translation unit 2:
#include "header.hpp"
void print()
{
	std::cout << val<long> << ’\n’; // OK: prints 42
}
```

也可以有默认模板类型：

```c++
template<typename T = long double> 
constexpr T pi = T{3.14159};
...
std::cout << pi<> << ’\n’; //outputs a long double
std::cout << pi<float> << ’\n’; //out
```

但不能省略尖括号。

同样，也可以用非类型参数对变量模板进行参数化，也可以将非类型参数用于参数器的初始化，如：

```c++
#include <iostream>
#include <array>
template<int N>
std::array<int,N> arr{}; // array with N elements, zero-initialized
template<auto N>
constexpr decltype(N) dval = N; // type of dval depends on passed value
int main()
{
	std::cout << dval<’c’> << ’\n’; // N has value ’c’ of type ch
	arr<10>[0] = 42; // sets first element of global arr
    for (std::size_t i=0; i<arr<10>.size(); ++i) { // uses values set in arr
   	 std::cout << arr<10>[i] << ’\n’;
    }
}
```

变量模板的一种应用场景是用于定义代表类模板成员的变量模板，比如如果定义了一个这样的类模板：

```c++
template<typename T>
class MyClass {
public:
	static constexpr int max = 1000;
}
```

那么就可以为MyClass的不同特例化版本定义不同的值：

```c++
template<typename T>
int myMax = MyClass<T>::max;
```

那么就有以下的使用方法：

```c++
auto i = myMax<std::string>;
//而不用
auto i = MyClass<std::string>::max;
```

类型萃取Suffix_v，从17开始，标准库用变量模板为其用来产生一个值（布尔型）的类型萃取定义了简化方式：

```c++
std::is_const_v<T>
//而不用
std::is_const<T>::value
```

主要是标准库做了如下定义：

```c++
namespace std {
    template<typename T>
    constexpr bool is_const_v = is_const<T>::value;
}
```

### 模板参数模板

还可以定义模板参数模板，比如：

```c++
template<typename T,
template<typename Elem> class Cont = std::deque>
class Stack {
private:
    Cont<T> elems; // elements
public:
    void push(T const&); // push element
    void pop(); // pop element
    T const& top() const; // return top element
    bool empty() const { // return whether the stack is empty
        return elems.empty();
    } …
};
```

从11开始，可以用别名模板取代Cont，但直到17才可以用typename取代class，如：

```c++
template<typename T, template<typename Elem> typename Cont =
std::deque>
class Stack { //ERROR before C++17 …
};
```

由于模板参数模板中的模板参数没有被用到，作为惯例可以省略它。

如果直接使用这个新版的Stack可能会遇到错误，原因是17之前要求template<typename Elem> typename Cont = std::deque中的模板参数必须和实际参数（deque）的模板参数匹配，作为变通，可以如下定义：

```c++
template<typename T, template<typename Elem,
typename Alloc = std::allocator<Elem>> class Cont = std::deque>
class Stack {
private:
    Cont<T> elems; // elements …
};
```

## 移动语义和enable_if<>

### 完美转发

希望将被传递的参数的基本特性转发出去：

- 可变对象被转发之后依旧是可变的；
- const对象被转发之后依旧是const的；
- 可移动对象被转发以后依旧是可移动的；

实现完美转发的惯用方法如下：

```c++
template<typename T>
void f(T&& val) {
	g(std::forward<T>(val));
}
```

只有当val是右值时，forward才将其转发为右值，否则则什么都不做。注意这里模板的T&&，其跟具体类型的右值引用是不一样的：

- 具体类型的X的X&&声明了一个右值引用参数，只能被绑定到一个可移动的对象上，它的值总是可变的，而且总是可以被窃取的；
- 模板参数T的T&&声明了一个转发引用（也成万能引用），可以被绑定到可变、不可变（比如const）或者可移动对象上；

### 特殊成员函数模板

特殊成员函数比如构造函数都可以是模板：

```c++
#include <utility>
#include <string>
#include <iostream>
class Person
{
private:
    std::string name;
public:
    // generic constructor for passed initial name:
    template<typename STR>
    explicit Person(STR&& n) : name(std::forward<STR>(n)) {
    	std::cout << "TMPL-CONSTR for ’" 
    }
    // copy and move constructor:
    Person (Person const& p) : name(p.name) {
    	std::cout << "COPY-CONSTR Person ’" << name << "’\n";
    }
    Person (Person&& p) : name(std::move(p.name)) {
    	std::cout << "MOVE-CONSTR Person ’" << name << "’\n";
    }
};
```

注意：

```c++
std::string s = "sname";
Person p1(s); // init with string object => calls TMPL-CONSTR
Person p2("tmp"); //init with string literal => calls TM
Person p3(p1); // ERROR
Person p4(std::move(p1)); // OK: move Person => calls MOVECONST
Person const p2c("ctmp"); //init constant object with string literal
Person p3c(p2c); // OK: copy constant Person => calls COPY
```

这是因为根据C++重载解析规则，对于一个非const左值的Person p，成员模板Person(STR&& n)比预定义的拷贝构造函数Person(Person const& p)更匹配，这里STR可以直接被替换成Person&，但对于拷贝构造函数还需要做进一步的const转换。提供一个非const的拷贝构造可以解决，但更好的解决办法是当参数是一个Person对象或一个可以转换为Person对象的表达式时，不要启动模板，可以用std::enable_if<>实现。

### 通过std::enable_if<>禁用模板

比如函数模板定义如下：

```c++
template<typename T>
typename std::enable_if<(sizeof(T) > 4)>::type
foo() {
}
```

std::enable_if<>是一种类型萃取，它会根据一个作为其（第一个）模板参数的编译器表达式决定其行为：

- 如果表达式结果为真，它的type成员会返回一个类型作为返回类型，如果没有第二个模板参数，返回类型是void，否则返回类型是第二个参数的类型；
- 如果表达式结果是false，则其成员类型是未定义的，根据模板的SFINAE的规则，包含std::enable_if<>表达式的函数模板被忽略掉；

从14开始的所有模板萃取都返回一个类型，因此可以使用一个与之对应的别名模板std::enable_if_t<>，这样就可以省略掉template和::type，如下：

```c++
template<typename T>
std::enable_if_t<sizeof(T) > 4>
foo() {
}
```

由于将enable_if表达式放在声明的中间不是一个明智的做法，因此使用std::enable_if<>的更常见的方法是使用一个额外的、有默认值的模板参数：

```c++
template<typename T, typename = std::enable_if_t<(sizeof(T) > 4)>>
void foo() {
}
```

如果认为这依然不够明智，并且希望模板的约束更加明显，可以用别名模板定义别名：

```c++
template<typename T>
using EnableIfSizeGreater4 = std::enable_if_t<(sizeof(T) > 4)>;
template<typename T, typename = EnableIfSizeGreater4<T>>
void foo() {}
```

### 使用enable_if<>

接下来可以解决我们的问题，当传递的模板参数类型不正确的时候（比如不是std::string或者可以转换成std::string类型）将对应的构造函数模板禁用，此时需要使用另一个标准库的类型萃取std::is_convertiable<FROM, TO>。在17中相应的构造函数模板定义为：

```c++
template<typename STR, typename = std::enable_if_t<std::is_convertiable_v<STR, std::string>>>
Person(STR&& n);
```

注意14中由于没有给产生一个值的类型萃取定义带_v的别名，必须使用如下的定义：

```c++
template<typename T>
using EnableIfString = std::enable_if_t<std::is_convertiable<T, std::string>::value>;
```

而在11中由于没有类型萃取定义带_t的别名，必须使用如下定义：

```c++
template<typename T>
using EnableIfString = typename std::enable_if<std::is_convertiable<T, std::string>::value>::type;
```

除了使用要求类型之间可以隐式转换的std::is_convertible<>之外，还可以使用std::is_constructible<>，它要求可以用显式转换来做初始化，它的参数顺序和std::is_convertible<>相反：

```c++
template<typename T>
using EnableIfString = std::enable_if_t<std::is_constructible_v<std::string, T>>;
```

注意我们不能通过使用enable_if<>来禁用copy/move构造函数以及赋值构造函数，这是因为成员函数模板不会被算作特殊成员函数（依然会生成默认构造函数）。可以采用这样的办法，定义一个接收const volatile的copy构造函数并标识为delete，这样做就不会再隐式声明一个接收const参数的copy构造函数，在此基础上，可以定义一个函数模板，对于nonvolatile的类型（对于volitile类型会报错），它会被优先选择：

```c++
class C
{
public:
	C(C const volatile&) == delete;
	template<typename T>
	C(T const&) {
		std::cout << "tmpl copy constructor\n";
	}
}
```

这样就可以对这个copy构造函数模板添加限制，比如：

```c++
template<typename U, typename=std::enable_if_t<!std::is_integral<U>::value>>
C (C<U> const &) {};
```

### 使用concept简化enable_if<>表达式

在C++20中，可以写出如下的代码：

```c++
template<typename STR>
requires std::is_convertible_v<STR, std::string>
Person(STR&& n): name(std::forward<STR>(n)) {}
```

甚至可以将其中模板的使用条件定义成通用的concept：

```c++
template<typename T>
concept ConvertibleToString = std::is_convertible_v<T, std::string>;
template<typename STR>
requires ConvertibleToString<STR>
Person(STR&& n): name(std::forward<STR>(n)) {}
```

## 按值引用还是按引用传递

自从11引入了移动语义之后，共有以下的传递方式：

1. X const&：const左值引用；
2. X&：非const左值引用；
3. X&&：右值引用；

对具体的类型，决定参数的方式已经很复杂，在参数未知的模板中，选择就更加困难，在前文中，曾建议模板中优先使用按值传递，除非遇到以下情况：

- 对象不允许copy；
- 参数被用于返回数据；
- 参数以及所有属性需要被模板转发到别的地方；
- 可以获得明显的性能提升；

### 按值传递

按值传递调用拷贝构造的成本可能很高，所以可以通过移动语义优化掉对象的拷贝。注意，不是所有的情况都会调用拷贝构造函数：

```c++
std::string returnString();
std::string s = "hi";
printV(s); //copy constructor
printV(std::string("hi")); //copying usually optimized away (if not,
move constructor)
printV(returnString()); // copying usually optimized away (if not, move
constructor)
printV(std::move(s)); // move construct
```

第二次和第三次调用传递的参数是纯右值，编译器会优化参数传递，从17开始，标准要求这一方案必须被实现，而在17之前，如果编译器没有优化这一类的拷贝，则至少应该尝试移动语义。

注意，按值传递还有一个特性，那就是参数类型会退化，裸数组会退化成指针，const和volatile等限制符会被删除（只会删除顶层const）。

### 按引用传递

按引用传递不一定能够提高性能，原因在于底层实现上按引用传递实际上通过传递参数的地址实现的，地址会被简单编码，从而提高调用者向被调用者传递的效率，但按地址传递可能使得编译器在编译调用者代码时有一些困惑，那就是被调用者会怎么处理这个地址，由于被调用者可能利用这个地址来随意更改其中的内容，因此编译器假设在这次调用之后，缓存中缓存器的值全部变为无效，而重新载入这些变量的值可能会很耗时（可能比拷贝对象的成本要高）。设为const引用能否被编译器识别从而阻止以上的过程？答案是不能，编译器并不能做这样的推断。不过对于inline函数，情况可能会好一些，编译器可以展开inline函数，那么就可以基于调用者和被调用者信息推断被传递地址中的值十分会被修改。

如果是按const引用传递，比如如下的模板：

```c++
template<typename T>
void printR (T const& arg) { …
}
std::string const c = "hi";
printR(c); // T deduced as std::string, arg is std::string const&
printR("hi"); // T deduced as char[3], arg is char const(&)[3]
int arr[4];
printR(arr); // T deduced as int[4], arg is int const(&)[4]
```

显然类型不会退化，而且推断T也不含const。

如果想要按非const引用传递，就需要使用非const引用（或指针），比如：

```c++
template<typename T>
void outR (T& arg) { 
    …
}
```

这种情况，通常不允许将临时变量或者通过move处理过的已存在变量用作其参数：

```c++
std::string returnString();
std::string s = "hi";
outR(s); //OK: T deduced as std::string, arg is std::string&
outR(std::string("hi")); //ERROR: not allowed to pass a temporary (prvalue)
outR(returnString()); // ERROR: not allowed to pass a temporary (prvalue)
outR(std::move(s)); // ERROR: not allowed to pass an xv
```

但以下的情况有些复杂：

```c++
std::string const c = "hi";
outR(c); // OK: T deduced as std::string const
outR(returnConstString()); // OK: same if returnConstString() returns const string
outR(std::move(c)); // OK: T deduced as std::string const
outR("hi"); // OK: T deduced as char const[3]
```

也就是说，当传递的参数是const时，arg的类型有可能被推断为const引用，这时可以传递一个右值作为参数，但模板其实期望的是左值。在上面的情况中，在函数模板内部，任何试图更改被传递参数的值的行为都是错误的。如果想要禁止向非const引用传递const对象，有以下的选择：

- 使用static_assert触发一个编译期错误：

  ```c++
  template<typename T>
  void outR(T& arg) {
  	static_assert(!std::is_const<T>::value, "out parameter of foo<T>(T&) is const");
  	...
  }
  ```

- 通过使用std::enable_if<>禁用该情况模板：

  ```c++
  template<typename T, typename=std::enable_if_t<!std::is_const<T>::value>>
  void outR(T& arg) {
  	...
  }
  ```

  或者在concepts被支持之后使用concepts：

  ```c++
  template<typename T>
  requires !std::is_const_v<T>
  void outR(T& arg) {
  	...
  }
  ```

对于按转发引用传递参数，考虑以下的代码：

```c++
std::string s = "hi";
passR(s); // OK: T deduced as std::string& (also the type of arg)
passR(std::string("hi")); // OK: T deduced as std::string, arg is
// std::string&&
passR(returnString()); // OK: T deduced as std::string, arg is
// std::string&&
passR(std::move(s)); // OK: T deduced as std::string, arg is
// std::string&&
passR(arr); // OK: T deduced as int(&)[4] (also the type of arg)
```

可以将任意类型的参数传递给转发引用，而且也不会被拷贝。

然而，以下的类型推断有点特殊：

```c++
std::string const c = "hi";
passR(c); //OK: T deduced as std::string const&
passR("hi"); //OK: T deduced as char const(&)[3] (also the type of arg)
// int arr[4];
passR(arr); //OK: T deduced as int (&)[4] (also the type of arg)
```

在以上三种情况下，都可以在passR的内部从arg的类型得知参数是一个右值还是一个const或者非const的左值。

转发引用虽然完美，也有缺点，由于其是唯一一个可以将模板参数T隐式推断为引用的情况，如果在模板内部直接用T声明一个未初始化的局部变量，就会触发错误（引用对象在创建时必须被初始化）：

```c++
template<typename T>
void passR(T&& arg) { // arg is a forwarding reference
	T x; // for passed lvalues, x is a reference, which requires an initializer …
}
foo(42); // OK: T deduced as int
int i;
foo(i); // ERROR: T deduced as int&, which makes the declaration of x
// in passR() invalid
```

### 使用std::ref()和std::cref()（限于模板）

可以使用\<functional\>中的std::ref()和std::cref()将参数按引用传递给模板：

```c++
template<typename T>
void printT (T arg) { 
    …
}
std::string s = "hello";
printT(s); //pass s By value
printT(std::cref(s)); // pass s “as if by reference”
```

std::cref并没有改变函数模板内部处理参数的方式，它只是用一个行为和引用类似的对象对参数进行了封装。事实上，其会创建一个std::reference_wrapper<>对象，该对象引用了原始参数，并按值传递给模板。该对象可能只支持一个操作：向原始类型的隐式类型转换，返回原始参数对象。注意，编译器必须知道需要将std::reference_wrapper<>对象转换为原始参数类型才会进行隐式转换，因此其通常只有在通过泛型代码传递对象时才正常工作，比如直接尝试输出T，就会因为std::reference_wrapper没有定义输出操作符而报错。

### 处理字符串常量和裸数组

有时候可能必须要对数组参数和指针参数做不同的实现，当然这样的情况下不能退化数组的类型，为了区分这两种情况，必须要检测被传递过来的参数是不是数组，通常有两种方法：

- 可以将模板定义成只接受数组作为参数：

  ```c++
  template<typename T, std::size_t L1, std::size_t L2>
  void foo(T (&arg1)[L1], T (&arg2)[L2])
  {
      T* pa = arg1; // decay arg1
      T* pb = arg2; // decay arg2
      if (compareArrays(pa, L1, pb, L2)) { 
      	…
      }
  }
  ```

- 可以使用类型萃取来检测参数是不是数组：

  ```c++
  template<typename T, typename =
  std::enable_if_t<std::is_array_v<T>>>
  void foo (T&& arg1, T&& arg2)
  {
  	…
  }
  ```

这些特殊的处理方式过于复杂，最好还是利用不同的函数名来专门处理数组参数，或者更进一步，让模板调用者使用std::vector或者std::array作为参数。

### 处理返回值

有一些情况也倾向于按引用返回：

- 返回容器或者字符串中的元素（比如通过[]运算符或者front方法访问元素）；
- 允许修改类对象的成员；
- 为链式调用返回一个对象（比如>>和<<运算符以及赋值运算符）

但最好还是确保函数模板采用按值返回的方式，使用函数模板T作为返回类型并不能保证返回值不会是引用，因为T在某些情况下会被隐式推断为引用类型，比如：

```c++
template<typename T>
T retR(T&& p) // p is a forwarding reference
{
	return T{…}; // OOPS: returns by reference when called for lvalues
}
```

即使函数模板声明为按值传递，也可以显式将T指定为引用类型：

```c++
template<typename T>
T retV(T p) //Note: T might become a reference
{
	return T{…}; // OOPS: returns a reference if T is a reference
}
int x;
retV<int&>(x); // retT() instantiated for T as in
```

为安全起见，有以下两种选择：

- 用类型萃取std::remove_reference\<T\>将T转为非引用类型：

  ```c++
  template<typename T>
  typename std::remove_reference<T>::type retV(T p)
  {
  	return T{...};
  }
  ```

  std::decay<>之类的类型萃取可能也有帮助，因为它们也会隐式的去掉类型的引用；

- 将返回值声明为auto，从而让编译器推断返回类型，因为auto会导致类型退化：

  ```c++
  template<typename T>
  auto retV(T p) // by-value return type deduced by compiler
  {
  	return T{…}; // always returns by value
  }
  ```

### 关于模板参数声明的推荐方法

有以下两种情况：

- 将参数声明按值传递，会对字符串常量和裸数组的类型进行退化，但对比较大的对象可能会影响性能。这种情况下，调用者仍能够通过std::cref和std::ref按引用传递参数；
- 将参数声明按引用传递，对比较大的对象能够提供比较好的性能，尤其是在以下的情况下：
  - 将已经存在的对象按照左值引用传递；
  - 将临时对象或者被move转换为可移动的对象按右值引用传递；
  - 或者将以上几种类型的对象按照转发引用传递；

一般来说，对函数模板有以下建议：

1. 默认情况下，将参数声明为按值传递，这样比较简单，对于字符串常量也可以正常工作，对于临时对象性能也不错，对于比较大的对象，为了避免成本高昂的拷贝，可以使用ref和cref；
2. 如果理由充分也可以不这样做：需要一个参数用于输出；使用模板是为了转发它的参数，那么就使用完美转发；重点考虑程序性能，而参数拷贝的成本又很高，那么就使用const应用。

另外，如无必要，不要过分泛型化。

## 编译期编程

模板的实例化发生在编译期间，可以和模板的某些特性相结合，能够产生一种C++自己内部的原始递归的“编程语言”。比如以下代码在编译期间就可以判断一个数是否是质数：

```c++
template<unsigned p, unsigned d> // p: number to check, d: current divisor
struct DoIsPrime {
	static constexpr bool value = (p%d != 0) && DoIsPrime<p, d-1>::value;
};
template<unsigned p> // end recursion if divisor is 2
struct DoIsPrime<p, 2> {
	static constexpr bool value = (p%2 != 0);
};
template<unsigned p> // primary template
struct IsPrime {
	// start recursion with divisor from p/2
	static constexpr bool value = DoIsPrime<p, p/2>::value;
};
// special cases (to avoid endless recursion with template
instantiation):
template<>
struct IsPrime<0> { static constexpr bool value = false; };
template<>
struct IsPrime<1> { static constexpr bool value = false; };
template<>
struct IsPrime<2> { static constexpr bool value = true; };
template<>
struct IsPrime<3> { static constexpr bool value = true; };
```

以9为例，表达式IsPrime<9>::value会首先展开成DoIsPrime<9, 4>::value，进一步展开成9%4!=0 && DoIsPrime<9,3>::value，然后进一步展开。这个过程都是在编译期间进行，最终IsPrime<9>::value会在编译期间被扩展成false。

### 通过constexpr进行计算

11引入了constexpr的特性，大大简化了各种类型的编译期计算。虽然11对其使用有着诸多限制（比如constexpr函数的定义通常都只能包含一个return语句），但14中大部分限制都被移除。目前，堆内存分配以及异常抛出还不被支持。

在11中，判断一个数是否是质数的方法如下：

```c++
constexpr bool
doIsPrime (unsigned p, unsigned d) // p: number to check, d: current
divisor
{
    return d!=2 ? (p%d!=0) && doIsPrime(p,d-1) // check this and smaller
    divisors
    : (p%2!=0); // end recursion if divisor is 2
}
constexpr bool isPrime (unsigned p)
{
    return p < 4 ? !(p<2) // handle special cases
    : doIsPrime(p,p/2); // start recursion with divisor from
    p/2
}
```

14中constexpr可以使用常规C++中大部分的控制结构，因此改进如下：

```c++
constexpr bool isPrime (unsigned int p)
{
    for (unsigned int d=2; d<=p/2; ++d) {
    if (p % d == 0) {
   		return false; // found divisor without remainder}
    }
    return p > 1; // no divisor without remainder found
}
```

注意，可以在编译期执行并不一定在编译期执行，如果变量被constexpr修饰，或者被定义于全局作用域或namespace作用域，会在编译期内计算，如果定义于块作用域，那么将由编译器决定是否在编译期间进行计算。

### 通过部分特例化来进行路径选择

示例如下：

```c++
// primary helper template:
template<int SZ, bool = isPrime(SZ)>
struct Helper;
// implementation if SZ is not a prime number:
template<int SZ>
struct Helper<SZ, false>
{ …
};
// implementation if SZ is a prime number:
template<int SZ>
struct Helper<SZ, true>
{ …
};
template<typename T, std::size_t SZ>
long foo (std::array<T,SZ> const& coll)
{
    Helper<SZ> h; // implementation depends on whether array has prime number as size 
    …
}
```

上述针对两种可能的情况实现了两种偏特例化版本，也可以将主模板用于其中一种情况，再特例化一个版本代表另一种情况：

```c++
// primary helper template (used if no specialization fits):
template<int SZ, bool = isPrime(SZ)>
struct Helper
{ …
};
// special implementation if SZ is a prime number:
template<int SZ>
struct Helper<SZ, true>
{ …
};
```

### SFINAE(Substitution Failure Is Not An Error，替换失败不是错误)

对于函数的重载，当函数调用的备选方案包括函数模板时，编译期首先要决定应该将什么样的模板参数用于各种模板方案，然后用这些参数替换函数模板的参数列表以及返回类型，最后评估替换后的函数模板和这个调用的匹配情况。这一过程可能会遇到问题：替换产生的结果可能没有意义。这一类型的替换不会导致错误，C++语言规则要求忽略掉这一类型的替换结果，这一原理称为SFINAE。有以下的例子：

```c++
template<typename T, unsigned N>
std::size_t len(T(&)[N]) {
	return N;
}
template<typename T>
typename T::size_type len(T const& t) {
	return t.size();
}

int a[10];
std::cout << len(a); // OK: only len() for array matches
std::cout << len("tmp"); //OK: only len() for array matches
std::vector<int> v;
std::cout << len(v); // OK: only len() for a type with size_type 
int* p;
std::cout << len(p); // ERROR: no matching len() function 
```

对于数组和字符串来说，其实对第二个函数模板也可以分别用int[10]和char const[4]来替换参数T，但这种替换在处理返回类型T::size_type时会导致错误，因此对于这种调用第二个函数模板会被忽略。

如果忽略掉那些替换之后返回值无效的备选项，那么编译器会选择另外一个参数类型匹配相差的备选项，比如：

```c++
//对所有类型的应急选项：
std::size_t len(...) {
	return 0;
}
```

额外提供了一个通用函数len，它总会匹配所有的调用，但其匹配情况也总是所有重载选项中最差的。

对于有些限制条件，并不总是能够容易的设计出合适的表达式来SFINAE掉函数模板，比如有size_type成员但没有size成员函数的参数类型，这种情况函数模板可能会被选择并在实例化的过程中导致错误，如下：

```c++
template<typename T>
typename T::size_type len(T const& t) {
	return t.size();
}
std::allocator<int> x;
std::cout << len(x) << ’\n’; //ERROR: len() selected, but x has no size()
```

处理这一情况有一种常用的模式：

- 通过尾置返回类型语法指定返回类型（函数名前使用auto，函数名后使用->指定返回类型）；
- 通过decltype和逗号运算符定义返回类型；
- 将所有需要成立的表达式放在逗号运算符的前面（为了预防可能会发生的运算符被重载的情况，需要将这些表达式的类型转换为void）；
- 在逗号运算符的末尾定义一个类型为返回类型的对象；

比如：

```c++
template<typename T>
auto len(T const& t) -> decltype((void)(t.size()), T::size_type())
{
	return t.size();
}
```

这里返回类型被定义为`decltype((void)(t.size()), T::size_type())`，其操作数是一组用逗号隔开的表达式，因此最后一个表达式T::size_type()会产生一个类型为返回类型的对象（decltype会将其转换为返回类型）。而在最后一个逗号之前的表达式都必须成立，之所以将其类型转换为void，是为了避免用户重载了该表达式对应类型的逗号运算符而导致的不确定性。

### 编译期if

部分特例化、SFINAE以及std::enable_if可以一起被用来禁用或者启用某个模板，而C++17在此基础上引入了同样在编译期基于某些条件禁用或启用相应模板的编译期if语句。通过使用if constexpr()语法，编译器会根据编译期表达式来决定是使用if语句的then对应的部分还是else对应的部分。例如：

```c++
template<typename T, typename... Types>
void print(T const& firstArg, Types const&... args) {
	std::cout << firstArg << '\n';
	if constexpr(sizeof...(args)>0) {
		print(args...); //code only available if sizeof…(args)>0 (since
C++17)
	}
}
```

## 在实践中使用模板

### 包含模式

对于模板来说，将模板声明和定义都放在头文件中的方法称为“包含模式”。

### 模板和inline

inline可以提高程序运行性能，其告知编译器优先在函数调用处将函数体做inline替换展开，而不是按常规的调用机制。但编译期可能会忽视这一点，但避不开的是使用inline可以允许函数定义在函数中出现多次。函数模板也跟inline函数类似，其可以定义在头文件中，在多个编译单元中出现。但这不意味函数模板会在默认情况下使用inline替换，这由编译器决定。如果想要自己决定是否进行inline替换，有时候只能通过编译期的具体属性实现，比如noinline和always_inline。

需要注意的是，函数模板在全特化之后和常规函数是一样的，除非其被定义为inline，否则只能被定义一次。

### 预编译头文件

编译器在编译头文件时有时需要耗费大量的时间，模板的引入则进一步加剧了这一问题，但编译期供应商提供了一种叫做预编译头文件的方案来降低预编译时间。

预编译头文件的方案实现基于这样的事实：在组织代码的时候，很多文件都以相同的几行开始，这样就可以单独编译这几行，并将编译完成的状态保存在一个预编译头文件中，所以以这几行开始的文件在编译时都会载入这个保存的状态，之后接着编译，重新载入这几行的编译状态被重新编译这几行要快很多很多，因此要让尽可能多的文件以尽可能多的相同的代码作为开始。

一种推荐的方式是创建一个通用的头文件，里面按层级组织，从最常用以及最稳定的头文件到那些期望一直都不会变化的头文件。

## 模板的基本术语

### 类模板还是模板类

C++中，struct、classes和unions都被称为class types，需要注意的是class types包含unions，但class不包含。

class template指的是这个类是模板，即它是一组class的参数化表达；而template class的含义更丰富些，用作类模板的同义词，也用作指代从template实例出的类，或者是用来指代名称是一个template-id（模板名+<模板参数>）的类。

### 替换、实例化和特例化

替换即用模板实参尝试替换模板参数。

从一个模板创建一个常规类、类型别名、函数、成员函数或者变量的过程称为模板实例化。

通过实例化或者不完全实例化产生的实体称为特例化。

### 声明和定义

声明是将一个名称引入到一个C++作用域，对于声明，如果细节已知或者是需要申请相关变量的存储空间，那么声明就变成了定义。对于一个类模板或者函数模板，如果有包含在{}中的主体的话，那么声明也会变成定义。

类型可以是完整的或者非完整的，非完整类型是以下情况之一：

- 一个被声明还没有被定义的class类型；
- 一个没有指定边界的数组；
- 一个存储非完整类型的数组；
- void类型；
- 一个底层类型未定义或者枚举值未定义的枚举类型；
- 任何一个被const或者volatile修饰的以上某种类型；

### 唯一定义法则

- 常规非inline函数和成员函数以及非inline的全局变量和静态数据成员，在整个程序中只能被定义一次；
- class类型（包括struct和union），模板（包含部分特例化但不能是全特例化）以及inline函数和变量，在一个编译单元只能被定义一次，而且不同编译单元的定义应该相同；

在后面的章节中，可链接实体指的是以下任意一种：一个函数或成员函数，一个全局变量或静态数据成员，以及通过模板产生的类似实体，只要对linker可见即可。

## 泛型库

### 可调用对象

C++中，一些类型既可以被作为函数调用参数使用，也可以按照f(...)的形式调用，因此可用作回调参数：

- 函数指针类型；
- 重载了operator()的class类型（有时被称为仿函数），这其中包含lambda函数；
- 包含一个可以产生一个函数指针或者函数引用的转换函数的class类型；

这样的类型统称为函数对象类型，其对应的值称为函数对象。

将函数对象应用于参数有以下几种情况：

```c++
#include <iostream>#include <vector>
#include "foreach.hpp"
// a function to call:
void func(int i)
{
	std::cout << "func() called for: " << i << ’\n’;
}
// a function object type (for objects that can be used as functions):
class FuncObj {
public:
    void operator() (int i) const { //Note: const member function
    std::cout << "FuncObj::op() called for: " << i << ’\n’;
    }
};
int main()
{
    std::vector<int> primes = { 2, 3, 5, 7, 11, 13, 17, 19 };
    foreach(primes.begin(), primes.end(), // range
    func); // function as callable (decays to pointer)
    foreach(primes.begin(), primes.end(), // range
    &func); // function pointer as callable
    foreach(primes.begin(), primes.end(), // range
    FuncObj()); // function object as callable
    foreach(primes.begin(), primes.end(), // range
    [] (int i) { //lambda as callable
    std::cout << "lambda called for: " << i << ’\n’;
    });
}

```



- 当把函数名当作函数参数传递时，并不是传递函数本体，而是传递其指针或者引用，当按值传递时，函数参数退化为指针，如果参数类型为模板参数，那么类型会被推断为指向函数的指针；和数组一样，按引用传递的函数类型不会退化，但函数类型不能真正用const限制，const会被省略；
- 显式传递一个函数指针跟第一条情况相同；
- 如果传递的是一个仿函数，就是将一个类的对象当作可调用对象进行传递，注意在定义operator的时候最好将其定义为const成员函数，否则一些框架或库不希望该调用会改变被传递对象的状态时，会遇到很不容易debug的error；
- lambda表达式会产生仿函数；

调用成员函数跟调用非静态成员函数有所不同，但幸运的是，17开始标准库引入了std::invoke()，其非常方便的统一了成员函数情况和常规函数情况。有这样的例子：

```c++
#include <utility>
#include <functional>
template<typename Iter, typename Callable, typename… Args>
void foreach (Iter current, Iter end, Callable op, Args const&…args)
{
    while (current != end) { //as long as not reached the end of the
        elements
        std::invoke(op, //call passed callable with
        args…, //any additional args
        *current); // and the current element
        ++current;
    }
}
```

invoke()会这样处理相关参数：

- 如果可调用对象是一个指向成员函数的指针，它会将args...的第一个参数当作this对象，其余的参数则被当作常规参数传递给可调用对象；
- 否则，所有的参数都被直接传递给可调用对象；

注意这里可调用对象和args...都不能使用完美转发，因为第一次调用可能会steal相关参数的值，导致在随后的调用中出现错误。

std::invoke()的一个常规用法是封装一个单独的函数调用（比如记录相关调用，测量所耗时长，或者准备一些上下文信息（比如为此启动一个线程））。此时可以通过完美转发可调用对象以及被传递参数来支持移动语义：

```c++
#include <utility> // for std::invoke()
#include <functional> // for std::forward()
template<typename Callable, typename… Args>
decltype(auto) call(Callable&& op, Args&&… args)
{
    return std::invoke(std::forward<Callable>(op), //passed callable
    with
    std::forward<Args>(args)…); // any additional args
}
```

注意这里的返回值，为了能够返回引用，需要使用decltype(auto)，其是14引入的，根据表达式决定了变量、返回值或者模板实参的类型。如果想要暂时将std::invoke的返回值存储在变量里，并在做了一些事情后返回，也必须将其声明为decltype(auto)类型：

```c++
decltype(auto) ret{std::invoke(std::forward<Callable>(op),
	std::forward<Args>(args)…)}; …
return ret
```

这里将ret声明为auto&&是不对的，其生命周期不会超出return。注意，如果可调用对象的返回值是void，将ret初始化为decltype(auto)也是不可以的，因为void是不完整类型，此时有如下的选择：

- 在当前行前面声明一个对象，并在其析构函数中实现期望的行为，比如：

  ```c++
  struct cleanup {
      ~cleanup() { … //code to perform on return
      }
  } dummy;
  return std::invoke(std::forward<Callable>(op),
  std::forward<Args>(args)…);
  ```

- 分别实现void和非void情况：

  ```c++
  #include <utility> // for std::invoke()
  #include <functional> // for std::forward()
  #include <type_traits> // for std::is_same<> and
  invoke_result<>
  template<typename Callable, typename… Args>
  decltype(auto) call(Callable&& op, Args&&… args)
  {
      if constexpr(std::is_same_v<std::invoke_result_t<Callable,
      Args…>, void>) {// return type is void:
          std::invoke(std::forward<Callable>(op),
          std::forward<Args>(args)…); …
          return;
      } else {
          // return type is not void:
          decltype(auto) ret{std::invoke(std::forward<Callable>(op),
          std::forward<Args>(args)…)}; …
          return ret;
       }
  }
  
  ```

### 其他一些实现泛型库的工具

比如：

```c++
#include <type_traits>
template<typename T>
class C
{
    // ensure that T is not void (ignoring const or volatile):
    static_assert(!std::is_same_v<std::remove_cv_t<T>,void>,
    "invalid instantiation of class C for void type");
public:
    template<typename V>
    void f(V&& v) {
    //if constexpr(std::is_reference_v<T>) { … // special code if T is a 			reference type
    }
    if constexpr(std::is_convertible_v<std::decay_t<V>,T>) { … // special code 		//if V is convertible to T
    }
    if constexpr(std::has_virtual_destructor_v<V>) { … // special code if V has 	//virtual destructor
    }
    }
};
```

这里用到了编译期的if特性，该特性从17开始启用，作为替代选项，这里也可以使用std::enable_if、部分特例化或者SFINAE。使用类型萃取要格外小心，比如：

```c++
std::remove_const_t<int const&>
```

这里由于引用不是const类型，所以不会有任何效果，所以删除引用跟删除const的顺序就很重要：

```c++
std::remove_const_t<std::remove_reference_t<int const&>> // int
std::remove_reference_t<std::remove_const_t<int const&>> // int const
```

另一种方法是直接调用：

```c++
std::decay_t<int const&>
```

函数模板std::addressof<>()会返回一个对象或者函数的准确地址，即使一个对象重载了运算符&也是如此。

std::declval()可以被用作某一类型的对象的引用的占位符，该函数模板没有定义，因此不能被调用（也不会创建对象），因此其只能被用作不会被计算的操作数，比如：

```c++
#include <utility>
template<typename T1, typename T2,
typename RT = std::decay_t<decltype(true ? std::declval<T1>() :
std::declval<T2>())>>
RT max (T1 a, T2 b)
{
	return b < a ? a : b;
}
```

为了避免在调用运算符?:的时候不得不调用T1和T2的默认构造函数，这里使用了std::declval，不要忘了使用decay来确保返回类型不会是一个引用，因为std::declval本身返回的是右值引用。

### 完美转发临时变量

某些情况下我们需要转发不是通过参数传递过来的数据，此时可以使用auto&&来创建一个可以被转发的变量，如：

```c++
template<typename T>
void foo(T x)
{
    auto&& val = get(x); …
    // perfectly forward the return value of get() to set():
    set(std::forward<decltype(val)>(val));
}
```

这样可以避免对中间变量的多余拷贝。

### 作为模板参数的引用

虽然可以通过显式引用来将模板参数变成引用，但有可能会带来问题，比如：

```c++
template<typename T, T Z = T{}>
class RefMem {
private:
 	T zero;
public:
    RefMem(): zero{Z} {}
};
int main()
{
    RefMem<int> rm1, rm2;
    rm1 = rm2; // OK
    RefMem<int&> rm3; // ERROR: invalid default value for N
    RefMem<int&, 0> rm4; // ERROR: invalid default value for N extern
    int null;
    RefMem<int&,null> rm5, rm6;
    rm5 = rm6; // ERROR: operator= is deleted due to reference member
}
```

如果尝试用引用对其实例化，情况变的复杂：

- 非模板参数的默认初始化不在可行；
- 不再能够直接用0来初始化非参数模板参数；
- 赋值运算符也不再可用，因为对于具有非静态引用成员的类，其默认赋值运算符会被删除；

为了禁止用引用类型进行实例化，一个简单的static_assert就够了：

```c++
template<typename T>
class optional
{
    static_assert(!std::is_reference<T>::value, "Invalid
    instantiation of optional<T> for references"); …
};
```

### 推迟计算

比如这样的一个例子：

```c++
template<typename T>
class Cont {
private:
    T* elems;
public: …
    typename
    std::conditional<std::is_move_constructible<T>::value, T&&,
    	T& >::type foo();
};
```

通过使用std::conditional来决定foo的返回类型是T&&还是T&，决策标准是看T是否支持移动语义，问题在于std::is_move_constructible要求其参数必须是完整类型，为了解决这一问题，需要使用一个成员模板代替现有foo的定义，这样将std::is_move_constructible的计算推迟到foo的实例化阶段：

```c++
template<typename T>
class Cont {
private:
    T* elems;
public:
    template<typename D = T>
    typename
    std::conditional<std::is_move_constructible<D>::value, T&&,
    	T&>::type foo();
};
```

实质是利用了模板只有被调用才会被实例化和两阶段编译检查的特性。

## 深入模板基础

### 参数化声明

C++目前支持四种基础模板：类模板、函数模板、变量模板以及别名模板。这四种模板也可以出现在类中，变量模板在类作用域中将作为一个静态数据成员模板：

```c++
class Collection {
  public:
  	template<typename T> 	// an in-class member class template definition
	class Node {
	  ...
	};
	
	template<typename T>	// an in-class (and therefore implicitly inline)
	T* alloc() {			// member function template definition
	  ...
	}
	
	template<typename T>	// a member variable template (since c++14)
	static T zero = 0;
	
	template<typename T>	// a member alias template
	using NodePtr = Node<T>*;
};
```

注意在c++17中，变量（包括静态数据成员）以及变量模板都是可以内联的，这意味着它们的定义可以跨越多个编译单元重复。以下代码展示了如何在类外定义别名模板以外的模板：

```c++
template<typename T>	// a namespace scope class template
class List {
  public:
    List() = default;	// because a template constructor is defined
	
	template<typename U>	// another member class template,
	class Handle;			// without its defination
	
	template<typename U>	// a member function template
	List (List<U> const&);	// (constructor)
	
	template<typename U>	// a member variable template (since C++14)
	static U zero;
};

template<typename T>	// out-of-class member class template definition
template<typename U>
class List<T>::Handle {
  ...
};

template<typename T>	// out-of-class member function template definition
template<typename T2>
List<T>::List(List<T2> const& b)
{
  ...
}

template<typename T>	// out-of-class static data member template definition
template<typename U>
U List<T>::zero = 0;
```

定义在类外的成员模板需要多个template<...>参数化子句：每个外围作用域的类模板一个，成员模板本身也需要一个。另外注意到构造器模板会禁用掉隐式声明的默认构造器，因此增加了一个默认的构造函数。

联合体模板也是可以的（其被视为一种类模板）：

```c++
template<typename T>
union AllocChunk {
	T object;
	unsigned char bytes[sizeof(T)];
};
```

函数模板可以有默认参数：

```c++
template<typename T>
void report_top(Stack<T> const&, int number = 10);
template<typename T>
void fill(Array<T>&, T const& = T{}); // T{} is zero for built-in types
```

17之后，静态成员可以在类模板内部使用inline关键字初始化：

```c++
template<int I>
class CupBoard {
	...
	inline static double totalWeight = 0.0;
};
```

成员函数模板不能被声明为virtual，这是因为虚函数机制依赖一个固定大小的虚表，其中存储了每一个虚函数条目，但成员函数模板直到整个程序被编译之前，实例化的个数都无法确定。与之相反，类模板的普通成员函数可以是virtual。

每个模板必须有一个名字，并且该名字必须是所属作用域内独一无二的，除了函数模板重载的场景。特别注意，与类类型不同，类模板无法与不同类型的实体共享名称：

```c++
int C;
...
class C;	// OK: class names and nonclass names are in a different "space"

int X;
...
template<typename T>
class X;	// ERROR: conflict with variable X

struct S;
...
template<typename T>
class S;	// ERROR: conflict with struct S
```

模板名称具有链接，但其无法拥有C链接：

```c++
extern "C++" template<typename T>
void normal();		// this is the default: the linkage specification could be left out

extern "C" template<typename T>
void invalid();		// ERROR: templates cannot have C linkage

extern "Java" template<typename T>
void javaLink();	// nonstandard, but maybe some compiler will someday 
					// support linkage compatible with Java generics
```

模板通常有外部链接，唯一的一些例外是命名空间作用域中具有静态限定符的函数模板、匿名空间的直接或间接的成员的模板（它们拥有内部链接）以及匿名类的成员模板（它们没有链接）：

```c++
template<typename T> 	// refers to the same entity as a declaration of the 
void external();		// same name (and scope) in another file

template<typename T>	// unrelated to a template with the same name in
static void internal();	// another file

template<typename T>	// redeclaration of the previous declaration
static void internal();

namespace {
  template<typename>	// also unrelated to a template with the same name
  void otherInternal();	// in another file, even one that similarly appears
}						// in an unnamed namespace

namespace {
  template<typename>	// redeclaration of the previous template declaration
  void otherInternal();
}

struct {
  template<typename T> void f(T) {} // no linkage: cannot be redeclared
} x;
```

当前，模板无法在函数作用域或局部类作用域中声明，但泛化的lambda可以。

### 模板参数

有三种基本类型的模板参数：类型参数、非类型模板参数、模板模板参数。

在模板声明内，类型参数的行为与类型别名非常相似，例如，当T是模板参数时，则类内不能有class T之类的名称。

非类型模板参数表示一个可以在编译期或链接期确定的常量值，这样的参数类型必须是以下之一：

- 整形或枚举型
- 指针类型
- 成员指针类型
- 左值引用类型（既可以是对象引用，也可以是函数引用）
- `std::nullptr_t`
- 包含`auto`或`decltype(auto)`的类型（17之后支持）

在某些情况下，非类型模板参数的声明也可以用关键字typename开头：

```c++
template<typename T, 			// a type parameter
		typename T::Allocator* Allocator>	// a nontype parameter
class List;

template<class X*>		// a nontype parameter of pointer type
class Y;
```

函数和数组类型可以被指定，但它们会退化隐式的调整为相应的指针类型：

```c++
template<int buf[5]> class Lexer;		// buf is really an int*
template<int* buf> class Lexer;			// OK: this is a redeclaration

template<int fun()> struct FuncWrap;	// fun really has pointer to
										// function type
template<int (*)()> struct FuncWrap;	// OK: this is a redeclaration
```

非类型模板参数的声明与变量声明相似，但它们不可以有非类型指示符，比如static、mutable等，它们可以有const和volatile限定符，但这种限定符出现在参数类型的最顶层就会被忽略（即对左值引用或指针来说支持底层const）：

```c++
template<int const length> class Buffer;	// const is useless here
template<int length> class Buffer;			// same as previous declaration
```

最后，在表达式使用时，非引用类型的非类型参数始终都是prvalues（pure right value，即纯右值），它们的地址无法被窃取也无法被赋值，另一方面左值引用类型的非类型参数可以像左值一样使用，右值引用是不允许的。

模板模板参数是类或别名模板的占位符，其声明与类模板很像，但不能使用关键字struct或union：

```c++
template<template<typename X> class C>		// OK
void f(C<int>* p);

template<template<typename X> struct C>		// ERROR: struct not valid here
void f(C<int>* p);

template<template<typename X> union C>		// ERROR: union not valid here
void f(C<int>* p);
```

C++17允许用typename替代class，因此，模板模板参数不仅可以由类模板替代，而且可以由别名模板替代：

```c++
template<template<typename X> typename C>		// OK since C++17
void f(C<int>* p);
```

模板模板参数的参数可以有默认模板参数。

从11开始，可以通过在模板参数名称之前引入省略号来将任何类型的模板参数转换为模板参数包（如果模板参数匿名，那么就在模板参数名称本该出现的位置之前）：

```c++
template<typename... Types>		// declares a template parameter pack named Types
class Tuple;
```

主模板中的类模板、变量模板和别名模板至多只可以有一个模板参数包，且模板参数包必须作为最后一个模板参数。函数模板则少了一些限制：允许多个模板参数包，只有模板参数包后面的每个模板参数都具有默认值或可以推导，比如：

```c++
template<typename... Types, typename Last>
class LastType;		// ERROR: template parameter pack is not the last template parameter

template<typename... TestTypes, typename T>
void runTests(T value);		// OK: template parameter pack is followed
							// by a deducible template parameter
template<unsigned...> struct Tensor;
template<unsigned... Dims1, unsigned... Dims2>
auto compose(Tensor<Dims1...>, Tensor<Dims2...>);	// OK: the tensor dimensions can be deduced
```

类和模板的偏特化声明可以有多个参数包，这与主模板不同，因为偏特化是通过与函数模板几乎相同的推导过程选择的：

```c++
template<typename...> Typelist;
template<typename X, typename Y> struct Zip;
template<typename... Xs, typename... Ys>
struct Zip<Typelist<Xs...>, Typelist<Ys...>>;
			// OK: partial specialization uses deduction to determine
			// the Xs and Ys substitutions
```

当然，类型参数包不能在自己的参数子句中进行扩展：

```c++
template<typename... Ts, Ts... vals> struct StaticValues {};
	// ERROR: Ts cannot be expanded in its own parameter  list
```

但嵌套模板可以创建类似的场景：

```c++
template<typename... Ts> struct ArgList {
  template<Ts... vals> struct Vals {};
};
ArgList<int, char, char>::Vals<3, 'x', 'y'> tada;
```

非模板参数包的任何类别的模板参数都可以配置默认参数，默认实参不能依赖自身的参数，因为参数的名称直到默认实参之后才在作用域生效，但其可以依赖前面的参数：

```c++
template<typename T, typename Allocator = allocator<T>>
class List;
```

只有在后续参数提供了默认参数时，类模板、变量模板或别名模板的模板参数才可以具有默认模板实参：

```c++
template<typename T1, typename T2, typename T3,
		 typename T4 = char, typename T5 = char>
class Quintuple;	// OK

template<typename T1, typename T2, typename T3 = char,
		 typename T4, typename T5>
class Quintuple;	// OK: T4 and T5 already have defaults

template<typename T1 = char, typename T2, typename T3, 
		 typename T4, typename T5>
class Quintuple;	// ERROR: T1 cannot have a default argument
					// because T2 doesn't have a default
```

不过函数模板的模板参数的默认模板实参不需要后续的模板参数必须有一个默认模板实参，比如：

```c++
template<typename R = void, typename T>
R* addressof(T& value);	// OK: if not explicitly specified, R will be void
```

默认模板实参不允许重复声明：

```c++
template<typename T = void>
class Value;

template<typename T = void>
class Value;	// ERROR: repeated default argument
```

许多上下文不允许使用默认模板实参：

- 偏特化：

  ```c++
  template<typename T>
  class C;
  ...
  template<typename T = int>
  class C<T*>;	// ERROR
  ```

- 参数包：

  ```c++
  template<typename... Ts = int> struct X;	// ERROR
  ```

  

- 类模板成员类外定义：

  ```c++
  template<typename T> 
  struct x
  {
  	T f();
  };
  
  template<typename T = int> // ERROR
  T X<T>::f() {		
    ...
  }
  ```

  

- 友元类模板声明：

  ```c++
  struct S {
    template<typename = void> friend struct F;
  };
  ```

  

- 友元函数模板声明，除非它是一个定义并且它在编译单元的其他任何地方都没有声明：

  ```c++
  struct S{
    template<typename = void> friend void f();	// ERROR: not a definition
    template<typename = void> friend void g() {	// OK so far
    }
  };
  
  template<typename> void g();	// ERROR: g() was given a default template argument
  								// when defined; no other declaration may exist here
  ```

### 模板实参

模板实参可以被各种不同类型的机制所判定：

- 显式模板实参：模板名称后面跟随在尖括号内显式指定的模板实参，这种叫做模板ID；
- 注入式类名：在类模板的作用域内，模板X的名称等价于模板ID，`X<P1, P2, ...>`；
- 默认模板实参：注意对于类模板或别名模板来说，即使模板参数有默认值，尖括号也不能省略；
- 实参推导：没有被显式指定的函数模板参数会通过函数调用的实参类型进行推导；

非类型实参是指那些替换非类型模板参数的值，这些值必须是以下几种：

- 另一个具有正确类型的非类型模板参数；
- 整型（或枚举）类型的编译器常量；
- 外部变量或函数的名称，其前面带有内置的一元&运算符，对于函数和数组变量，可以省略&。此类模板实参与指针类型的非类型参数匹配，17放宽了要求，允许任何的常量表达式产生一个指向函数或变量的指针；
- 对于引用类型的非类型参数，前一种（但不带&）实参是有效实参，同样17也放宽了约束，允许任意的常量表达式应用于函数或变量；
- 成员指针常量，即类似&C::m；
- 空指针常量对指针或成员函数指针的非类型参数都是合法的；

17以前，将实参与作为指针或引用的参数进行匹配时，不会考虑用户定义的转换和派生类到基类的转换。以下是一些例子：

```c++
template<typename T, T nontypeParam>
class C;

C<int, 33>* c1;		// integer type
int a;
C<int*, &a>* c2;	// address of an external variable

void f();
void f(int);
C<void (*)(int), f>* c3;	// name of a function: overload resolution selects
							// f(int) in this case; the & is implied
							
template<typename T> void templ_func();
C<void(), &templ_func<double>>* c4;	// function template instantiations are functions
struct X {
  static bool b;
  int n;
  constexpr operator int() const { return 42; }
};

C<bool&, X::b>* c5;	// static class members are acceptable variable/function names

C<int X::*, &X::n>* c6;	// an example of a pointer-to-member constant

C<long, X{}>* c7;		// OK: X is the first converted to int via a constexpr conversion
						// function and then to long via a standard integer conversion
```

模板实参的一个限制就是编译器或链接器必须在程序构建时有能力表示它们的值，运行前无法知晓的值（像局部变量的地址）就不行，尽管如此，有一些常量竟然是无效的：

- 浮点数；
- 字符串字面量（11之前空指针常量也不行）

字符串字面值的一个问题在于相同的字面量可以存在不同的地址，可以采取一种迂回的方法来解决这个问题，这涉及到引入了一个附加变量来保存字符串：

```c++
template<char const *str>
class Message {
  ...
};

extern char const hello[] = "Hello Wolrd!";
char const hello11[] = "Hello World!";

void foo()
{
  static char const hello17[] = "Hello World!";
  
  Message<hello> msg03;		// OK in all versions
  Message<hello11> msg11;	// OK since C++11
  Message<hello17> msg17;	// OK since C++17
}
```

从上面可以看出，声明为引用或指针的非类型模板参数必须是一个在C++版本中拥有外部链接的常量表达式，从11开始内部链接也可，17则是只要有任意某个链接即可。

这里也有一些不合法的例子：

```c++
template<typename T, T nontypeParam>
class C;

struct Base {
  int i;
} base;

struct Derived : public Base {
} derived;

C<Base*, &derived>* err1;		// ERROR: derived-to-base conversions are not considered

C<int&, base.i>* err2;			// ERROR: fields of variables aren't considered to be variables

int a[10];
C<int*, &a[0]>* err3;			// ERROR: addresses of array elements aren't acceptable either
```

在17之前，模板模板实参的默认实参会被忽略，即模板不认为这是默认值。17之后放宽了这一规则，只需要模板模板参数至少被相应的模板模板实参特化，在17之前，以下代码非法：

```c++
#include <list>
	// declares in namespace std:
	// template<typename T, typename Allocator=allocator<T>>
	// class list;
template<typename T1, typename T2, template<typename> class Cont> // Cont expects one parameter
class Rel {
  ...
};

Rel<int, double, std::list> rel;	// ERROR before C++17: std::list has more than 
									// one template parameter
```

原因在于std::list拥有多于一个的模板参数，第二个参数拥有默认值，在17之前这将被忽略。相比较之下，可变参数模板是个例外：

```c++
#include <list>

template<typename T1, typename T2, 
		 template<typename... > class Cont>		// Cont expects any number of 
class Rel {										// type parameters
  ...
};

Rel<int, double, std::list> rel;				// OK: std::list has two template parameters
												// but can be used with one argument
```

当两组模板实参的每一对参数值相同时，其被视为等效的，如下：

```c++
template<typename T, int I>
class Mix;

using Int = int;

Mix<int, 3*3>* p1;
Mix<int, 4+5>* p2;	// p2 has the same type as p1
```

成员函数模板生成的函数永远不会覆盖(override)虚函数。

构造器模板生成的构造器永远不会是拷贝或移动构造器。类似的，赋值操作符模板生成的赋值操作符函数也永远不会是拷贝赋值或是移动赋值操作符函数。（然而，由于隐式调用拷贝赋值或移动赋值操作符函数的情景相对少，所以这一般不会引起问题。）

### 可变模板

sizeof...表达式是包展开的一个例子，以下也是：

```c++
template<typename ...Types>
class MyTuple : public Tuple<Types..> {
	// extra operations provided only for MyTuple
};

MyTuple<int, float> t2;	// inherits from Tuple<int, float>
```

每个包展开都有一个模式，是一个被实参包的每个实参所替代的类型或表达式，比如：

```c++
template<typename... Types>
class PtrTuple : public Tuple<Types*...> {
  // extra operations provided only for PtrTuple
};

PtrTuple<int, float> t3;	// Inherits from Tuple<int*, float*>
```

包展开基本可以在语法提供逗号分隔列表的任何位置使用，比如：

- 基类列表；
- 构造器中的基类初始化列表；
- 调用实参列表；
- 初始化列表；
- lei、函数或别名模板的模板参数列表；
- 函数可以抛出的一场列表（11开始不建议使用，17之后不再允许）；
- 在属性内；
- 指定某个声明的对其方式时；
- 指定lambda表达式捕获列表时；
- 函数类型的参数列表；
- 在using声明中（17开始支持）；

示例如下：

```c++
template<typename... Mixins>
class Point : public Mixins... {	// base class pack expansion
    double x, y, z;
  public:
    Point() : Mixins()... { }		// base class initializer pack expansion
	template<typename Visitor>
	void visitMixins(Visitor visitor) {
	  visitor(static_cast<Mixins&>(*this)...);	// call argument pack expansion
	}
};

struct Color { char red, green, blue; };
struct Label { std::string name; };
Point<Color, Label> p;		// inherits from both Color and Label
```

包展开也在模板参数列表中创建非类型模板参数包时使用：

```c++
template<typename... Ts>
struct Values {
  template<Ts... Vs>
  struct Holder {
  };
};

int i;
Values<char, int, int*>::Holder<'a', 17, &i> valueHolder;
```

17引入的折叠表达式可以应用于除了.、->和[]以外的所有二元操作符。给定一个未展开表达式模式pack和一个非模式表达式value，17允许使用任意操作符op写出：

```c++
(pack op ... op value) //作为操作符右折叠
(value op ... op pack) //作为操作符左折叠
```

17还支持一元右折叠和一元左折叠，但要注意空展开，除了以下三种特例：

- 单一折叠`&&`对空展开产生一个值`true`。
- 单一折叠`||`对空展开产生一个值`false`。
- 单一折叠`,`会产生表达式`void`。

### 友元

友元有以下特点：

1. 友元的声明必须唯一；
2. 友元函数声明时可以直接定义；

友元类特别的一点是能够将类模板的特定实例声明为友元：

```c++
template<typename T>
class Node;

template<typename T>
class Tree {
  friend class Node<T>;
  ...
};
```

注意，类模板必须在其实例之一成为类或类模板的友元时是可见的（即有完整的定义），对普通类来说，无此要求：

```c++
template<typename T>
class Tree {
  friend class Factory;	// OK even if first declaration of Factory
  friend class Node<T>;	// error if Node isn't visible
};
```

11也增加了让模板参数作友元的语法：

```c++
template<typename T>
class Wrap {
  friend T;
  ...
};
```

如果T不是一个类类型，友元会被忽略。

函数模板的实例可以作为友元，只要友元函数名称后跟一个尖括号子句即可。

如果名称后没有跟尖括号子句，那么有两种可能：

1. 如果名字没有限定符（换句话说，不包含`::`），它永远不会是一个模板实例。如果友元声明时不存在可见的匹配的非模板函数，此处的友元声明就作为该函数的第一次声明。该声明也可以是一个定义。
2. 如果名称带有限定符（包含`::`），该名称必须可以引用到一个此前声明过的函数或函数模板。非模板函数会比函数模板优先匹配。然而，这里的友元声明不能是一个定义。这里有个例子来说明这一区别：

```c++
void multiply(void*);	// ordinary function

template<typename T>
void multiply(T);		// function template

class Comrades {
  friend void multiply(int) { }	// defines a new function ::multiply(int)
  
  friend void ::multiply(void*); //refers to the ordinary function above,
  								// not the the multiply<void*> instance
  friend void ::multiply(int);	// refers to an instance of the template
  friend void ::multiply<double*>(double*);	// qualified names can also have angle brackets,
  											// but a template must be visible
  friend void ::error() { }	// ERROR: a qualified friend cannot be a definition
};
```

函数模板也可以在类模板中定义，此时只有在它真正被使用到时才会实例化。通常，这要求友元函数以友元函数的类型使用类模板本身，这使得在类模板上表示函数变得更容易，就好像它们在命名空间中可见一样：

```c++
template<typename T>
class Creator {
  friend void feed(Creator<T>) { //every T instantiates a different function ::feed()
    ...
  }
};

int main()
{
  Creator<void> one;
  feed(one);		// instantiates ::feed(Creator<void>)
  Creator<double> two;
  feed(two);		// instantiates ::feed(Creator<double>)
}
```

友元模板如下：

```c++
class Manager {
  template<typename T>
  friend class Task;
  
  template<typename T>
  friend void Schedule<T>::dispatch(Task<T>*);
  
  template<typename T>
  friend int ticket() {
    return ++Manager::counter;
  }
  
  static int counter;
};
```

与普通的友元声明一样，当名称是不含限定符的函数名时友元模板也可以是一个定义，函数名后不接尖括号子句。友元模板只能定义主模板和主模板的成员。主模板的偏特化和显式特化都会被自动视为友元。

## 模板中的名称

### 名称的分类

有以下两种名称的概念：

1. 如果名称作用域由操作符::或成员访问操作符.或->显式指定，则称该名称为受限名称；
2. 如果一个名称以某种方式依赖于模板参数，那么该名称就是一个依赖型名称；

### 名称查找

参数依赖查找（ADL）产生的动机：

```c++
template<typename T>
T max(T a, T b)
{
  return b < a ? a : b;
}
namespace BigMath {
  class BigNumber {
    ...
  };
  
  bool operator < (BigNumber const &, BigNumber const &);
  ...
}

using BigMath::BigNumber;

void g(BigNumber const& a, BigNumber const& b)
{
  ...
  BigNumber x = ::max(a,b);
  ...
}
```

由于max模板不认识BigMath命名空间，普通的查找无法找到BigNumber适用的operator<。ADL主要应用于非限定名称，如果普通查找找到了以下内容，那么ADL不会发生：

- 成员函数名称；
- 变量名称；
- 类型名称；
- 块作用域函数声明名称；

如果把被调用函数名称用圆括号括起来，ADL也会被禁用。否则，如果名称后的括号里面有实参表达式列表，则ADL会查找这些实参关联的命名空间和类，例如某一类型是一个class X的指针，那么关联的类和命名空间就包括X和X所属的任何命名空间和类。唯一例外是ADL会忽略using指示符。

```c++
#include <iostream>

namespace X {
  template<typename T> void f(T);
}

namespace N {
  using namespace X;
  enum E { e1 };
  void f(E) {
    std::cout << "N::f(N::E) called\n";
  }
}

void f(int)
{
  std::cout << "::f(int) called\n";
}

int main()
{
  ::f(N::e1);	// qualified function name: no ADL
  f(N::e1);		// ordinary lookup finds ::f() and ADL finds N::f(),
  				// the latter is preferred
}
```

### 解析模板

一个模板无法引用另一个模板的名称，因为其他模板的内容可能因特化而使得原来的名称失效，例如以下的例子：

```c++
template<typename T>
class Trap {
  public:
    enum { x };	// #1 x is not a type here
};

template<typename T>
class Victim {
  public:
    int y;
	void poof() {
	  Trap<T>::x * y;	// #2 declaration or multiplication?
	}
};

template<>
class Trap<void> {	// evil specialization!
  public:
    using x = int;	// #3 x is a type here
};

boid boom(Victim<void>& bomb)
{
  bomb.poof();
}
```

C++这样来解决这个问题，一个依赖型受限名称并不代表一个类型，除非在名字前面加上typename前缀。当类型名称具有以下性质，就应该添加typename前缀：

1. 名称受限，且本身没有跟::组成更加受限的名称；
2. 名称不是详细类型说明符的一部分（例如class、struct、union或enum起始关键字）；
3. 名称不在指定基类继承的列表中，也不在引入构造函数的成员初始化列表中；
4. 名称依赖于模板参数；
5. 名称是某个未知的特化的成员，这意味着由限定器命名的类型指代一个未知的特化；

示例如下：

```c++
template<typename T>				// 1
struct S : typename X<T>::Base {	// 2
  S() : typename X<T>::Base(typename X<T>::Base(0)) {	// 3 4
  }
  
  typename X<T> f() {				// 5
    typename X<T>::C * p;			// declaration of pointer p	// 6
	X<T>::D *q;
  }
  
  typename X<int>::C *s;			// 7
  
  using Type = T;
  using OtherType = typename S<T>::Type;	// 8
}
```

每个出现的`typename`，不管正确与否，都被标了号。第一个`typename`表示一个模板参数。前面的规则没有应用于此。第二个和第三个`typename`由于上述规则的第三条而被禁止。这两个上下文中，基类的名称不能用`typename`引导。然而，第四个`typename`是必不可少的，因为这里基类的名称既不是位于初始化列表，也不是位于派生类的继承列表，而是为了基于实参`0`构造一个临时`X<T>::Base`表达式（也可以是某种强制类型转换）。第5个`typename`同样不合法，因为它后面的名称`X<T>`并不是一个受限名称。对于第6个`typename`，如果期望声明一个指针，那么这个`typename`是必不可少的。下一行省略了关键字`typename`，因此也就被编译器解释为一个乘法表达式。第7个`typename`是可选（可有可无）的，因为它符合前面的两条规则，但不符合后面的两条规则。第8个`typename`也是可选的，因为它指代的是一个当前实例的成员（也就不满足最后一条规则）。

最后一条判断`typename`前缀是否需要的规则有时候难以评估，因为它取决于判断类型所指代的是一个当前实例还是一个未知的特化这一事实。在这种场景中，最简单安全的方法就是直接添加`typename`关键字来指示受限名称是一个类型。`typename`关键字，尽管它是可选的，也会提供一个意图上的说明。

依赖型模板名称的示例：

```c++
template<typename T>
class Shell {
  public:
    template<int N>
	class In {
	  public:
	    template<int M>
		class Deep {
		  public:
		    virtual void f();
		};
	};
};

template<typename T, int N>
class Weird {
  public:
    void case1 (typename Shell<T>::template In<N>::template Deep<N>* p) {
	  p->template Deep<N>::f();		// inhibit virtual call
	}
	void case2 (typename Shell<T>::template In<N>::template Deep<N>& p) {
	  p.template Deep<N>::f();		// inhibit virtual call
	}
};
```

限定符前面的名称或表达式的类型需要依赖某个模板参数，并且紧跟限定符后面的是一个模板id，那么就应该使用关键字template。

Using声明会从命名空间和类引入名称，有以下的例子：

```c++
class BX {
  public:
    void f(int);
	void f(char const*);
	void g();
};

class DX : private BX {
  public:
    using BX::f;
};
```

从以上的例子中可以看出，using声明可以让以前不能访问的成员变成可访问的。但可能也会有问题，引入名称之后我们不知道这个名称是一个类型还是一个模板，或是别的东西：

```c++
template<typename T>
class BXT {
  public:
    using Mystery = T;
	template<typename U>
	struct Magic;
};

template<typename T>
class DXTT : private BXT<T> {
  public:
    using typename BXT<T>::Mystery;
	Mystery* p;		// would be a syntax error without the earlier typename
};
```

当想要使用using声明引入依赖型名称来指定类型时，必须显式的插入typename关键字前缀，但当这个名称是模板时，则没有这样的标准：

```c++
template<typename T>
class DXTM : private BXT<T> {
  public:
    using BXT<T>::template Magic;	// ERROR: not standard
	Magic<T>* plink;				// SYNTAX ERROR: Magic is not a known template
};
```

但可以迂回的采用如下的方案：

```c++
template<typename T>
class DXTM : private BXT<T> {
  public:
    template<typename U>
	  using Magic = typename BXT<T>::template Magic<T>;	// Alias template
	  Magic<T>* plink;									// OK
};
```

考虑这样的例子：

```c++
namespace N {
  class X {
    ...
  };
  
  template<int I> void select(X*);
}

void g(N::X* xp)
{
  select<3>(xp);	// ERROR: no ADL!
}
```

出错原因在于直到确定select是一个模板之前都无法确定\<3\>是一个模板实参列表，但可以在调用前引入select的函数模板声明来解决这个问题：

```c++
template<typename T> void select();
```

依赖表达式最常见的是类型依赖表达式，如下：

```c++
template<typename T> void typeDependent1(T x)
{
  x;	// the expression type-dependent, because the type of x can vary
}
```

还有这样的例子：

```c++
template<typename T> void typeDependent2(T x)
{
  f(x);		// the expression is type-dependent, because x is type-dependent
}
```

f(x)的类型可能因实例的变化而有所不同，因此两阶段查找可能在不同的实例中找到完全不同的函数名f。

除此之外，还有值依赖表达式：

```c++
template<int N> void valueDependent1()
{
  N;	// the expression is value-dependent but not type-dependent;
  		// because N has a fixed type but a varying constant type
}
```

有趣的是，一些操作符，比如sizeof，拥有一个已知的结果类型，可以将一个类型依赖操作数转换成一个值依赖表达式，例如：

```c++
template<typename T> void valueDependent2(T x)
{
  sizeof(x);	// the expression is value-dependent but not type-dependent
}
```

当所有的模板实例都会产生错误时，编译期允许在解析模板时诊断错误。

### 派生和类模板

非依赖型基类示例如下：

```c++
template<typename X>
class Base {
  public:
    int basefield;
	using T = int;
};

class D1 : public Base<Base<void>> {	// not a template case really
  public:
    void f() { basefield = 3; }			// usual access to inherited member
};

template<typename T>
class D2 : public Base<double> {		// nondependent base
  public:
    void f() { basefield = 7; }			// usual access to inherited member
	T strange;							// T is Base<double>::T, not the template parameter!
};
```

注意，当非受限名称在模板继承中被找到时，非依赖型基类会优先考虑该名称之后才轮到模板参数列表。

对于依赖型基类，考虑以下的问题：

```c++
template<typename T>
class DD : public Base<T> {		// dependent base
  public:
    void f() { basefield = 0; }	// #1 problem
};

template<>	// explicit specialization
class Base<bool>{
  public:
    enum { basefield = 42 };	// #2 tricky!
};

void g(DD<bool>& d)
{
  d.f();						// #3 oops?
}
```

在1处将basefield与int类型进行了绑定了，那么2处的特例化会更改basefield的意义，当在3处使用时就会报错。为了解决这个问题，标准规定非依赖型名称不会在依赖型基类中进行查找，为了解决上述问题，可以将非依赖型名称改为依赖型，比如：

```c++
template<typename T>
class DD1 : public Base<T> {
  public:
    void f() { this->basefield = 0; }	// lookup delayed
};
```

还可以使用受限名称来引入依赖性：

```c++
template<typename T>
class DD2 : public Base<T> {
  public:
    void f() { Base<T>::basefield = 0; }
};
```

这种情况要格外小心，如果原来的非受限的非依赖型名称被用于虚函数调用的话，那么这种引入依赖性的限定会禁止虚函数调用。

## 模板的多态性

### 动态多态

即通过继承虚函数并重写实现的多态

### 静态多态

模板也可以用来实现多态，但其不依赖于对基类中公共行为的重写，但其要求不同的类型必须使用相同的语法操作（相关函数的名称相同）。在定义上，具体的类之间相互独立。示例如下：

```c++
//动态多态
void myDraw (GeoObj const& obj) // GeoObj is abstract base
class
{
	obj.draw();
}
//静态多态
template<typename GeoObj>
void myDraw (GeoObj const& obj) // GeoObj is template
parameter
{
	obj.draw();
}
```

其主要区别在于将GeoObj作为模板参数而不是公共基类。完整的示例如下：

```c++
#include "coord.hpp"
// concrete geometric object class Circle
// - not derived from any class
class Circle {
public:
    void draw() const;
    Coord center_of_gravity() const；
 …
};
// concrete geometric object class Line
// - not derived from any class
class Line {
public:
    void draw() const;
    Coord center_of_gravity() const; …
};
…
    
#include "statichier.hpp"
#include <vector>
// draw any GeoObj
template<typename GeoObj>
void myDraw (GeoObj const& obj)
{
	obj.draw(); // call draw() according to type of object
}
// compute distance of center of gravity between two GeoObjs
template<typename GeoObj1, typename GeoObj2>
Coord distance (GeoObj1 const& x1, GeoObj2 const& x2)
{
    Coord c = x1.center_of_gravity() - x2.center_of_gravity();
    return c.abs(); // return coordinates as absolute values
}
// draw homogeneous collection of GeoObjs
template<typename GeoObj>
void drawElems (std::vector<GeoObj> const& elems)
{
    for (unsigned i=0; i<elems.size(); ++i) {
    elems[i].draw(); // call draw() according to type of element
	}
}
int main()
{
    Line l;
    Circle c, c1, c2;
	myDraw(l); // myDraw<Line>(GeoObj&) => Line::draw()
    myDraw(c); // myDraw<Circle>(GeoObj&) =>
    Circle::draw()
    distance(c1,c2); //distance<Circle,Circle>
    (GeoObj1&,GeoObj2&)
    distance(l,c); // distance<Line,Circle>(GeoObj1&,GeoObj2&)
    // std::vector<GeoObj*> coll; //ERROR: no heterogeneous
    collection possible
    std::vector<Line> coll; // OK: homogeneous collection
    possible
    coll.push_back(l); // insert line
    drawElems(coll); // draw all lines
}
```

静态多态的限制：所有的类型必须在编译期可知。

### 动态多态vs静态多态

动态多态有以下优点：

- 可以很优雅的处理异质集合；
- 可执行文件的大小可能会比较小；
- 代码可以被完整的编译，没有必须要公开的代码；

静态多态的优点：

- 内置类型的集合可以被很容易的实现；
- 产生的代码可能会更快（不需要指针的重定向，先验的非虚也更容易被inline）；
- 即使某个类型只提供了部分的接口也可以用于静态多态，只要不使用那些没有实现的接口；

通常认为静态多态要比动态多态更类型安全，因为其所有的绑定都在编译期进行了检查。

### 使用concepts

其代表为了能够成功的实例化模板，模板参数必须要满足的一组约束条件。其示例如下：

```c++
#include "coord.hpp"
template<typename T>
concept GeoObj = requires(T x) {
    { x.draw() } -> void;
    { x.center_of_gravity() } -> Coord; …
};
```

### 新形势下的设计模式

比如桥接模式，转变如下图：

![bridgePatten](..\image\template\bridgePatten.png)

## 萃取的实现

### 一个例子：对一个序列求和

比如这个求和的例子：

```c++
#ifndef ACCUM_HPP
#define ACCUM_HPP
template<typename T>
T accum (T const* beg, T const* end)
{
    T total{}; // assume this actually creates a zero value
    while (beg != end) {
        total += *beg;
        ++beg;
    }
    return total;
}
#endif //ACCUM_HPP
```

如果对一些字符求和，由于字符的值基本上都是通过ASCII码决定的，这样很容易返回一个值超出char类型的范围，虽然可以通过增加一个返回值参数来解决，但这完全是可以避免的，可以在T和与之对应的返回值类型之间建立某种联系，可通过偏特化实现：

```c++
template<typename T>
struct AccumulationTraits;
template<>
struct AccumulationTraits<char> {
	using AccT = int;
};
template<>
struct AccumulationTraits<short> {
	using AccT = int;
};
template<>
struct AccumulationTraits<int> {
	using AccT = long;
};
template<>
struct AccumulationTraits<unsigned int> {
	using AccT = unsigned long;
};
template<>
struct AccumulationTraits<float> {
	using AccT = double;
};

#ifndef ACCUM_HPP
#define ACCUM_HPP
#include "accumtraits2.hpp"
template<typename T>
auto accum (T const* beg, T const* end)
{
// return type is traits of the element type
    using AccT = typename AccumulationTraits<T>::AccT;
    AccT total{}; // assume this actually creates a zero value
    while (beg != end) {
        total += *beg;
        ++beg;
    }
    return total;
}
#endif //ACCUM
```

AccumulationTraits模板就被称为萃取模板，它提取了其参数类型的特性。

萃取一般代表的都是特定主类型的额外的类型信息，但额外信息不局限于类型信息，还可以将常量以及其他数值类和一个类型关联起来。比如为前面求和的初始值添加一个值萃取（值特性？）：

```c++
template<typename T>
struct AccumulationTraits;
template<>
struct AccumulationTraits<char> {
    using AccT = int;
    static AccT const zero = 0;
};
template<>
struct AccumulationTraits<short> {
    using AccT = int;
    static AccT const zero = 0;
};
template<>
struct AccumulationTraits<int> {
    using AccT = long;
    static AccT const zero = 0;
};

using AccT = typename AccumulationTraits<T>::AccT;
AccT total = AccumulationTraits<T>::zero; // init total by trai
```

这一类型的不足之处在于，C++只允许我们在类中对一个整型或枚举类型的static const成员进行初始化。constexpr的static数据成员好一些，允许对float类型以及其他字面值进行类内初始化：

```c++
template<>
struct AccumulationTraits<float> {
    using Acct = float;
    static constexpr float zero = 0.0f;
};
```

但是无论是const还是constexpr都禁止对非字面值类型进行这一类的初始化，比如用户自定义的任意精度的BigInt类型，因为它可能将一部分信息存储在堆中，或者是因为我们需要的构造函数不是constexpr的：

```c++
class BigInt {
	BigInt(long long); …
};…
template<>
struct AccumulationTraits<BigInt> {
    using AccT = BigInt;
    static constexpr BigInt zero = BigInt{0}; // ERROR: not a literal type
构造函数不是 constexpr 的
};
```

一个比较直接的解决方案是，不在类中定义值萃取：

```c++
template<>
struct AccumulationTraits<BigInt> {
    using AccT = BigInt;
    static BigInt const zero; // declaration only
};
```

然后在源文件中初始化：

```c++
BigInt const AccumulationTraits<BigInt>::zero = BigInt{0}；
```

这样通常会有些低效，因为编译期通常不知道它在其他文件中的变量定义，17中可以使用inline变量解决这个问题：

```c++
template<>
struct AccumulationTraits<BigInt> {
    using AccT = BigInt;
    inline static BigInt const zero = BigInt{0}; // OK since C++17
}；
```

在17之前的一种解决方法是对那些不总是生成整型值的值萃取，使用inline成员函数，同样的，如果成员函数返回的是字面值类型，可以将该函数声明为constexpr：

```c++
template<>
struct AccumulationTraits<float> {
    using AccT = double;
    static constexpr AccT zero() {
        return 0;
    }
};
template<>
struct AccumulationTraits<BigInt> {
    using AccT = BigInt;
    static BigInt zero() {
        return BigInt{0};
    }
};

AccT total = AccumulationTraits<T>::zero(); // init total by trait
function
```

现在很明了了，萃取是一种能够提供所有所需要的调用参数的信息的手段。

还可以有参数化的萃取，即满足某些特殊的需求：

```c++
#ifndef ACCUM_HPP
#define ACCUM_HPP
#include "accumtraits4.hpp"
template<typename T, typename AT = AccumulationTraits<T>>
auto accum (T const* beg, T const* end)
{
typename AT::AccT total = AT::zero();
    while (beg != end) {
        total += *beg;
        ++beg;
    }
    return total;
}
#endif //ACCU
```

### 萃取还是策略以及策略类

除了上文的求和，我们也可以求积，如果是字符串可以将它们连接起来，即便是求一个序列中的最大值，也可以转化为一个累积问题，在上面的例子中，唯一需要变的操作是total += *beg。我们可以称这一操作为累积操作中的一个策略：

```c++
#ifndef ACCUM_HPP
#define ACCUM_HPP
#include "accumtraits4.hpp"
#include "sumpolicy1.hpp"
template<typename T,
typename Policy = SumPolicy,
typename Traits = AccumulationTraits<T>>
auto accum (T const* beg, T const* end)
{
    using AccT = typename Traits::AccT;
    AccT total = Traits::zero();
    while (beg != end) {
    Policy::accumulate(total, *beg);
    	++beg;
    }
    return total;
}
#endif //ACCUM_HP

#ifndef SUMPOLICY_HPP
#define SUMPOLICY_HPP
class SumPolicy {
public:
    template<typename T1, typename T2>
    static void accumulate (T1& total, T2 const& value) {
    	total += value;
    }
};
#endif //SUMPOLICY

#include "accum6.hpp"
#include <iostream>
class MultPolicy {
public:
    template<typename T1, typename T2>
    static void accumulate (T1& total, T2 const& value) {
    	total *= value;
    }
};
int main()
{
    // create array of 5 integer values
    int num[] = { 1, 2, 3, 4, 5 };
    // print product of all values
    std::cout << "the product of the integer values is " <<
    accum<int,MultPolicy>(num, num+5) << ’\n’;
}
```

策略和萃取有很多相似点，只是它们更侧重于行为，而不是类型。

上述的示例是用有成员模板的常规类实现的，也可以使用类模板设计策略类接口的方式，此时就可以当作模板模板参数使用：

```c++
#ifndef SUMPOLICY_HPP
#define SUMPOLICY_HPP
template<typename T1, typename T2>
class SumPolicy {
public:
    static void accumulate (T1& total, T2 const& value) {
	    total += value;
    }
};
#endif //

#ifndef ACCUM_HPP#define ACCUM_HPP
#include "accumtraits4.hpp"
#include "sumpolicy2.hpp"
template<typename T,
template<typename,typename> class Policy = SumPolicy,
typename Traits = AccumulationTraits<T>>
auto accum (T const* beg, T const* end)
{
    using AccT = typename Traits::AccT;
    AccT total = Traits::zero();
    while (beg != end) {
        Policy<AccT,T>::accumulate(total, *beg);
        ++beg;
	}
	return total;
}
#endif//ACCUM_HPP
```

通过模板模板参数访问策略类的主要优势是让一个策略类通过一个依赖于模板参数的类型携带一些状态信息会更容易一些。

### 类型函数

传统上我们定义的函数可以称之为值函数，它们接收一些值作为参数并返回一个值作为结果，对于模板，还可以定义类型函数：它们接收一些类型作为参数并返回一个类型或者常量作为结果，比如：

```c++
template<typename T>
struct TypeSize {
	static std::size_t const value = sizeof(T);
};
int main()
{
    std::cout << "TypeSize<int>::value = " << TypeSize<int>::value << ’
    \n’;
}
```

我们希望有这样一个类型函数，当给与一个容器类型时，它可以返回相应的元素类型，这可以通过偏特化实现：

```c++
#include <vector>
#include <list>
template<typename T>
struct ElementT; // primary template
template<typename T>
struct ElementT<std::vector<T>> { //partial specialization for
std::vector
	using Type = T;
};
template<typename T>
struct ElementT<std::list<T>> { //partial specialization for std::list
	using Type = T;
};

#include "elementtype.hpp"
#include <vector>
#include <iostream>
#include <typeinfo>
template<typename T>
void printElementType (T const& c)
{
    std::cout << "Container of " <<
    typeid(typename ElementT<T>::Type).name() << " elements.\n";
}
int main()
{
    std::vector<bool> s;
    printElementType(s);
    int arr[42];
    printElementType(arr);
}
```

这是在不知道具体类型函数存在的情况下去实现类型函数，但在某些情况下，类型函数是和其所适用的类型一起被设计的，此时实现就可被简化：

```c++
template<typename C>
struct ElementT {
	using Type = typename C::value_type;
};
```

虽然如此，依然建议为类模板的类型参数提供相应的成员类型定义，这样在泛型代码中更容易访问它们，如：

```c++
template<typename T1, typename T2, …>
class X {
public:
    using … = T1;
    using … = T2; …
};
```

除了可以被用来访问主参数类型的某些特性，萃取还可以被用来做类型转换，比如为某个类型添加或移除引用、const以及volatile限制符。

比如我们可以实现一个RemoveReferenceT萃取，将引用类型转换为底层对象或函数的类型，非引用类型不变：

```c++
template<typename T>
struct RemoveReferenceT {
	using Type = T;
};
template<typename T>
struct RemoveReferenceT<T&> {
	using Type = T;
};
template<typename T>
struct RemoveReferenceT<T&&> {
	using Type = T;
};
template<typename T>
using RemoveReference = typename RemoveReference<T>::Type;
```

标准库提供了std::remove_reference<>萃取。

也可以添加引用：

```c++
template<typename T>
struct AddLValueReferenceT {
using Type = T&;
};
template<typename T>
using AddLValueReference = typename AddLValueReferenceT<T>::Type;
template<typename T>
struct AddRValueReferenceT {
using Type = T&&;
};
template<typename T>
using AddRValueReference = typename AddRValueReferenceT<T>::Type;
```

如果不对其进行偏特化的话，用别名模板就可以了：

```c++
template<typename T>
using AddLValueReferenceT = T&;
template<typename T>
using AddRValueReferenceT = T&&
```

这样对void类型可能不适用，因此可以有些特化的实现：

```c++
template<>
struct AddLValueReferenceT<void> {
	using Type = void;
};
template<>
struct AddLValueReferenceT<void const> {
	using Type = void const;
};
template<>
struct AddLValueReferenceT<void volatile> {
	using Type = void volatile;
};
template<>
struct AddLValueReferenceT<void const volatile> {
	using Type = void const volatile;
}；
```

有了这些偏特化之后，上述别名模板必须被实现为类模板，因为别名模板不能被特化。C++有相应的std::add_lvalue_reference<>和std::add_rvalue_reference<>。

我们也可以移除限制符：

```c++
template<typename T>
struct RemoveConstT {
	using Type = T;
};
template<typename T>
struct RemoveConstT<T const> {
	using Type = T;
};
template<typename T>
using RemoveConst = typename RemoveConstT<T>::Type;
```

转换萃取也可以是多功能的，比如创建一个可用来移除const和volatile的RemoveCVT萃取：

```c++
#include "removeconst.hpp"
#include "removevolatile.hpp"
template<typename T>
struct RemoveCVT : RemoveConstT<typename RemoveVolatileT<T>::Type>
{
};
template<typename T>
using RemoveCV = typename RemoveCVT<T>::Type;
```

其别名模板可以被进一步简化为：

```c++
template<typename T>
using RemoveCV = RemoveConst<RemoveVolatile<T>>;
```

处理函数到指针的退化：

```c++
template<typename R, typename… Args>
struct DecayT<R(Args…)> {
	using Type = R (*)(Args…);
};
template<typename R, typename… Args>
struct DecayT<R(Args…, …)> {
    using Type = R (*)(Args…, …);
};
```

第二种用来C风格的可变参。

预测型萃取，会返回其他的类型，比如判断两个类型是否相等：

```c++
template<typename T1, typename T2>
struct IsSameT {
	static constexpr bool value = false;
};
template<typename T>
struct IsSameT<T, T> {
	static constexpr bool value = true;
};

```

我们可以设法定义一个别名模板：

```c++
template<typename T1, typename T2>
constexpr bool isSame = IsSameT<T1, T2>::value;
```

标准库提供了std::is_same<>。

通过为可能的输出结果true和false提供不同的类型，可以大大提高对IsSameT的定义，比如声明一个BoolConstant模板以及两个可能的实例TrueType和FalseType：

```c++
template<bool val>
struct BoolConstant {
using Type = BoolConstant<val>;
static constexpr bool value = val;
};
using TrueType = BoolConstant<true>;
using FalseType = BoolConstant<false>;
```

默认提供了成员value。至于判断两个类型是否匹配，可以让相应的IsSameT分别继承：

```c++
#include "boolconstant.hpp"
template<typename T1, typename T2>
struct IsSameT : FalseType{};
template<typename T>
struct IsSameT<T, T> : TrueType{};
```

这样IsSameT\<T, int\>的返回类型会被隐式的转换为基类TrueType或者FalseType，这样不仅提供了value成员，还允许在编译期间将相应的需求派发到对应的函数实现或模板的偏特化：

```c++
#include "issame.hpp"
#include <iostream>
template<typename T>
void fooImpl(T, TrueType)
{
	std::cout << "fooImpl(T,true) 
}
template<typename T>
void fooImpl(T, FalseType)
{
	std::cout << "fooImpl(T,false) for other type called\n";
}
template<typename T>
void foo(T t)
{
    fooImpl(t, IsSameT<T,int>{}); // choose impl. depending on whether T
    is int
}
int main()
{
    foo(42); // calls fooImpl(42, TrueType)
    foo(7.7); // calls fooImpl(42, FalseType)
}
```

从11开始标准库提供了std::true_type和std::false_type。

另外还有结果类型萃取，例如对混合类型进行求和，比如：

```c++
template<typename T1, typename T2>
struct PlusResultT {
using Type = decltype(T1() + T2());
};
template<typename T1, typename T2>
using PlusResult = typename PlusResultT<T1, T2>::Type;

template<typename T1, typename T2>
Array<typename PlusResultT<T1, T2>::Type>
operator+ (Array<T1> const&, Array<T2> const&);
```

以上的方法需要对T1和T2进行值初始化，这两个类型需要有可访问的、未被删除的默认构造函数。要想规避这点，可以使用std::declval<>，其可以在不需要默认构造函数的情况下为类型T生成一个值，变动如下：

```c++
#include <utility>
template<typename T1, typename T2>
struct PlusResultT {
	using Type = decltype(std::declval<T1>() + std::declval<T2>());
};
template<typename T1, typename T2>
using PlusResult = typename PlusResultT<T1, T2>::Type;
```

### 基于SFINAE的萃取

可以用来在编译期间判断特定类型和表达式的有效性，比如可以通过萃取来判断一个类型是否有某个特定的成员，是否支持某个特定的操作，或者该类型本身是不是一个类。基于SFINAE的两个主要技术是：用SFINAE排除某些重载函数，以及用SFINAE排除某些偏特化。

一个基础的实现如下：

```c++
#include "issame.hpp"
template<typename T>
struct IsDefaultConstructibleT {
private:
    // test() trying substitute call of a default constructor for
    //T passed as U :
    template<typename U, typename = decltype(U())>
    static char test(void*);// test() fallback:
    template<typename>
    static long test(…);
public:
    static constexpr bool value =
    IsSameT<decltype(test<T>(nullptr)), char>::value;
};

IsDefaultConstructibleT<int>::value //yields true
struct S {
	S() = delete;
};
IsDefaultConstructibleT<S>::value //yield
```

第一个重载函数只有在所需的检查成功时才会被匹配，第二个则是应急的。这里对U()的结果施加了deltype操作，这样就可以用结果初始化一个类型参数了。

注意，不能在第一个test声明里直接使用模板参数，因为所有参数为T的成员函数都会被执行模板参数替换，对于一个不可默认构造的类型会直接报错。

还有另一种实现策略，比较老式的：

```c++
template<…> static char test(void*);
template<…> static long test(...);
enum { value = sizeof(test<…>(0)) == 1};
```

会基于返回值类型的大小判断使用了哪个重载函数。然而，在某些平台上，sizeof(char)的值可能等于sizeof(long)的值，为了确保返回值类型在所有的平台上都有不同的值，可以如下操作：

```c++
using Size1T = char;
using Size2T = struct { char a[2]; };
//或者
using Size1T = char(&)[1];
using Size2T = char(&)[2];

template<…> static Size1T test(void*); // checking test()
template<…> static Size2T test(…); // fallback
```

如前文所述，返回bool值的萃取，应该返回一个继承自std::true_type或std::false_type的值，使用这一方式，也可以解决某些平台sizeof(char)==sizeof(long)的问题。为了实现这个，需要定义一个IsDefaultConstructibleT，该萃取本身继承一个辅助类的Type成员，该辅助类会返回需要的基类。我们可以简单的将test的返回值类型用作对应的基类，如果匹配成功，继承的就是true_type，否则就是false_type。示例如下：

```c++
#include <type_traits>
template<typename T>
struct IsDefaultConstructibleHelper {
private:
    // test() trying substitute call of a default constructor for
    T passed as U:
    template<typename U, typename = decltype(U())>
    static std::true_type test(void*);
    // test() fallback:
    template<typename>
    static std::false_type test(…);
public:
	using Type = decltype(test<T>(nullptr));
};
template<typename T>
struct IsDefaultConstructibleT :
IsDefaultConstructibleHelper<T>::Type {
};
```

下面使用偏特化的方式来判断T是否可以被初始化：

```c++
// 别名模板，helper to ignore any number of template parameters:
template<typename …> using VoidT = void;
// primary template:
template<typename, typename = VoidT<>>
struct IsDefaultConstructibleT : std::false_type
{ };
// partial specialization (may be SFINAE’d away):
template<typename T>
struct IsDefaultConstructibleT<T, VoidT<decltype(T())>> :
std::true_type
{ };
```

这里跟前文的例子有些类似，有意思的是第二个模板参数的默认值被设定为一个辅助别名模板VoidT，这使得我们能够定义各种使用了任意数量的编译期类型构造的偏特化。如果对于某个特定类型T，其默认构造函数无效，那么SIFINEAE就使得该偏特化被丢弃，并最终使用主模板，否则该偏特化就是有效的。17标准库引入了与VoidT对应的类型萃取std::void_t<>，__cpp_lib_void_t被用来标识一个库中是否实现了std::void_t宏。

还可以将泛型lambda用于SFINAE，比如：

```c++
#include <utility>
// helper: checking validity of f (args…) for F f and Args… args:
template<typename F, typename… Args,
typename = decltype(std::declval<F>() (std::declval<Args&&>()…))>
std::true_type isValidImpl(void*);
// fallback if helper SFINAE’d out:
template<typename F, typename… Args>
std::false_type isValidImpl(…);
// define a lambda that takes a lambda f and returns whether calling
f with args is valid
inline constexpr
auto isValid = [](auto f) {
    return [](auto&&… args) {
        return decltype(isValidImpl<decltype(f),
        	decltype(args)&&…>(nullptr)){};
    };
};
// helper template to represent a type as a value
template<typename T>
struct TypeT {
	using Type = T;
};
// helper to wrap a type as a value
template<typename T>
constexpr auto type = TypeT<T>{};
// helper to unwrap a wrapped type in unevaluated contexts
template<typename T>
T valueT(TypeT<T>); // no definition
```

17之前，lambda表达式不能出现在const表达式中，因此上述代码只有在17中才有效。

### IsConvertibleT

示例是一种判断一种类型是否可以转换成另外一种类型的萃取：

```c++
#include <type_traits> // for true_type and false_type
#include <utility> // for declval
template<typename FROM, typename TO>
struct IsConvertibleHelper {
private:
    // test() trying to call the helper aux(TO) for a FROM passed as F :
    static void aux(TO);
    template<typename F, typename T,
    typename = decltype(aux(std::declval<F>()))>
    static std::true_type test(void*);
    // test() fallback:
    template<typename, typename>
    static std::false_type test(…);
public:
	using Type = decltype(test<FROM>(nullptr));
};
template<typename FROM, typename TO>
struct IsConvertibleT : IsConvertibleHelper<FROM, TO>::Type {
};
template<typename FROM, typename TO>
using IsConvertible = typename IsConvertibleT<FROM, TO>::Type;
template<typename FROM, typename TO>
constexpr bool isConvertible = IsConvertibleT<FROM, TO>::value;
```

为了处理一些特殊情况，为辅助类模板引入一个额外的模板参数：

```c++
template<typename FROM, typename TO, bool = IsVoidT<TO>::value ||
IsArrayT<TO>::value || IsFunctionT<TO>::value>
struct IsConvertibleHelper {
    using Type = std::integral_constant<bool, IsVoidT<TO>::value &&
    IsVoidT<FROM>::value>;
};
template<typename FROM, typename TO>
struct IsConvertibleHelper<FROM,TO,false> { … //previous implementation of IsConvertibleHelper here
};
```

标准库里对应的实现为std::is_convertible<>。

### 探测成员

另一种基于SFINAE的萃取的应用是创建一个可以判断一个给定类型T是否含有名为x的成员的萃取。实现如下：

```c++
#include <type_traits>
// defines true_type and false_type
// helper to ignore any number of template parameters:
template<typename …> using VoidT = void;
// primary template:
template<typename, typename = VoidT<>>
struct HasSizeTypeT : std::false_type
{};
// partial specialization (may be SFINAE’d away):
template<typename T>
struct HasSizeTypeT<T, VoidT<typename T::size_type>> : std::true_type
{} ;
```

注意，如果类型成员size_type是私有的，那么HasSizeTypeT会返回false，因为这个萃取模板没有访问该类型的特殊权限，也就是说，该萃取实际是测试我们是否能够访问类型成员size_type。

还要注意处理引用类型：

```c++
struct CXR {
	using size_type = char&; // Note: type size_type is a reference type
};
std::cout << HasSizeTypeT<CXR>::value; // OK: prints true
std::cout << HasSizeTypeT<CXR&>::value; // OOPS: prints false
std::cout << HasSizeTypeT<CXR&&>::value; // OOPS: prints false
```

引用类型确实没有成员，为了让它正常，我们需要使用removeReference萃取，使得模板参数是其指向的类型：

```c++
template<typename T>
struct HasSizeTypeT<T, VoidT<RemoveReference<T>::size_type>> :
std::true_type {
};
```

注意，对于注入类的名字，上述检测类型成员的萃取也会返回true：

```c++
struct size_type {
};
struct Sizeable : size_type {
};
static_assert(HasSizeTypeT<Sizeable>::value, "Compiler bug: Injected
class name missing")
```

这是因为size_type会将其自身的名字当作类型成员，而且这一成员会被继承。

在定义了诸如HasSizeTypeT的萃取之后，很自然想要将其参数化，以对任意名称的类型成员探测。然而，目前这一功能只能通过宏来实现，否则就使用泛型lambda：

```c++
#include <type_traits> // for true_type, false_type, and void_t
#define
DEFINE_HAS_TYPE(MemType) \
template<typename, typename = std::void_t<>> \
struct HasTypeT_##MemType \
: std::false_type {
}; \
template<typename T> \
struct HasTypeT_##MemType<T, std::void_t<typename T::MemType>> \
: std::true_type { } // ; intentionally skipped

#include "hastype.hpp"
#include <iostream>
#include <vector>
DEFINE_HAS_TYPE(value_type);
DEFINE_HAS_TYPE(char_type);
int main()
{
std::cout << "int::value_type: " << HasTypeT_value_type<int>::value
<< ’\n’;
std::cout << "std::vector<int>::value_type: " <<
HasTypeT_value_type<std::vector<int>>::value << ’\n’;
std::cout << "std::iostream::value_type: " <<
HasTypeT_value_type<std::iostream>::value << ’\n’;
std::cout << "std::iostream::char_type: " <<
HasTypeT_char_type<std::iostream>::value << ’\n’;
}
```

还可以修改上述萃取，使其能够测试数据成员和成员函数：

```c++
#include <type_traits> // for true_type, false_type, and void_t
#define
DEFINE_HAS_MEMBER(Member) \
template<typename, typename = std::void_t<>> \
struct HasMemberT_##Member \
: std::false_type { }; \
template<typename T> \
struct HasMemberT_##Member<T,
std::void_t<decltype(&T::Member)>> \
: std::true_type { } // ; intentionally skipped
```

当&::Member无效的时候，偏特化实现会被SFINAE掉，为了使条件有效，必须满足：

- Member必须是能够被识别出没有歧义的T的一个成员（不能是重载成员，也不能是多重继承中名字相同的成员的名字）；
- 成员必须可访问；
- 成员必须是非类型成员以及非枚举成员（否则前面的&会无效）；
- 如果是static成员，那么对应的类型必须没有使得&T::member无效的operator&；

这种方式只可以用来测试是否存在唯一一个与特定名称对应的成员。当然我们是可以简单测试是否可以调用某个函数，即便它是重载的：

```c++
#include <utility> // for declval
#include <type_traits> // for true_type, false_type, and void_t
// primary template:
template<typename, typename = std::void_t<>>
struct HasBeginT : std::false_type {
};
// partial specialization (may be SFINAE’d away):
template<typename T>
struct HasBeginT<T, std::void_t<decltype(std::declval<T>
().begin())>> : std::true_type {
};
```

我们甚至可以探测多个表达式的组合：

```c++
#include <utility> // for declval
#include <type_traits> // for true_type, false_type, and void_t
// primary template:
template<typename, typename, typename = std::void_t<>>
struct HasLessT : std::false_type
{};
// partial specialization (may be SFINAE’d away):
template<typename T1, typename T2>
struct HasLessT<T1, T2, std::void_t<decltype(std::declval<T1>() <
std::declval<T2>())>>: std::true_type
{};
```

以下介绍使用泛型Lambda探测成员：

```c++
#include "isvalid.hpp"
#include<iostream>
#include<string>
#include<utility>
int main()
{
    using namespace std;
    cout << boolalpha;
    // define to check for data member first:
    constexpr auto hasFirst = isValid([](auto x) ->
    decltype((void)valueT(x).first) {});
    cout << "hasFirst: " << hasFirst(type<pair<int,int>>) << ’\n’; //
    true
    // define to check for member type size_type:
    constexpr auto hasSizeType = isValid([](auto x) -> typename
    decltype(valueT(x))::size_type { });
    struct CX {
   		using size_type = std::size_t;
    };
    cout << "hasSizeType: " << hasSizeType(type<CX>) << ’\n’; // true
    if constexpr(!hasSizeType(type<int>)) {
        cout << "int has no size_type\n"; 
        …	
	}
    // define to check for <:
    constexpr auto hasLess = isValid([](auto x, auto y) ->
    decltype(valueT(x) < valueT(y)) {});
    cout << hasLess(42, type<char>) << ’\n’; //yields true
    cout << hasLess(type<string>, type<string>) << ’\n’; //yields true
	cout << hasLess(type<string>, type<int>) << ’\n’; //yields false
	cout << hasLess(type<string>, "hello") << ’\n’; //yields true
}
```

### 其他的萃取技术

ifThenElse类型模板接受一个bool型模板参数，并根据该参数从另外两个类型参数中做选择：

```c++
#ifndef IFTHENELSE_HPP
#define IFTHENELSE_HPP
// primary template: yield the second argument by default and rely on
// a partial specialization to yield the third argument
// if COND is false
template<bool COND, typename TrueType, typename FalseType>
struct IfThenElseT {
	using Type = TrueType;
};
// partial specialization: false yields third argument
template<typename TrueType, typename FalseType>
struct IfThenElseT<false, TrueType, FalseType> {
	using Type = FalseType;
};
template<bool COND, typename TrueType, typename FalseType>
using IfThenElse = typename IfThenElseT<COND, TrueType,
FalseType>::Type;
#endif //IFTHENELSE_HPP

```

注意，此模型在最终选择之前，then和else分支的模板参数都会计算，因此要保证两个分支都没有问题。以下这个例子就不行：

```c++
// ERROR: undefined behavior if T is bool or no integral type:
template<typename T>
struct UnsignedT {
    using Type = IfThenElse<std::is_integral<T>::value
    && !std::is_same<T,bool>::value, typename std::make_unsigned<T>::type,
    T>;
};
```

因为在实例化UnsignedT\<bool\>的时候，typename std::make_unsigned\<T\>会报错。为了解决这一问题，需要再引入一层额外的间接层，从而让IfThenElse的参数本身用类型函数去封装结果：

```c++
// yield T when using member Type:
template<typename T>
struct IdentityT {
	using Type = T;
};
// to make unsigned after IfThenElse was evaluated:
template<typename T>
struct MakeUnsignedT {
	using Type = typename std::make_unsigned<T>::type;
};
template<typename T>
struct UnsignedT {
    using Type = typename IfThenElse<std::is_integral<T>::value
    && !std::is_same<T,bool>::value,
    MakeUnsignedT<T>,
    IdentityT<T>
    >::Type;
};
```

这样一来，在最终IfThenElse做出选择之前，类型函数不会真正计算，而是由IfThenElse选择合适类型实例。之所以能够这么做是因为未被选择的封装类型永远不会完全实例化。

标准库中与之对应的模板为std::conditional<>。

还可以使用萃取探测不抛出异常的操作，比如探测pair的移动构造函数抛出异常：

```c++
Pair(Pair&& other)
noexcept(IsNothrowMoveConstructibleT<T1>::value &&
IsNothrowMoveConstructibleT<T2>::value)
: first(std::forward<T1>(other.first)),
second(std::forward<T2>(other.second))
{}

#include <utility> // for declval
#include <type_traits> // for bool_constant
template<typename T>
struct IsNothrowMoveConstructibleT
: std::bool_constant<noexcept(T(std::declval<T>()))>
{};
```

该实现还可以继续优化，因为其不是SFINAE友好的，在真正做计算之前，要判断用来计算结果的表达式是否有效，这里增加一个默认值为void的模板参数，并根据移动构造函数是否可用对其偏特化：

```c++
#include <utility> // for declval
#include <type_traits> // for true_type, false_type, and
bool_constant<>
// primary template:
template<typename T, typename = std::void_t<>>
struct IsNothrowMoveConstructibleT : std::false_type
{ };
// partial specialization (may be SFINAE’d away):
template<typename T>
struct IsNothrowMoveConstructibleT<T,
std::void_t<decltype(T(std::declval<T>()))>>
: std::bool_constant<noexcept(T(std::declval<T>()))>
{};
```

标准库对应的萃取为std::is_move_constructible<>。

使用别名模板包装萃取可以使得代码简洁，但也有一些缺点：

1. 别名模板不能被特化；
2. 对别名模板的使用最终会让该类型被实例化，这样对于给定类型就很难避免对其进行无意义的实例化；

对最后一点的表述指的是别名模板不能和元函数转发一起使用。

对于返回数值的萃取需要使用一个::value来生成萃取的结果，在这种情况下，constexpr修饰的变量模板提供了一种简化代码的方法，比如：

```c++
template<typename T1, typename T2>
constexpr bool IsSame = IsSameT<T1,T2>::value;
template<typename FROM, typename TO>
constexpr bool IsConvertible = IsConvertibleT<FROM, TO>::value;
```

14开始，对于类型萃取的type成员，一般别名模板会有一个\_t后缀，17开始对于value成员，与之对应的变量模板会有一个\_v后缀。

### 类型分类

主要用于判断模板参数的类型是内置类型、指针类型、class类型或等等。比如，我们定义一个可以判断某个类型是不是基础类型的模板，默认情况下，我们认为不是基础类型，对于基础类型则是进行了特化：

```c++
#include <cstddef> // for nullptr_t
#include <type_traits> // for true_type, false_type, and
bool_constant<>
// primary template: in general T is not a fundamental type
template<typename T>
struct IsFundaT : std::false_type {
};
// macro to specialize for fundamental types
#define MK_FUNDA_TYPE(T) \
template<> struct IsFundaT<T> : std::true_type { \
};
MK_FUNDA_TYPE(void)
MK_FUNDA_TYPE(bool)
MK_FUNDA_TYPE(char)
MK_FUNDA_TYPE(signed char)
MK_FUNDA_TYPE(unsigned char)
MK_FUNDA_TYPE(wchar_t)
MK_FUNDA_TYPE(char16_t)
MK_FUNDA_TYPE(char32_t)
MK_FUNDA_TYPE(signed short)
MK_FUNDA_TYPE(unsigned short)
MK_FUNDA_TYPE(signed int)
MK_FUNDA_TYPE(unsigned int)
MK_FUNDA_TYPE(signed long)
MK_FUNDA_TYPE(unsigned long)
MK_FUNDA_TYPE(signed long long)
MK_FUNDA_TYPE(unsigned long long)
MK_FUNDA_TYPE(float)
MK_FUNDA_TYPE(double)
MK_FUNDA_TYPE(long double)
MK_FUNDA_TYPE(std::nullptr_t)
#undef MK_FUNDA_TYPE
```

对于指针类型，可以有以下的实现：

```c++
template<typename T>
struct IsPointerT : std::false_type { //primary template: by default
not a pointer
};
template<typename T>
struct IsPointerT<T*> : std::true_type { //partial specialization for
pointers
    using BaseT = T; // type pointing to
};
```

左值引用和右值引用也与之类似：

```c++
template<typename T>
struct IsLValueReferenceT : std::false_type { //by default no lvalue
reference
};
template<typename T>
struct IsLValueReferenceT<T&> : std::true_type { //unless T is lvalue
references
	using BaseT = T; // type referring to
};

template<typename T>
struct IsRValueReferenceT : std::false_type { //by default no rvalue
reference
};
template<typename T>
struct IsRValueReferenceT<T&&> : std::true_type { //unless T is rvalue
reference
	using BaseT = T; // type referring to
};
```

两者又可组合成IsReferenceT<>萃取：

```c++
#include "islvaluereference.hpp"
#include "isrvaluereference.hpp"
#include "ifthenelse.hpp"
template<typename T>
class IsReferenceT
: public IfThenElseT<IsLValueReferenceT<T>::value,
    IsLValueReferenceT<T>,
    IsRValueReferenceT<T>
    >::Type {
};
```

对于数组的萃取，其偏特化的模板参数要比主模板要多：

```c++
#include <cstddef>
template<typename T>
struct IsArrayT : std::false_type { //primary template: not an array
};
template<typename T, std::size_t N>
struct IsArrayT<T[N]> : std::true_type { //partial specialization for
arrays
    using BaseT = T;
    static constexpr std::size_t size = N;
};
template<typename T>
struct IsArrayT<T[]> : std::true_type { //partial specialization for
unbound arrays
    using BaseT = T;
    static constexpr std::size_t size = 0;
};
```

判断指向成员的指针：

```c++
template<typename T>
struct IsPointerToMemberT : std::false_type { //by default no
pointer-to-member
};
template<typename T, typename C>
struct IsPointerToMemberT<T C::*> : std::true_type { //partial
specialization
    using MemberT = T;
    using ClassT = C;
};
```

识别函数类型比较复杂，要用参数包来捕获所有的参数：

```c++
#include "../typelist/typelist.hpp"
template<typename T>
struct IsFunctionT : std::false_type { //primary template: no function
};
template<typename R, typename… Params>
struct IsFunctionT<R (Params…)> : std::true_type
{ //functions
    using Type = R;
    using ParamsT = Typelist<Params…>;
    static constexpr bool variadic = false;
};
template<typename R, typename… Params>
struct IsFunctionT<R (Params…, …)> : std::true_type { //variadic
functions
    using Type = R;
    using ParamsT = Typelist<Params…>;
    static constexpr bool variadic = true;
};
```

然而，这些并不能包含所有的函数类型，因为函数类型可能带一些修饰符。

判断class的类型比较困难，但我们可以利用以下的特性：只有class类型可以被用于指向成员的指针类型的基础，即X Y::*中Y只能是class类型：

```c++
#include <type_traits>
template<typename T, typename = std::void_t<>>
struct IsClassT : std::false_type { //primary template: by default no
class
};
template<typename T>
struct IsClassT<T, std::void_t<int T::*>> // classes can have
pointer-to-member
: std::true_type {
};
```

### 策略萃取

前文所述的萃取大多是判断模板参数的特性，这一类叫做特性萃取，但有些萃取定义的是如何处理某些数据，这种叫做策略萃取。比如为了处理拷贝可能的代价，定义策略萃取模板将预期的参数类型T映射到最佳的参数类型T或者T const&：

```c++
template<typename T>
struct RParam {
using Type = typename IfThenElseT<sizeof(T) <=2*sizeof(void*),
    T,
    T const&>::Type;
};
template<typename T>
struct RParam<Array<T>> {
using Type = Array<T> const&;
};

#include "rparam.hpp"
#include "rparamcls.hpp"
// function that allows parameter passing by value or by reference
template<typename T1, typename T2>
void foo (typename RParam<T1>::Type p1, typename RParam<T2>::Type p2)
{ …
}
int main()
{
    MyClass1 mc1;
    MyClass2 mc2;
    foo<MyClass1,MyClass2>(mc1,mc2);
}
```

这样也有缺点，那就是调用foo的时候不能使用参数推断，因为模板参数只能出现在函数参数的限制符中，因此调用时必须显式的指明模板参数，一种权宜之计是使用提供了完美转发的inline封装函数，但需要假设编译器将省略inline函数：

```c++
#include "rparam.hpp"
#include "rparamcls.hpp"
// function that allows parameter passing by value or by reference
template<typename T1, typename T2>
void foo_core (typename RParam<T1>::Type p1, typename RParam<T2>::Type
p2)
{ …
}
// wrapper to avoid explicit template parameter passing
template<typename T1, typename T2>
void foo (T1 && p1, T2 && p2)
{
	foo_core<T1,T2>(std::forward<T1>(p1),std::forward<T2>(p2));
}
int main()
{
    MyClass1 mc1;
    MyClass2 mc2;
    foo(mc1,mc2); // same as foo_core<MyClass1,MyClass2> (mc1,mc2)
}
```

## 基于类型属性的重载

### 算法特化

在一个泛型算法中引入更为特化的变体，这一设计和优化方法称为算法特化，比如：

```c++
template<typename T>
void swap(T& x, T& y)
{
    T tmp(x);
    x = y;
    y = tmp;
}

template<typename T>
void swap(Array<T>& x, Array<T>& y)
{
    swap(x.ptr, y.ptr);
    swap(x.len, y.len);
}
```

### 标记派发

算法特化的一个方式是用一个唯一的、可区分特定变体的类型来标记不同算法变体的实现，比如以下advanceIter算法的变体实现：

```c++
template<typename Iterator, typename Distance>
void advanceIterImpl(Iterator& x, Distance n,
std::input_iterator_tag)
{
    while (n > 0) { //linear time
    ++x;
    --n;
}
}
template<typename Iterator, typename Distance>
void advanceIterImpl(Iterator& x, Distance n,
std::random_access_iterator_tag)
{
	x += n; // constant time
}
```

然后通过advanceIter函数模板将其参数连同与之对应的tag一起转发出去：

```c++
template<typename Iterator, typename Distance>
void advanceIter(Iterator& x, Distance n)
{
    advanceIterImpl(x, n, typename
    std::iterator_traits<Iterator>::iterator_category())
}

namespace std {
    struct input_iterator_tag { };
    struct output_iterator_tag { };
    struct forward_iterator_tag : public input_iterator_tag { };
    struct bidirectional_iterator_tag : public forward_iterator_tag
    { };
    struct random_access_iterator_tag : public
    bidirectional_iterator_tag { };
}
```

当算法用到的特性具有天然的层次结构，并且存在一组为这些标记提供了值的萃取机制的时候标记派发可以很好的工作，如果算法特化依赖于专有类型属性的话，标记派发就没这么方便，需要更强大的技术。

### Enable/Disable函数模板

标准库提供了std::enable_if，这里实现一个辅助工具，引入了一个对应的模板别名，为了避免名称冲突，这里称之为EnableIf。实现如下：

```c++
template<bool, typename T = void>
struct EnableIfT {
};
template< typename T>
struct EnableIfT<true, T> {
    using Type = T;
};
template<bool Cond, typename T = void>
using EnableIf = typename EnableIfT<Cond, T>::Type;
```

有了这个辅助类，我们就可以实现随机访问版本的advanceIter算法：

```c++
template<typename Iterator>
constexpr bool IsRandomAccessIterator =
IsConvertible< typename std::iterator_traits<Iterator>::iterator_category,
	std::random_access_iterator_tag>;
template<typename Iterator, typename Distance>
EnableIf<IsRandomAccessIterator<Iterator>>
advanceIter(Iterator& x, Distance n){
	x += n; // constant time
}
```

EnableIf的使用意味着只有当Iterator参数是随机访问迭代器的时候函数模板才可以使用（而且返回类型为void），当不是时函数模板会从待选项移除。现在已经可以显式的为特定类型激活其所适用的模板，但还需要激活不够特化的模块，这里还使用了EnableIf：

```c++
template<typename Iterator, typename Distance>
EnableIf<!IsRandomAccessIterator<Iterator>>
advanceIter(Iterator& x, Distance n)
{
    while (n > 0) {//linear time
        ++x;
        --n;
    }
}
```

EnableIf通常用于函数模板的返回类型，但该方法不适用构造函数模板以及类型转换模板，因为其都没有指定返回类型，对于这一问题，可以将EnableIf嵌入一个默认模板参数来解决：

```c++
#include <iterator>
#include "enableif.hpp"
#include "isconvertible.hpp"
template<typename Iterator>
constexpr bool IsInputIterator = IsConvertible< typename
std::iterator_traits<Iterator>::iterator_category,
std::input_iterator_tag>;
template<typename T>
class Container {
public:
    // construct from an input iterator sequence:
    template<typename Iterator, typename =
    EnableIf<IsInputIterator<Iterator>>>
    Container(Iterator first, Iterator last);
    // convert to a container so long as the value types are convertible:
    template<typename U, typename = EnableIf<IsConvertible<T, U>>>
    operator Container<U>() const;
};
```

但如果重载时可能就会出现错误，因此此时的重载可能只是模板参数不同，但模板参数不同恰恰不被认为是重载，解决方法是引入另一个模板参数：

```c++
// construct from an input iterator sequence:
template<typename Iterator, typename =
EnableIf<IsInputIterator<Iterator>
&& !IsRandomAccessIterator<Iterator>>>
Container(Iterator first, Iterator last);
template<typename Iterator, typename = 
EnableIf<IsRandomAccessIterator<Iterator>>, typename = int> // extra
dummy parameter to enable both constructors
Container(Iterator first, Iterator last); //
```

到了17，有了constexpr if，某些情况下就可以不使用EnableIf，比如可以这样重写advanceIter()：

```c++
template<typename Iterator, typename Distance>
void advanceIter(Iterator& x, Distance n) {
    if constexpr(IsRandomAccessIterator<Iterator>) {
        // implementation for random access iterators:
        x += n; // constant time
    }else if constexpr(IsBidirectionalIterator<Iterator>) {
        // implementation for bidirectional iterators:
        if (n > 0)
        	{for ( ; n > 0; ++x, --n) { //linear time for positive n
            }
        } else {
                for ( ; n < 0; --x, ++n) { //linear time for negative n
            }
        }
    }else {
        // implementation for all other iterators that are at least input 	iterators:
        if (n < 0) {
        	throw "advanceIter(): invalid iterator category for negative n";
        }
        while (n > 0) { //linear time for positive n only
            ++x;
            --n;
        }
  	}
}
```

### 类的特化

类模板的偏特化可以被用来提供一个可选的、为特定模板参数进行了特化的实现，与函数模板的重载很像。这次的示例是一个以key和value的类型为模板参数的泛型Dictionary类模板，只要key提供了operator==运算符，就实现一个简单的Dictionary：

```c++
template<typename Key, typename Value>
class Dictionary
{
private:
	vector<pair<Key const, Value>> data;
public:
    //subscripted access to the data:
    value& operator[](Key const& key)
    {
        // search for the element with this key:
        for (auto& element : data) {
            if (element.first == key){
                return element.second;
            }
        }
        // there is no element with this key; add one
        data.push_back(pair<Key const, Value>(key, Value()));
        return data.back().second;
    }…
};
```

为了将EnableIf用于类模板的偏特化，先为Dictionary引入一个未命名的、默认的模板参数：

```
template<typename Key, typename Value, typename = void>
class Dictionary
{ … // vector implementation as above
};
template<typename Key, typename Value>
class Dictionary<Key, Value, EnableIf<HasLess<Key> && !HasHash<Key>>> 
{ … // map implementation as above
};
template typename Key, typename Value>
class Dictionary Key, Value, EnableIf HasHash Key>>>
{
private:
	unordered_map Key, Value> data;
public:
    value& operator[](Key const& key) {
    return data[key];
    }…
};
```

同样的，标记派发也可以被用于在不同模板特化版本之间做选择，比如实现一个Advance\<Iterator\>：

```c++
// primary template (intentionally undefined):
template<typename Iterator,
typename Tag = BestMatchInSet< typename
std::iterator_traits<Iterator> ::iterator_category,
std::input_iterator_tag,
std::bidirectional_iterator_tag,
std::random_access_iterator_tag>>
class Advance;
// general, linear-time implementation for input iterators:
template<typename Iterator>
class Advance<Iterator, std::input_iterator_tag>
{
public:
    using DifferenceType = typename
    std::iterator_traits<Iterator>::difference_type;
    void operator() (Iterator& x, DifferenceType n) const
    {
        while (n > 0) {
            ++x;
            --n;
        }
    }
};

// bidirectional, linear-time algorithm for bidirectional iterators:
template<typename Iterator>
class Advance<Iterator, std::bidirectional_iterator_tag>
{
public:
	using DifferenceType =typename
    std::iterator_traits<Iterator>::difference_type;
    void operator() (Iterator& x, DifferenceType n) const
    {
        if (n > 0) {
            while (n > 0) {
                ++x;
                --n;
            }
        } else {
        while (n < 0) {
            --x;
            ++n;
            }
        }
    }
};
// bidirectional, constant-time algorithm for random access iterators:
template<typename Iterator>
class Advance<Iterator, std::random_access_iterator_tag>
{
public:
    using DifferenceType =
    typename std::iterator_traits<Iterator>::difference_type;
    void operator() (Iterator& x, DifferenceType n) const
    {
        x += n;
    }
}
```

这里面比较困难的就是BestMatchInSet的实现，使用了重载解析：

```c++
// construct a set of match() overloads for the types in Types…:
template<typename… Types>
struct MatchOverloads;
// basis case: nothing matched:
template<>
struct MatchOverloads<> {
	static void match(…);
};
// recursive case: introduce a new match() overload:
template<typename T1, typename… Rest>
struct MatchOverloads<T1, Rest…> : public MatchOverloads<Rest…>
{
    static T1 match(T1); // introduce overload for T1
    using MatchOverloads<Rest…>::match;// collect overloads from bases
};
// find the best match for T in Types…
template<typename T, typename… Types>
struct BestMatchInSetT {
	using Type = decltype(MatchOverloads<Types…>::match(declval<T> ()));
};
template<typename T, typename… Types>
using BestMatchInSet = typename BestMatchInSetT<T, Types…>::Type;
```

## 模板和继承

### 空基类优化

标准指出，在空类被用作基类时，如果不给它分配内存并不会导致其被存储到与其他同类型对象或者字对象的地址上，那么就可以不给它分配内存：

```c++
#include <iostream>
class Empty {
using Int = int;// type alias members don’t make a class nonempty
};
class EmptyToo : public Empty {
};
class EmptyThree : public EmptyToo {
}；
int main()
{
    std::cout << "sizeof(Empty): " << sizeof(Empty) << ’\n’;
    std::cout << "sizeof(EmptyToo): " << sizeof(EmptyToo) << ’\n’;
    std::cout << "sizeof(EmptyThree): " << sizeof(EmptyThree) << ’\n’;
}
```

如果编译期实现了EBCO，那么打印出来的三个类大小将是相同的，但是它们的结果也都不会是0，这意味着实现了空基类的优化，如果三个类大小不同，意味着没有实现EBCO。考虑一种EBCO不适用的情况：

```c++
#include <iostream>!
class Empty {
using Int = int; // type alias members don’t make a class nonempty
};
class EmptyToo : public Empty {
};
class NonEmpty : public Empty, public EmptyToo {
};
int main(){
    std::cout <<"sizeof(Empty): " << sizeof(Empty) <<’\n’;
    std::cout <<"sizeof(EmptyToo): " << sizeof(EmptyToo) <<’\n’;
    std::cout <<"sizeof(NonEmpty): " << sizeof(NonEmpty) <<’\n’;
}
```

NonEmpty不再是一个空类，其内存布局如下所示：

![EBCO](..\image\template\EBCO.png)

这样子，可以通过两个指针保证所指向的对象不是同一个对象。

EBCO有什么作用呢，考虑下面的例子：

```c++
template<typename T1, typename T2>
class MyClass {
private:
    T1 a;
    T2 b;
    …
};
```

其中的模板参数完全可能被空class类型替换，这样会浪费一个字的内存，这一内存浪费可以通过把模板参数当作基类来避免：

```c++
template<typename T1, typename T2>
class MyClass : private T1, private T2 {
};
```

然而这样也有缺点：

- 当T1或T2被一个非class类型或者union类型替换时，该方法不再适用；
- 当两个模板参数被同一种类型替换的时候，该方法不再适用；
- 用来替换T1或者T2的类型可能是final的，此时尝试派生出新类可能会触发错误；

当已知模板参数只会被class类型替换，以及需要支持另一个模板参数时候，可以使用另一种方法，那就是使用EBCO将可能为空的类型参数与别的参数合并，比如：

```c++
template<typename CustomClass>
class Optimizable {
private:
	BaseMemberPair<CustomClass, void*> info_and_storage; …
};

#ifndef BASE_MEMBER_PAIR_HPP
#define BASE_MEMBER_PAIR_HPP
template<typename Base, typename Member>
class BaseMemberPair : private Base {
private:
	Member mem;
public:// constructor
    BaseMemberPair (Base const & b, Member const & m)
    : Base(b), mem(m) {
    }
    // access base class data via first()
    Base const& base() const {
    	return static_cast<Base const&>(*this);
    }
    Base& base() {
    	return static_cast<Base&>(*this);
    }
    // access member data via second()
    Member const& member() const {
    	return this->mem;
    }
    Member& member() {
    	return this->mem;
    }
};
#endif // BASE_MEMBER_PAIR_HPP
```

当Base是一个空基类时，显然得到了优化。

### The Curiously Recurring Template Pattern(CRTP)

将派生类作为模板参数传递给其某个基类的一项技术，最简单的实现如下：

```c++
template<typename Derived>
class CuriousBase { …
};
class Curious : public CuriousBase<Curious> { …
};

```

这个例子中使用了非依赖性基类，Curious并不是一个模板类，因此它对在依赖性基类中遇到的名称可见性问题是免疫的。事实上，我们同样可以使用以下的方式：

```c++
template<typename Derived>
class CuriousBase { 
	…
};
template<typename T>
class CuriousTemplate : public CuriousBase<CuriousTemplate<T>> { 
    …
};
```

将派生类通过模板参数传递给基类，基类可以在不使用虚函数的情况下定制派生类的行为，这使得CRTP对那些只能被实现为成员函数的情况或者依赖于派生类的特性的情况很有帮助。

一个CRTP的简单应用是追踪一个class类型实例化出了多少对象：

```c++
#include <cstddef>
template<typename CountedType>
class ObjectCounter {
private:
	inline static std::size_t count = 0; // number of existing objects
protected:
    // default constructor
    ObjectCounter() {
        ++count;
    }
    // copy constructor
    ObjectCounter (ObjectCounter<CountedType> const&) {
        ++count;
    }
    // move constructor
    ObjectCounter (ObjectCounter<CountedType> &&) {
        ++count;
    }
    // destructor
    ~ObjectCounter() {
        --count;
    }	
public:
    // return number of existing objects:
    static std::size_t live() {
    	return count;
    }
};
```

为了在类内部初始化count成员使用了inline，在17之前必须在类模板外面定义它。

下面介绍The Barton-Nackman Trick，假设现在有一个需要定义operator==的类模板Array，一个可能的方案是将该运算符定义为类模板的成员，但由于第一个参数是this，第二个参数是Array类型，我们肯定希望该运算符的参数是对称的，因此选择将其定义为命名空间的函数：

```c++
template<typename T>
class Array {
public: …
};
template<typename T>bool operator== (Array<T> const& a, Array<T> const&
b)
{
    …
}
```

这带来的一个问题是函数模板不能重载，则当前作用域不能再声明其他的operator==模板，为了解决这个问题，Barton和Nackman通过将operator==定义为class内部的一个常规友元函数解决了这个问题：

```c++
template<typename T>
class Array {
static bool areEqual(Array<T> const& a, Array<T> const& b);
public: …
    friend bool operator== (Array<T> const& a, Array<T> const& b)
    {
    	return areEqual(a, b);
    }
};
```

由于operator==会被隐式的当作inline函数，因此这里委托了一个静态成员函数，这样就不必每个类都需要保存一个operator==函数指针。

然而从94年开始，如果要查找友元函数，意味着在函数的调用参数中，至少要有一个参数包含了friend函数的关联类，这意味着这种方法不那么有效了。

运算符的实现，可以通过模板将其泛型化：

```c++
template<typename T>
bool operator!= (T const& x1, T const& x2) {
	return !(x1 == x2);
}
```

事实上，在标准库\<utility\>头文件中已经包含了类似的定义，但是一些别的定义（如!=，>，<=等）在标准化的过程中被放到了namespace std::rel_ops中。使用这些其实有个问题，那就是相比于用于定义的需要从派生类到基类转换的!=operator，通用的!=operator总是被优先选择，这有时会导致意料之外的结果。另一种基于CRTP的运算符模板形式，则允许程序去选择泛型的运算符定义：

```c++
template<typename Derived>
class EqualityComparable
{
public:
    friend bool operator!= (Derived const& x1, Derived const& x2)
    {
    	return !(x1 == x2);
    }
};
class X : public EqualityComparable<X>
{
public:
    friend bool operator== (X const& x1, X const& x2) {
    // implement logic for comparing two objects of type X
    }
};
int main()
{
    X x1, x2;
    if (x1 != x2) { }
}
```

这样就将一部分行为分解到了基类，同时保存了派生类的标识，这使得可以基于一些简单的运算符为大量的运算符提供统一的定义。

我们可以根据上述的定义容易的定义一个指向简单链表的迭代器：

```c++
template<typename T>
class ListNode
{
public:
    T value;
    ListNode<T>* next = nullpt；
    ~ListNode() { delete next; }
};

template<typename T>
class ListNodeIterator
: public IteratorFacade<ListNodeIterator<T>, T,
std::forward_iterator_tag>
{
	ListNode<T>* current = nullptr;
public:
    T& dereference() const {
    	return current->value;
    }
    void increment() {
    	current = current->next;
    }
    bool equals(ListNodeIterator const& other) const {
    	return current == other.current;
    }
    ListNodeIterator(ListNode<T>* current = nullptr) :
    current(current) { }
};
```

上述代码的缺点是，需要将dereference、advance等暴露成public接口，为了避免这一缺点，可以重写IteratorFacade，通过一个单独的访问类来执行所有作用于CRTP派生类的运算符操作：

```c++
// ‘friend’ this class to allow IteratorFacade access to core iterator operations:
class IteratorFacadeAccess
{
    // only IteratorFacade can use these definitions
    template<typename Derived, typename Value, typename Category,
    typename Reference, 
    friend class IteratorFacade;
    // required of all iterators:
    template<typename Reference, typename Iterator>
    static Reference dereference(Iterator const& i) {
    	return i.dereference();
    }…
    // required of bidirectional iterators:
    template<typename Iterator>
    static void decrement(Iterator& i) {
    	return i.decrement();
    }
    // required of random-access iterators:
    template<typename Iterator, typename Distance>
    static void advance(Iterator& i, Distance n) {
    	return i.advance(n);
    }…
};
```

除此之外，还可以基于这种迭代器创造迭代器的适配器，可将底层迭代器投射到一些指向数据成员的指针，比如：

```c++
template<typename Iterator, typename T>
class ProjectionIterator
: public IteratorFacade<ProjectionIterator<Iterator, T>, T, typename
std::iterator_traits<Iterator>::iterator_category, T&, typename
std::iterator_traits<Iterator>::difference_type>
{
    using Base = typename std::iterator_traits<Iterator>::value_type;
    using Distance = typename std::iterator_traits<Iterator>::difference_type;
    Iterator iter;
    T Base::* member;
    friend class IteratorFacadeAccess 
    …
    //implement core iterator operations for IteratorFacade
public:
    ProjectionIterator(Iterator iter, T Base::* member)
        : iter(iter), member(member) { }
    T& dereference() const {
    	return (*iter).*member;
    }

};
template<typename Iterator, typename Base, typename T>
auto project(Iterator iter, T Base::* member) {
	return ProjectionIterator<Iterator, T>(iter, member);
}

#include <vector>
#include <algorithm>
#include <iterator>
int main()
{
    std::vector<Person> authors = { {"David", "Vandevoorde"},
    {"Nicolai", "Josuttis"},
    {"Douglas", "Gregor"} };
    std::copy(project(authors.begin(), &Person::firstName),
    project(authors.end(), &Person::firstName),
    std::ostream_iterator<std::string>(std::cout, "\n"));
}
```

### Mixins（混合）

一般来说，我们可以通过继承来客制化一个类型的行为，这里介绍一种新的方法，Mixins，可以客制化一个类型的行为但是不需要从其进行继承，事实上，Mixins反转了常规的继承方向，因为新的类型被作为类模板的基类混合进了继承层级中，而不是被创建为一个新的派生类。这一方式允许在引入新的数据成员以及某些操作的时候不需要复制相关接口。一个支持mixins的类模板通常会接受一组任意数量的class，从之进行派生：

```c++
template<typename… Mixins>
class Point : public Mixins…
{
public:
    double x, y;
    Point() : Mixins()…, x(0.0), y(0.0) { }
    Point(double x, double y) : Mixins()…, x(x), y(y) { }
};

class Label
{
public:
    std::string label;
    Label() : label("") { }
};
using LabeledPoint = Point<Label>;
```

当和CRTP一起使用的时候，Mixins会更加强大，每一个mixins都是一个以派生类为模板参数的类模板，这样允许对派生类做额外的客制化：

```c++
template<template<typename>… Mixins>
class Point : public Mixins<Point>…
{
public:
    double x, y;
    Point() : Mixins<Point>()…, x(0.0), y(0.0) { }
    Point(double x, double y) : Mixins<Point>()…, x(x), y(y) { }
};
```

Minxins还允许间接的参数化派生类的其他特性，比如成员函数的虚拟性：

```c++
#include <iostream>
class NotVirtual {
};
class Virtual {
public:
	virtual void foo() {
	}
};
template<typename… Mixins>
class Base : public Mixins…
{
public：
    // the virtuality of foo() depends on its declaration
    // (if any) in the base classes Mixins…
    void foo() {
    	std::cout << "Base::foo()" << ’\n’;
    }
};
template<typename… Mixins>
class Derived : public Base<Mixins…> {
public:
    void foo() {
    	std::cout << "Derived::foo()" << ’\n’;
    }
};
int main()
{
    Base<NotVirtual>* p1 = new Derived<NotVirtual>;
    p1->foo(); // calls Base::foo()
    Base<Virtual>* p2 = new Derived<Virtual>;
    p2->foo(); // calls Derived::foo()
}
```

该技术提供了这样一种工具，使用它可以设计出一个既可以用来实例化具体的类，也可以通过继承对其进行扩展的类模板。

### Named Template Arguments（命名的模板参数）

此项技术主要为了解决默认参数可能过多，当指定某个参数时需要同时指定此参数之前的非默认参数。技术方案是将默认类型放在一个基类中，然后通过派生将其重载，相比直接指定类型参数，我们会通过辅助类提供相关信息：

```c++
template<typename PolicySetter1 = DefaultPolicyArgs,
typename PolicySetter2 = DefaultPolicyArgs,
typename PolicySetter3 = DefaultPolicyArgs,
typename PolicySetter4 = DefaultPolicyArgs>
class BreadSlicer {
    using Policies = PolicySelector<PolicySetter1,
    PolicySetter2,
    PolicySetter3,
    PolicySetter4>;
    // use Policies::P1, Policies::P2, … to refer to the various policies …
};

// PolicySelector<A,B,C,D> creates A,B,C,D as base classes
// Discriminator<> allows having even the same base class more than once
template<typename Base, int D>
class Discriminator : public Base {
};
template<typename Setter1, typename Setter2,
typename Setter3, typename Setter4>
class PolicySelector : public Discriminator<Setter1,1>,
public Discriminator<Setter2,2>,
public Discriminator<Setter3,3>,
public Discriminator<Setter4,4>
{
};

// name default policies as P1, P2, P3, P4
class DefaultPolicies {
public:
    using P1 = DefaultPolicy1;
    using P2 = DefaultPolicy2;
    using P3 = DefaultPolicy3;
    using P4 = DefaultPolicy4;
};

// class to define a use of the default policy values
// avoids ambiguities if we derive from DefaultPolicies more than once
class DefaultPolicyArgs : virtual public DefaultPolicies {
};

template<typename Policy>
class Policy1_is : virtual public DefaultPolicies {
public:
	using P1 = Policy; // overriding type alias
};
template<typename Policy>
class Policy2_is : virtual public DefaultPolicies {
public:
	using P2 = Policy; // overriding type alias
};
template<typename Policy>
class Policy3_is : virtual public DefaultPolicies {
public:
	using P3 = Policy; // overriding type alias
};
template<typename Policy>
class Policy4_is : virtual public DefaultPolicies {
public:
	using P4 = Policy; // overriding type alias};
}

template<…>
class BreadSlicer { …
public:
    void print () {
    	Policies::P3::doPrint();
    } …
};
```

## 桥接static和dynamic多态

### 广义函数指针

std::functional<>是一种高效的、广义形式的C++函数指针，提供了与函数指针相同的基本操作：

- 在调用者对函数一无所知的情况下可以被用来调用该函数；
- 可以被拷贝、move以及赋值；
- 可以被另一个（函数签名一致）函数初始化或赋值；
- 如何没有函数与之绑定，状态为null；

与函数指针优点更在于其可以被用来存储lambda以及其他任意实现了合适的operator()的函数对象。

下面来构造一个FunctionPtr来模仿std::functional<>：

```
// primary template:
template<typename Signature>
class FunctionPtr;
// partial specialization:
template<typename R, typename… Args>
class FunctionPtr<R(Args…)>
{
private:
	FunctorBridge<R, Args…>* bridge;
public:
    // constructors:
    FunctionPtr() : bridge(nullptr) {
    }
    FunctionPtr(FunctionPtr const& other); // see
    functionptrcpinv.hpp
    FunctionPtr(FunctionPtr& other)
    : FunctionPtr(static_cast<FunctionPtr const&>(other)) {
    }
    FunctionPtr(FunctionPtr&& other) : bridge(other.bridge) {
    	other.bridge = nullptr;
    }
    //construction from arbitrary function objects:
    template<typename F> FunctionPtr(F&& f); // see
    functionptrinit.hpp// assignment operators:
    FunctionPtr& operator=(FunctionPtr const& other) {
        FunctionPtr tmp(other);
        swap(*this, tmp);
        return *this;
    }
    FunctionPtr& operator=(FunctionPtr&& other) {
        delete bridge;
        bridge = other.bridge;
        other.bridge = nullptr;
        return *this;
    }
    //construction and assignment from arbitrary function objects:
    template<typename F> FunctionPtr& operator=(F&& f) {
        FunctionPtr tmp(std::forward<F>(f));
        swap(*this, tmp);
        return *this;
    }
    // destructor:
    ~FunctionPtr() {
    	delete bridge;
    }
    friend void swap(FunctionPtr& fp1, FunctionPtr& fp2) {
    	std::swap(fp1.bridge, fp2.bridge);
    }
    explicit operator bool() const {
    return bridge == nullptr;
    }
    // invocation:
    R operator()(Args… args) const; // see functionptr-cpinv.hpp
};
```

### 桥接接口

FunctorBridge类模板负责持有以及维护底层函数对象，其被实现为一个抽象基类，为FunctionPtr的动态多态打下基础：

```c++
template<typename R, typename… Args>
class FunctorBridge
{
public:
    virtual ~FunctorBridge() {
    }
    virtual FunctorBridge* clone() const = 0;
    virtual R invoke(Args… args) const = 0;
};
```

其提供了一些必要操作：一个析构函数，一个用来copy的clone函数，一个用来调用底层函数对象的invoke操作。有了这些虚函数就可以继续实现FunctionPtr的拷贝构造和函数调用操作符：

```c++
template<typename R, typename… Args>
FunctionPtr<R(Args…)>::FunctionPtr(FunctionPtr const& other)
: bridge(nullptr)
{
    if (other.bridge) {
        bridge = other.bridge->clone();
    }
}
template<typename R, typename… Args>
R FunctionPtr<R(Args…)>::operator()(Args&&… args) const
{
	return bridge->invoke(std::forward<Args>(args)…);
}
```

### 类型擦除

FunctorBridge每一个实例都是一个抽象类，为了支持所有可能的函数对象，我们可能需要无限多个派生类，幸运的是，我们可以通过用其所存储的函数对象的类型对派生类进行参数化：

```c++
template<typename Functor, typename R, typename… Args>
class SpecificFunctorBridge : public FunctorBridge<R, Args…> {
Functor functor;
public:
    template<typename FunctorFwd>
    SpecificFunctorBridge(FunctorFwd&& functor)
    : functor(std::forward<FunctorFwd>(functor)) {
    }
    virtual SpecificFunctorBridge* clone() const override {
   		return new SpecificFunctorBridge(functor);
    }
    virtual R invoke(Args&&… args) const override {
    	return functor(std::forward<Args>(args)…);
    }
};
```

SpecificFunctorBridge实例会在FunctionPtr被实例化的时候顺带产生，其剩余实现如下：

```c++
template<typename R, typename… Args>
template<typename F>
FunctionPtr<R(Args…)>::FunctionPtr(F&& f)
: bridge(nullptr)
{
    using Functor = std::decay_t<F>;
    using Bridge = SpecificFunctorBridge<Functor, R, Args…>;
    bridge = new Bridge(std::forward<F>(f));
}
```

一旦新开辟的Bridge实例被赋值给数据成员bridge，由于从派生类到基类的转换（Bridge*->FunctorBridge\<R, Args...\>\*），特定类型F的额外信息将会丢失，这样我们可不管具体的类型，统一到基类进行处理。该实现的一个特点是在生成Functor的类型的时候使用了std::decay，使得被推断出来的类型F可以被存储，比如它会将指向函数类型的引用decay成函数指针类型，并移除顶层const，volatile和引用。

### 可选桥接

加入以下功能：检测两个FunctionPtr对象是否会调用相同的函数，为了实现这一功能，在SpecificFunctorBridge加入equals操作：

```c++
virtual bool equals(FunctorBridge<R, Args…> const* fb) const override
{
    if (auto specFb = dynamic_cast<SpecificFunctorBridge const*> (fb))
    {
    	return functor == specFb->functor;
    }
    //functors with different types are never equal:
    return false;
}
```

最后为FunctionPtr实现operator==：

```c++
friend bool
operator==(FunctionPtr const& f1, FunctionPtr const& f2) {
    if (!f1 || !f2) {
    	return !f1 && !f2;
    }
    return f1.bridge->equals(f2.bridge);
}
friend bool
operator!=(FunctionPtr const& f1, FunctionPtr const& f2) {
	return !(f1 == f2);
}
```

这种实现也有一个缺点：如果FunctionPtr被一个没有实现合适的Operator==的函数对象（比如lambda）赋值，或者被这一类对象初始化，那么会遇到编译错误，这一问题是由于类型擦除导致的：因为在给FunctionPtr赋值或初始化的时候会丢失函数对象的类型信息，该信息就包含调用函数对象的operator==所需要的信息，幸运的是我们可以使用基于SFINAE的萃取技术，在调用operator==之前确认是否可用：

```c++
#include <utility> // for declval()
#include <type_traits> // for true_type and false_type
template<typename T>
class IsEqualityComparable
{
private:
    // test convertibility of == and ! == to bool:
    static void* conv(bool); // to check convertibility to bool
    template<typename U>
    static std::true_type test(decltype(conv(std::declval<U
    const&>() == std::declval<U const&>())),
    decltype(conv(!(std::declval<U const&>() == std::declval<U
    const&>()))));
    // fallback:
    template<typename U>
    static std::false_type test(…);
public:
    static constexpr bool value = decltype(test<T>(nullptr,
    nullptr))::value;
};
```

两个test重载，一个包含了封装在decltype中用来测试的表达式，另一个通过省略号来接收任意数量的参数，第一个test试图通过==来比较两个T const类型的对象，然后确保两个结果可以被隐式的转换成bool，并将可以转换为bool的结果传递给operator!=，如果两个运算符都正常，参数类型将是void*。

使用IsEqualityComparable可以构建一个TryEquals类模板，要么会调用==运算符，要么在没有可用的时候抛出异常：

```c++
#include <exception>
#include "isequalitycomparable.hpp"
template<typename T, bool EqComparable =
IsEqualityComparable<T>::value>
struct TryEquals
{
    static bool equals(T const& x1, T const& x2) {
    	return x1 == x2;
    }
};
class NotEqualityComparable : public std::exception
{ };
template<typename T>
struct TryEquals<T, false>
{
    static bool equals(T const& x1, T const& x2) {
    	throw NotEqualityComparable();
    }
}

virtual bool equals(FunctorBridge<R, Args…> const* fb) const override
{
    if (auto specFb = dynamic_cast<SpecificFunctorBridge const*>(fb)) {
   		return TryEquals<Functor>::equals(functor, specFb->functor);
    }
    //functors with different types are never equal:
    return false;
}
```

## 元编程

元编程的特性之一就是在编译期间就可以进行一部分用户定义的计算。

元编程分为值元编程，类型元编程。类型元编程，比如考虑以下的例子：

```c++
// primary template: in general we yield the given type:
template<typename T>
struct RemoveAllExtentsT {
	using Type = T;
};
// partial specializations for array types (with and without bounds):
template<typename T, std::size_t SZ>
struct RemoveAllExtentsT<T[SZ]> {
	using Type = typename RemoveAllExtentsT<T>::Type;
};
template<typename T>
struct RemoveAllExtentsT<T[]> {
	using Type = typename RemoveAllExtentsT<T>::Type;
};
template<typename T>
using RemoveAllExtents = typename RemoveAllExtentsT<T>::Type;
```

这里的RemoveAllExtents就是一种类型元函数，它会从类型中移除任意数量的顶层数组层：

```c++
RemoveAllExtents<int[]> // yields int
RemoveAllExtents<int[5][10]> // yields int
RemoveAllExtents<int[][10]> // yields int
RemoveAllExtents<int(*)[5]> // yields int(*)[5]
```

元函数通过偏特化来匹配高层次的数组，递归的调用自己并最终完成任务。

通过使用数值元编程和类型元编程，可以在编译期间计算数值和类型，不过元编程能做的不仅仅只有这些：我们可以在编译期间，以编程的方式组合一些有运行期效果的代码，称之为混合元编程。比如两个array的相乘：

```c++
template<typename T, std::size_t N>
struct DotProductT {
    static inline T result(T* a, T* b)
    {
    	return *a * *b + DotProduct<T, N-1>::result(a+1,b+1);
    }
};
// partial specialization as end criteria
template<typename T>
struct DotProductT<T, 0> {
    static inline T result(T*, T*) {
    	return T{};
    }
};
template<typename T, std::size_t N>
auto dotProduct(std::array<T, N> const& x,
std::array<T, N> const& y)
{
	return DotProductT<T, N>::result(x.begin(), y.begin());
}
```

例子中为了支持偏特化所以用了struct，使用了内联保证递归实例化的效率。

以下是另一个例子，要用一个基于主单位的分数来记录相关单位:

```c++
template<unsigned N, unsigned D = 1>
struct Ratio {
    static constexpr unsigned num = N; // numerator
    static constexpr unsigned den = D; // denominator
    using Type = Ratio<num, den>;
};

// implementation of adding two ratios:
template<typename R1, typename R2>
struct RatioAddImpl
{
private:
    static constexpr unsigned den = R1::den * R2::den;
    static constexpr unsigned num = R1::num * R2::den + R2::num *
    R1::den;
public:
	typedef Ratio<num, den> Type;
};
// using declaration for convenient usage:
template<typename R1, typename R2>
using RatioAdd = typename RatioAddImpl<R1, R2>::Type;

using R1 = Ratio<1,1000>;
using R2 = Ratio<2,3>;
using RS = RatioAdd<R1,R2>; //RS has type Ratio<2003,2000>
std::cout << RS::num << ’/’ << RS::den << ’\n’; //prints 2003/3000
using RA = RatioAdd<Ratio<2,3>,Ratio<5,7>>; //RA has type
Ratio<29,21>
std::cout << RA::num << ’/’ << RA::den << ’\n’; //prints 29/2

// duration type for values of type T with unit type U:
template<typename T, typename U = Ratio<1>>
class Duration {public:
    using ValueType = T;
    using UnitType = typename U::Type;
    private:
    ValueType val;
public:
    constexpr Duration(ValueType v = 0)
    : val(v) {
    }
    constexpr ValueType value() const {
    	return val;
    }
};

// adding two durations where unit type might differ:
template<typename T1, typename U1, typename T2, typename U2>
auto constexpr operator+(Duration<T1, U1> const& lhs,
Duration<T2, U2> const& rhs)
{
    // resulting type is a unit with 1 a nominator and
    // the resulting denominator of adding both unit type fractions
    using VT = Ratio<1,RatioAdd<U1,U2>::den>;
    // resulting value is the sum of both values
    // converted to the resulting unit type:
    auto val = lhs.value() * VT::den / U1::den * U1::num +
    rhs.value() * VT::den / U2::den * U2::num;
    return Duration<decltype(val), VT>(val);
}

int x = 42;
int y = 77;
auto a = Duration<int, Ratio<1,1000>>(x); // x milliseconds
auto b = Duration<int, Ratio<2,3>>(y); // y 2/3 seconds
auto c = a + b; //computes resulting unit type 1/3000 seconds//and
generates run-time code for c = a*3 + b*2000
```

其中Ratio类表示分数，num和den分别表示分子和分母。

### 反射元编程的维度

值元编程除了constexpr函数之外，还可以使用跟类型元编程相似的递归实例化：

```c++
// primary template to compute sqrt(N)
template<int N, int LO=1, int HI=N>
struct Sqrt {
    // compute the midpoint, rounded up
    static constexpr auto mid = (LO+HI+1)/2;
    // search a not too large value in a halved interval
    static constexpr auto value = (N<mid*mid) ?
    Sqrt<N,LO,mid-1>::value : Sqrt<N,mid,HI>::value;
};
// partial specialization for the case when LO equals HI
template<int N, int M>
struct Sqrt<N,M,M> {
	static constexpr auto value = M;
};
```

这段代码中使用了static，其生命周期在编译期可以确定，编译器可以在编译期对这些变量进行计算和优化。这段代码的效率远不如constexpt函数友好。一个综合的元编程解决方案应该在以下三个维度中选择：

- 计算维度；
- 反射维度；
- 生成维度；

反射维度是以编程方式检测程序特性的能力，生成维度指为程序生成额外代码的能力。计算维度中我们已经认识了递归实例化和constexpr计算，对于反射维度，类型萃取一章中也介绍了部分方案。

### 递归实例化的代价

比如原先的sqrt模板，有这样的代码：

```c++
(16<=8*8) ? Sqrt<16,1,8>::value
: Sqrt<16,9,16>::value
```

它并不是只计算真正用到了的分支，同样也会计算没有用到的分支，不止如此，代码试图访问实例化来的类的成员，所以该类中所有的成员都会被实例化，最终实例化出的实例几乎是N的两倍。幸运的是，有一些技术可以被用来降低实例化的数目，比如：

```c++
#include "ifthenelse.hpp"
// primary template for main recursive step
template<int N, int LO=1, int HI=N>
struct Sqrt {
    // compute the midpoint, rounded up
    static constexpr auto mid = (LO+HI+1)/2;
    // search a not too large value in a halved interval
    using SubT = IfThenElse<(N<mid*mid),
    Sqrt<N,LO,mid-1>,
    Sqrt<N,mid,HI>>;
    static constexpr auto value = SubT::value;
};
// partial specialization for end of recursion criterion
template<int N, int S>
struct Sqrt<N, S, S> {
	static constexpr auto value = S;
};
```

这里比较重要的一点是为一个类模板的实例定义类型别名，不会导致编译期去实例化该实例，在调用SubT::value的时候，只有真正被赋值给SubT的那一个实例才会被完全实例化。

### 计算完整性

从Sqrt的例子可以看出，一个模板元程序可能包括以下内容：

- 状态变量：模板参数；
- 循环结构：通过递归实现；
- 执行路径选择：通过条件表达式或者偏特例化实现；
- 整数运算；

### 递归实例化和递归模板参数

考虑以下递归模板：

```c++
template<typename T, typename U>
struct Doublify {
};
template<int N>
struct Trouble {
    using LongType = Doublify<typename Trouble<N-1>::LongType,
    typename Trouble<N-1>::LongType>;
};
template<>
struct Trouble<0> {
	using LongType = double;
};
Trouble<10>::LongType ouch;
```

其会越来越复杂的类型实例化Doublify，比如：

![Doublify](..\image\template\Doublify.png)

这样看来Trouble\<N\>::LongType类型的复杂度与N成指数关系，通常这种情况给编译器的压力要比有递归实例化但是没有递归模板参数的情况要大，这里的问题在于编译器会用一些支离破碎的名字来表达这些类型，这些支离破碎的名字会用相同的方式去编码模板的实例化，在早期c++中，这一编码方式的实现和模板签名的长度成正比。新的C++实现使用聪明的压缩技术来降低名称编码，但除此之外其他情况没有改善，因此在组织递归实例化代码的时候最好不要让模板参数也嵌套递归。

### 枚举值还是静态常量

在早期C++中枚举值是唯一可以在类的声明中创建可用于类成员的真正的常量（常量表达式）的方式，比如：

```c++
// primary template to compute 3 to the Nth
template<int N>
struct Pow3 {
	enum { value = 3 * Pow3<N-1>::value };
};
// full specialization to end the recursion
template<>
struct Pow3<0> {
	enum { value = 1 };
};

```

在98中引入了类内静态常量初始化的概念，因此上面代码可以改写为：

```c++
// primary template to compute 3 to the Nth
template<int N
struct Pow3 {
	static int const value = 3 * Pow3<N-1>::value;
};
// full specialization to end the recursion
template<>
struct Pow3<0> {
	static int const value = 1;
};
```

但这样有一个问题，静态常量成员是左值，因此如果有这种情况：

```c++
void foo(int const&);
foo(Pow3<7>::value);
```

编译器需要传递地址，因此必须实例化静态成员并为之开辟内存，这样就不是一个纯正的编译期程序了。枚举值不是左值（也就是说没有地址），因此将其按引用传递，不会用到静态内存，几乎等效于将计算值按照字面值传递。后来，C++引入了constexpr静态数据成员，其使用不限于整型数据，虽然这没有解决上文中关于地址的问题，即便如此，它也是用来产生元程序结果的常规方法，其优点是它可以有正确的类型（相对于人工的枚举而言），而且用auto声明静态成员类型时，可以对类型进行推断，17引入了inline的静态数据成员，解决了上面提到的地址问题，而且可以和constexpr一起使用。

## 类型列表

对于类型元编程，核心的数据结构是typelist，指的是包含了类型的列表，它提供了典型的列表操作方法：遍历列表中的元素，添加元素或者删除元素。但是它和大多数运行期的数据结构都不同，它的值不允许修改，向类型列表添加一个元素不会修改原始列表，而是会创建一个新的。

类型列表通常是按照类模板特例的形式实现的，它将自身的内容编码到了参数包中。一种直接的实现方式如下：

```c++
template<typename… Elements>
class Typelist
{};
```

下面是一个包含了所有有符号整型的类型列表：

```c++
using SignedIntegralTypes =
Typelist<signed char, short, int, long, long>;
```

操作这个类型列表可能有很多元函数，比如Front：

```c++
template<typename List>
class FrontT;
template<typename Head, typename… Tail>
class FrontT<Typelist<Head, Tail…>>
{
public:
	using Type = Head;
};
template<typename List>
using Front = typename FrontT<List>::Type;
```

这里使用了偏特化的实现，只要有类型存在，就会使用特化的模板，如果为空，会使用主模板。

同理也可以向列表中添加元素，只需要将所有存在的元素捕获到一个包中，之后创建一个包含了所有元素的特例即可：

```c++
template<typename List, typename NewElement>
class PushFrontT;
template<typename… Elements, typename NewElement>
class PushFrontT<Typelist<Elements…>, NewElement> {
public:
	using Type = Typelist<NewElement, Elements…>;
};
template<typename List, typename NewElement>
using PushFront = typename PushFrontT<List, NewElement>::Type;
```

### 类型列表的算法

基础的类型列表操作可以组合起来实现更有意思的列表操作，比如将PushFront作用于PopFront可以实现对第一个元素的替换：

```c++
using Type = PushFront<PopFront<SignedIntegralTypes>, bool>;
// equivalent to Typelist<bool, short, int, long, long>
```

我们实现列表比较基础的操作，索引，比如提取第N个元素，其实现方式是使用一个递归的元程序遍历typelist的元素，直到找到所需的：

```c++
// recursive case:
template<typename List, unsigned N>
class NthElementT : public NthElementT<PopFront<List>, N-1>
{};
// basis case:
template<typename List>
class NthElementT<List, 0> : public FrontT<List>
{ };
template<typename List, unsigned N>
using NthElement = typename NthElementT<List, N>::Type;
```

先看基本情况，对FrontT\<List\>进行public继承，这样其可以使用Type类型成员，偏特化部分用来遍历成员。

有些类型列表算法会想要查找列表中的数据，比如找出列表中最大的类型，同样可以通过递归模板元程序实现：

```c++
template<typename List>
class LargestTypeT;
// recursive case:
template<typename List>
class LargestTypeT
{
private:
    using First = Front<List>;
    using Rest = typename LargestTypeT<PopFront<List>>::Type;
public:
    using Type = IfThenElse<(sizeof(First) >= sizeof(Rest)), First,
    Rest>;
};
// basis case:
template<>
class LargestTypeT<Typelist<>>
{
public:
	using Type = char;
};
template<typename List>
using LargestType = typename LargestTypeT<List>::Type;
```

代码中显式的用到了空的类型列表Typelist<>，这样有点不太好，因为它可能会妨碍到其他类型的类型列表，为了解决这一问题，引入了IsEmpty元函数，可以用来判断一个类型列表是否为空：

```c++
template<typename List>
class IsEmpty
{
public:
	static constexpr bool value = false;
};
template<>
class IsEmpty<Typelist<>> {
public:
	static constexpr bool value = true;
};

template<typename List, bool Empty = IsEmpty<List>::value>
class LargestTypeT;
// recursive case:
template<typename List>
class LargestTypeT<List, false>
{
private:
    using Contender = Front<List>;
    using Best = typename LargestTypeT<PopFront<List>>::Type;
public:
    using Type = IfThenElse<(sizeof(Contender) >=
    sizeof(Best)),Contender, Best>;
};
// basis case:
template<typename List>
class LargestTypeT<List, true>
{
public:
	using Type = char;
};
template<typename List>
using LargestType = typename LargestTypeT<List>::Type;
```

为了支持列表的PushBack，需要对PushFront做一点小修改：

```c++
template<typename List, typename NewElement>
class PushBackT;
template<typename… Elements, typename NewElement>
class PushBackT<Typelist<Elements…>, NewElement>
{
public:
	using Type = Typelist<Elements…, NewElement>;
};
template<typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;

template<typename List, typename NewElement, bool =
IsEmpty<List>::value>
class PushBackRecT;
// recursive case:
template<typename List, typename NewElement>
class PushBackRecT<List, NewElement, false>
{
    using Head = Front<List>;
    using Tail = PopFront<List>;
    using NewTail = typename PushBackRecT<Tail, NewElement>::Type;
public:
	using Type = PushFront<Head, NewTail>;
};
// basis case:
template<typename List, typename NewElement>
class PushBackRecT<List, NewElement, true>
{
public:
	using Type = PushFront<List, NewElement>;
};
// generic push-back operation:
template<typename List, typename NewElement>
class PushBackT : public PushBackRecT<List, NewElement> { };
template<typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;
```

下面介绍列表的反转：

```c++
template<typename List, bool Empty = IsEmpty<List>::value>
class ReverseT;
template<typename List>
using Reverse = typename ReverseT<List>::Type;
// recursive case:
template<typename List>
class ReverseT<List, false>:public PushBackT<Reverse<PopFront<List>>,
Front<List>> { };
// basis case:
template<typename List>
class ReverseT<List, true>{
public:
	using Type = List;
};

```

结合Reverse，可以实现移除列表中最后一个元素的PopBack:

```c++
template<typename List>
class PopBackT {
public:
	using Type = Reverse<PopFront<Reverse<List>>>;
};
template<typename List>
using PopBack = typename PopBackT<List>::Type;
```

该算法先反转整个列表，然后删除首元素并将剩余列表再次反转，从而实现删除末尾元素的目的。

列表类型的转换，为了实现这个目的，相应的算法应该接受一个类型列表和一个元函数作为参数，并返回一个将该元函数作用于类型列表中每个元素之后得到的新的类型列表。算法如下：

```c++
template<typename List, template<typename T> class MetaFun, bool Empty
= IsEmpty<List>::value>
class TransformT;
// recursive case:
template<typename List, template<typename T> class MetaFun>
class TransformT<List, MetaFun, false>
: public PushFrontT<typename TransformT<PopFront<List>,
MetaFun>::Type, typename MetaFun<Front<List>>::Type>
{};
// basis case:
template<typename List, template<typename T> class MetaFun>
class TransformT<List, MetaFun, true>
{
public:
using Type = List;
};
template<typename List, template<typename T> class MetaFun>
using Transform = typename TransformT<List, MetaFun>::Type;
```

类型列表的累加需要用到转换算法，它会将类型列表中的所有元素组合成一个值，它接受一个包含元素T1、T2、……、TN的类型列表T，一个初始类型I，和一个接收两个类型作为参数的元函数F，并最终返回一个类型。其实现方式遵循了标准的递归元编程模式：

```c++
template<typename List,
    template<typename X, typename Y> class F,
    typename I,
	bool = IsEmpty<List>::value>
class AccumulateT;
// recursive case:
template<typename List,
template<typename X, typename Y> class F,
typename I>
class AccumulateT<List, F, I, false>
: public AccumulateT<PopFront<List>, F,
typename F<I, Front<List>>::Type>
{};
// basis case:
template<typename List,
template<typename X, typename Y> class F,
typename I>
class AccumulateT<List, F, I, true>
{
public:
	using Type = I;
};
template<typename List,
template<typename X, typename Y> class F,
typename I>
using Accumulate = typename AccumulateT<List, F, I>::Type;
```

插入排序，和其他算法类似，其递归过程会将列表分成第一个元素和剩余元素，然后对剩余元素进行递归排序，并将头元素插入到合适的位置，其实现如下：

```c++
template<typename List, template<typename T, typename U> class Compare,
bool = IsEmpty<List>::value>
class InsertionSortT;
template<typename List, template<typename T, typename U> class Compare>
using InsertionSort = typename InsertionSortT<List, Compare>::Type;
// recursive case (insert first element into sorted list):
template<typename List, template<typename T, typename U> class Compare>
class InsertionSortT<List, Compare, false>
: public InsertSortedT<InsertionSort<PopFront<List>, Compare>,
Front<List>, Compare>
{};
// basis case (an empty list is sorted):
template<typename List, template<typename T, typename U> class Compare>
class InsertionSortT<List, Compare, true>
{
public:
	using Type = List;
};

#include "identity.hpp"
template<typename List, typename Element,
template<typename T, typename U> class Compare, bool =
IsEmpty<List>::value>
class InsertSortedT;
// recursive case:
template<typename List, typename Element, template<typename T,
typename U> class Compare>
class InsertSortedT<List, Element, Compare, false>
{
    // compute the tail of the resulting list:
    using NewTail = typename IfThenElse<Compare<Element,
    Front<List>>::value, IdentityT<List>,
    InsertSortedT<PopFront<List>,
    Element, Compare>>::Type;
    // compute the head of the resulting list:
    using NewHead = IfThenElse<Compare<Element, Front<List>>::value,
	Element, Front<List>>;
public:
	using Type = PushFront<NewTail, NewHead>;
};
// basis case:
template<typename List, typename Element, template<typename T,
typename U> class Compare>
class InsertSortedT<List, Element, Compare, true>
: public PushFrontT<List, Element>
{};
template<typename List, typename Element,template<typename T, typename
U> class Compare>
using InsertSorted = typename InsertSortedT<List, Element,
Compare>::Type;
```

### 非类型列表

有很多种方法可以用来生成一个包含编译期数值的类型列表，一个简单的方法是定义一个类模板CTValue，然后用它表示类型列表中某种类型的值：

```c++
template<typename T, T Value>
struct CTValue
{
	static constexpr T value = Value;
};

using Primes = Typelist<CTValue<int, 2>, CTValue<int, 3>,
    CTValue<int, 5>, CTValue<int, 7>,
    CTValue<int, 11>>;
```

这样就可以对列表中的数值进行数值计算，比如这些素数的乘积：

```c++
template<typename T, typename U>
struct MultiplyT;
template<typename T, T Value1, T Value2>
struct MultiplyT<CTValue<T, Value1>, CTValue<T, Value2>> {
public:
	using Type = CTValue<T, Value1 * Value2>;
};
template<typename T, typename U>
using Multiply = typename MultiplyT<T, U>::Type;

Accumulate<Primes, MultiplyT, CTValue<int, 1>>::value
```

但这样太复杂了，尤其是所有数值的类型相同时，可以引入CTTypelist模板别名来优化，它提供了一组包含在Typelist中、类型相同的数值：

```c++
template<typename T, T… Values>
using CTTypelist = Typelist<CTValue<T, Values>...>;
using Primes = CTTypelist<int, 2, 3, 5, 7, 11>;
```

这一方式的缺点是别名终归只是别名，当遇到错误时，错误信息可能一直会打印到底层的Typelist，导致错误信息过于冗长，为了解决这一问题，可以定义一个可以直接存储数值的、全新的列表类Valuelist：

```c++
template<typename T, T… Values>
struct Valuelist {
};
template<typename T, T… Values>
struct IsEmpty<Valuelist<T, Values…>> {
	static constexpr bool value = sizeof…(Values) == 0;
};
template<typename T, T Head, T… Tail>
struct FrontT<Valuelist<T, Head, Tail…>> {
    using Type = CTValue<T, Head>;
    static constexpr T value = Head;
};
template<typename T, T Head, T… Tail>
struct PopFrontT<Valuelist<T, Head, Tail…>> {
	using Type = Valuelist<T, Tail…>;
};
template<typename T, T… Values, T New>
struct PushFrontT<Valuelist<T, Values…>, CTValue<T, New>> {
	using Type = Valuelist<T, New, Values…>;
};
template<typename T, T… Values, T New>
struct PushBackT<Valuelist<T, Values…>, CTValue<T, New>> {
	using Type = Valuelist<T, Values…, New>;
};
```

在17中，可以通过一个可推断的非类型参数（结合auto）来优化CTValue的实现：

```c++
template<auto Value>
struct CTValue
{
	static constexpr auto value = Value;
};
using Primes = Typelist<CTValue<2>, CTValue<3>, CTValue<5>, CTValue<7>,
CTValue<11>>;
```

### 对包扩展相关算法的优化

通过适用包展开，可以将类型列表迭代的任务转移给编译器，比如：

```c++
template<typename… Elements, template<typename T> class MetaFun>
class TransformT<Typelist<Elements…>, MetaFun, false>
{
public:
	using Type = Typelist<typename MetaFun<Elements>::Type…>;
};
```

也可以基于索引值从一个已有列表中选择一些元素，并生成新的列表：

```c++
template<typename Types, typename Indices>
class SelectT;
template<typename Types, unsigned… Indices>
class SelectT<Types, Valuelist<unsigned, Indices…>>
{
public:
	using Type = Typelist<NthElement<Types, Indices>…>;
};
template<typename Types, typename Indices>
using Select = typename SelectT<Types, Indices>::Type;
```

### Cons-style Typelists

在引入变参模板之前，类型列表通常参照LISP的cons单元的实现方式，用递归数据结构实现，每一个cons单元包含一个值（列表的head）和一个嵌套列表，这个嵌套列表可以是另一个cons单元或者一个空的列表nil，实现如下：

```c++
class Nil { };
template<typename HeadT, typename TailT = Nil>
class Cons {
public:
    using Head = HeadT;
    using Tail = TailT;
};
using TwoShort = Cons<short, Cons<unsigned short>>;
using SignedIntegralTypes = Cons<signed char, Cons<short, Cons<int,
Cons<long, Cons<long long, Nil>>>>>;

```

向这样一个cons-style列表中提取第一个元素，只需要直接访问其头部元素：

```c++
template<typename List>
class FrontT {
public:
	using Type = typename List::Head;
};
template<typename List>
using Front = typename FrontT<List>::Type;
```

向其头部追加一个元素只需要在当前类型列表外包上一层：

```c++
template<typename List, typename Element>
class PushFrontT {
public:
	using Type = Cons<Element, List>;
};
template<typename List, typename Element>
using PushFront = typename PushFrontT<List, Element>::Type;
```

要删除首元素，只需要提取出当前列表的Tail即可：

```c++
template<typename List>
class PopFrontT {
public:
	using Type = typename List::Tail;
};
template<typename List>
using PopFront = typename PopFrontT<List>::Type;

```

至于IsEmpty的实现，只需要对Nil进行特例化：

```c++
template<typename List>
struct IsEmpty {
	static constexpr bool value = false;
};
template<>
struct IsEmpty<Nil> {
	static constexpr bool value = true;
};
```

## 元组

容器包含不同的类型，通过位置信息索引。

类型列表Typelist描述了一组可以在编译期间操作的类型，元组则描述了可以在运行期间操作的存储，比如：

```c++
template<typename… Types>
class Tuple { … // implementation discussed below
};
Tuple<int, double, std::string> t(17, 3.14, "Hello, World!");
```

通常会使用模板元编程和typelist来创建存储数据的tuple。

### 基本的元组设计

元组包含了对模板参数列表中每一个类型的存储，这部分存储可以通过函数模板get进行访问，对于元组t，其用法为get\<I\>(t)。比如get\<1\>(t)会返回指向double 3.14的引用。其递归实现是基于这样的思路：一个包含了N个元素的元组可以被存储为一个单独的元素和一个包含了剩余N-1个元素的元组，对于元素为空的元组，只需要当作特例处理即可：

```c++
template<typename… Types>
class Tuple;
// recursive case:
template<typename Head, typename… Tail>
class Tuple<Head, Tail…>
{
private:
    Head head;
    Tuple<Tail…> tail;
public:
	// constructors:
    Tuple() {
    }
    Tuple(Head const& head, Tuple<Tail…> const& tail): head(head),
    tail(tail) {
    }…
    Head& getHead() { return head; }
    Head const& getHead() const { return head; }
    Tuple<Tail…>& getTail() { return tail; }
    Tuple<Tail…> const& getTail() const { return tail; }
};
// basis case:
template<>
class Tuple<> {
// no storage required
};
```

函数模板get则会通过遍历这个递归的结构来提取所需要的元素：

```c++
// recursive case:
template<unsigned N>
struct TupleGet {
    template<typename Head, typename… Tail>
    static auto apply(Tuple<Head, Tail…> const& t) {
    	return TupleGet<N-1>::apply(t.getTail());
    }
};
// basis case:
template<>
struct TupleGet<0> {
    template<typename Head, typename… Tail>
    static Head const& apply(Tuple<Head, Tail…> const& t) {
    	return t.getHead();
    }
};
template<unsigned N, typename… Types>
auto get(Tuple<Types…> const& t) {
	return TupleGet<N>::apply(t);
}
```

这里的get只是封装了一个简单的对TupleGet的静态成员函数的调用，在不能对函数模板进行部分特例化的情况下，这是一个有效的变通方法。

为了让元组的使用更加方便，还应该允许用一组相互独立的值或者另一个元组来构造一个新的元组：

```c++
Tuple() {
}
Tuple(Head const& head, Tuple<Tail…> const& tail)
: head(head), tail(tail)
{
}
Tuple(Head const& head, Tail const&… tail)
: head(head), tail(tail…)
{}
```

用户可能会希望用移动构造来初始化元组的一些元素，或者用一个类型不相同的数值类初始化元组的某个元素，因此需要用完美转发来初始化元组：

```c++
template<typename VHead, typename… VTail>
Tuple(VHead&& vhead, VTail&&… vtail)
: head(std::forward<VHead>(vhead)), tail(std::forward<VTail>(vtail)…)
{
}
```

下面的实现则允许用一个元组去构建另一个元组：

```c++
template<typename VHead, typename… VTail>
Tuple(Tuple<VHead, VTail…> const& other)
: head(other.getHead()), tail(other.getTail())
{ }
```

然而这个构造函数不适用于类型转换：给定上文中的t，试图用它来创建一个元素之间类型兼容的元组会遇到错误：

```c++
// ERROR: no conversion from Tuple<int, double, string> to long
Tuple<long int, long double, std::string> t2(t);
```

这个因为这个调用会更匹配用一组数值去初始化一个元组的构造函数模板，而不是用一个元组去初始化另一个元组的构造函数模板，为了解决这一问题，需要用到6.3节介绍的std::enable_if，在tail长度与预期不同的时候就禁用相关模板：

```c++
template<typename VHead, typename… VTail, typename = std::enable_if_t<sizeof…
(VTail)==sizeof… (Tail)>>
Tuple(VHead&& vhead, VTail&&… vtail)
: head(std::forward<VHead>(vhead)), tail(std::forward<VTail>(vtail)…)
{ }
template<typename VHead, typename… VTail, typename = std::enable_if_t<sizeof…
(VTail)==sizeof… (Tail)>>
Tuple(Tuple<VHead, VTail…> const& other)
: head(other.getHead()), tail(other.getTail()) { }
```

函数模板会通过类型推断来决定生成元组中元素的类型，这使得用一组数值来创建一个元组变的简单：

```c++
template<typename… Types>
auto makeTuple(Types&&… elems)
{
	return Tuple<std::decay_t<Types>…>(std::forward<Types> (elems)…);
}
```

这里会将字符串常量和裸数组转换成指针，并去除元素的const和引用属性。

### 基础元组操作

例如比较操作：

```c++
// basis case:
bool operator==(Tuple<> const&, Tuple<> const&)
{
	// empty tuples are always equivalentreturn true;
}
// recursive case:
template<typename Head1, typename… Tail1,
    typename Head2, typename… Tail2,
    typename = std::enable_if_t<sizeof…(Tail1)==sizeof…(Tail2)>>
bool operator==(Tuple<Head1, Tail1…> const& lhs, Tuple<Head2, Tail2…> const& rhs)
{
    return lhs.getHead() == rhs.getHead() &&
    	lhs.getTail() == rhs.getTail();
}

```

接下来是打印的例子：

```c++
#include <iostream>
void printTuple(std::ostream& strm, Tuple<> const&, bool isFirst = true)
{
	strm << ( isFirst ? ’(’ : ’)’ );
}
template<typename Head, typename… Tail>
void printTuple(std::ostream& strm, Tuple<Head, Tail…> const& t, bool isFirst =
true)
{
    strm << ( isFirst ? "(" : ", " );
    strm << t.getHead();
    printTuple(strm, t.getTail(), false);
}
template<typename … Types>
std::ostream& operator<<(std::ostream& strm, Tuple<Types…> const& t)
{
    printTuple(strm, t);
    return strm;
}
```

### 元组的算法

元组是一种提供了以下各种功能的容器：可以访问并修改其元素的能力（通过 get<>），创 建新元组的能力（直接创建或者通过使用 makeTuple<>创建），以及将元组分割成 head 和 tail 的能力（通过使用 getHead()和 getTail()）。使用这些功能足以创建各种各样的元组算法， 比如添加或者删除元组中的元素，重新排序元组中的元素，或者选取元组中元素的某些子集。

我们可以将元组用作类型列表，如果忽略掉元组模板在运行期间的相关部分，可以发现其结构和Typelist完全一样，事实上，通过使用一些部分特例化，可以将元组变成一个功能完整的Typelist：

```c++
// determine whether the tuple is empty:
template<>
struct IsEmpty<Tuple<>> {
	static constexpr bool value = true;
};
// extract front element:
template<typename Head, typename… Tail>
class FrontT<Tuple<Head, Tail…>> {
public:
	using Type = Head;
};
// remove front element:
template<typename Head, typename… Tail>
class PopFrontT<Tuple<Head, Tail…>> {
public:
	using Type = Tuple<Tail…>;
};
// add element to the front:
template<typename… Types, typename Element>
class PushFrontT<Tuple<Types…>, Element> {
public:
	using Type = Tuple<Element, Types…>;
};
// add element to the back:
template<typename… Types, typename Element>
class PushBackT<Tuple<Types…>, Element> {
public:
	using Type = Tuple<Types…, Element>;
};
```

接下来要实现如何添加以及删除元素：

```c++
template<typename… Types, typename V>
PushFront<Tuple<Types…>, V>
pushFront(Tuple<Types…> const& tuple, V const& value)
{
	return PushFront<Tuple<Types…>, V>(value, tuple);
}

// basis case
template<typename V>
Tuple<V> pushBack(Tuple<> const&, V const& value)
{
	return Tuple<V>(value);
}
// recursive case
template<typename Head, typename… Tail, typename V>
Tuple<Head, Tail…, V>
pushBack(Tuple<Head, Tail…> const& tuple, V const& value)
{
    return Tuple<Head, Tail…, V>(tuple.getHead(),
    					pushBack(tuple.getTail(), value));
}

template<typename… Types>
PopFront<Tuple<Types…>> popFront(Tuple<Types…> const& tuple)
{
	return tuple.getTail();
}
```

元组的反转实现如下：

```c++
// basis case
Tuple<> reverse(Tuple<> const& t)
{
	return t;
}
// recursive case
template<typename Head, typename… Tail>
Reverse<Tuple<Head, Tail…>> reverse(Tuple<Head, Tail…> const& t)
{
	return pushBack(reverse(t.getTail()), t.getHead());
}
```

上文中反转元组在运行期间效率非常低，为了展现这一问题，引入可以计算其实例可以被copy次数的类：

```c++
template<int N>
struct CopyCounter
{
    inline static unsigned numCopies = 0;
    CopyCounter()
    {
    }
    CopyCounter(CopyCounter const&) {
    	++numCopies;
    }
};

//创建并反转一个包含CopyCounter实例的元组：
void copycountertest()
{
    Tuple<CopyCounter<0>, CopyCounter<1>, CopyCounter<2>, CopyCounter<3>,
    CopyCounter<4>> copies;
    auto reversed = reverse(copies);
    std::cout << "0: " << CopyCounter<0>::numCopies << " copies\n";
    std::cout << "1: " << CopyCounter<1>::numCopies << " copies\n";
    std::cout << "2: " << CopyCounter<2>::numCopies << " copies\n";
    std::cout << "3: " << CopyCounter<3>::numCopies << " copies\n";
    std::cout << "4: " << CopyCounter<4>::numCopies << " copies\n";
}
//结果如下：
0: 5 copies
1: 8 copies
2: 9 copies
3: 8 copies
4: 5 copies
```

我们希望可以通过索引列表将简单的一致长度的元组反转，使其只进行一次拷贝，比如：

```c++
auto reversed = makeTuple(get<4>(copies), get<3>(copies), get<2>(copies),
get<1>(copies), get<0>(copies));
```

在14中引入的标准类型std::integer_sequence通常被用来表示索引列表。索引列表是一种包含了数值的类型列表，因此可以使用24.3中的Valuelist，上文中的索引列表可以写成`Valuelist\<unsigned, 4, 3, 2, 1, 0\>`。那么如何生成一个索引列表，一种方式是使用一个简单的元函数：

```c++
// recursive case
template<unsigned N, typename Result = Valuelist<unsigned>>
struct MakeIndexListT
: MakeIndexListT<N-1, PushFront<Result, CTValue<unsigned, N-1>>>
{};
// basis case
template<typename Result>
struct MakeIndexListT<0, Result>
{
	using Type = Result;
};
template<unsigned N>
using MakeIndexList = typename MakeIndexListT<N>::Type;
```

现在就可以结合MakeIndexlist和24.2.4节介绍的类型列表的Reverse算法，生成需要的索引列表：

```c++
using MyIndexList = Reverse<MakeIndexList<5>>;
// equivalent to Valuelist<unsigned, 4, 3, 2,1,0>
```

为了实现反转，需要将索引列表中的索引捕获进一个非类型参数包，可以将reverse分成两部分实现：

```c++
template<typename… Elements, unsigned… Indices>
auto reverseImpl(Tuple<Elements…> const& t, Valuelist<unsigned, Indices…>) //11中通过尾置返回类型 -> decltype(makeTuple(get<Indices>(t)…))
{
	return makeTuple(get<Indices>(t)…);
}
template<typename… Elements>
auto reverse(Tuple<Elements…> const& t) //-> decltype(reverseImpl(t, Reverse<MakeIndexList<sizeof… (Elements)>>())
{
	return reverseImpl(t, Reverse<MakeIndexList<sizeof…(Elements)>>());
}
```

还可以实现洗牌和选择算法：

```c++
template<typename… Elements, unsigned… Indices>
auto select(Tuple<Elements…> const& t, Valuelist<unsigned, Indices…>)
{
	return makeTuple(get<Indices>(t)…);
}
```

### 元组的展开

在需要将一组相关的数值存储到一个变量中时，元组会很有用，某些情况下，可能需要展开元组。我们可以创建一个函数模板apply接收一个函数和一个元组作为参数，然后以展开的元组元素作为参数去调用这个参数：

```c++
template<typename F, typename… Elements, unsigned… Indices>
auto applyImpl(F f, Tuple<Elements…> const& t,
Valuelist<unsigned, Indices…>) ->decltype(f(get<Indices>(t)…))
{
	return f(get<Indices>(t)…);
}
template<typename F, typename… Elements, unsigned N = sizeof…(Elements)>
auto apply(F f, Tuple<Elements…> const& t) ->decltype(applyImpl(f, t,
MakeIndexList<N>()))
{
	return applyImpl(f, t, MakeIndexList<N>());
}
```

### 元组的优化

上文实现的元组，其存储方式所需要的空间要比严格意义上需要的多，其中一个问题是tail成员最终会是一个空的数值（因为所有非空的元组都会以一个空的元组作为结束，而任意数据成员总会至少占用一个字节的内存）。为了提高元组的存储效率，可以使用21.1的空基类优化，让元组继承自一个尾元组，而不是将尾元组作为一个成员，比如：

```c++
// recursive case:
template<typename Head, typename… Tail>
class Tuple<Head, Tail…> : private Tuple<Tail…>
{
private:
	Head head;
public:
    Head& getHead() { return head; }
    Head const& getHead() const { return head; }
    Tuple<Tail…>& getTail() { return *this; }
    Tuple<Tail…> const& getTail() const { return *this; }
};
```

这种实现也有问题，颠倒了元组元素在构造函数中被初始化的顺序，现在tail要比head成员早构造，这一问题可以通过将head成员放入自身的基类中，并让这个基类在基类列表中排在tail前面来解决：

```c++
template<typename... Types>
class Tuple;
template<typename T>
class TupleElt
{
T value;
public:
    TupleElt() = default;
    template<typename U>
    TupleElt(U&& other) : value(std::forward<U>(other) { }
    T& get() { return value; }
    T const& get() const { return value; }
};
// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...>
: private TupleElt<Head>, private Tuple<Tail...>
{
public:
    Head& getHead() {
        // potentially ambiguous
        return static_cast<TupleElt<Head> *>(this)->get();
    }
    Head const& getHead() const {
        // potentially ambiguous
        return static_cast<TupleElt<Head> const*>(this)->get();
    }
    Tuple<Tail...>& getTail() { return *this; }
    Tuple<Tail...> const& getTail() const { return *this; }
};
// basis case:
template<>
class Tuple<> {
// no storage required
};
```

但这个方法又引入了一个新的问题：如果一个元组包含两个类型相同的元素，我们将不能从中提取元素，因为此时从Tuple\<int, int\>到TupleElt\<int\>的转换（自派生类到基类）不是唯一的。为了保证Tuple中每一个TupleElt基类都是唯一的，一种方式是将高度信息（也就是tail元组的长度信息）编码进元组，比如最后一个元素的高度会存为0，倒数第一个会存为1：

```c++
template<unsigned Height, typename T>
class TupleElt {
	T value;
public:
    TupleElt() = default;
    template<typename U>
    TupleElt(U&& other) : value(std::forward<U>(other)) { }
    T& get() { return value; }
    T const& get() const { return value; }
};

template<typename... Types>
class Tuple;
// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...>
: private TupleElt<sizeof...(Tail), Head>, private Tuple<Tail...>
{
	using HeadElt = TupleElt<sizeof...(Tail), Head>;
public:
    Head& getHead() {
    	return static_cast<HeadElt *>(this)->get();
    }
    Head const& getHead() const {
    	return static_cast<HeadElt const*>(this)->get();
    }
    Tuple<Tail...>& getTail() { return *this; }
    Tuple<Tail...> const& getTail() const { return *this; }
};
// basis case:
template<>
class Tuple<> {
// no storage required
};
```

接下来是实现一个常数时间的get，其思路为当用一个（基类类型）的参数去适配一个（派生类类型）的参数时，模板参数推导会为基类推断出模板参数的类型，因此如果我们能够计算出目标元素的高度，就不用遍历所有的索引，也能够基于从Tuple的特化结果向TupleElt\<H, T\>的转化提取出相应的元素：

```c++
template<unsigned H, typename T>
T& getHeight(TupleElt<H,T>& te)
{
	return te.get();
}
template<typename... Types>
class Tuple;
template<unsigned I, typename... Elements>
auto get(Tuple<Elements...>& t) ->
decltype(getHeight<sizeof...(Elements)-I-1>(t))
{
	return getHeight<sizeof...(Elements)-I-1>(t);
}
```

查找工作是调用getHeight时的参数推导执行的：由于H是在函数调用时显式指定，因此它的值是确定的，这样就会有一个TupleElt会被匹配到，其目标参数T则是通过推断得到，这里必须要将getHeight声明为Tuple的friend，否则无法执行派生类向private父类的转换，比如：

```c++
// inside the recursive case for class template Tuple:
template<unsigned I, typename… Elements>
friend auto get(Tuple<Elements…>& t)
-> decltype(getHeight<sizeof…(Elements)-I-1>(t));
```

### 元组下标

使用24.3介绍的CTValue，可以将数值索引编码进一个类型，将其用于元组下标运算符定义的代码如下：

```c++
template<typename T, T Index>
auto& operator[](CTValue<T, Index>) {
	return get<Index>(*this);
}

auto t = makeTuple(0, ’1’, 2.2f, std::string{"hello"});
auto a = t[CTValue<unsigned, 2>{}];
auto b = t[CTValue<unsigned, 3>{}]；
```

为了使得常量索引更方便，可以用constexpr实现一种字面常量运算符，专门用来直接从_c结尾的常规字面常量计算出所需的编译期数值字面常量：

```c++
#include "ctvalue.hpp"
#include <cassert>
#include <cstddef>
// convert single char to corresponding int value at compile time:
constexpr int toInt(char c) {
    // hexadecimal letters:
    if (c >= ’A’ && c <= ’F’) {
        return static_cast<int>(c) - static_cast<int>(’A’) + 10;
    }
    if (c >= ’a’ && c <= ’f’) {
   		return static_cast<int>(c) - static_cast<int>(’a’) + 10;
    }
    // other (disable ’.’ for floating-point literals):
    assert(c >= ’0’ && c <= ’9’);
    return static_cast<int>(c) - static_cast<int>(’0’);
}
// parse array of chars to corresponding int value at compile time:
template<std::size_t N>
constexpr int parseInt(char const (&arr)[N]) {
    int base = 10; // to handle base (default: decimal)
    int offset = 0; // to skip prefixes like 0x
    if (N > 2 && arr[0] == ’0’) {
        switch (arr[1]) {
            case ’x’: //prefix 0x or 0X, so hexadecimal
            case ’X’:
                base = 16;
                offset = 2;
                break;
            case ’b’: //prefix 0b or 0B (since C++14), so binary
            case ’B’:
                base = 2;offset = 2;
                break;
            default: //prefix 0, so octal
                base = 8;
                offset = 1;
                break;
    	}
	}
// iterate over all digits and compute resulting value:
    int value = 0;
    int multiplier = 1;
    for (std::size_t i = 0; i < N - offset; ++i) {
        if (arr[N-1-i] != ’\’’) { //ignore separating single quotes (e.g. in 		1’
        000)
            value += toInt(arr[N-1-i]) * multiplier;
            multiplier *= base;
        }
    }
    return value;
}
// literal operator: parse integral literals with suffix _c as sequence of chars:
template<char… cs>
constexpr auto operator"" _c() {
return CTValue<int, parseInt<sizeof…(cs)>({cs…})>{};
}
```

## 可辨识联合

联合对应的类型将包含单个值，该值是从一些可能类型中选择的类型。本章开发了一个类模板Variant，可以动态存储给定的一组值类型的一个值，类似于17中的std::variant<>。比如：

```c++
Variant<int, double, string> field;
```

可以存储整型、双精度或者字符串，但只能存储其中一个值。可以使用成员函数is\<T\>来测试Variant当前是否包含类型为T的值，然后使用成员函数get\<T\>获取存储值。

### 存储

一个Variant需要存储一个值，还需要存储一个辨别器，用来指示哪个是可能的类型：

```c++
#include <new> // for std::launder()

template<typename... Types>
class VariantStorage {
    using LargestT = LargestType<Typelist<Types...>>;
    alignas(Types...) unsigned char buffer[sizeof(LargestT)];
    unsigned char discriminator = 0;
public:
    unsigned char getDiscriminator() const { return discriminator; }
    void setDiscriminator(unsigned char d) { discriminator = d; }
    void* getRawBuffer() { return buffer; }
    const void* getRawBuffer() const { return buffer; }

    template<typename T>
    T* getBufferAs() { return std::launder(reinterpret_cast<T*>(buffer)); }
    template<typename T>
    T const* getBufferAs() const {
    	return std::launder(reinterpret_cast<T const*>(buffer));
    }
};
```

### 设计

与Tuple类型一样，可以使用继承来为Types列表提供每个类型的行为，与Tuple不同的是，这些基类不会存储，每个基类使用21.2中讨论的奇异递归模板模式（CRTP），通过派生最多的类型访问共享变量。下面的类模板提供了在变量的活动值为T类型时，对缓冲区进行操作所需的操作：

```c++
#include "findindexof.hpp"
template<typename T, typename... Types>
class VariantChoice {
    using Derived = Variant<Types...>;
    Derived& getDerived() { return *static_cast<Derived*>(this); }
    Derived const& getDerived() const {
    	return *static_cast<Derived const*>(this);
    }
protected:
 // compute the discriminator to be used for this type
 constexpr static unsigned Discriminator =
 FindIndexOfT<Typelist<Types...>, T>::value + 1;
public:
 VariantChoice() { }
 VariantChoice(T const& value); // see variantchoiceinit.hpp
 VariantChoice(T&& value); // see variantchoiceinit.hpp
 bool destroy(); // see variantchoicedestroy.hpp
 Derived& operator= (T const& value); // see variantchoiceassign.hpp
 Derived& operator= (T&& value); // see variantchoiceassign.hpp
};
    
template<typename List, typename T, unsigned N = 0,
	bool Empty = IsEmpty<List>::value>
struct FindIndexOfT;
// recursive case:
template<typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, false>
   : public IfThenElse<std::is_same<Front<List>, T>::value,
	std::integral_constant<unsigned, N>,
FindIndexOfT<PopFront<List>, T, N+1>>
{
};
// basis case:
template<typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, true>
{
};
```

Variant的框架如下：

```c++
template<typename... Types>
class Variant
: private VariantStorage<Types...>,
private VariantChoice<Types, Types...>...
{
    template<typename T, typename... OtherTypes>
    friend class VariantChoice; // enable CRTP
    ...
};
```

### 值的查询与提取

