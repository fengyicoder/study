# Qt Quick核心编程

## QML语言基础

qml文件的后缀是.qml，其实就是一个文本文件。一个简单的示例如下：

```
import QtQuick 2.2
Rectangle {
	width: 320;
	height: 480;
	Image {
		source: "images/IMG_001.jpg";
		anchors.centerIn: parent;
	}
}
```

上述代码定义了Rectangle跟Image两个对象；

以下是一个有意义的示例：

```
Botton {
	text: "Quit";
	style: ButtonStyle {
		background: Rectangle {
			implicitWidth: 70;
			implicitHeight: 25;
			border.width: control.activeFocus ? 2 : 1;
		}
	}
}
```

这段代码定制了按钮风格，指定Rectangle对象来作为按钮的背景，在按钮有焦点时边框宽度为2，没有焦点时宽度为1。另外，上面的代码使用了control.activeFocus，这说明在表达式中可以引用其他对象及其属性，这样做的时候待赋值的属性和引用的对象的那个属性建立了关联，当被引用属性发生变化时，表达式的值会重新计算，而待赋值的属性也会变化。

每个对象都可以指定一个唯一的id，可以通过这个id来引用其他的对象，有示例如下：

```
Rectangle {
	width: 320;
	height: 480;
	Button {
		id: openFile;
		text: "打开";
		anchors.left: parent.left;
		anchors.leftMargin： 6;
		anchors.top: parent.top;
		anchors.topMargin: 6;
	}
	Button {
		id: quit;
		text: “退出”;
		anchors.left: openFile.right;
		anchors.leftMargin: 4;
		anchors.bottom: openFile.bottom;
	}
}
```



在QML中，注释跟C++一样，单行以//开始，多行以"/*"开始，以"\*/"结束。

QML的属性名的首字母一般以小写开始，采用驼峰命名的方法。可以在QML文档中使用的类型大概有三类：

- 由QML语言本身提供的类型；
- 由QML模块（比如Qt Quick）提供的模型；
- 导出到QML环境中的C++类型；

1. 基本类型：包括Int、real、double、bool、string、color、list、font等；QML中对象的属性有类型安全检查机制；

2. id属性：同一个QML文件中不同对象的id属性的值不能重复，id属性的值首字母必须是小写字母或下划线，不能包含字母、数字及下划线以外的字符；

3. 列表属性：列表属性类似于如下所示，包含在方括号内，以逗号相隔：

   ```
   Item {
   	children: [
   		Image{},
   		Text{}
   	]
   }
   ```

   可以用[value1, value2, ...]来给列表赋值，length属性提供了列表内元素的个数，列表内的元素通过数组下标来访问。另外，列表内只能包含QML对象，不能包含任何基本类型的字面值，如果非要包含，需要使用var变量。有如下的使用示例：

   ```
   Item {
   	children: [
   		Text{
   			text: "textOne";
   		},
   		Text {
   			text: "textTwo";
   		}
   	]
   	Component.onCompleted: {
   		for (var i=0; i<children.length; i++)
   			console.log("text of label ", i, " : ", children[i].text)
   	}
   }
   ```

4. 信号处理器：相当于Qt中的槽，一般是on<Signal>这种形式出现，如下所示：

   ```
   ...
   onClicked: {
   	Qt.quit();
   }
   ```

   当触发了clicked信号的时候，会调用Qt.quit()退出应用。

5. 分组属性：使用一个.符号或者分组符号将相关的属性形成一个逻辑组，类似这样的代码片段，`font.pixelSize: 18;`；

6. 附加属性：附加到一个对象上的额外的属性，示例如下：

   ```c++
   import QtQuick 2.2
   Item {
   	width: 100;
   	height: 100;
   	focus: true;
   	Keys.enable: false;
   }
   ```

也可以自定义属性，语法如下：

```c++
property <propertyType> <propertyName> [ : <value> ]  
```

其类似于成员变量，可以初始化，但没有public或者protect等访问权限设置。当属性定义之后会自动这个属性创建一个属性值改变的信号和相应的信号处理器onxxxChanged，首字母大写；属性名以一个小写字母开头，只能包含字母、数字、下划线，类型可以是QML基本类型，enumeration以int代替，还可以是QML对象类型，var类型是泛型的，支持任何类型的属性值。

## Qt Quick入门

项目的main.cpp文件如下所示：

```c++
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

```

上述代码定义了一个QQmlApplicationEngine实例，代表QML引擎，然后使用了load加载了放在qrc的主QML文档main.qml中，最后启动qt应用的主事件循环。

以下是main.qml文档的示例：

```
import QtQuick 2.2
import QtQuick.Window 2.1
Window {
	visible: true;
	width: 360;
	height: 360;
	MouseArea {
		anchors.fill: parent;
		onClicked: {
			Qt.quit();
		}
	}
	Text {
		text: qsTr("Hello Qt Quick App");
		anchors.centerIn: parent;
	}
}
```

显然，一个qml文档由import语句与QML对象树组成。`import QtQuick.Window 2.1`导入了Window模块，其包含的主要类型就是Window，代表一个QML应用的顶层窗口，对应的C++类型为QQuickWindow。

QML元素的默认属性，通常是用来接收那些没有显式使用"property: value"形式初始化对象的，比如Window类型的默认属性是data，一个list<Object>类型的列表，如果在Window对象中声明了其他对象，又没有赋给某个属性，那这个对象就会被存入data列表中。

除了Window及其派生类外，QML中其他的可见元素大多是Item的派生类，Item的默认属性也是list<Object>类型的data。

启动Qt Quick APP模式有两种：

- QQmlApplicationEngine搭配Window；
- QQuickView搭配Item；

两者不同在于使用QQuickView显示QML文档，对窗口的控制权（比如设置窗口标题、Icon、窗口的最小尺寸等）在C++代码中，而使用QQmlApplicationEngine加载以WIndow为根对象的QML文档，QML文档则拥有窗口的完整控制权，可以直接设置标题窗口尺寸等属性，以下是一个使用QQuickView的例子：

```c++
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQuickView viewer;
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    viewer.setSource(QUrl("qrc:///main.qml"));
    viewer.show();

    return app.exec();
}
```



Window对象可以创建一个新的顶层窗口来作为Qt Quick的活动场景，它在桌面上的位置，由x、y属性决定，其大小由width、height属性决定，除此之外，还可以使用minimumWidth、minimumHeight来限制窗口的最小尺寸，使用maxiumWidth、maxiumHeight来限制窗口的最大尺寸。

作为受窗口管理系统管理的一个窗口，其显示状态有：正常、最小化、最大化、全屏以及隐藏。PC操作系统的窗口管理系统多数支持鼠标拖动改变窗口的大小和位置。可通过Window对象的visibility属性来设置窗口的显示状态，可取以下的值：

- Window.Windowed，窗口占屏幕的一部分，窗口管理系统支持同时显示多窗口时才有效；
- Window.Minimized，最小化到任务栏上的一个图标；
- Window.Maximized，最大化，占用任务栏以外的所有空间，但标题栏依旧显示；
- Window.FullScreen，全屏显示，占用整个屏幕，标题栏隐藏；
- Window.AutomaticVisibility，给Window一个默认的显示状态，它的实际值与平台实现有关；
- Window.Hidden，隐藏，窗口不可见，与visible属性的效果一样；

窗口的显示隐藏由布尔类型的属性visible控制，设置为true显示窗口，为false隐藏窗口；

color属性用来设置窗口的背景颜色，可以用"blue"、"#RRGGBB"、Qt.rgba()等形式为其赋值；opacity属性用来设置窗口透明度，取值范围为0-1.0；title属性用来设置窗口标题，为字符串类型；activeFocusItem属性类型为Item，保存窗口中拥有活动焦点的Item，可能为null。contentOrientation属性用来设置窗口的内容布局方向，可以取以下值：

- Qt.PrimaryOrientation，使用显示设备的首选方向；
- Qt.LandscapeOrientation，横屏；
- Qt.PortraitOrientation，竖屏；
- Qt.InvertedLandscapeOrientation，相对于横屏模式，旋转了180°；
- Qt.InvertedPortraitOrientation，相对于竖屏模式，旋转了180°；

需要注意的是contentOrientation属性设置的内容方向与Android智能手机的横屏、竖屏模式是不同的概念，要想使得应用在Android手机上固定采用横屏或竖屏模式，则需要修改AndroidManifest.xml中的activity元素的android:screenOrientation属性为landscape或protrait。

一个Qt Quick应用可能会有多个窗口，窗口之间的关系由modality（模态）属性决定，一个模态的窗口会阻止其他窗口获取输入事件，modality可以取以下的值：

- Qt.NonModal，非模态
- Qt.WindowModal，窗口级别的模态，设置此属性的窗口只针对另一个窗口是模态的；
- Qt.ApplicationModal，应用级别的模态，设置此属性的窗口会阻止同一应用的其他窗口获取输入；

一个针对Android的Qt Quick应用能否有多个窗口，取决于Android系统的OpenGL是否支持Stencil Buffer（模板缓冲）,如果不支持模板缓冲，使用多窗口时可能会挂掉。

ApplicationWindow是Window的派生类，使用时需要使用如下的语句来引入Controls模块：`import QtQuick.Controls 1.2`。ApplicationWindow有点像QMainWindow，有menuBar、toolBar、statusBar属性，分别来设置菜单、工具栏、状态栏，还有contentItem，可以用来设置内容元素的尺寸属性。



可以简单这么理解，QML相当于C++语言本身，而Qt Quick相当于STL。

Rectangle用来绘制一个填充矩形，其有很多属性，width跟height用来指定宽高，color可以用来指定填充颜色，gradient用来设置渐变色供填充使用，如果同时指定了color跟gradient，那么gradient，那么gradient生效，如果color指定为transparent，那么只绘制边框不填充；border.width用来指定边框的宽度，border.color指定边框颜色，Rectangle还可以绘制圆角矩形，只需要设置radius属性就行了。

以下是一个绘制Rectangle的简单的示例：

```c++
Rectangel {
	width: 320;
	height: 480;
	color: "blue";
	border.color: "#808080";
	border.width: 2;
	radius: 12;
}
```

关于颜色，在QML中可以使用颜色名字，例如blue、red等，也可以使用"#RRGGBB"，还可以使用Qt.rgba()、Qt.lighter()等方法构造；color类型有r、g、b、a四个属性。

渐变色的类型是Gradient，其通过两个或多个颜色值来设置，使用GradientStop来指定一个颜色和它的位置（取值在0.0和1.0之间），以下是一个简单的示例：

```
Rectangle {
	width: 100;
	height: 100;
	gradient: Gradient {
		GradientStop { position: 0.0; color: "#202020";}
		GradientStop { position: 0.33; color: "blue";}
		GradientStop { position: 1.0; color: "#FFFFFF";}
	}
}
```

在Qt5.0中，只有垂直方向的线性渐变可以用于Item，其他的方向可以通过指定rotation来间接实现。

Item是Qt Quick中所有可视元素的基类，其定义了绘制图元所需要的大部分通用属性，比如x、y、width、height、锚定和按键处理。另外，其还有一个z属性，决定其在图层的位置；属性opacity可以指定一个图元的透明度，取值在0.0和1.0之间。以下是一个简单的示例：

```c++
import QtQuick 2.0

Rectangle {
    width: 300;
    height: 200;
    Rectangle {
        x: 20;
        y: 20;
        width: 150;
        height: 100;
        color: "#000080";
        z: 0.5;
    }
    Rectangle {
        x: 100;
        y: 70;
        width: 100;
        height: 100;
        color: "#00c000";
        z: 1;
        opacity: 0.6;
    }
}
```

另外，clip属性用来根据自己的位置和大小来裁剪它自己和孩子们的显示范围，如果为true，则孩子们全部显示在父item内部，为false则有些孩子可能在父Item外面。

虽然Item本身不可见，但可以使用Item来分组其他的可视图元，通过Item的children或visibleChildren属性来访问孩子元素，如下所示：

```c++
...
Item {
	id: gradientGroup;
	Rectangle {
		x: 20;
		y: 20;
		width: 120;
		height: 120;
		gradient: Gradient {
			GradientStop{ position: 0.0; color: "#202020";}
			GradientStop{ position: 1.0; color: "#A0A0A0";}
		}
	}
	.......
}
Component.onCompleted: {
	......
	for (var i=0; i<gradientGroup.children.length; i++) {
		console.log("child", i, "x=", gradientGroup.children[i].x);
	}
}
```

anchors是锚布局，提供了一种方式可以指定一个元素与其他元素的关系来确定元素在界面中的位置。可以进行如下的想象，一个目标可以通过left、right、top、bottom、horizontalCenter、verticalCenter以及baseline确定，如下图所示：

![image-20220503101101323](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220503101101323.png)

上图没有标注基线，基线是定位文本的，对于没有文本的图元，baseline与top一致。

除了对齐锚线，可以指定上下左右四个边的留白，如下图所示：

![image-20220503101306427](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220503101306427.png)

Item通过附加属性Keys来处理按键，以下是一个简单的例子：

```c++
Rectangle {
    width: 300;
    height: 200;
    color: "#c0c0c0";
    focus: true;
    Keys.enabled: true;
    Keys.onEscapePressed: Qt.quit();
    Keys.onBackPressed: Qt.quit();
    Keys.onPressed: {
        switch(event.key){
            case Qt.Key_0:
            case Qt.Key_1:
            case Qt.Key_2:
            case Qt.Key_3:
            case Qt.Key_4:
            case Qt.Key_5:
            case Qt.Key_6:
            case Qt.Key_7:
            case Qt.Key_8:
            case Qt.Key_9:
                event.accepted=true;
                keyView.text = event.key - Qt.Key_0;
                break;
        }
    }
    Text {
        id: keyView;
        font.bold: true;
        font.pixelSize: 24;
        text: qsTr("text");
        anchors.centerIn: parent;
    }
}
```

Text元素可以显示纯文本或者富文本，其有font、text、color、elide、textFormat、wrapMode、horizontalAlignment、verticalAlignment等属性。当不指定textFormat属性时，会自动检测文本是纯文本还是富文本，如果明确是富文本可以显式指定此属性。以下是一个简单的示例：

```c++
Rectangle {
	width: 320;
	height: 200;
	Text {
		width: 150;
		height: 100;
		wrapMode: Text.WordWrap;
        font.bold: true;
        font.pixelSize: 24;
        font.underline: true;
        text: "Hello Blue Text";
        anchors.centerIn: parent;
        color: "blue";
	}
}
```

需要说明的是，如果没有明确指定一个Text对象的宽高，它将自己需要多大的空间，除非设置了wrapMode，否则Text为了适应文本的宽度，width有可能会变的很大，可能超出父Item的边界；如果希望在有限的空间内显示一行长文本，显示不下时省略处理可以设置elide属性。

QML中的Button和QPushButton类似，用户点击会触发一个clicked信号，在QML文档中可以为clicked指定信号处理器，简单的示例如下：

```c++
Rectangle {
    width: 320;
    height: 240;
    color: "gray";
    Button {
        text: "Quit";
        anchors.centerIn: parent;
        onClicked: {
            Qt.quit();
        }
    }
}
```

Bottom有以下的属性，text属性指定按钮文字，checkable属性设置Button是否可选，若可选，checked属性保存选中状态；iconName属性指定图标名字，iconSource通过URL的方式指定icon的位置，iconName的优先级高于iconSource；isDefault属性指定按钮是否为默认，如果是默认的，用户按enter键就会触发；pressed属性保存按钮的按下状态；menu属性允许给按钮设置一个菜单；action属性允许设定按钮的action，action可以定义按钮的checked、text、tooltip、iconSource等属性，还可以绑定clicked信号；activeFocusOnPress属性指定按钮被按下时是否获取焦点，默认是false；stype属性用来定制按钮风格；还可以使用ButtonStype来定义按钮的背景和文本；

ButtonStyle类有background、label与control三个属性，background属性的类型为Component，用来绘制Button的背景，label属性的类型也为Component，用来定制按钮的文本，control属性指向ButtonStyle的按钮对象，可以用其来访问按钮的各个状态，以下是一个简单的示例：

```c++
Rectangle {
    width: 300;
    height: 200;
    Button {
        text: "Quit";
        anchors.centerIn: parent;
        style: ButtonStyle {
            background: Rectangle {
                implicitWidth: 70;
                implicitHeight: 25;
                border.width: control.pressed ? 2 : 1;
                border.color: (control.hovered || control.pressed)
                                ? "green" : "#888888";
            }
        }
        onClicked: Qt.quit();
    }

}
```

如果有多个按钮同时用到一个风格，重复写未免效率太低，此时可以利用Component，示例如下：

```
Component {
	id: btnStyle;
	ButtonStyle {
		......
	}
}
Button {
	...
	style: btnStyle;
}
Button {
	...
	style: btnStyle;
}
```

使用Image来显示图片，只要是Qt支持的，例如jpg、png、bmp、gif、svf等都可以显示，但只能显示静态图片，对于gif等格式只会显示第一帧，如果要显示动画，可以使用AnimatedSprite或者AnimatedImage。

Image也有width与height等属性，若没有设置，则会显示图片本身大小，fillMode属性可以设置图片的填充模式，其支持Image.Stretch（拉伸）、Image.PreserveAspectFit（等比缩放）、Image.PreserveAspectCrop（等比缩放，最大化填充Image，必要时裁剪图片）、Image.Tile（在水平和垂直两个方向平铺，就像贴瓷砖）、Image.TileVertically（垂直平铺）、Image.TileHorizontally（水平平铺）、Image.Pad（保持图片原样不做变换）等模式。

Image默认会阻塞式的加载图片，一旦图片过大，则会非常耗时，此时可以设置asynchronous属性为true来开启异步加载模式，这种模式下会使用一个线程来加载图片。

Image也支持从网络加载图片，它的source属性类型是url，可接受Qt支持的任意一种网络协议，比如http、ftp等，当Image识别到提供的source是网络资源时会自动启动异步加载模式。

BusyIndicator用来显示一个等待图元，在处理一些耗时操作时可以与用户交互。running属性是一个布尔值，为true时显示，style属性允许定制BusyIndicator，以下是一个简单的示例：

```
BusyIndicator {
	id: busy;
	running: true;
	anchors.centerIn； parent;
	z: 2;
}
```

FileDialog是文件对话框，其visible属性的默认值为false，如果要显示对话框，则需要调用open方法或设置此属性为true；selectExisting属性的默认值为true，表示选择已有文件或文件夹，false则用于供用户创建文件或文件夹名字；selectFolder的默认值为false表示选择文件，为true表示选择文件夹；selectMultiple默认为false表示单选，为true表示多选，当selectExisting为false时selectMultiple应该为false；当用户选择了一个文件时，fileUrl保存了该文件的路径，如果选择了多个文件，则fileUrls是一个列表，folder存放的是用户选择的文件夹的位置。

## ECMAScript初探

一个完整的JavaScript实现包含三个不同的部分：

- 核心（ECMAScript）
- 文档对象模型（DOM）
- 浏览器对象模型（BOM）

ECMAScript语言的标准是几家大公司基于JavaScript和JScript锤炼定义出来的，其可以为不同种类的宿主环境提供核心的脚本编程能力，主要描述了以下的内容：

- 语法
- 类型
- 语句
- 关键字
- 保留字
- 运算符
- 对象

作为一种全新的编程语言，QML有三个核心，ECMAScript是其中之一，另外两个是Qt对象系统跟Qt Quick标准库。

ECMAScripte的语法与C++类似，也是区分大小写的，变量没有特定的类型，只用var运算符进行定义，可以初始化为任意的值，可以随时改变变量所存储的数据类型，但其实应该尽量避免，以下是简单的示例：

```c++
var background = "white";
var i = 0;
var children = new Array();
var focus = true;
```

需要注意的是，QML引入的信号是有安全类型检测的，在定义信号时，可以使用特定的类型来定义参数，比如signal colorChanged(color clr)。

变量命名需要遵守两条简单的规则，第一个字符必须是字母、下划线或者美元符号，余下的字符可以是下划线、美元符号或者任何字母或数字，一般采用驼峰命名法。

ECMAScript有以下的关键字列表：break、case、catch、continue、default、delete、do、else、finally、for、function、if、in、instanceof、new、return、switch、this、throw、try、typeof、var、void、while、with；

保留字是留给将来的，可能会作为关键字，另外，保留字也不能用作变量名或函数名，有以下的保留字：abstract、boolean、byte、char、class、const、debugger、double、enum、export、extends、final、float、goto、implements、import、int、interface、long、native、package、private、protected、public、short、static、super、synchronized、throws、transient、volatile；

在ECMAScript中，变量可以存放两种类型的值，即原始值和引用值，当一个变量取值为ECMAScript的5种原始类型时，它就可能被存储在栈上。ECMAScript有5种类型，即Undefined、Null、Boolean、Number与String，ECMAScript提供了typeof运算符来判断一个值的类型，如果这个值是原始类型，则typeof会返回其具体的类型名字，如果这个值是引用值，那么typeof会统一返回object作为类型名字。

Undefined类型只有一个值，即undefined，当变量未初始化时，其默认值就是undefined，当函数没有明确的返回值时，返回的值也是undefined。

Null类型也只有一个值null。

Boolean是ECMAScript最常用的类型之一，有true跟false两个值。

Number类型比较特殊，既可以表示32位的整数，也可以表示64位的浮点数；数字类型的最大值是Number.MAX_VALUE，最小值是Number.MIN_VALUE，但表达式计算生成的数值可以不落在这两个数之间，当计算生成的数值大于Number.MAX_VALUE时，将被赋值为Number.POSITIVE_INFINITY，即正无穷大，反之将被赋值为Number.NEGATIVE_INFINITY，即负无穷大，但其实ECMAScript有专门表示无穷大的值，即Infinity，-Infinity表示负无穷大，它们分别与上述POSITIVE_INFINITY和NEGATIVE_INFINITY对应；使用isFinit可以判断一个数是否是有穷的，还有一个特殊值是NaN，表示非数，通常在类型转换时可能产生NaN，注意NaN非常奇葩，它不等于它自己，因此判断时使用isNaN方法。

String类型作为原始类型存在，其存储Unicode字符，对于的Qt C++类型为QString，当使用C++和QML混合编程时，C++中的QString被映射为String。String类型的变量是只读的，只能访问不能修改。



在ECMAScript中，类型转换非常简单，Boolean、Number、String三种类型都提供了toString方法；注意的是ECMAScript支持按基转换，如果不支持数基，那么不管原来用什么形式声明的Number类型在toString之后都会以10进制输出；

parseInt()跟parseFloat()都可以把非数字的原始值转换为数字，但只能应用于String类型，如果是其他的类型，返回的将是NaN；parseInt和parseFloat都会扫描字符串，直到遇到第一个非数字字符时停止。还有注意以下几点，parseInt也支持基模式，代表浮点数的字符串必须以十进制的形式表示。

ECMAScript也支持强制类型转换，有以下的三种转换：

- Boolean(value)，把value转换成Boolean类型；
- Number(value)，把value转换成数字；
- String(value)，把value转换成字符串；

Number()与parseInt和parseFloat之间的不同在于Number转换的是整个值，而后者只转换到第一个无效字符之前；

String()可以把任何值转换为字符串而不发生错误，包括null或undefined。



对象是由new运算符加上要实例化的类型的名字创建。Object类是所有ECMAScript类中的基类，其具有以下的属性：

- constructor，指向创建对象的函数，对于Object类就指向object函数；
- prototype，对该对象的对象原型的引用，类似于C++中的类声明，不同的是，ECMAScript允许在运行时改变对象原型。
- hasOwnProperty，判断对象是否具有某个属性；
- isPrototypeOf，判断该对象是否为另一个对象的原型；
- propertyIsEnumerable，判断给定属性是否可用for...in语句进行枚举；
- toString，返回对象的字符串表示；
- valueOf，返回最适合该对象的原始值；

在ECMAScript中，对象的属性可以动态的增删，例如`person.year=20;`，对象的方法是一个函数也可以动态的增加，然后按照函数的方式调用，例如:

```c++
person.printInfo = function printInfo() {
	console.log("name -", this.name, "year -", this.year);
}
person.printInfo();
```

甚至可以使用数组下标访问属性和方法：

```c++
console.log(person["name"]);
person["printInfo"]();
```

一般多数自定义属性和方法都是可以枚举的，而内置对象或宿主对象的多数核心属性是不能枚举的，枚举的使用示例如下：

```c++
for(var prop in person) {
	console.log(prop, ",", person[prop]);
}
```

对象的字面值表示方法其实前面已经见过了，就是下面的样子：

```c++
var person = {
	"name": "Jon";
	"year": 20;
}
```

String类是String原始类型的对象表示法，我们可以做以下的事情：

- 字符串长度：使用length属性，例如`console.log(str.length);`；

- 访问单个字符，可以使用下标的方法来进行访问，另外charCodeAt方法可返回指定位置字符对应的Unicode编码，简单的使用如下：

  ```c++
  var str = new String("I\'m a string'");
  console.log(str.charAt(2));
  console.log(str[0]);
  console.log(str.charCodeAt(1));
  ```

- 查找子串，indexOf方法从字符串的开头检索子串，lastIndexOf方法从字符串的结尾开始检索子串，返回给定子串在本对象代表的字符串中的位置，如果找不到则返回-1。search方法检索字符串中指定的子字符串，或检索与正则表达式相匹配的子字符串，其区分大小写，match方法可在字符串内检索指定的值或者寻找匹配指定正则表达式的一个或多个子串，如果检索到了符合规则的子串，它会返回一个存放所有子串的数组。

- 字符串比较，可使用>、<、==三个运算符，但需要注意其只是针对Unicode编码进行比较，localeCompare方法在比较字符串时，默认采用底层操作系统提供的排序规则；

- 连接字符串，可通过concat方法，也可以直接通过+操作，另外，这两种方法连接字符串都不会引入分隔符，如果想要分隔字符串，可以使用Array对象存储字符串，之后调用Array的join方法；

- 提取子串，可以使用slice和substring两个方法，比较特殊的是如果给slice的参数是个负数，比如-3，则会取字符串的后三个字符作为子串，而substring方法会将负数作为0来处理，substr方法也可以提取子串，其第一个参数指定起始位置，第二个参数指定要提取的字符个数；将字符串转小写，可以使用toLowerCase或者toLocaleLowerCase方法，转换大写，可以使用toUpperCase或者toLocaleUpperCase方法；

- 字符串替换，replace方法可将母串中的部分子串替换为指定的新字符串；

- 使用arg()进行值替换，语法如下`string arg(value);`，其中value可以是数字、字符串、布尔值以及对象等，

- 它用来替换发出调用的字符串的%1，%2等占位符，对于对象，使用toString方法转换后的结果来替换对应的占位符，简单的示例如下：

  ```c++
  var expression = "%1 < %2 = %3";
  var result = expresson.arg(7).arg(8).arg(”true“);
  ```

常常用正则表达式来处理字符串，在Qt C++中有QRegExp，在QML中有RegExp，在ECMAScript中支持两种构造正则表达式的方法：

- 字面值语法：/pattern/attributes；
- 创建RegExp对象：new RegExp（pattern和attributes）；

在"/String/i"中，i是修饰符，表示忽略大小写，对应的RegExp对象可以如下构建：`RegExp("String", "i");`或者`new RegExp("String", "i")`，查找语句可以如下写：

```c++
var str = new String("I\'m a string");
str.match(/String/i);
str.search(new RegExp("String", "i"));
str.search(RegExp("String", "i"));
```

在QML中可以使用的修饰符有三个：

- "i"，匹配时忽略大小写；
- "g"，全局匹配；
- "m"，执行多行匹配；

元字符是拥有特殊含义的字符，例如`\d+`，就是指一个或多个数字。常见的元字符如下：

- "."，匹配除换行符以外的任意字符；
- "\w"，匹配字母数字下划线或汉字；
- "\s"，匹配任意的空白符；
- "\S"，匹配非空白字符；
- "\d"，匹配数字；
- "\D"，匹配非数字字符；
- "\b"，匹配单词的开始或结束；
- "^"，匹配字符串的开始；
- "$"，匹配字符串的结束；

经常使用+和*这两个量词来匹配重复，以下是一些说明：

- *，重复零次或多次；
- +，重复一次或多次；
- ？，重复零次或一次；
- {n}，重复n次；
- {n,}，重复n次或更多次；
- {n,m}，重复n到m次；

字符集合举一个例子，[xyz]表示匹配x、y、z中的一个；

Array代表动态数组，其大小可以动态变化，数组中的元素、类型可以不同，创建数组有以下的方法：

```c++
var a = new Array();
var a = new Array(10);
var a = new Array(10, 6, 3, 21, 22, 30, 8);
```

访问数组可通过下标进行访问，修改数组可以使用push向数组尾部插入元素，返回数组新的长度，pop方法删除并返回最后一个元素，shift方法删除并返回数组的第一个元素，unshift向数组的开始添加一个元素并返回新的数组长度，reverse方法颠倒数组中的元素，其会改变原数组，sort可以对数组进行排序并返回对原数组的引用，也会改变原来的数组，可选参数sortby可指定比较大小的函数，若不指定，默认转换成字符串进行比较，结果进行升序排序；join方法可将数组中所有的元素组合成一个字符串，字符串之间可用给定的分隔符来填充，toString可将数组转换成字符串，toLocaleString方法可将数组转换为本地字符串；concat方法可连接两个或多个数组，它不会改变现有数组，而是返回一个新的数组对象；slice(start, end)方法是将start到end之间的元素放到一个新数组中返回，splice(index, howmany, item1,...,itemN)是删除从index开始的howmany个元素，将item1到itemN插入到数组中，如果删除了元素splice会返回被删除元素组成的数组，其会改变原数组；也可使用数组的字面量来表示数组，如`var a = [2, 3, 4, "?", "Quick"]`。

Math对象用来执行数学运算，其有点特殊不必使用new运算符构造对象，可以直接使用，例如：

```c++
var pi = Math.PI;
var textColor = Qt.rgba(Math.random(), Math.random(), Math.random());
```

Date用于创建处理日期，当使用无参构造函数创建Date对象时，该对象会自动把当前日期和时间保存为其初始值，实际上Date对象还可以用下面的方式创建：

```c++
new Date(value);
new Date(dataString);
new Date(year, month, day, hour, minute, second, millisecond);
```

对应于：

```c++
var birthday1 = new Date("2009-10-21T22:24:00");
var birthday2 = new Date(2009, 10, 21);
var birthday3 = new Date(2009, 10, 21, 22, 24, 0);
```

有时候我们或许需要定时操作，可以如下操作：

```c++
var start = Date.now();
var future = Date.UTC(2023,6,1,12);
var distance = future - start;
console.log(distance);
```

需要注意的是，每次创建对象，存储在变量中的都是该对象的引用，而不是对象本身，另外，ECMAScript有垃圾回收机制，因此不需要手动delete，把对象的引用设置为null，就可以解除对对象的引用，下次垃圾收集器运行的时候，就会销毁对象。



ECMAScript不支持函数重载，基本的语法如下：

```c++
function functionName(arg1, arg2, ..., argN)
{ }
```

参数不必指定类型，另外默认都是有返回值的，即便没有显式使用return语句，也会返回undefined。有以下简单的示例：

```c++
function add(number1, number2) {
	var result = number1 + number2;
	console.log(number1, "+", number2, "=", result);
	return result;
}

...
var ret = add(100, 34);
```

ECMAScript中的String也支持"+="运算符，不过其与QString不一样，QString会改变字符串，而String则会生成一个新的对象来保存结果，虽然变量名相同。

void、typeof、instanceof、new、delete都是关键字运算符，其中void比较特殊，用于舍弃表达式的值返回undefined，delete用于删除对象的属性；

对于使用[]来访问某个对象的一个不存在的属性，不会报错，会返回undefined；



console提供了输入日志信息、断言、计时器、计数器、性能分析等功能。可以使用console.log()来输出调试信息，例如：

- console.log()；
- console.debug()；
- console.info()；
- console.warn()；
- console.error()；

比如log，其可以打印字符串、数字、数组以及任意的toString()的对象；

也可以使用console.assert()来进行断言；

也可以使用计时器功能，简单示例如下：

```c++
console.time("regexp");
var str = "We are dogs;\nYour dogs;\nWe want meat.\nPlease.";
var lines = str.match(/^We.*/mg);
console.log(lines.length);
console.log(lines);
console.timeEnd("regexp");
```



ECMAScript的内置对象值得是不需要实例化就可以使用的本地对象，包括Global、Math、JSON。在ECMScript中有个概念：没有独立的函数和属性，例如undefined或null，它们必须从属于某个对象，这个对象就是Global对象；

JSON对象是一个单例对象，包含parse和stringify，parse(text, [, reviver])，text是JSON字符串，可选参数reviver是一个接受两个参数的函数用于过滤和转换结果；stringify(value [, replace [, space]])用于将ECMAScript对象转换为JSON格式的字符串。



QML作为ECMAScript的宿主环境，还提供了一些宿主相关的类型，比如int、bool、real、double、url、list、enumeration等等。url代表资源的位置，可以是一个文件名、相对位置或网络位置，例如：

```c++
Image {
	source: "images/yourimage.png";
}
Image {
	source: "http://www.baidu.com/img/baidu_jgylogo3.gif";
}
Image {
	source: "qrc:/images/yourimage.png";
}
```

当C++与QML混合编程时，QUrl导出到QML会映射为url，反之亦然。

list可以保存对象列表，不能存储原始值；

enumeration代表枚举类型，可使用<Type>.<value>的形式使用；

QML实现了ECMAScript的规范，还提供了宿主对象Qt，有着大量的方法，可查阅资料使用。

## Qt Quick事件处理

要想处理各种事件，离不开信号与槽。

信号处理器就相当于Qt中的槽，其命名一般是以on<Signal>这种形式，例如Button元素有一个名为clicked的信号，那么信号处理器是这样的：

```c++
onClicked: {
	Qt.quit();
}
```

这种情况下的信号是元素本身发出的，也有可能要处理的信号不是当前的元素发出的，而是其他类型，比如处理按键的Keys，这种就是附加信号处理器。

在QML语法中，有附加属性和附加信号处理器的概念，这是附加到一个对象上的额外的属性，也就是说，这是由外在的对象所提供的，例如如下的例子：

```c++
Item {
	width: 100;
	height: 100;
	focus: true;
	Keys.enabled: false;
	Keys.onReturnPressed: console.log("Return key was pressed");
}
```

Item对象可以访问和设置Keys.enabled和Keys.onReturnPressed的值，enabled是Keys的一个属性，onReturnPressed是为Keys对象的returnPressed信号准备的附加信号处理器。

上面的两种处理器都是将响应信号的代码放在元素内部，通过ECMAScript代码块实现，但其实也可以通过Connections来实现。前文所述都是通过on<Signal>这种方式，但其实并不方便，比如：

- 需要将多个对象连接到同一个QML信号上；
- 需要在发出信号的对象的作用域之外来建立连接；
- 发射信号的对象没有在QML中定义；

Connections的用法如下：

```c++
Connections {
	target: area;
	on<Signal>: function or code block;
}
```

以下是一个实际的例子：

```c++
Rectangle {
	width: 320;
	height: 240;
	color: "gray";
    Button {
        id: changeButton;
        ......
    }
	......
	Connections {
		target: changeButton;
		onClicked: {
			text1.color = Qt.rgba(Math.random(),
							Math.random(), Math.random(), 1);
			text2.color = Qt.rgba(Math.random(),
							Math.random(), Math.random(m), 1);
		}
	}
}
```

目前所讨论的信号都是QML已有的信号类型，大致可分为两类，一类由用户输入产生，例如按键、鼠标等，另一类由对象状态或属性变化产生，当属性变化时会自动发出一个信号，形如”<property>Changed“，它对应的信号处理器形如”on<Property>Changed“，但有一些属性变化时不会发射信号，而且有的<property>Changed信号有参数，有的没有。要想直到属性有没有与信号关联，可以先找到QML所对应的C++类型，从其声明中来确认，方法如下：

```
Rectangle {
	width: 320;
	height: 240;
	color: "gray";
	Image {
		id: image1;
	}
	Component.onCompleted: {
		console.log("QML Image\'s C++ type - ", image1);
	}
}
```

console.log会打印出类名信息，比如QML Image对应的C++类型就是QQuickImage，之后根据Qt SDK的安装目录可以找到头文件。

当然，我们也可以定义自己的信号，方式如下：

```c++
signal <name>[([<type><parameter name>[, ...]])]
```

有以下的代码实例：

```c++
Rectangle {
	width: 320;
	height: 240;
	color: "#C0C0C0";
	Text {
		id: coloredText;
		anchors.horizontalCenter: parent.horizontalCenter;
		anchors.top: parent.top;
		anchors.topMargin: 4;
		text: "Hello World";
		font.pixelSize: 32;
	}
	Component {
		id: colorComponent;
		Rectangle {
			id: colorPicker;
			width: 50;
			height: 30;
			signal colorPicked(color clr);
			MouseArea {
				anchors.fill: parent;
				onPressed: colorPicker.colorPicked(colorPicker.color);
			}
		}
	}
	Loader {
		id: redLoader;
		anchors.left: parent.left;
		anchors.leftMargin: 4;
		anchors.bottom: parent.bottom;
		anchors.bottomMargin: 4;
		sourceComponent: colorComponent;
		onLoaded: {
			item.color = "red";
		}
	}
	Loader {
		id: blueLoader;
		anchors.left: redLoader.right;
		anchors.leftMargin: 4;
		anchors.bottom: parent.bottom;
		anchors.bottomMargin: 4;
		sourceComponent: colorComponent;
		onLoader: {
			item.color = "blue";
		}
	}
	Connections {
		target: redLoader.item;
		onColorPicked: {
			coloredText.color = clr;
		}
	}
	Connections {
		target: blueLoader.item;
		conColorPicked: {
			coloredText.color = clr;
		}
	}
}
```

分析代码可知，在组件colorComponent中定义了信号colorPicked，并在鼠标按下时触发， 使用Loader来动态创建组件，并指定sourceComponent，并指定color属性，之后通过Connections来定义信号处理器，即当鼠标按下时处理信号的操作时设置颜色。

除了信号处理器以及使用Connections对象，在QML中还有一种更一般的方式，回想一下在Qt C++中我们通过QObject::connect来使用信号和槽，对应的，在QML中，signal其实是一个对象，其也有connect与disconnect方法。有以下的使用示例：

```
Rectangle {
	id: replay;
	signal messageReceived(string person, string notice);
	Component.onCompleted: {
		relay.messageReceived.connect(sendToPost);
		relay.messageReceived.connect(sendToTelegraph);
		relay.messageReceived.connect(sendToEmail);
		relay.messageReceived("Tom", "Happy Birthday");
	}
	
	function sendToPost(person, notice) {
		console.log("Sending to post: " + person + ", " + notice);
	}
	function sendToTelegraph(person, notice) {
		console.log("Sending to telegraph: " + person + ", " + notice);
	}
	function sendToEmail(person, notice) {
		console.log("Sending to email: " + person + ", " + notice);
	}
}
```

上面是信号连接方法，其实信号也可以连接信号，如下：

```c++
Rectangle {
	id: forwarder;
	width: 100;
	height: 100;
	signal send();
	onSend: console.log("Send clicked");
	MouseArea {
		id: mousearea;
		anchors.fill: parent;
		onClicked: console.log("MouseArea clicked");
	}
	Component.onCompleted: {
		mousearea.clicked.connect(send);
	}
}
```

这里定义了一个信号send，并将MouseArea对象的clicked信号连接到了Rectangle的send信号上。



下面是关于鼠标事件的学习，例程如下：

```c++
Rectangle {
    width: 320;
    height: 240;
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton | Qt.RightButton;
        onClicked: {
            if(mouse.button == Qt.RightButton) {
                Qt.quit();
            }
            else if(mouse.button == Qt.LeftButton) {
                color = Qt.rgba((mouse.x % 255)/255.0,
                                 (mouse.y % 255)/255.0, 0.6, 1.0);
            }
        }
        onDoubleClicked: {
            color = "gray";
        }
    }
}
```

MouseArea对象可以附加到一个Item上供Item处理鼠标事件，其本身是一个不可见的Item，它有很多的属性，enabled用来控制是否处理鼠标事件，默认是true，acceptedButtons属性设定接受哪些鼠标按键产生的按键（左键、右键、中键），作为一个Item，MouseArea也拥有anchor属性，可以使用来描述有效的鼠标区域。在例程中，使用了onClicked和onDoubleClicked两个信号处理器，对应了MouseArea的clicked和doubleClicked信号，clicked信号的参数是MouseEvent类型，名为mouse，可以在信号处理器中直接使用mouse来查询鼠标事件的详情，比如哪个button按下；



以下是一个键盘的事件例程：

```c++
Rectangle {
    width: 320;
    height: 480;
    color: "gray";
    focus: true;
    Keys.enabled: true;
    Keys.onEscapePressed: {
        Qt.quit();
    }
    Keys.forwardTo: [moveText, likeQt];
    Text {
        id: moveText;
        x: 20;
        y:20;
        width: 200;
        height: 30;
        text: "Moving Text";
        color: "blue";
        font: {
            bold: true;
            pixelSize: 24;
        }
        Keys.enabled: true;
        Keys.onPressed: {
            switch(event.key){
            case Qt.Key_Left:
                x -= 10;
                break;
            case Qt.Key_Right:
                x += 10;
                break;
            case Qt.Key_Down:
                y += 10;
                break;
            case Qt.Key_Up:
                y -= 10;
                break;
            default:
                return;
            }
            event.accepted = true;
        }
    }
    CheckBox {
        id: likeQt;
        text: "Like Qt Quick";
        anchors.left: parent.left;
        anchors.leftMargin: 10;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 10;
        z: 1;
    }
}
```

Keys对象是Qt Quick提供的专门供Item处理按键事件的对象，其定义了很多针对特定按键的信号，比如returnPressed、escapePressed、downPressed等等，另外还有更普通的pressed和released信号，它们有一个名字是event、类型是KeyEvent的参数，包含了按键的详细信息；KeyEvent代表了一个按键事件，如果一个按键被处理，event.accepted应被设置为true，以免事件继续传递；Keys有三个属性：enable属性控制是否处理按键；forwardTo属性是列表类型，表示传递按键事件给列表内的对象，如果某个对象accept了某个按键，那么位列其后的对象就不会收到按键事件；priority属性设置Keys附加属性的优先级，有两种，即在Item之前处理按键、在Item之后处理按键。另外，如果想要让某个元素处理按键，则需要给其焦点，即将Item的focus属性设置为true。



在QML中，Timer类代表定时器，其接收的信号为triggered，interval属性指定定时周期，单位为毫秒，默认1000毫秒，repeat设定定时器是周期性触发还是一次性触发，默认一次性触发，running属性设定为true时定时器就开始工作，triggeredOnStart属性设置为true，那么定时器先触发一次再进行定时，除此之外还有start、stop、restart等方法。有以下的例程：

```c++
Rectangle {
    width: 320;
    height: 240;
    color: "gray";
    QtObject {
        id: attrs;
        property int counter;
        Component.objectName: {
            attrs.counter = 10;
        }
    }
    Text {
        id: countShow;
        anchors.centerIn: parent;
        color: "blue";
        font.pixelSize: 40;
    }
    Timer {
        id: countDown;
        interval: 1000;
        repeat: true;
        triggeredOnStart: true;
        onTriggered: {
            countShow.text = attrs.counter;
            attrs.counter -= 1;
            if (attrs.counter < 0)
            {
                countDown.stop();
                countShow.text = "Clap Now!";
            }
        }
    }
    Button {
        id: startButton;
        anchors.top: countShow.bottom;
        anchors.topMargin: 20;
        anchors.horizontalCenter: countShow.horizontalCenter;
        text: "Start";
        onClicked: {
            attrs.counter = 10;
            countDown.start();
        }
    }
}
```



Qt Quick中的PinchArea代表捏拉手势，其是Item的派生类，除了继承自Item的属性外，还有两个专属属性enabled和pinch，其中enabled默认值为true，如果设置为false则pinchArea不起作用，捏拉区域对鼠标、触摸事件就变透明。pinch属性描述捏拉手势的详情，是一个组合属性，包括target、active、minimumScale、maximumScale、minimumRotation、maximumRotaion、dragAxis、minimumX、maximumX、minimumY、maximumY等属性。target指明捏拉手势要操作的Item，active属性表示目标Item是否正在被拖动，minimumScale、maximumScale设置最小、最大缩放系数，minimuxRotation、maximumRotation设置最小最大旋转角度，dragAxis设置沿着x抽、y轴还是xy轴拖动，也可以禁止拖动，只需要给dragAxis赋值Pinch.NoDrag即可。PinchArea有三个信号：PinchStarted()、pinchUpdated()以及pinchFInished，它们都有一个名为pinch的参数，类型是PinchEvent。PinchEvent具有下列属性：

- accepted，在onPinchStarted信号处理器中设置为true，表明要响应pinchEvent，Qt会持续更新事件，设置为false，Qt就不会再发pinchEvent事件；
- angle，表示最近两个触点之间的角度，previousAngle是上一次事件的角度，rotation是从捏拉手势开始到当前事件的总的旋转角度；
- scale，表示最近两个触点之间的缩放系数，previous是上一次事件的缩放系数；
- center，两个触点的中心点，previousCenter是上一次事件的中心点，startCenter是事件开始时的中心点；
- point1、point2保存当前触点的位置，startPoint1、startPoint2保存第二个触点按下时两个触点的位置；
- pointCount保存到现在为止的触点总数。

pinchStarted信号在第一次识别到捏拉手势时发出，如果要处理它就要将pinch参数的accepted属性设置为true。当在onPinchStarted信号处理器中接受了TouchEvent事件后，Qt就会不断的发送新事件，pinchUpdated信号就会不断的发射，可以在信号处理器中通过pinch参数来更新PinchArea寄生的Item的状态；pinchFInished信号在手指离开屏幕时触发。

要使用PinchArea来变换一个Item，有以下的方法：

- 设定target属性，将其指向要变换的Item，然后PinchArea会在合适的时候变换它；
- 处理pinchStarted、pinchUpdated、pinchFinished信号，在信号处理器中变换目标Item，这种方式更灵活，甚至可以同时处理多个Item。

以下是用PinchArea来旋转和缩放一个矩形，第一种情况使用pinch.target：

```c++
Rectangle {
	width: 360;
	height: 360;
	focus: true;
	Rectangle {
		width: 100;
		height: 100;
		color: "blue";
		id: transformRect;
		anchors.centerIn: parent;
	}
	PinchArea {
		anchors.fill: parent;
		pinch.maximumScale: 20;
		pinch.minimumScale: 0.2;
		pinch.minimumRotation: 0;
		pinch.maximumRotation: 90;
		pinch.target: transformRect;
	}
}
```

使用信号的方式如下：

```
Rectangle {
	width: 360;
	height: 360;
	focus: true;
	Rectangle {
		width: 100;
		height: 100;
		color: "blue";
		id: transformRect;
		anchors.centerIn: parent;
	}
	PinchArea {
		anchors.fill: parent;
		pinch.maximumScale: 20;
		pinch.minimumScale: 0.2;
		pinch.minimumRotation: 0;
		pinch.maximumRotation: 90;
		onPinchStarted: {
			pinch.accepted = true;
		}
		onPinchUpdated: {
			transformRect.scale *= pinch.scale;
			transformRect.rotation += pinch.rotation;
		}
		onPinchFinished: {
			transformRect.scale *= pinch.scale;
			transformRect.rotation += pinch.rotation;
		}
	}
}
```

也可使用多点触摸，MultiPointTouchArea本身是一个不可见的Item，可以放在其他可见的Item内来跟踪多点触摸，继承自Item的enabled属性为true时启用触摸处理，为false时禁用，MultiPointTouchArea变透明，mouseEnabled属性控制是否响应鼠标事件，为true时鼠标作为一个触点处理，false时会忽略。minimumTouchPoints、maximumTouchPoints分别设置处理的触点个数，touchPoints时列表属性保存用户定义的用于和Item绑定的触点。有如下的示例：

```c++
Window {
	visible: true;
	width: 360;
	height: 360;
	MultiPointTouchArea {
		anchors.fill: parent;
		touchPoints:[
			TouchPoint{
				id: tp1;
			},
			TouchPoint{
				id: tp2;
			},
			TouchPoint{
				id: tp2;
			}
		]
	}
	Image {
		source: "ic_qt.png";
		x: tp1.x;
		y: tp1.y;
	}
	Image {
		source: "ic_android.png";
		x: tp2.x;
		y: tp2.y;
	}
	Image {
		source: "ic_android_usb.png";
		x: tp3.x;
		y: tp3.y;
	}
}
```

MultiPointTouchArea还有pressed、released、updated、touchUpdated、canceled、gestureStarted这些信号，使用时再查找其使用方法；



## 组件与动态对象

Component（组件）是由Qt框架或开发者封装好的、只暴露了必要接口的QML类型，可以重复利用，其通过属性、信号、函数和外部世界交互。一个组件既可以定义在独立的QML文件中，也可以嵌入到其他的QML文档中，如果一个组件比较小且只在某个QML文档中使用，或者一个组件从逻辑上看从属于某个QML文档，那么就可以采用嵌入的方式来定义该组件，示例如下：

```
Component {
	id: colorComponent;
	Rectangle {
		id: colorPicker;
		width: 50;
		height: 30;
		......
	}
}
```

定义一个Component与定义一个QML文档类似，只能包含一个Item，而且在这个Item之外不能定义任何数据，除了id。Component通常用于给一个view提供图形化组件，比如ListView::delegate属性就需要一个Component来指定如何显示列表的每一个项，又比如ButtonStyle::background属性也需要一个Component来指定如何绘制Button的背景。

Component不是Item的派生类，而是从QQmlComponent继承而来，虽然可以通过其顶层Item为其他的view提供可视组件，但它本身不可见，可以这么理解，定义的组件是一个新的类型，必须被实例化才能显示，而要实例化一个嵌入在QML文档中的组件，可以通过Loader。

下面看如何把一个Component单独定义在一个QML文档中，例如：

```c++
Control {
	id: indicator;
	property bool running: true;
	Accessible.role: Accessible.Indicator;
	Accessible.name: "busy";
	style: Qt.createComponent(Settings.style + 
				"/BusyIndicatorStyle.qml", indicator);
}
```

定义Component时要遵守一个约定，组件名必须和QML文件名一致，另外组件名的第一个字母必须大写，因此，虽然BusyIndicator.qml文件中的顶层Item是Control，但使用时却是以BusyIndicator为组件名。

以下是单独文件中定义的ColorPicker组件：

```
import QtQuick 2.2
Rectangle {
	id: colorPicker;
	width: 50;
	height: 30;
	signal colorPicked(color clr);
	function configureBorder() {
		colorPicker.border.width = colorPicker.focus ? 2 : 0;
		colorPicker.border.color = colorPicker.focus ? "#90D750" : "#808080";
	}
	...
	Component.onCompleted: {
		configureBorder();
	}
}
```

由上面的两个例子可以看出，在单独文件内定义组件，不需要Component对象，Component.onCompleted在组件加载完后调用；使用时直接如下：

```c++
Rectangle {
	width: 320;
	height: 240;
	...
	ColorPicker {
		id: redColor；
		color: "red";
		...
	}
}
```

定义在单独文件中的Component，除了可以像上述那样使用之外，还可以用Loader来动态加载，根据需要创建对象；



使用Loader来动态加载QML组件，可以把Loader作为占位符使用，在需要显示某个元素时再使用Loadder将其加载进来。

Loader可以使用source属性加载一个QML文档，也可以通过sourceComponent属性加载一个Component对象，当Loader的source或sourceComponent属性发生变化时，其之前加载的Component会自动销毁，新对象会被加载，将source设置为空字符串或sourceComponent设置为undefined时，将会销毁当前加载的对象，相关的资源也会释放而Loader对象也变成空对象。Loader的item属性指向它加载的顶层item，可通过其来访问其暴露的接口，如：

```c++
Loader {
	id: redLoader;
	...
	sourceComponent: colorComponent;
	onLoaded: {
		item.color = "red";
	}
}
```

通过sourceComponent来加载组件，通过item属性来选择组件的颜色；对于信号的访问，可以如下使用：

```c++
Connections {
	target: redLoader.item;
	onColorPicked: {
		coloredText.color = clr;
	}
}
```

虽然Loader本身是Item的派生类，但没有加载Component的Loadder对象是不可见的，只是一个占位符，但一旦加载了Component，那么Loader的位置大小会影响Component。以下是处理按键的示例：

```c++
Rectangle {
    width: 320;
    height: 240;
    color: "#EEEEEE";
    Text {
        id: coloredText;
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.top;
        anchors.topMargin: 4;
        text: "Hello World!";
        font.pixelSize: 32;
    }
    Component {
        id: colorComponent;
        Rectangle {
            id: colorPicker;
            width: 50;
            height: 30;
            signal colorPicked(color clr);
            property Item loader;
            border.width: focus ? 2 : 0;
            border.color: focus ? "#90D750" : "#808080";
            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    colorPicker.colorPicked(colorPicker.color);
                    loader.focus = true;
                }
            }
            Keys.onReturnPressed: {
                colorPicker.colorPicked(colorPicked.color);
                event.accepted = true;
            }
            Keys.onSpacePressed: {
                colorPicker.colorPicker(colorPicked.color);
                event.accepted = true;
            }
        }
    }
    Loader {
        id: redLoader;
        width: 80;
        height: 60;
        focus: true;
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        sourceComponent: colorComponent;
        KeyNavigation.right: blueLoader;
        KeyNavigation.tab: blueLoader;
        onLoaded: {
            item.color = "red";
            item.focus = true;
            item.loader = redLoader;
        }
        onFocusChanged: {
            item.focus = focus;
        }
    }
    Loader {
        id: blueLoader;
        anchors.left: redLoader.right;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        sourceComponent: colorComponent;
        KeyNavigation.right: redLoader;
        KeyNavigation.tab: redLoader;
        onLoaded: {
            item.color = "blue";
            item.loader = blueLoader;
        }
        onFocusChanged: {
            item.focus = focus;
        }
    }
    Connections {
        target: redLoader.item;
        onColorPicked: {
            coloredText.color = clr;
        }
    }
    Connections {
        target: blueLoader.item;
        onColorPicked: {
            coloredText.color = clr;
        }
    }
}
```

要想Loader加载的Item能处理按键事件，必须将Loader对象的focus属性设置为true，有因为Loader本身也是焦点敏感的的对象，所以如果item已经处理了按键事件，就应该将事件的accepted属性设置为true，防止事件继续传递。

也可使用Loader加载独立文件的中的Component，只需要设置source属性即可，例如：

```c++
    Loader {
        id: blueLoader;
        anchors.left: redLoader.right;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        source: "ColorPicker.qml";
        KeyNavigation.right: redLoader;
        KeyNavigation.tab: redLoader;
        onLoaded: {
            item.color = "blue";
        }
        onFocusChanged: {
            item.focus = focus;
        }
    }
```

还可以使用Loader动态创建与销毁组件，如下：

```c++
Rectangle {
    width: 320;
    height: 240;
    color: "#EEEEEE";
    id: rootItem
    property var colorPickerShow: false;
    Text {
        id: coloredText;
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.top;
        anchors.topMargin: 4;
        text: "Hello World!";
        font.pixelSize: 32;
    }

    Button {
        id: ctrlButton;
        text: "show";
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        onClicked: {
            if (rootItem.colorPickerShow) {
                redLoader.sourceComponent = undefined;
                blueLoader.source = "";
                rootItem.colorPickerSow = false;
                ctrlButton.text = "Show";
            }
            else {
                redLoader.source = "ColorPicker.qml";
                redLoader.item.colorPicked.connect(onPickedRed);
                blueLoader.source = "ColorPicker.qml";
                blueLoader.item.colorPicked.connect(onPickedBlue);
                redLoader.focus = true;
                rootItem.colorPickerShow = true;
                ctrlButton.text = "Hide";

            }
        }
    }

    Loader {
        id: redLoader;
        anchors.left: ctrlButton.left;
        anchors.leftMargin: 4;
        anchors.bottom: ctrlButton.bottom;
        KeyNavigation.right: blueLoader;
        KeyNavigation.tab: blueLoader;
        onLoaded: {
            if (item != null) {
                item.color = "red";
                item.focus = true;
            }
        }
        onFocusChanged: {
            if (item != null) {
                item.focus = focus;
            }
        }
    }
    Loader {
        id: blueLoader;
        anchors.left: redLoader.left;
        anchors.leftMargin: 4;
        anchors.bottom: redLoader.bottom;
        KeyNavigation.right: redLoader;
        KeyNavigation.tab: redLoader;
        onLoaded: {
            if (item != null) {
                item.color = "blue";
            }
        }
        onFocusChanged: {
            if (item != null) {
                item.focus = focus;
            }
        }
    }
    function onPickedBlue(clr) {
        coloredText.color = clr;
        if(!blueLoader.focus) {
            blueLoader.focus = true;
            redLoader.focus = false
        }
    }

    function onPickedRed(clr) {
        coloredText.color = clr;
        if(!redLoader.focus) {
            redLoader.focus = true;
            blueLoader.focus = false
        }
    }
}
```

创建时设置组件的source，销毁时将其置为undefined。



## Qt Quick元素布局

在Qt Quick中有两套与元素布局相关的类库，一套是Item Position（定位器），一套是Item Layout（布局），当然前面其实还有锚布局。定位器包括Row（行定位器）、Column（列定位器）、Grid（表格定位器）、Flow（流式定位器）；布局管理器包括行布局（RowLayout）、列布局（ColumnLayout）、表格布局（GridLayout）。

定位器是一种容器元素，用于管理界面中的其他元素，需要注意的是，定位器不会改变它管理的元素的大小。

Row沿着一行安排它的孩子，如果使用了Row，就不必使用Item的x、y、anchors等属性。

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    Row {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        spacing: 4;
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
    }
}
```

Row有一个spacing属性，用来指定其管理的item之间的间隔，还有一个layoutDirection属性，可以指定布局方向，取值为Qt.LeftToRight时从左到右放置Item，这是默认行为，取值为Qt.RightToLeft时从右向左放置item；Row还有add、move、populate三个Transition类型的属性，分别指定应用于Item添加，Item移动、定位器初始化创建items三种场景的过渡动画。

Column与Row类似，是在垂直方向上安排它的子Item，它本身也是一个Item，可以使用anchors布局来决定它在父item的位置，spacing属性描述了子Item之间的间隔：

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    Column {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        spacing: 4;
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
    }
}
```

Grid在一个网格上安置子Item，可通过rows和columns来设定表格的行列数，如果不设置，默认只有4列，行数会根据实际的Item自动计算，rowSpacing和columnSpacing指定行列间距，单位是像素。flow属性设置流模式，Grid.LeftToRight是默认值，这种模式从左到右挨着放，取值为Grid.TopToBottom设置从上到下挨着放，horizontalItemAlignment和verticalItemAlignment指定单元格对齐方式。示例如下：

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    Grid {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        rows: 3;
        columns: 3;
        rowSpacing: 4;
        columnSpacing: 4;
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
    }
}
```

Flow与Grid类似，但其没有显式的行列数，它会计算子item的尺寸并与自身尺寸进行比较，按需折行，其flow属性默认为Flow.LeftToRight，sapcing属性描述Item之间的距离；

另外，其实定位器可以嵌套，例如如下代码：

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    Row {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        spacing: 4;
        Column {
            spacing: 4;
            ColorPicker {
                color: Qt.rgba(Math.random(),
                               Math.random(), Math.random(), 1.0);
                onColorPicked: setTextColor(clr);
            }
            ColorPicker {
                color: Qt.rgba(Math.random(),
                               Math.random(), Math.random(), 1.0);
                onColorPicked: setTextColor(clr);
            }
        }

        Column {
            spacing: 4;
            ColorPicker {
                color: Qt.rgba(Math.random(),
                               Math.random(), Math.random(), 1.0);
                onColorPicked: setTextColor(clr);
            }
            ColorPicker {
                color: Qt.rgba(Math.random(),
                               Math.random(), Math.random(), 1.0);
                onColorPicked: setTextColor(clr);
            }
        }
    }
}
```



布局管理器会自动调整item大小来适应尺寸变化；要想使用布局管理器，需要导入Layouts模块，如下`import QtQuick.Layouts 1.1`；

GridLayout是布局管理器中最复杂的对象，与Qt C++中的QGridLayout功能类似，它在一个表格中管理它的item，如果用户调整界面尺寸，GridLayout会自动重新调整item的位置；GridLayout会根据flow属性来排列元素，columns属性指定列数，rows属性指定行数：

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    GridLayout {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        rows: 3;
        columns: 3;
        rowSpacing: 4;
        columnSpacing: 4;
        flow: GridLayout.TopToBottom;
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
    }
}
```

GridLayout所管理的Item，可以使用以下的附加属性：

- Layout.row
- Layout.column
- Layout.rowSpan
- Layout.columnSpan
- Layout.minimumWidth
- Layout.minimumHeight
- Layout.preferedWidth
- Layout.preferedHeight
- Layout.maximumWidth
- Layout.maximumHeight
- Layout.fillWidth
- Layout.fillHeight
- Layout.alignment

RowLayout可以看作是只有一行的GridLayout，其行为与Row类似，不同在于其有以下的附加属性：

- Layout.minimumWidth;
- Layout.minimumHeight;
- Layout.preferredWidth;
- Layout.perferredHeight;
- Layout.maximumWidth;
- Layout.maximumHeight;
- Layout.fillWidth;
- Layout.fillHeight;
- Layout.alignment;

有以下简单的示例：

```c++
Rectangle {
    width: 360;
    height: 240;
    color: "#EEEEEE";
    id: rootItem;
    Text {
        id: centerText;
        text: "A Single Text.";
        anchors.centerIn: parent;
        font.pixelSize: 24;
        font.bold: true;
    }
    function setTextColor(clr) {
        centerText.color = clr;
    }
    RowLayout {
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 4;
        anchors.right: parent.right;
        anchors.rightMargin: 4
        spacing: 4;
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
        }
        ColorPicker {
            color: Qt.rgba(Math.random(),
                           Math.random(), Math.random(), 1.0);
            onColorPicked: setTextColor(clr);
            Layout.fillWidth: true;
        }
    }
}
```

ColumnLayout可以看作只有一列的GridLayout，其行为与Column类似，不同之处其有以下的附加属性:

- Layout.minimumWidth;
- Layout.minimumHeight;
- Layout.preferredWidth;
- Layout.perferredHeight;
- Layout.maximumWidth;
- Layout.maximumHeight;
- Layout.fillWidth;
- Layout.fillHeight;
- Layout.alignment;



## Qt Quick常用元素介绍

TextInput用于编辑一行文本，类似QLineEdit，支持使用validator或inputMask对输入做范围限制，也可以设置echoMode实现密码框的效果；font分组属性允许设置TextInput元素所用字体的各种属性，包括字体族、大小、粗细、斜体、下划线等，还有一些属性与Text元素一样，如text属性可以设置或获取元素的文本，horizontalAlignment和verticalAlignment用于设定文本对齐方式，wrapMode设置文本超过控件宽度时的换行策略，color设置文字颜色，contentWidth、contentHeight返回文本的宽高，不过TextInput不支持使用HTML标记的富文本。length属性返回编辑框中的字符个数，maximumLength设置编辑框允许输入的字符串的最大长度，超过这个长度就会被截断。

TextInput的cursor是光标，可使用cursorDelegate来定制其外观，而QLineEdit的cursor的样子则很难定制，cursorPosition可以设置或返回光标位置，cursorVisible设置或返回光标的可见状态，cursorRectangle是只读属性，返回光标所在矩形，定制的cursorDelegate会受这个属性影响，当cursorRectangle变化时，cursorDelegate的尺寸和位置会跟着变化；

假如使用TextInput来输入密码，可以设置echoMode属性为TextInput.Password、TextInput.PasswordEchoOnEdit或TextInput.NoEcho，而echoMode默认为TextInput.Normal，即输入什么显示什么，如果echoMode不为默认值，则displayText属性就保存显示给用户的文本，而text属性则保存实际的输入；另外TextInput还支持粘贴、撤销、重做等特性；

inputMask是个字符串，用来限制可输入的字符，可以包含允许的字符和分隔符，后面还可以跟一个可选的分号以及可用于补空白的字符；TextInput目前支持IntValidator、DoubleValidator、RegExpValidator，如果设置了validator属性，那么就只能输入符合validator所界定范围的字符，IntValidator可以设置一个整数范围，top、bottom设定最大值最小值，有以下的使用示例：

```c++
TexxtInput {
	width: 120;
	height: 30;
	font.pixelSize: 20;
	anchors.centerIn: parent;
	validator: IntValidator{top: 15; bottom: 6};
	focus: true;
}
```

RegExpValidator则是提供一个正则表达式作为验证器，例如：

```c++
TexxtInput {
	width: 120;
	height: 30;
	font.pixelSize: 20;
	anchors.centerIn: parent;
	validator: RegExpValidator{
		regExp: new RegExp("(2[0-5]{2}|2[0-4][0-9]|1?[0-9]{1,2})
		\\.(2[0-5]{2}|2[0-4][0-9]|1?[0-9]{1,2})
		\\.(2[0-5]{2}|2[0-4][0-9]|1?[0-9]{1,2})
		\\.(2[0-5]{2}|2[0-4][0-9]|1?[0-9]{1,2})");
	}
	inputMask: "000.000.000.000;_";
	focus: true;
}
```

TextInput允许用户选择文本，如果selectByMouse属性设置为true，用户就可以使用鼠标来编辑文本框中的文字，selectedText是只读属性，保存用户选中的文字，selectionStart、selectionEnd表示选中的起止位置，selectedTextColor表示选中的文本颜色，selectionColor表示选中框的颜色；当用户按了回车键或确认键，或编辑框失去焦点时，会发出accepted和editingFinished信号，开发者可以实现onAccepted和onEditingFinished信号处理器来处理，但如果设置了InputMask或validator，那么只有编辑框的文本符合限制条件时这两个条件才会触发。

TextField的基本功能与TextInput类似，其文本颜色使用textColor属性指定，TextInput没有背景，而TextField有背景，其背景可通过TextFieldStyle的background属性来设定，比较TextField与TextInput来比较，其实就多了个背景，可以如下的使用：

```c++
TextField {
	style: TextFieldStyle {
		textColor: "black";
		background: Rectangle {
			radius: 2;
			implicitWidth: 100;
			implicitHeight: 24;
			border.color: "#333";
			border.width: 1;
		}
	}
}
```

另外，TextInput可定制cursor，而TextField不行。



Qt Quick中有两个块编辑控件，TextEdit和TextArea；

TextEdit是Qt Quick提供的多行文本编辑框，其大多属性与TextInput类似，有以下的属性：textDocument属性可以结合QSyntaxHighlighter来实现语法高亮，textFormat用于指定文本格式，时纯文本还是富文本，还是自动检测，默认是TextEdit.PlainText；lineCount属性返回编辑框内文本行数；linkActivated信号，在点击内嵌的链接时触发，linkHovered信号在鼠标悬停在文本内嵌的链接上方时触发；当一行文本超出一行时，wrapMode可决定如何换行，TextEdit.WordWrap代表在单词边界处折行、TextEdit.NoWrap代表不折行超出宽度的文本不显示、TextEdit.WrapAnywhere代表折行不考虑边界、TextEdit.Wrap代表尽量在单词折行处折行；修改文本可以用append、insert、cut、paste、remove等方法，获取文本可以使用getText方法或text属性，另外TextEdit、TextInput、Text等元素的背景都不能定制，如果想要一个背景，可以在对象的底下放一个Z序更小的Rectangle对象。

TextArea与TextEdit类似，其文本颜色使用textColor属性指定，有背景，背景色可通过TextAreaStyle的background属性来设置；TextAreaStyle用来定制TextArea的外观，如字体、背景色等等，使用示例如下：

```c++
TextArea {
	id: textView;
	anchors.fill: parent;
	wrapMode: TextEdit.WordWrap;
	style: TextAreaStyle {
		backgroundColor: "black";
		textColor: "green";
		selectionCOlor: "steelblue";
		selectedTextColor: "#a00000";
	}
}
```

TextArea不可定制cursor，但支持文本滚动，因为它继承自ScrollView。

## C++与QML混合编程

C++与QML对象之间可以相互访问。

QML其实是ECMAScript的扩展，融合了Qt Object系统，Qt提供了两种在QML环境中使用C++对象的方式：

- 在C++中实现一个类，注册为QML环境的一个类型，在QML环境中使用该类型创建对象；
- 在C++中构造一个对象，注册为QML环境的一个属性，在QML环境中使用该属性；

一个类或对象能导出到QML中的前提条件是：

- 从QObject或QObject的派生类继承；
- 使用Q_OBJECT宏；

一旦满足了以上的条件，这个类就可以进入Qt强大的元对象系统，类的某些方法或属性才可以通过字符串形式的名字来调用，具有什么特征的属性或方法可以被QML访问呢？

1. 信号、槽：只要是信号或槽都可以在QML中访问，无论是C++的信号调用QML的方法，或者是QML对象的信号调用C++对象的槽，抑或是C++的信号与槽，以下是ColorMaker类是声明：

   ```c++
   class ColorMaker : public QObject
   {
       Q_OBJECT
   public:
       ColorMaker(QObject *parent = nullptr);
       ~ColorMaker();
   
   signals:
       void colorChanged(const QColor &color);
       void currentTime(const QString &strTime);
   
   public slots:
       void start();
       void stop();
   };
   ```

2. Q_INVOKABLE宏：在类的成员函数前使用Q_INVOKEABLE来修饰，就可以让该方法被元对象系统调用，这个宏必须放在返回类型之前，示例如下：

   ```c++
       Q_INVOKABLE GenerateAlgorithm algorithm() const;
       Q_INVOKABLE void setAlgorithm(GenerateAlgorithm algorithm);
   ```

   使用可直接使用\${Object}.\${method}来访问。

3. Q_ENUMS宏：Q_ENUMS宏可定义在元对象系统中可使用的枚举类型，例如：

   ```c++
       Q_OBJECT
       Q_ENUMS(GenerateAlgorithm)
   public:
       ColorMaker(QObject *parent = nullptr);
       ~ColorMaker();
       
       enum GenerateAlgorithm {
           RandomRGB,
           RandomRed,
           RandomGreen,
           RandomBlue,
           LinearIncrease
       };
   ```

   一旦注册了枚举类型，在QML中可以使用\${CLASS_NAME}.\${ENUM_VALUE}的形式访问，比如ColorMaker.LinearIncrease。

4. Q_PROPERTY宏：用来定义可被元对象系统访问的属性，通过定义属性，可在QML中访问修改，也可在属性变化时发射特定的信号，要想使用这个宏，必须是QObject的后裔。有这样一个简单的示例，`Q_PROPERTY(int x READ x)`，其中type是int类型，name是x，通过READ标记设定通过x()方法可读取属性，其中常用选项分别有以下三个：

   - READ：当没有指定MEMBER标记时则READ标记必不可少，声明一个读取属性的函数，该函数一般无参，返回定义的类型；
   - WRITE：声明一个设定属性的函数，指定的函数无返回值，只有一个与属性类型匹配的参数；
   - NOTIFY：给属性关联一个信号，当属性的值发生改变时就会触发该信号。信号的参数一般是定义的属性；

以下是一个完整的实现：

```c++
class ColorMaker : public QObject
{
    Q_OBJECT
    Q_ENUMS(GenerateAlgorithm)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor timeColor READ timeColor)

public:
    ColorMaker(QObject *parent = nullptr);
    ~ColorMaker();

    enum GenerateAlgorithm {
        RandomRGB,
        RandomRed,
        RandomGreen,
        RandomBlue,
        LinearIncrease
    };

    QColor color() const;
    void setColor(const QColor &color);
    QColor timeColor() const;

    Q_INVOKABLE GenerateAlgorithm algorithm() const;
    Q_INVOKABLE void setAlgorithm(GenerateAlgorithm algorithm);

signals:
    void colorChanged(const QColor &color);
    void currentTime(const QString &strTime);

public slots:
    void start();
    void stop();

protected:
    void timerEvent(QTimerEvent *e);

private:
    GenerateAlgorithm m_algorithm;
    QColor m_currentColor;
    int m_nColorTimer;
};

ColorMaker::ColorMaker(QObject *parent) : QObject(parent),
    m_algorithm(RandomRGB), m_currentColor(Qt::black), m_nColorTimer(0)
{
    qsrand(QDateTime::currentDateTime().toTime_t());
}

ColorMaker::~ColorMaker()
{
}

QColor ColorMaker::color() const
{
    return m_currentColor;
}

void ColorMaker::setColor(const QColor &color)
{
    m_currentColor = color;
    emit colorChanged(m_currentColor);
}

QColor ColorMaker::timeColor() const
{
    QTime time = QTime::currentTime();
    int r = time.hour();
    int g = time.minute()*2;
    int b = time.second()*4;
    return QColor::fromRgb(r, g, b);
}

ColorMaker::GenerateAlgorithm ColorMaker::algorithm() const
{
    return m_algorithm;
}

void ColorMaker::setAlgorithm(GenerateAlgorithm algorithm)
{
    m_algorithm = algorithm;
}

void ColorMaker::start()
{
    if (m_nColorTimer == 0)
    {
        m_nColorTimer = startTimer(1000);
    }
}

void ColorMaker::stop()
{
    if (m_nColorTimer > 0)
    {
        killTimer(m_nColorTimer);
        m_nColorTimer = 0;
    }
}

void ColorMaker::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == m_nColorTimer)
    {
        switch (m_algorithm) {
        case RandomRGB:
            m_currentColor.setRgb(qrand() % 255, qrand() % 255, qrand() % 255);
            break;
        case RandomRed:
            m_currentColor.setRed(qrand() % 255);
            break;
        case RandomGreen:
            m_currentColor.setGreen(qrand() % 255);
            break;
        case RandomBlue:
            m_currentColor.setBlue(qrand() % 255);
            break;
        case LinearIncrease:
            {
                int r = m_currentColor.red() + 10;
                int g = m_currentColor.green() + 10;
                int b = m_currentColor.blue() + 10;
                m_currentColor.setRgb(r % 255, g % 255, b % 255);
            }
            break;
        }
        emit colorChanged(m_currentColor);
        emit currentTime(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    else {
        QObject::timerEvent(e);
    }
}
```

实现一个可供QML访问的类之后，还需要将C++类型注册为QML类型，以及如何使用：

1. 实现C++类；
2. 注册QML类型；
3. 在QML中导入类型；
4. 在QML中创建由C++导出的类型的实例并使用；

下面会分别进行介绍：

1. 注册QML类型：有多种方法，例如使用qmlRegisterSingletonType来注册单例，qmlRegisterType注册非单例，qmlRegisterTypeNotAvailable注册一个类型来占位，qmlRegisterUncreatableType用来注册一个具有附加属性的附加类型，可参考Qt SDK。这里只讨论qmlRegisterType，其有两个原型，如下所示：

   ```c++
   template<typename T>
   int qmlRegisterType(const char *uri, int versionMajor,
   					int versionMinor, const char *qmlName);
   template<typename T, int metaObjectRevision>
   int qmlRegisterType(const char *uri, int versionMajor,
   					int versionMinor, const char *qmlName);
   ```

   前者可以用来注册一个新类型，后者可以为特定的版本注册类型，我们还需要包含QQmlEngine或者QtQml头文件。

   拿`import QtQuick.Controls 1.2`来举例，其中QtQuick.Controls就是包名url，1.1是版本，是versionMajor和versionMinor的组合，qmlName则是QML中可以使用的类名，有以下的示例：

   ```c++
   int main(int argc, char *argv[])
   {
   //    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
   
       QGuiApplication app(argc, argv);
       qmlRegisterType<ColorMaker>("an.qt.ColorMaker", 1, 0, "ColorMaker");
       QQuickView viewer;
       viewer.setResizeMode(QQuickView::SizeRootObjectToView);
       viewer.setSource(QUrl("qrc:///main.qml"));
       viewer.show();
   
   //    QQmlApplicationEngine engine;
   //    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
   //    if (engine.rootObjects().isEmpty())
   //        return -1;
   
       return app.exec();
   }
   ```

   注册动作如前文所述，且要放在QML上下文之前，否则不起作用。

2. 在QML中导入C++注册的类型：一旦注册好了QML类型，就可以在QML文档中引入注册的包然后使用注册的类型，比如这样引入`import an.qt.ColorMaker 1.0`。

3. 在QML中创建C++导入类型的实例：可以如下的方法使用：

   ```c++
       Text {
           id: timeLabel;
           anchors.left: parent.left;
           anchors.leftMargin: 4;
           anchors.top: parent.top;
           anchors.topMargin: 4;
           font.pixelSize: 26;
       }
       ColorMaker {
           id: colorMaker;
           color: Qt.green;
       }
   ```

上文说到还可以将C++类型导入到QML的属性进行使用，需要进行以下的步骤：

1. 注册属性：有如下的示例：

   ```c++
    QGuiApplication app(argc, argv);
       QQuickView viewer;
       viewer.setResizeMode(QQuickView::SizeRootObjectToView);
       viewer.rootContext()->setContextProperty("colorMaker", new ColorMaker);
       viewer.setSource(QUrl("qrc:///main.qml"));
       viewer.show();
   ```

   viewer.rootContext()返回的是QQmlContext对象，其代表一个QML上下文，它的setContextProperty方法将一个在堆上创建的ColorMaker对象设置为一个属性，需要注意的是，ColorMaker对象需要我们自己删除。如果使用了QQmlApplicationEngine+Window的程序结构，main函数应该如下所示：

   ```
   int main(int argc, char *argc[])
   {
   	QGuiApplication app(argc, argv);
   	QQmlApplicationEngine engine;
   	engine.rootContext()->setContextProperty("colorMaker", new ColorMaker);
   	engine.load(QUrl(QStringLiteral("qrc:///main.qml")));
   	return app.exec();
   }
   ```

2. 在QML中使用关联到C++对象的属性：一旦导出了属性，就可以在QML中使用了，不需要import语句，与属性关联的对象，它的信号、槽、可调用的方法（使用Q_INVOKABLE宏修饰）、属性都可以直接使用，只是不能通过类名来引用枚举值了，可使用对象或数字字面量。



上述说明了QML中如何使用C++类型或对象，下面学习如何在C++使用QML对象。Qt最核心的一个特性就是元对象系统，通过元对象系统，可查询QObject的某个派生类的类名、有哪些信号、槽、属性、可调用方法等信息，也可以使用QMetaObject::invokeMethod()调用QObject的某个注册到元对象系统的方法，对于使用Q_PROPERTY定义的属性，可以使用QObject的property方法访问属性，如果属性定义了WRITE方法，还可以使用setProperty修改属性。所以只要找到QML环境中的某个对象，就可以通过元对象系统来访问它的属性、信号、槽等。

QObject类的构造函数有一个parent参数，可以指定一个对象的父亲，借助这样的构造，可以形成以根Item为根的一棵对象树。QObject定义了一个属性objectName，通过这个对象名字属性，调用findChild和findChildren可用于查找对象，其函数原型如下：

```c++
T QObject::findChild(const QString &name = QString(), \
	Qt::FindChildOptions options = \
	Qt::FindChilddrenRecursively) const;
QList<T> QObject::findChildren(const QString &name=\
	QString(), Qt::FindChildOptions options=\
	Qt::FindChildrenRecursively) const;
QList<T> QObject::findChildren(const QRegExp &regExp, \
	Qt::FindChildOptions options=\
	Qt::FindChildrenRecursively) const;
QList<T> QObject::findChildren(const QRegularExpression &re, \
	Qt::FindChildOptions options=\
	Qt::FindChildrenRecursively) const;
```

现在来看下如何查询一个或多个对象，例如`QPushButton *button = parentWidget->findChild<QPushButton*>("button1")`，查找parentWidget名为"button

1"、类型为QPushButton的孩子；对于`QList<QWidget*> widgets = parentWidget->findChildren<QWidget*>("widgetname")`，返回parentWidget的所有名为”widgetname“，返回parentWidget的所有名为"widgetname"的QWidget类型的孩子列表。

如果调用一个对象的信号、槽、可调用方法则是使用QMetaObject的invokeMethod方法，这是一个静态方法，其函数原型如下，由于参数过多所以省略了一些：

```
bool QMetaObject::invokeMethod(QObject *obj,
	const char *member,
	Qt::ConnectionType type,
	QGenericReturnArgument ret,
	......)
```

返回true说明调用成功，返回false则要么是没有那个方法，要么是参数类型不匹配，第一个参数是被调用对象的指针，第二个参数是方法名字，第三个参数是连接类型，如果被调用的对象和发起调用的线程是同一个线程，那么可以使用Qt::DirectConnection、Qt::AutoConnection或者Qt::QueuedConnection，如果被调用对象在另一个线程，可使用Qt::QueuedConnection，第四个参数用来接收返回值，对于要传递给被调用方法的参数，使用QGenericArgument来表示，使用Q_ARG宏来构造一个参数，定义为`QGenericArgument Q_ARG(Type, const Type &value)`，返回类型是类似的，使用QGenericReturnArgument表示，可以使用Q_RETURN_ARG宏来构造一个接收返回值的参数，定义为`QGenericReturnArgument Q_RETURN_ARG(Type, Type &value)`。下面来看看如何使用：

```c++
QString retVal;
QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
					Q_RETURN_ARG(QString, retVal),
					Q_ARG(QString, "sqrt"),
					Q_ARG(int, 42),
					Q_ARG(double, 9.7));
```

如果要让一个线程对象退出，可以如下调用`QMetaObject::invokeMethod(thread, "quit", Qt::QueuedConnection)`；

## Model/View

Model-View-Controller（MVC）源自设计模式，将系统分解为模型、视图、控制器，模型代表数据，通过接口向外提供服务，视图是呈现的可视化界面，控制器是中间人，从模型拉数据给视图，或者模型变换通知视图更新，Qt中的MVC引入了Delegate的概念，即Model-View-Delegate，示意图如下所示：

![image-20220514220510089](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220514220510089.png)

QAbstractItemModel是大多数模型类型的祖先，比如QAbstractListModel、QAbstractProxyModel、QAbstractTableModel、QFileSystemModel、QStringListModel、QStandardItemModel等；QAbstractItemView是大多数视图类的祖先，如QListView、QTableView、QTreeView等；QAbstractItemDelegate则是Qt Model-View框架中所有Delegates的抽象基类，其又延申出QStyleItemDelegate和QItemDelegate两个分支，如果要实现自己的Delegate，可以从这两个类中选择一个作为基类，Qt推荐使用QStyleItemDelegate。

ListView用来显示一个条目列表，条目对于的数据来自Model，而外观则由Delegate决定，Model可以是QML内建类型，如ListModel、XmlListModel，也可以是C++中实现的QAbstractItemModel或QAbtractListModel的派生类，示例如下：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneDelegate;
        Item {
            id: wrapper;
            width: parent.width;
            height: 30;
            MouseArea {
                anchors.fill: parent;
                onClicked: wrapper.ListView.view.currentIndex = index;
            }
            RowLayout {
                anchors.left: parent.left;
                anchors.verticalCenter: parent.verticalCenter;
                spacing: 8;
                Text {
                    id: coll;
                    text: name;
                    color: wrapper.ListView.isCurrentItem ? "red" : "black";
                    font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                    Layout.preferredWidth: 120;
                }
                Text {
                    text: cost;
                    color: wrapper.ListView.isCurrentItem ? "red" : "black";
                    font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                    Layout.preferredWidth: 80;
                }
                Text {
                    text: manufacturer;
                    color: wrapper.ListView.isCurrentItem ? "red" : "black";
                    font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                    Layout.fillWidth: true;
                }
            }

        }
    }
    ListView {
        id: listView;
        anchors.fill: parent;
        delegate: phoneDelegate;
        model: ListModel {
            id: phoneModel;
            ListElement {
                name: "iPhone 3GS";
                cost: "1000";
                manufacturer: "Apple";
            }
            ListElement {
                name: "iPhone 4";
                cost: "1800";
                manufacturer: "Apple";
            }
            ListElement {
                name: "iPhone 4S";
                cost: "2300";
                manufacturer: "Apple";
            }
            ListElement {
                name: "iPhone 5";
                cost: "4900";
                manufacturer: "Apple";
            }
            ListElement {
                name: "B199";
                cost: "1590";
                manufacturer: "HuaWei";
            }
            ListElement {
                name: "MI 2S";
                cost: "1999";
                manufacturer: "XiaoMi";
            }
            ListElement {
                name: "GALAXY S5";
                cost: "4699";
                manufacturer: "Samsung";
            }
        }
        focus: true;
        highlight: Rectangle{
            color: "lightblue";
        }
    }
}
```

上述代码中model属性初始化了一个ListModel，ListModel是专门定义列表数据的，其内部维护一个ListElement列表，一个ListElement对象就代表一条数据，共同构成一个ListElement的一个或多个信息被称为role，其形式是键值对；在ListElement定义的role可在Delegate中通过role-name来访问，而Delegate则使用Row管理三个Text对象来展现这三个role，Text对象的text属性被绑定在role-name上；ListView的delegate属性类型是Component，其顶层元素是Row，Row内嵌了三个Text对象，ListView给delegate暴露了一个index属性，代表当前delegate实例对应的Item的索引位置，可通过其来访问数据；ListView还定义了delayRemove、isCurrentItem、nextSection、previousSection、section、view等附加属性以及add、remove两个附加信号，可以在delegate中直接访问，但只有delegate的顶层Item才可以直接使用这些附加属性和信号，非顶层Item要通过顶层的id来访问这些附加属性。

通过header属性设置一个Component，ListView可以显示自定义的表头，表头将放在ListView的最开始，所有的Item之前，当使用方向键浏览Item或用鼠标在ListView内拖动时表头随着拖动可能会变得不可见，以下是示例：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneModel;
        ListModel {
                ListElement {
                    name: "iPhone 3GS";
                    cost: "1000";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4";
                    cost: "1800";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4S";
                    cost: "2300";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 5";
                    cost: "4900";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "B199";
                    cost: "1590";
                    manufacturer: "HuaWei";
                }
                ListElement {
                    name: "MI 2S";
                    cost: "1999";
                    manufacturer: "XiaoMi";
                }
                ListElement {
                    name: "GALAXY S5";
                    cost: "4699";
                    manufacturer: "Samsung";
                }
            }
        }
        Component {
            id: headerView;
            Item {
                width: parent.width;
                height: 30;
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: "name";
                        font.pixelSize: 20;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: "cost";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: "manufacturer";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        Component {
            id: phoneDelegate;
            Item {
                id: wrapper;
                width: parent.width;
                height: 30;
                MouseArea {
                    anchors.fill: parent;
                    onClicked: wrapper.ListView.view.currentIndex = index;
                }
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: name;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: cost;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: manufacturer;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        ListView {
            id: listView;
            anchors.fill: parent;
            delegate: phoneDelegate;
            model: phoneModel.createObject(listView);
            header: headerView;
            focus: true;
            highlight: Rectangle {
                color: "lightblue";
            }
        }

}
```

footer属性允许指定ListView的页脚，footerItem保存了footer组件创建出来的Item对象，这个对象会被添加进ListView的末尾，在所有可见的Item之后，示例如下：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneModel;
        ListModel {
                ListElement {
                    name: "iPhone 3GS";
                    cost: "1000";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4";
                    cost: "1800";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4S";
                    cost: "2300";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 5";
                    cost: "4900";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "B199";
                    cost: "1590";
                    manufacturer: "HuaWei";
                }
                ListElement {
                    name: "MI 2S";
                    cost: "1999";
                    manufacturer: "XiaoMi";
                }
                ListElement {
                    name: "GALAXY S5";
                    cost: "4699";
                    manufacturer: "Samsung";
                }
            }
        }
        Component {
            id: headerView;
            Item {
                width: parent.width;
                height: 30;
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: "name";
                        font.pixelSize: 20;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: "cost";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: "manufacturer";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        Component {
            id: footerView;
            Text {
                width: parent.width;
                font.italic: true;
                color: "blue";
                height: 30;
                verticalAlignment: Text.AlignVCenter;
            }
        }

        Component {
            id: phoneDelegate;
            Item {
                id: wrapper;
                width: parent.width;
                height: 30;
                MouseArea {
                    anchors.fill: parent;
                    onClicked: wrapper.ListView.view.currentIndex = index;
                }
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: name;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: cost;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: manufacturer;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        ListView {
            id: listView;
            anchors.fill: parent;
            delegate: phoneDelegate;
            model: phoneModel.createObject(listView);
            header: headerView;
            footer: footerView;
            focus: true;
            highlight: Rectangle {
                color: "lightblue";
            }
            onCurrentIndexChanged: {
                if (listView.currentIndex >= 0) {
                    var data = listView.model.get(listView.currentIndex);
                    listView.footerItem.text = data.name + " ," + data.cost + " , " + data.manufacturer;
                } else {
                    listView.footerItem.text = " ";
                }
            }
        }

}

```



如果想要访问数据，ListModel的count属性表示Model中有多少条数据，int类型，dynamicRoles属性为布尔值，为true时表示Model中的role对应的值的类型可以动态改变，默认是false，要设置此属性必须要在添加数据之前，而且一旦使能了此属性，ListModel的性能会大大下降；ListModel的get方法接受一个int型参数，用来获取指定索引位置的数据，返回一个QML对象，然后可以想使用属性那样来访问数据。

如果想要删除一条或多条数据，可使用ListModel的remove(int index, int count)方法，第一个参数指明要删除的数据的索引位置，第二个参数表示要删除的数据条数，默认值为1；如果想要清空一个Model，可以调用clear方法。简单的使用示例如下：

```c++
MouseArea {
                    anchors.fill: parent;
                    onClicked: wrapper.ListView.view.currentIndex = index;
                    onDoubleClicked: wrapper.ListView.view.model.remove(index);
                }
```

如果想要修改Model的数据，可使用setProperty(int index, string property, variant value)方法，第一个参数是数据的索引，第二个是数据内role的名字，第三个是role的值，比如`listView.model.setProperty(5, "cost", 16999)；如果想要替换某一条数据，可以使用`set(int index, jsobject dict)方法，比如`listView.model.set(0, {"name": "Z5S mini", "cost":1999, "manufacturer": "Zhongxing"})`；

要向Model的尾部添加数据，可以使用append方法，其参数是jsobject；指定位置添加数据可以使用Insert方法，第一个参数是整型代表插入位置，第二个参数是jsobject，使用示例如下：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneModel;
        ListModel {
                ListElement {
                    name: "iPhone 3GS";
                    cost: "1000";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4";
                    cost: "1800";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4S";
                    cost: "2300";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 5";
                    cost: "4900";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "B199";
                    cost: "1590";
                    manufacturer: "HuaWei";
                }
                ListElement {
                    name: "MI 2S";
                    cost: "1999";
                    manufacturer: "XiaoMi";
                }
                ListElement {
                    name: "GALAXY S5";
                    cost: "4699";
                    manufacturer: "Samsung";
                }
            }
        }
        Component {
            id: headerView;
            Item {
                width: parent.width;
                height: 30;
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        text: "Name";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: "Cost";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: "Manufacturer";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        Component {
            id: footerView;
            Item {
                id: footerRootItem;
                width: parent.width;
                height: 30;
                property alias text: txt.text;
                signal clean();
                signal add();
                Text {
                    anchors.left: parent.left;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    id: txt;
                    font.italic: true;
                    color: "blue";
                    verticalAlignment: Text.AlignVCenter;
                }
                Button {
                    id: clearAll;
                    anchors.right: parent.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    text: "Clear";
                    onClicked: footerRootItem.clean();
                }
                Button {
                    id: addOne;
                    anchors.right: clearAll.left;
                    anchors.rightMargin: 4;
                    anchors.verticalCenter: parent.verticalCenter;
                    text: "Add";
                    onClicked: footerRootItem.add();
                }
            }
        }

        Component {
            id: phoneDelegate;
            Item {
                id: wrapper;
                width: parent.width;
                height: 30;
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        wrapper.ListView.view.currentIndex = index;
                        mouse.accepted = true;
                    }
                    onDoubleClicked:{
                        wrapper.ListView.view.model.remove(index);
                        mouse.accepted = true;
                    }
                }

                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: name;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: cost;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: manufacturer;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        ListView {
            id: listView;
            anchors.fill: parent;
            delegate: phoneDelegate;
            model: phoneModel.createObject(listView);
            header: headerView;
            footer: footerView;
            focus: true;
            highlight: Rectangle {
                color: "lightblue";
            }
            onCurrentIndexChanged: {
                if (listView.currentIndex >= 0) {
                    var data = listView.model.get(listView.currentIndex);
                    listView.footerItem.text = data.name + " ," + data.cost + " , " + data.manufacturer;
                } else {
                    listView.footerItem.text = " ";
                }
            }
            function addOne() {
                model.append({
                                "name": "MX3",
                                 "cost": "1799",
                                 "manufacturer": "MeiZu"
                             });
            }

            Component.objectName: {
                listView.footerItem.clean.connect(listView.model.clear);
                listView.footerItem.add.connect(listView.addOne);
            }
        }
}
```

ListView也提供了add、remove、move、populate、displaced几种场景的过渡动画效果，这些场景对应的属性类型都是Transition。

add属性指定向ListView新增一个Item时针对Item应用的过渡动画，但ListView第一次初始化或者Model发生变化导致的Item创建过程不会触发add过渡动画，而是应用populate动画，另外注意尽量不要在add动画中改变Item高度，否则会引起Item重新布局，使用示例如下：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneModel;
        ListModel {
                ListElement {
                    name: "iPhone 3GS";
                    cost: "1000";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4";
                    cost: "1800";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4S";
                    cost: "2300";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 5";
                    cost: "4900";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "B199";
                    cost: "1590";
                    manufacturer: "HuaWei";
                }
                ListElement {
                    name: "MI 2S";
                    cost: "1999";
                    manufacturer: "XiaoMi";
                }
                ListElement {
                    name: "GALAXY S5";
                    cost: "4699";
                    manufacturer: "Samsung";
                }
            }
        }
        Component {
            id: headerView;
            Item {
                width: parent.width;
                height: 30;
                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        text: "Name";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: "Cost";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: "Manufacturer";
                        font.bold: true;
                        font.pixelSize: 20;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        Component {
            id: footerView;
            Item {
                id: footerRootItem;
                width: parent.width;
                height: 30;
                signal add();
                signal insert();
                Button {
                    id: addOne;
                    anchors.right: parent.right;
                    anchors.rightMargin: 4;
                    anchors.verticalCenter: parent.verticalCenter;
                    text: "Add";
                    onClicked: footerRootItem.add();
                }
                Button {
                    id: insertOne;
                    anchors.right: addOne.left;
                    anchors.rightMargin: 4;
                    anchors.verticalCenter: parent.verticalCenter;
                    text: "Insert";
                    onClicked: footerRootItem.insert();
                }
            }
        }

        Component {
            id: phoneDelegate;
            Item {
                id: wrapper;
                width: parent.width;
                height: 30;
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        wrapper.ListView.view.currentIndex = index;
                        mouse.accepted = true;
                    }
                    onDoubleClicked:{
                        wrapper.ListView.view.model.remove(index);
                        mouse.accepted = true;
                    }
                }

                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: name;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: cost;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: manufacturer;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        ListView {
            id: listView;
            anchors.fill: parent;
            delegate: phoneDelegate;
            model: phoneModel.createObject(listView);
            header: headerView;
            footer: footerView;
            focus: true;
            highlight: Rectangle {
                color: "lightblue";
            }
            add: Transition {
                ParallelAnimation {
                    NumberAnimation {
                        property: "opacity";
                        from: 0;
                        to: 1.0;
                        duration: 1000;
                    }
                    NumberAnimation {
                        property: "y";
                        from: 0;
                        duration: 1000;
                    }
                }
            }
            function addOne() {
                model.append({
                                "name": "MX3",
                                 "cost": "1799",
                                 "manufacturer": "MeiZu"
                             });
            }
            function insertOne() {
                model.insert(Math.round(Math.random() * model.count), {
                                "name": "HTC One E8",
                                 "cost": "2999",
                                 "manufacturer": "HTC"
                             });
            }

            Component.onCompleted: {
                listView.footerItem.add.connect(listView.addOne);
                listView.footerItem.insert.connect(listView.insertOne);
            }
        }
}
```

displaced属性用于指定通用的、由于Model变化导致Item移位时的动画效果，而相应的addDisplaced、moveDisplaced、removeDisplaced则用于指定由特定的add、move、remove操作引起的移位动画，如果同时指定displaced和xxxDisplaced，那么后者生效；

remove属性指定将一个Item从ListView中移除时应用的过渡动画，当动画执行时，Item已经移除，此时对Item的任何操作都是非法的，简单的应用如下：

```
remove: Transition {
	SequentialAnimation {
		properties: "y";
		to: 0;
		duration: 600;
	}
	NumberAnimation {
		property: "opacity";
		to: 0;
		duration: 400;
	}
}
```

move属性指定一个Item移动时应用的过渡动画；populate属性指定一个过渡动画，在ListView第一次实例化或者因Mode变化而需要创建Item时应用。

ListView的section复合属性可以用来描述分组的规则以及显示策略，section.property属性指定分组的依据，对应role-name，例如`section.property:"manufacturer"`；section.criteria指定section.property的判断条件，有ViewSection.FullString和ViewSection.FirstCharacter两种，即全串匹配和首字母匹配，匹配时不区分大小写；section.delegate设定一个Component，决定如何显示每个section；section.labelPositioning决定当前或下一个section标签的显示策略，可以是下列枚举值中的一个：

- ViewSection.InlineLabels，默认方式，分组标签嵌入到Item之间显示；
- ViewSection.CurrentLabelAtStart，当view移动时，当前分组的标签附着在view的开始；
- ViewSection.NextLabelAtEnd，当view移动时，下一个分组标签附着在view的尾端。

ListView内的每个Item都有一些与section相关的附加属性，名字分别是ListView.section、ListVIew.previousSection、ListView.nextSection，需要注意的是对ListView分组并不会引起ListView自动按分组来整理Item的顺序，示例如下：

```c++
Rectangle {
    width: 360;
    height: 300;
    color: "#EEEEEE";
    Component {
        id: phoneModel;
        ListModel {
                ListElement {
                    name: "iPhone 3GS";
                    cost: "1000";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4";
                    cost: "1800";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 4S";
                    cost: "2300";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "iPhone 5";
                    cost: "4900";
                    manufacturer: "Apple";
                }
                ListElement {
                    name: "B199";
                    cost: "1590";
                    manufacturer: "HuaWei";
                }
                ListElement {
                    name: "MI 2S";
                    cost: "1999";
                    manufacturer: "XiaoMi";
                }
                ListElement {
                    name: "GALAXY S5";
                    cost: "4699";
                    manufacturer: "Samsung";
                }
            }
        }
        Component {
            id: phoneDelegate;
            Item {
                id: wrapper;
                width: parent.width;
                height: 30;
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        wrapper.ListView.view.currentIndex = index;
                        mouse.accepted = true;
                    }
                    onDoubleClicked:{
                        wrapper.ListView.view.model.remove(index);
                        mouse.accepted = true;
                    }
                }

                RowLayout {
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    spacing: 8;
                    Text {
                        id: coll;
                        text: name;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 120;
                    }
                    Text {
                        text: cost;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.preferredWidth: 80;
                    }
                    Text {
                        text: manufacturer;
                        color: wrapper.ListView.isCurrentItem ? "red" : "black";
                        font.pixelSize: wrapper.ListView.isCurrentItem ? 22 : 18;
                        Layout.fillWidth: true;
                    }
                }

            }
        }
        Component {
            id: sectionHeader;
            Rectangle {
                width: parent.width;
                height: childrenRect.height;
                color: "lightsteelblue";
                Text {
                    text: section;
                    font.bold: true;
                    font.pixelSize: 20;
                }
            }
        }

        ListView {
            id: listView;
            anchors.fill: parent;
            delegate: phoneDelegate;
            model: phoneModel.createObject(listView);
            focus: true;
            highlight: Rectangle {
                color: "lightblue";
            }
            section.property: "manufacturer";
            section.criteria: ViewSection.FullString;
            section.delegate: sectionHeader;
        }
}
```



GridView与ListView类似，最大的不同在于Item的呈现方式，可以将ListView对应QListView的ListMode显示模式，GridView对应IconMode显示模式，GridView有一个flow属性，指定Item的流模式，有从左到右（GridView.LeftToRight）和从上到下（GridView.TopToBottom）两种模式；与ListView类似，只有提供一个Model一个delegate就可以正常使用GridView了。示例如下：

```
GradView {
	id: videoView;
	anchors.fill: parent;
	cellWidth: 120;
	cellHeight: 190;
	delegate: videoDelegate;
	model: videoModel.createObject(videoView);
	focus: true;
	hightlight: Rectangle {
		height: videoView.cellHeight - 8;
		color: "lightblue";
	}
}
```

如果默认的单元格大小不能满足需求，可以设置cellWidth、cellHeight两个属性来指定单元格大小；另外有一个与方向键有关的属性KeyNavigationWraps，当为true时，可使用左右按键浏览Item，连续按右键抵达列表末尾，再按右键跳转到列表开始，左键与其相反，上下键只在列内循环，当为false，焦点到达末尾时不进行跳转。