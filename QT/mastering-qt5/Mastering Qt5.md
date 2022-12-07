# Mastering Qt5

## 前言

C++是一种强大的语言。与Qt相结合，您将拥有一个兼顾性能和易用性的跨平台的框架。Qt是一个非常庞大的框架，在许多领域（如GUI、线程、网络等方面）都提供了一些工具。在Qt诞生后的25年里，它随着每个版本的发布而发展和壮大。

本书旨在指导您如何通过C++14一些新的特性（lambda、智能指针、枚举类等）来充分利用Qt的优点。这两种技术协同，为您带来一个安全而强大的开发工具箱。在整本书中，我们试图强调一个干净的架构，它可以让您在复杂的环境中创建和维护您的应用程序。

每个章节都基于一个项目示例，该项目是所有讨论的基础，以下是我们会在本书中看到的一些新的内容：

- 揭开qmake的秘密；
- 深入了解model/view架构，学习如何使用这种模式来构建复杂的应用程序；
- 学习移动设备中的QML和Qt quick应用程序；
- 使用QML和JavaScript来开发Qt 3D组件；
- 展示如何使用Qt来开发插件和SDK；
- 涵盖了Qt中的多线程技术；
- 使用sockets来构建IPC机制；
- 使用XML、JSON和二进制格式序列化数据；

我们将涵盖这所有的一切甚至更多。

请注意，如果您想要获得一些开发技巧或者查看一些能使你开发更愉快的代码片段，你可以随时查阅第14章，Qt建议和诀窍。

最为重要的是，享受编写Qt应用程序的乐趣。

### 这本书的内容

第一章，*Get Your Qt Feet Wet*，奠定Qt的基础，使用todo程序来刷新您的记忆。本章介绍了Qt项目的结构，如何使用设计器，信号和槽机制的基本原理，并介绍了C++14的新特性；

第二章，*Discovering QMake Secrets*，深入了解Qt编译系统的核心：qmake。本章介绍了qmake的工作原理、如何使用以及如何通过设计一个系统监控应用程序来使用特定平台的代码来构建一个Qt应用程序；

第三章，*Dividing Your Project and Ruling Your Code*，分析Qt model/view架构，学习如何通过开发具有应用程序核心逻辑的自定义库来组织项目。项目示例是一个持久的gallery application；

第四章，*Conquering the Desktop UI*，学习model/view架构的UI透视图，该架构使用Qt Widget程序，依赖上一章节完成的库；

第五章，*Domination the Mobile UI*，添加gallery application的移动版本的缺失部分。本章节涵盖了QML使用,Qt Quick控件以及QML/C++的使用；

第六章，*Even Qt Deserves a Slice of Raspberry Pi*，继续以Qt 3D的视角来走上Qt Quick程序的开发之路。本章涵盖了如何构建针对树莓派的3D贪吃蛇游戏；

第七章，*Third-Party Libraries Without a Headache*，涵盖如何将第三方库集成到Qt项目中。OpenCV将和一个图像过滤的程序集成在一起，该程序还会提供一个定制的QDesigner插件；

第八章，*Animations, It’s Alive, Alive*，通过添加动画和分发自定义的SDK来让开发者添加自己的过滤器的能力来扩展图像过滤器应用程序；

第九章，*Keeping Your Sanity with Multithreading*，通过构建多线程Mandelbrot分形绘图程序来研究Qt提供的多线程工具；

第十章，*Need IPC*，通过将计算转移到其他进程并使用套接字来管理通信，扩展Mandelbrot分形绘图程序，使得以最小代价来工作；

第十一章，*Having Fun with Serialization*，在drum machine application中涵盖了多种序列化格式（JSON、XML和二进制），可以录制和加载声音循环；

第十二章，*You Shall (Not) Pass with QTest*，向drum machine application中添加测试，并研究如何使用Qt测试框架进行单元测试、基准测试和GUI事件模拟；

第十三章，*All Packet and Ready to Deploy*，深入了解如何在所有的桌面操作系统（Windows，Linux和Mac）打包应用程序；

第十三章，*Qt Hat Tips and Tricks*，收集了一些使用Qt开发的技巧和窍门。展示了如何在Qt Creator中管理会话，一些有用的Qt Creator快捷键，如何自定义log并将其保存到磁盘等等；

## 第一章 Get Your Qt Feet Wet

如果你了解C++但从未接触过Qt，或者如果你做过了一些中级的Qt程序，本章都会保证你的Qt基础是安全的，以便能够学习后续章节的高级概念。

我们将将你如何使用Qt Creator来创建一个简单的todo程序，这个程序会展示你可以创建/更新/删除的一些列任务。我们将介绍Qt Creator和Qt Designer接口，介绍信号/槽机制，使用自定义信号/槽来创建自定义的小部件以及将其集成到程序中。

你在实现todo app中将会使用C++14中这些新的语义：lambda，自动变量以及for循环。这些概念中的每一个都会被深入讲解并在整本书中使用。

在本章结束后，你将能够使用Qt widgets和新的C++语义来创建具有灵活UI的桌面应用程序。

在本章中，我们会涵盖以下的主题：

- Qt项目的基础架构；
- Qt Designer接口；
- UI基础；
- 信号和槽；
- 自定义QWidget；
- C++14的lambda，auto和for each；

### 创建一个项目

创建todo程序之后，整个项目的层次如下所示：

![image-20220528174726209](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220528174726209.png)

图示中.pro文件是Qt项目配置文件。随着Qt添加特定的文件格式和C++关键字，会执行中间的构建步骤，解析所有文件生成最终的文件，整个过程由Qt SDK的可执行文件qmake来执行，它会为你的项目产生最终的MakeFIle文件。

一个基本的.pro文件通常包含：

- 使用的Qt模块（core，gui等等）
- target name（如todo，todo.exe等等）
- 项目模板（app、lib等等）
- sources，headers和forms

Qt和C++14附带了一些很棒的功能，本书将在所有的项目中展示它们。对于GCC和CLANG编译器，必须在.profile文件中添加`CONFIT += c++14`，示例如下：

```
QT += core gui
CONFIG += c++14
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = todo
TEMPLATE = app
SOURCE += main.cpp \
			MainWindow.cpp
			
HEADERS += MainWindow.h \
FORMS += MainWindow.ui \
```

其中MainWindow.h和MainWindow.cpp文件是MainWIndow类的头文件/源文件。这些文件包含了由wizard创建的默认的GUI。

MainWindow.ui是XML格式的UI设计文件，可以很容易的使用Qt Designer来进行编辑。该工具是一个WYSIWYG编辑器，可以帮助你添加和调整图形组件（widgets）。

### MainWindow structure

MainWindow.h的头文件如下所示：

```c++
#include <QMainWindow>
namespace Ui {
class MainWindow;
}

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
private:
	Ui::MainWindow *ui;
};
```

添加了Q_OBJECT宏定义，使得类可以定义自己的信号/槽和更多的Qt元系统的特性，这些特性之后会讲解。

在类的构造函数中，接受一个参数parent，此参数是QWidget指针默认为null。

QWidget是UI组件，可以是一个标签、一个文本框、一个按钮等等。如果你在window，layout和其他的UI组件之间定义了父子继承关系，那么管理你的应用就会更加的容易。在以上的例子中，析构函数删除父类就足够了，因为它的析构函数会同时删除其子类。

MainWindow类继承自Qt框架中的QMainWindow。在私有域中，我们定义了一个ui成员变量，可以认为它是UI设计文件MainWindow.ui在C++中的代表，通过它可以与C++中的UI组件（QLabel，QPushButton等）进行交互，如下图：

![image-20220529173539978](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220529173539978.png)

MainWindow.cpp如下所示：

```c++
#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)：
	QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
}
MainWindow::~MainWindow()
{
	delete ui;
}
```

### 信号和槽

Qt框架通过以下三个概念来做到灵活的信息交换：信号，槽和连接：

- 信号是一个对象发送的消息；
- 槽是信号触发时调用的函数；
- 连接函数指定了信号应该连接到哪个槽；

以下是你会喜欢Qt中信号和槽的原因：

- 槽仍然是一个普通的函数，所以你可以自己调用；
- 一个信号可以连接到不同的槽上；
- 一个槽可以被不同被连接的信号调用；
- 可以连接不同对象的信号和槽，甚至对象处于不同的线程间都可以；

注意，为了能够将一个信号连接到一个槽上，它们的方法签名必须匹配，参数的数量、顺序以及类型都必须相同。注意信号和槽从不会返回值。

以下是Qt连接的示例：

```c++
connect(sender, &Sender::signalName, receiver, &Receiver::slotName);
```

其中：

- sender：发送信号的对象；
- &Sender::signalName：指向成员信号函数的指针；
- receiver：接收和处理信号的对象；
- &Receiver::slotName：指向接收者的成员信号槽函数的指针；

也可以自定义槽，比如当点击ui->addTaskButton时，自定义的addTask槽就会触发，如下：

```c++
class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
public slots:
	void addTask();
private:
	Ui::MainWindow *ui;
};
```

Qt使用特定的slot关键字来标识槽，由于一个槽是一个函数，因此你可以根据需要来调整其可见性（public，protected或者private）。

对于QList和QVector的使用，如果需要频繁在开始或中间插入对象，就使用QList，如果希望连续存储，就使用QVector。

### Emitting a custom signal using lambdas

除了上文的添加任务，我们也可以删除任务，在删除任务的时候，我们需要通知其所有者和父级（MainWindow）removeTaskButtonQPushButton已被单击，可在Task.h中自定义删除信号remove来实现：

```c++
……
signals:
    void removed(Task* task);
……
```

由于信号仅用于通知另一个类，因此不需要public关键字。信号只是发送给接收者的一个通知，这意味着*remove(Task* task)函数没有函数体，可以如下写：

```c++
connect(ui->removeButton, &QPushButton::clicked, [this]{
        emit removed(this);
    });
```

Qt5允许connect中接受一个匿名函数。emit是Qt的宏定义，其将立即触发连接的槽。正如前文所述，remove没有函数体，它的目的就是通知注册的slot由一个事件。

## 第二章 Discovering QMake Secrets

本章解决了创建依赖于特定平台代码的跨平台应用程序的问题，我们将看到qmake对项目编译的影响。

你会学习如何创建一个系统监控应用，来检索window、linux和mac的平均cpu负载以及可使用的内存。对于这种依赖操作系统的应用程序，架构是保证应用程序可靠和可维护的关键。

在本章结束时，你将能够创建和组织一个跨平台应用程序，该程序使用特定于平台的代码并显示Qt Charts部件。而且，qmake将不再神秘。

本例的实现将使用策略模式和单例模式，示意图如下：

![image-20220607212825295](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220607212825295.png)

UI部分只会了解和使用SysInfo类，特定平台的实现类由Sysinfo类实例化，调用者不需要知道任何SysInfo子类的信息。由于SysInfo是单例，因此所有部件的访问都非常容易。示例如下：

```c++
class SysInfo
{
public:
    SysInfo();
    virtual ~SysInfo();
    
    virtual void init() = 0;
    virtual double cpuLoadAverage() = 0;
    virtual double memoryUsed() = 0;
};
```

pro文件节选如下：

```
QT += core gui charts
CONFIG += C++14
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = ch02-sysinfo
TEMPLATE = app
windows {
    SOURCES += SysInfoWindowsImpl.cpp
    HEADERS += SysInfoWindowsImpl.h
}

linux {
    SOURCES += SysInfoLinuxImpl.cpp
    HEADERS += SysInfoLinuxImpl.h
}

macx {
    SOURCES += SysInfoMacImpl.cpp
    HEADERS += SysInfoMacImpl.h
}
```

源文件通过宏定义进行选择，如下：

```c++
#ifdef Q_OS_WIN
    #include "SysInfoWindowsImpl.h"
#elif defined(Q_OS_MAC)
    #include "SysInfoMacImpl.h"
#elif defined(Q_OS_LINUX)
    #include "SysInfoLinuxImpl.h"
#endif

SysInfo& SysInfo::instance()
{
    #ifdef Q_OS_WIN
        static SysInfoWindowsImpl singleton;
    #elif defined(Q_OS_MAC)
        static SysInfoMacImpl singleton;
    #elif defined(Q_OS_LINUX)
        static SysInfoLinuxImpl singleton;
    #endif
}
```

其他示例源码的构建此处不做赘述，直接介绍pro文件。一个.pro文件是一个qmake项目文件，描述了项目使用的源代码和头文件，是Makefile的平台无关定义。其中：

- QT：项目中使用模块的列表；
- CONFIG：项目的配置列表，这里表明了对C++14的支持；
- TMPLATE：生成Makefile时使用的项目模板，这里app表示qmake将生成针对二进制文件的Makefile文件。

如果还想在调试时添加一些文件，可以像下面这样写：

```
windows {
    SOURCES += SysInfoWindowsImpl.cpp
    HEADERS += SysInfoWindowsImpl.h
    debug {
        SOURCES += DebugClass.cpp
        HEADERS += DebugClass.h
    }
}
```

这种内嵌等同于与操作，或操作像下面的样子：

```
windows|unix {
	SOURCES += SysInfoWindowsAndLinux.cpp
}
```

甚至我们还可以用if/else，如下：

```
windows|unix {
SOURCES += SysInfoWindowsAndLinux.cpp
} else:macx {
SOURCES += SysInfoMacImpl.cpp
} else {
SOURCES += UltimateGenericSources.cpp
}
```

qmake中的操作符有几个比较特殊的：

- -=：将值从列表中删除；
- *=：仅在该值不存在时将其添加到列表中；
- ~=：用指定的值替换任何与正则表达式相匹配的值；

以下是message的简单介绍，有如下的.pro文件：

```
COMPILE_MSG = "Compiling on"
windows {
    SOURCES += SysInfoWindowsImpl.cpp
    HEADERS += SysInfoWindowsImpl.h
    message($$COMPILE_MSG windows)
}
linux {
    SOURCES += SysInfoLinuxImpl.cpp
    HEADERS += SysInfoLinuxImpl.h
    message($$COMPILE_MSG linux)
}
macx {
    SOURCES += SysInfoMacImpl.cpp
    HEADERS += SysInfoMacImpl.h
    message($$COMPILE_MSG mac)
}
```

通过使用message，当我们在不同平台编译时会打出相应信息。除了message之外，还有以下有用的函数：

- error(string)：显示字符串并立即退出编译；
- exists(filename)：测试文件是否存在；
- include(filename)：包含另一个.pro文件，方便细化.pro。

## 第三章 Dividing Your Project and Ruling Your Code

本章主要介绍如何正确划分项目并最大程度利用Qt框架。设计项目减少耦合的较为常用的方法是将引擎与用户界面分开，例如对gallery的设计就可以划分成以下三个模块：

![image-20220612190240975](C:\Users\fengy\AppData\Roaming\Typora\typora-user-images\image-20220612190240975.png)

子项目架构如下所示：

- gallery-core：应用程序逻辑核心的库，数据类（或者说是业务类），持久存储以及通过单一接口可使存储可用的UI模型；
- gallery-desktop：Qt部件程序，依赖gallery-core来检索数据并显示给用户；
- gallery-mobile：针对移动平台的QML应用；

在Qt中，一种解耦的模式是Model/View模型，其中Model负责管理数据，View负责向用户展示试图，而Controller则负责Model和View的交互。其中需要注意以下代码片段：

```c++
QModelIndex AlbumModel::addAlbum(const Album& album)
{
    int rowIndex = rowCount();
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);
    unique_ptr<Album> newAlbum(new Album(album));
    mDb.albumDao.addAlbum(*newAlbum);
    mAlbums->push_back(move(newAlbum));
    endInsertRows();
    return index(rowIndex, 0);
}
......
QModelIndex PictureModel::addPicture(const Picture& picture)
{
    int rows = rowCount();
    beginInsertRows(QModelIndex(), rows, rows);
    unique_ptr<Picture>newPicture(new Picture(picture));
    mDb.pictureDao.addPictureInAlbum(mAlbumId, *newPicture);
    mPictures->push_back(move(newPicture));
    endInsertRows();
    return index(rows, 0);
}
```

其中beginInsertRows、endInsertRows、beginInsertRows、endInsertRows都是model函数，会触发对应信号。

## 第四章 Conquering the Desktop UI

将应用连接到库可以如下操作，这里以gallery-core和gallery-desktop为例，右键单击项目，选择Add libary|Internal library | gallery-core|Next|Finish，gallery-desktop.pro更新如下：

```
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gallery-desktop
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../gallery-core/release/ -lgallery-core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../gallery-core/debug/ -lgallery-core
else:unix: LIBS += -L$$OUT_PWD/../gallery-core/ -lgallery-core

INCLUDEPATH += $$PWD/../gallery-core
DEPENDPATH += $$PWD/../gallery-core

```

LIBS变量指定要链接的库，其中-L前缀提供库的路径，-l前缀提供库名，示例如下：`LIBS += -L<pathToLibrary> -l<libraryName>`。默认情况下，Windows上编译Qt项目会创建一个调试和发布子目录，这也是要根据平台创建不同LIBS版本的原因。通过LIBS变量，已经可以找到库文件，但还需要给出库头文件的位置，所以这里将gallery-core的源路径加入到INCLUDEPATH和DEPENDPATH中。另外，\$\$OUT_PWD是输出目录的，\$\$PWD是.pro文件的绝对路径。为了确保qmake在桌面应用程序之前编译共享库，需要更新ch03-gallery-core.pro文件：

```
TEMPLATE = subdirs

SUBDIRS += \
    gallery-core \
    gallery-desktop

gallery-desktop.depends = gallery-core
```

尽量使用depends属性，而不是 `CONFIG += ordered`这种，因为前者可以帮助提升编译速度。

创建一个UI程序，免不了使用一些图像声音等的文件，这些都保存在Qt资源文件中，可采用如下的步骤创建资源文件，Add New|Qt|Qt Resource File，会创建一个默认的资源文件，并添加在.pro文件中，如下所示：

```
RESOURCES += resource.qrc
```

可以用Resource Edit和Plain Text Edit来编辑文件，前者展示的是视图，后者展示的是xml文件，如下所示：

```
<RCC>
<qresource prefix="/">
<file>icons/album-add.png</file>
<file>icons/album-delete.png</file>
<file>icons/album-edit.png</file>
<file>icons/back-to-gallery.png</file>
<file>icons/photo-add.png</file>
<file>icons/photo-delete.png</file>
<file>icons/photo-next.png</file>
<file>icons/photo-previous.png</file>
</qresource>
</RCC>
```

编译时qmake和rcc会将资源文件编进应用二进制程序中。

在图片程序中，由于缩略图的生成必须在gallery-desktop而不是gallery-core中完成，因此决定扩展PictureModel类的行为，这里决定使用QAbstractProxyModel及其子类来实现，其有两个子类：QIdentityProxyModel子类代理原模型而不进行修改，如果想要修改data方法的内容这个类是合适的；QSortFilterProxyModel子类代理原模型，能够对传递的数据进行排序和过滤。实现如下：

```c++
#include <QIdentityProxyModel>
#include <QHash>
#include <QPixmap>

class PictureModel;

class ThumbnailProxyModel : public QIdentityProxyModel
{
public:
    ThumbnailProxyModel(QObject* parent=nullptr);
    QVariant data(const QModelIndex& index, int role) const override;
    void setSourceModel(QAbstractItemModel* sourceModel) override;
    PictureModel* pictureModel() const;

private:
    void generateThumbnails(const QModelIndex& startIndex, int count);
    void reloadThumbnails();

private:
    QHash<QString, QPixmap*> mThumbnails;
};
```

其中重写了data与setSourceModel方法，前者向ThumbnailProxyModel的客户端发送数据，后者注册注册信号到原模型中。其实现如下：

```c++
#include "ThumbnailProxyModel.h"

#include "PictureModel.h"

const unsigned int THUMBNAIL_SIZE = 350;

ThumbnailProxyModel::ThumbnailProxyModel(QObject* parent) :
    QIdentityProxyModel(parent),
    mThumbnails()
{
}

QVariant ThumbnailProxyModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DecorationRole) {
        return QIdentityProxyModel::data(index, role);
    }

    QString filepath = sourceModel()->data(index, PictureModel::Roles::FilePathRole).toString();
    return *mThumbnails[filepath];
}

void ThumbnailProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    QIdentityProxyModel::setSourceModel(sourceModel);
    if (!sourceModel) {
        return;
    }

    connect(sourceModel, &QAbstractItemModel::modelReset, [this] {
        reloadThumbnails();
    });

    connect(sourceModel, &QAbstractItemModel::rowsInserted, [this](const QModelIndex& parent, int first, int last) {
        generateThumbnails(index(first, 0), last - first + 1);
    });
}

PictureModel* ThumbnailProxyModel::pictureModel() const
{
    return static_cast<PictureModel*>(sourceModel());
}

void ThumbnailProxyModel::reloadThumbnails()
{
    qDeleteAll(mThumbnails);
    mThumbnails.clear();
    generateThumbnails(index(0, 0), rowCount());
}

void ThumbnailProxyModel::generateThumbnails(const QModelIndex& startIndex, int count)
{
    if (!startIndex.isValid()) {
        return;
    }

    const QAbstractItemModel* model = startIndex.model();
    int lastIndex = startIndex.row() + count;
    for(int row = startIndex.row(); row < lastIndex; row++) {
        QString filepath = model->data(model->index(row, 0), PictureModel::Roles::FilePathRole).toString();
        QPixmap pixmap(filepath);
        auto thumbnail = new QPixmap(pixmap
                                     .scaled(THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
        mThumbnails.insert(filepath, thumbnail);
    }
}

```

如上所示，QIdentityProxyModel可以在不破坏原有model的情况下向其添加行为。

在ui设计上，如果想要添加自定义组件，例如我们自己定义的AlbumWidget，可以先添加QWidget组件，之后右击提升型别，选中自己的组件。

## 第五章 Dominating the Mobile UI

本章主要讲述使用qml来制作ui。main函数如下所示：

```c++
#include <QGuiApplication>
#include <QQmlApplicationEngine>
int main(int argc, char *argv[])
{
QGuiApplication app(argc, argv);
QQmlApplicationEngine engine;
engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
return app.exec();
}
```

由于本章不使用Qt widget，因此这里用了QGuiApplication而不是QApplication。下面是一个简单的qml文件示例：

```c++
ApplicationWindow {

    readonly property alias pageStack: stackView

    id: app
    visible: true
    width: 768
    height: 1280
    color: Style.windowBackground

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: AlbumListPage {}
    }

    onClosing: {
        if (Qt.platform.os == "android") {
            if (stackView.depth > 1) {
                close.accepted = false
                stackView.pop()
            }
        }
    }
}
```

这里使用property alias定义了一个属性别名，方便从其他组件访问，另外，为了防止QML组件破坏stackView，设置为只读属性。

在qml文件中也可以声明单例类型，需要两步，有如下的qml文件：

```
pragma Singleton
import QtQuick 2.0

QtObject {
    property color text: "#000000"

    property color windowBackground: "#eff0f1"
    property color toolbarBackground: "#eff0f1"
    property color pageBackground: "#fcfcfc"
    property color buttonBackground: "#d0d1d2"

    property color itemHighlight: "#3daee9"
}

```

第一步就是通过`pragma Singleton`声明单例，第二步可以通过在C++实现或者创建qmldir文件实现，如下：

```
singleton Style 1.0 Style.qml
```

声明了一个名为Style的单例，版本为1.0。使用如下：

```
import QtQuick 2.0
import QtQuick.Controls 2.0
import "."

ToolBar {
     background: Rectangle {
         color: Style.toolbarBackground
     }
}
```

qml单例需要显式导入才能加载qmldir文件，所以这里使用了import "."。

## 第六章 Even Qt Deserves a Slice of Raspberry Pi

 本章主要包括以下内容：

- Qt3D模块的架构；
- 交叉编译的原则；
- 如何建立自己的Qt Creator工具包以及在Raspberry Pi上部署自己的游戏；
- 如何处理各平台的差异和限制；
- 工厂模式；
- 如何使用JavaScript和qml完成一个游戏引擎；
- QML profiler的使用；

要渲染一个3D的红苹果，大致需要以下的组件：

- 一个网格组件，保存苹果的顶点；
- 一个材料组件，在网格上应用纹理或者为其着色；

Qt3D类没有继承自Item，因此不能直接与QML的组件一起使用。QNode类是所有Qt3D node类的基类，其依赖QObject类来定义亲子关系，每个QNode类还有一个唯一标识自己的id变量。

虽然QNode不能直接与Qt Quick类型一起使用，但它们可以被添加进Q3DScene元素（或者Qml中的Scene3D ）中，Q3DScene作为它们的容器，可以添加到Qt Quick Item中。

以下来展示相关的示例，main.cpp如下：

```c++
#include <QGuiApplication>
#include <QtQui/QOpenGLContext>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlEngine>

int main(int argc, char *argv[])
{
//    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    qputenv("QT3D_GLSL100_WORKAROUND", "");
    QSurfaceFormat format;
    if (QOpenGLContext::openGLModuleType() == 
            QOpenGLContext::LibGL) {
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
    }
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    
    QQuickView view;
    view.setFormat(format);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    QObject::connect(view.engine(), &QQmlEngine::quit,
                     &app, &QGuiApplication::quit);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}
```

利用 QSurfaceFormat来处理OpenGL以定制化QQuickView view。

GameArea.qml文件定义如下：

```c++
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Extras 2.0
import QtQuick 2.6 as QQ2

Entity {
    id: root
    property alias gameRoot: root
    Camera {
        id: camera
        property real x: 24.5
        property real y: 14.0
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        aspectRatio: 16/9
        nearPlane: 0.1
        farPlane: 1000.0
        position: Qt.vector3d(x, y, 33.0)
        upVector: Qt.vector3d(0.0, 1.0, 0.0)
        viewCenter: Qt.vector3d(x, y, 0.0)
    }

    RenderSettings {
        id: frameFraph
        activeFrameGraph: ForwardRenderer {
            clearColor: Qt.rgba(0, 0, 0, 1)
            camera: camera
        }
    }

    KeyboardDevice {
        id: keyboardController
    }

    InputSettings {
        id: inputSettings
    }

    KeyboardHandler {
        id: input
        sourceDevice: keyboardController
        focus: true
        onPressed: {}
    }
    
    QQ2.Component.onCompleted: {
        console.log("Start game...");
        timer.start()
    }
    
    QQ2.Timer {
        id: timer
        interval: initialTimeInterval
        repeat: true
        onTriggered: {}
    }

    components: [frameFraph, input]

}
```

## 第七章 Third-Party Libraries Without a Headache

本章目的是创建一个qt design可以使用的插件，其配置文件示例如下：

```c++
QT += widgets uiplugin
CONFIG += plugin
CONFIG += c++14
TEMPLATE = lib 
DEFINES += FILTERPLUGINDESIGNER_LIBRARY

TARGET = $$qtLibraryTarget($$TARGET)
INSTALLS += target
```

其中QT和CONFIG中的uiplugin和plugin都是必须的。默认情况下，我们的TARGET是filter-plugin-designer，`$$qtLibraryTarget()`函数会根据我们的平台来更新，比如window下会添加后缀d。最后，将target添加到INSTALLS这里，主要是为了把生成的library文件安装到特定文件夹。



## 第九章 Keeping Your Sanity with Multithreading

本章了解Qt中线程如何工作。Qthread是Qt线程系统的中心类，一个QThread实例管理一个线程。我们可以重写QThread派生类中的run方法，其会在QThread框架中执行，以下是启动一个线程的示例：

```c++
QThread thread;
thread.start()
```

start方法调用时会自动调用run函数，并发射started信号，只有在这个时候，新的执行线程才被创建，当run函数执行完毕时，会发射finished信号，这与信号/槽机制相似。Qt是一个事件驱动的框架，主事件循环（或GUI循环），处理事件则刷新UI。

每个QThread都有自己的事件循环，可以处理主循环以外的事件，如果没有重写，run函数会调用QThread::exec()函数，来启动事件对象的事件循环。我们也可以重写QThread类并调用自己的exec()函数，例如：

```c++
class Thread: public QThread
{
Q_OBJECT
protected:
	void run()
	{
		Object* myObject = new Object();
		connect(myObject, &Object::started, this, &Thread::doWork);
		exec();
	}
private slots: 
	void doWork();
};
```

仅在exec()执行时，Thread事件循环才会处理started信号，它将阻塞并等待，直到exit被调用。

需要注意的是事件循环中的事件值得是从线程创建或移动到该线程的所有对象。

在QCoreApplication对象之前创建的线程没有线程亲和性，因此，不会向其发送任何事件。

通过下面的例子来看一下线程如何通信：

```c++
class Thread: public QThread
{
	Q_OBJECT
	void run {
		emit result("I < 3 threads");
	}
signals:
	void result(QString data);
};
Thread* thread = new Thread(this);
connect(thread, &Thread::result, this, &MainWindow::handleResult);
connect(thread, &Thread::finished, thread, &QObject::deleteLater);
thread->start();
```

上面的信号为何能够跨线程通信，这就要看connect的完整签名：

```
QObject::connect(
const QObject *sender, const char *signal,
const QObject *receiver, const char *method,
Qt::ConnectionType type = Qt::AutoConnection)
```

关键字Qt::AutoConnection是默认值，来看看Qt::ConnectionType的完整取值：

- Qt::AutoConnection：如果接收者位于发出信号的进程中，则使用此值，否则使用Qt::QueuedConnection；
- Qt::DirectConnection：槽在信号发出时立即被调用，该槽在发信线程中执行；
- Qt::QueuedConnection：当控制信号返回接收线程的事件循环时调用该槽，该槽在接收线程中执行；
- Qt::BlockingQueuedConnection：与Qt::QueuedConnction相同，只是发信线程阻塞直到槽执行完毕返回，如果接收者位于信号线程则不得使用此值，否则程序将死锁；
- Qt::UniqueConnection：此标志可使用或操作与前述任意一种连接类型连接，当设置此标志，如果连接已经存在，则连接将失效；

当使用Qt::AutoConnection时，只有当有效发出信号时，ConnectionType才会解析。

Qt也支持worker模型，可将线程从实际的进程中分离出来，如下：

```c++
class Worker: public QObject
{
	Q_OBJECT
public slots:
	void doWork()
	{
		emit result("workers are the best");
	}
signals:
	void result(QString data);
}
// Somewhere in MainWindow
QThread* thread = new Thread(this);
Worker* worker = new Worker();
worker->moveToThread(thread);
connect(thread, &QThread::finished,
worker, &QObject::deleteLater);
connect(this, &MainWindow::startWork,
worker, &Worker::doWork);
connect(worker, &Worker::resultReady,
this, handleResult);
thread->start();
// later on, to stop the thread
thread->quit();
thread->wait();
```

实际工作对象与线程分离开来，通过使用moveToThread，worker对象被转移到thread线程中，另外，一旦调用了start，我们就不能转移对象。

Qt中的互斥锁通过QMutex提供，如下：

```c++
QMutex mutex；
int number = 1;
mutex.lock();
number *= 2;
mutex.unlock();
```

这种情况下可能会忘记解锁，所以Qt提供了QMutexLocker类来简化这种情况：

```c++
QMutex mutex;
QMutexLocker locker(&mutex);
......
```

更高级的用法，可以使用QThreadPool类来管理线程池。要在线程池管理的线程中执行代码，可以定义相关的处理类，但要继承QRunnable，如下：

```c++
class Job: public QRunnable
{
	void run()
	{ ... }
}
Job* job = new Job();
QThreadPool::globalInstance()->start(job);
```

QThreadPool::start()拥有job的所有权，并将在run完成时自动删除。

如果想要手动管理线程可以使用QThreadPool::idealThreadCount()来获得可以使用的线程数量。Qt也提供了一种更高级的api，可以避免使用互斥锁等手段。这种方法依赖QFuture类执行函数，并期望稍后得到结果：

```c++
void longRunningFunction();
QFuture<void> future = QtConcurrent::run(longRunningFunction);
```

处理函数会在线程池中分离出的线程中执行。有参的使用如下：

```c++
QImage processGrayscale(QImage& image);
QImage lenna;
QFuture<QImage> future = QtConcurrent::run(processGrayscale, lenna);
QImage grayscaleLenna = future.result();
```

为了避免阻塞，我们可以使用QFutureWatcher来辅助，如下：

```
QFutureWatcher<QImage> watcher;
connect(&watcher, &QFutureWatcher::finished, this, &QObject::handleGrayscale);
QImage processGrayscale(QImage& image);
QImage lenna;
QFuture<QImage> future = QtConcurrent::run(processGrayscale, lenna);
watcher.setFuture(future);
```

当拿到期望值后，会调用processGrayscale槽进行处理；

Qt也提供了MapReduce和FilterReduce的实现，MapReduce是一种编程模型：

- 在CPU的多核之间映射和分发对数据集的处理；
- 缩减或聚合结果提供给调用者；

如下：

```c++
QList image = ...;
QImage processGrayscale(QImage& image);
QFuture<void> future = QtConcurrent::mapped(images, processGrayscale);
```

该操作可以通过QtConncurrent::mapped而不是QtConcurrent::blockingMapped来阻塞；

MapReduce的实际使用如下：

```c++
QList images = ...;
QImage processGrayscale(QImage& image);
void combineImage(QImage& finalImage, const QImage& inputImage);
QFuture<void> future = QtConcurrent::mappedReduced(
						images,
						processGrayscale,
						combineImage);
```

processGrayscale的中间结果会被合并到finalImage中，每个线程只调用一次combineImage，因此无需对结果加锁。

FilterReduce与之类似，允许我们对输入列表进行过滤而不是转换。

可使用setMaxThreadCount(x)来设置QThreadPool的线程数。

有如下的代码示例：

```c++
MandelbrotWidget::MandelbrotWidget(QWidget *parent) : QWidget(parent),
    mMandelbrotCalculator(), mThreadCalculator(this), mScaleFactor(DEFAULT_SCALE),
    mLastMouseMovePosition(), mMoveOffset(DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y), mAreaSize(), mIterationMax(ITERATION_MAX)
{
    mMandelbrotCalculator.moveToThread(&mThreadCalculator);
    connect(this, &MandelbrotWidget::requestPicture,
            &mMandelbrotCalculator,
            &MandelbrotCalculator::generatePicture);
    mThreadCalculator.start();
}
```

为了防止干扰UI线程，第一件事就是更改线程的亲和性，使得计算处理委托给计算线程。

析构时退出线程：

```c++
MandelbrotWidget::~MandelbrotWidget()
{
    mThreadCalculator.quit();
    mThreadCalculator.wait(1000);
    if (!mThreadCalculator.isFinished()) {
        mThreadCalculator.terminate();
    }
}
```



## 第十章 Need IPC？Get Your Minions to Work

本章主要介绍了在不同进程之间的线程共享数据。IPC（进程间通信）的第一个工具就是TCP/IP套接字。示例如下：

```c++
QTcpServer* tcpServer = new QTcpServer(this);
tcpServer->listen(QHostAddress::Any, 5000);

connect(tcpServer, &QTcpServer::newConnection, [tcpServer]{
	QTcpSocket *tcpSocket = tcpServer->nextPendingConnection();
	QByteArray response = QString("Hello").toLatin1();
	tcpSocket->write(response);
	tcpSocket->disconnectFromHost();
	qDebug() << "Send response and close the socket";
});
```

第一步是实例化一个QTcpServer类，然后调用listen函数，可以提供网络接口并指定服务器必须侦听的端口，例如本例中在端口5000上监听所有的网络地址。当客户端与服务器建立连接时，QTcpServer::newConnection()信号会触发，会首先搜索与客户端连接服务端，之后通过socket发送消息，发送消息之后与客户端断开连接。

测试可以通过各平台的telnet命令来测试，客户端的使用如下：

```c++
QTcpSocket *tcpSocket = new QTcpSocket(this);
tcpSocket->connectToHost("127.0.0.1", 5000);
connect(tcpSocket, &QTcpSocket::connected, [tcpSocket]{
	qDebug() << "connected";
});
connect(tcpSocket, &QTcpSocket::readyRead, [tcpSocket]{
	qDebug() << QString::fromLatin1(tcpSocket->readAll());
});
connect(tcpSocket, &QTcpSocket::disconnected, [tcpSocket]{
	qDebug() << "disconnected";
});
```

这种通信是双向的，所以客户端也可以写数据，服务器可以读，但注意要删除服务器部分中的disconnectFromHost()调用来保持通信活跃。

第二种IPC技术是共享内存，通过QSharedMemory来提供一种跨平台的方法创建和使用跨多个进程的共享内存。这些进程必须在同一台计算机上运行，共享内存由密钥标识，所有进程必须使用相同的密钥来共享相同的共享内存段。第一个进程创建共享内存，如下：

```
QString sharedMessage("Hello");
QByteArray sharedData = shareMessage.toLatin1();
QSharedMemory* sharedMemory = new QSharedMemory("sharedMemoryKey", this);
sharedMemory->create(sharedMessage.size());
sharedMemroy->lock();
memcpy(sharedMemory->data(), sharedData.data(), sharedData.size());
sharedMemory->unlock();
```

销毁QShareMemory类将调用detach()函数将进程与共享内存分离。

以下是其他线程如何使用共享内存：

```c++
QSharedMemory* sharedMemory = new QSharedMemory("sharedMemoryKey", this);
sharedMemory->attach();
sharedMemroy->lock();
QByteArray sharedData(sharedMemory->size(), '\0');
memcpy(sharedData.data(), sharedMemory->data(), sharedMemory->size());
sharedMemory->unlock();
QString sharedMessage = QString::fromLatin1(sharedData);
qDebug() << sharedMessage;
sharedMemory->detach();
```

注意，create和attach默认指定QShareMemory::ReadWrite访问，还可以使用QShareMemory::ReadOnly访问。

另一种IPC方式是使用QProcess类，主进程将额外的应用以子进程的形式启动，使用标准的输入输出设备进行通信。示例如下：

```c++
QProcess* childProcess = new QProcess(this);
connect(childProcess, &QProcess::readyReadStandardOutput, [childProcess] {
	qDebug().noquote() << "[*]" << childProcess->readAll();
});
connect(childProcess, &QProcess::started, [childProcess] {
	childProcess->write("Sophie\n");
});
childProcess->start("/path/to/hello");
```

最后以一种IPC机制是D-Bus协议。这种协议最有用的一点是甚至可以在总线上使用信号/槽机制，即从一个程序发出信号，另一个程序的槽进行处理。KDE和GNOME等linux桌面环境使用D-Bus，这意味着可以使用D-Bus控制桌面。D-Bus的概念如下：

- bus：用于多对多通信，定义了两种总线：system和session；
- service name：总线上服务的标识符；
- message：应用程序发送的消息，如果使用总线，则消息包括目的地；

在qt安装目录下有Qt D-Bus viewer工具，可以显示总线上所有的对象和消息。以下是使用方法：

```c++
//HelloService.h
class HelloService : public QObject
{
	Q_OBJECT
public slots:
    QString sayHello(const QString &name);
};
//HelloService.cpp
QString HelloService::sayHello(const QString& name)
{
    qDebug().noquote() << name << " is here!";
    return QString("Hello %1!").arg(name);;
}
```

首先进行了声明，之后注册DBus服务：

```c++
HelloService helloService;
QString serviceName("org.masteringqt.QtDBus.HelloService");
QDBusConnection::sessionBus().registerService(serviceName);
QDBusConnection::sessionBus().registerObject("/",
&helloService, QDBusConnection::ExportAllSlots);
```

这段代码注册了一个新服务，并公开了它的所有的槽。

```c++
QString serviceName("org.masteringqt.QtDBus.HelloService");
QDBusInterface serviceInterface(serviceName, "/");
QDBusReply<QString> response = serviceInterface.call(
"sayHello", "Lenna");
qDebug().noquote() << response;
```

通过serviceInterface对象来调用远端方法sayHello。

在编写代码的时候，有以下的示例：

```c++
struct Message {

    enum class Type {
        WORKER_REGISTER,
        WORKER_UNREGISTER,
        ALL_JOBS_ABORT,
        JOB_REQUEST,
        JOB_RESULT,
    };

    Message(const Type type = Type::WORKER_REGISTER,
            const QByteArray& data = QByteArray()) :
        type(type),
        data(data)
    {
    }

    ~Message() {}

    Type type;
    QByteArray data;
};
```

这里使用了C++11的枚举类，可以防止命名污染，隐式转换，以及可以前向声明。

```c++
struct JobRequest
{
    int pixelPositionY;
    QPointF moveOffset;
    double scaleFactor;
    QSize areaSize;
    int iterationMax;
};

inline QDataStream& operator<<(QDataStream& out, const JobRequest& jobRequest)
{
    out << jobRequest.pixelPositionY
        << jobRequest.moveOffset
        << jobRequest.scaleFactor
        << jobRequest.areaSize
        << jobRequest.iterationMax;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, JobRequest& jobRequest)
{
    in >> jobRequest.pixelPositionY;
    in >> jobRequest.moveOffset;
    in >> jobRequest.scaleFactor;
    in >> jobRequest.areaSize;
    in >> jobRequest.iterationMax;
    return in;
}

Q_DECLARE_METATYPE(JobRequest)
```

这里使用了Q_DECLARE_METATYPE宏，该宏使得Qt元对象系统知道JobRequest，这需要能够与QVariant结合使用该类，具体的使用会在后面看到。

QDataStream类实现了C++基本类型和几种Qt类型（QBrush、QColor、QString等）的序列化，示例如下：

```c++
QFile file("myfile");
file.open(QIODevice::WriteOnly);
QDataStream out(&file);
out << QString("QDataStream saved my day");
out << (qint32)42;
```

实际使用可以如下这么使用：

```c++
inline void sendMessage(QTcpSocket& socket,
                        Message::Type messageType,
                        QByteArray& data,
                        bool forceFlush = false)
{
    Message message(messageType, data);

    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << message;
    socket.write(byteArray);
    if (forceFlush) {
        socket.flush();
    }
}
```

阅读消息可以如下写：

```c++
inline std::unique_ptr<std::vector<std::unique_ptr<Message>>> readMessages(QDataStream& stream)
{
    auto messages = std::make_unique<std::vector<std::unique_ptr<Message>>>();
    bool commitTransaction = true;
    while (commitTransaction
                    && stream.device()->bytesAvailable() > 0) {
        stream.startTransaction();
        auto message = std::make_unique<Message>();
        stream >> *message;
        commitTransaction = stream.commitTransaction();
        if (commitTransaction) {
            messages->push_back(std::move(message));
        }
    }
    return messages;
}
```

## 第十一章 Having Fun with Serialization

QVariant可用来进行数据的序列化，可通过variant.toX()这样的语法进行多种数据类型的转换。当在variant中储存值时，其被储存在一个私有的union中，如下：

```c++
union Data
{
    char c;
    uchar uc;
    short s;
    signed char sc;
    ushort us;
    ...
    qulonglong ull;
    QObject *o;
    void *ptr;
    PrivateShared *shared;
} data;
```

注意到列表的末尾，包含复杂的类型QObject\*与void\*。我们可以使用Q_DECLARE_METATYPE(SoundEvent)来注册自定义类型，此时，QVariant也变得可用。以下是QVariant与其他类型的转换：

```c++
SoundEvent soundEvent(4365, 0);
QVariant stored;
stored.setValue(soundEvent);
SoundEvent newEvent = stored.value<SoundEvent>();
qDebug() << newEvent.timestamp;
```

QVariant也有一个缺点，那就是它没法处理json与xml这种数据，这种时候就可以使用QVariantMap了，其只是QMap\<QString, QVariant\>的typedef。其使用示例如下：

```c++
QVariant SoundEvent::toVariant() const
{
    QVariantMap map;
    map.insert("timestamp", timestamp);
    map.insert("soundId", soundId);
    return map;
}
void SoundEvent::fromVariant(const QVariant& variant)
{
    QVariantMap map = variant.toMap();
    timestamp = map.value("timestamp").toLongLong();
    soundId = map.value("soundId").toInt();
}

QVariant Track::toVariant() const
{
    QVariantMap map;
    map.insert("duration", mDuration);
    QVariantList list;
    for (const auto& soundEvent : mSoundEvents) {
    	list.append(soundEvent->toVariant());
    }
    map.insert("soundEvents", list);
    return map;
}
void Track::fromVariant(const QVariant& variant)
{
    QVariantMap map = variant.toMap();
    mDuration = map.value("duration").toLongLong();
    QVariantList list = map.value("soundEvents").toList();
    for(const QVariant& data : list) {
        auto soundEvent = make_unique<SoundEvent>();
        soundEvent->fromVariant(data);
        mSoundEvents.push_back(move(soundEvent));
    }
}
```

对于json文件，qt提供了一个QJsonDocument类来读写json文件，注意，此类只接受QVariantMap、QVariantList或QStringList数据类型，以下是一个简单的示例：

```c++
void JsonSerializer::save(const Serializable& serializable,
const QString& filepath, const QString& /*rootName*/)
{
    QJsonDocument doc =
    QJsonDocument::fromVariant(serializable.toVariant());
    QFile file(filepath);
    file.open(QFile::WriteOnly);
    file.write(doc.toJson());
    file.close();
}

void JsonSerializer::load(Serializable& serializable,
const QString& filepath)
{
    QFile file(filepath);
    file.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    serializable.fromVariant(doc.toVariant());
}
```

toJson方法转换成UTF-8编码。

对于xml文件，xml表示如下：

```c++
<[name]> type="[type]">[data]</[name]>
```

一个示例如下：

```c++
void XmlSerializer::writeVariantValueToStream(
const QVariant& variant, QXmlStreamWriter& stream)
{
	stream.writeCharacters(variant.toString());
}
void XmlSerializer::writeVariantListToStream(
const QVariant& variant, QXmlStreamWriter& stream)
{
    QVariantList list = variant.toList();
    for(const QVariant& element : list) {
    	writeVariantToStream("item", element, stream);
    }
}
void XmlSerializer::writeVariantMapToStream(
const QVariant& variant, QXmlStreamWriter& stream)
{
    QVariantMap map = variant.toMap();
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        writeVariantToStream(i.key(), i.value(), stream);
    }
}

QVariant XmlSerializer::readVariantValueFromStream(
QXmlStreamReader& stream)
{
    QXmlStreamAttributes attributes = stream.attributes();
    QString typeString = attributes.value("type").toString();
    QString dataString = stream.readElementText();
    QVariant variant(dataString);
    variant.convert(QVariant::nameToType(
    typeString.toStdString().c_str()));
    return variant;
}

QVariant XmlSerializer::readVariantListFromStream(QXmlStreamReader& stream)
{
    QVariantList list;
    while(stream.readNextStartElement()) {
    	list.append(readVariantFromStream(stream));
    }
    return list;
}

QVariant XmlSerializer::readVariantMapFromStream(
QXmlStreamReader& stream)
{
    QVariantMap map;
    while(stream.readNextStartElement()) {
        map.insert(stream.name().toString(),
        readVariantFromStream(stream));
    }
    return map;
}

void XmlSerializer::writeVariantToStream(const QString& nodeName,
const QVariant& variant, QXmlStreamWriter& stream)
{
    stream.writeStartElement(nodeName);
    stream.writeAttribute("type", variant.typeName());
    switch (variant.type()) {
        case QMetaType::QVariantList:
            writeVariantListToStream(variant, stream);
            break;
        case QMetaType::QVariantMap:
            writeVariantMapToStream(variant, stream);
            break;
        default:
            writeVariantValueToStream(variant, stream);
            break;
    }
    stream.writeEndElement();
}

QVariant XmlSerializer::readVariantFromStream(QXmlStreamReader& stream)
{
    QXmlStreamAttributes attributes = stream.attributes();
    QString typeString = attributes.value("type").toString();
    QVariant variant;
    switch (QVariant::nameToType(
        typeString.toStdString().c_str())) {
        case QMetaType::QVariantList:
            variant = readVariantListFromStream(stream);
            break;
        case QMetaType::QVariantMap:
            variant = readVariantMapFromStream(stream);
            break;
        default:
            variant = readVariantValueFromStream(stream);
            break;
    }
    return variant;
}

void XmlSerializer::save(const Serializable& serializable, const QString&
filepath, const QString& rootName)
{
    QFile file(filepath);
    file.open(QFile::WriteOnly);
    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    writeVariantToStream(rootName, serializable.toVariant(),
    stream);
    stream.writeEndDocument();
    file.close();
}

void XmlSerializer::load(Serializable& serializable,
const QString& filepath)
{
    QFile file(filepath);
    file.open(QFile::ReadOnly);
    QXmlStreamReader stream(&file);
    stream.readNextStartElement();
    serializable.fromVariant(readVariantFromStream(stream));
}

```

最后是处理二进制文件，如下：

```c++
void BinarySerializer::save(const Serializable& serializable,
const QString& filepath, const QString& /*rootName*/)
{
    QFile file(filepath);
    file.open(QFile::WriteOnly);
    QDataStream dataStream(&file);
    dataStream << serializable.toVariant();
    file.close();
}
void BinarySerializer::load(Serializable& serializable, const QString& filepath)
{
    QFile file(filepath);
    file.open(QFile::ReadOnly);
    QDataStream dataStream(&file);
    QVariant variant;
    dataStream >> variant;
    serializable.fromVariant(variant);
    file.close();
}

```



## 第十二章 You Shall (Not) Pass with QTest

一个测试工程的配置文件示例如下：

```c++
QT += core gui multimedia widgets testlib
CONFIG += c++14 console
TARGET = drum-machine-test
TEMPLATE = app
include(../drum-machine-src.pri)
DRUM_MACHINE_PATH = ../drum-machine
INCLUDEPATH += $$DRUM_MACHINE_PATH
DEPENDPATH += $$DRUM_MACHINE_PATH
SOURCES += main.cpp
```

从上面可以看出，需要对testlib进行使能。

Qt的Test基于以下的设定：

- 一个测试用例就是一个QObject类；
- 一个私有槽函数就是一个测试函数；
- 一个测试用例可以包含多个测试函数；

但需要注意以下名字的私有槽函数不是测试函数：

- initTestCase()：在第一个测试函数之前调用；
- init()：在每个测试函数之前调用；
- cleanup()：在每个测试函数之后调用；
- cleanupTestCase()：在最后一个测试函数之后调用；

QVERIFY可以用来执行数据之间的比对，比如QVERIFY(data == DUMMY_FILE_CONTENT)。但其实也可以用QCOMPARE()，比如QCOMPARE(dummy.myInt, 1)。

QTest还提供另一个有趣的宏，QTEST_MAIN，其可以产生完成的main函数，使用方法如下：

```c++
QTEST_MAIN(TestJsonSerializer)
#include "testjsonserializer"
```

注意，使用了QTEST_MAIN宏之后要移除自己的main函数，不然main函数会有两份。

如果想要自定义主函数，示例如下：

```c++
#include "TestJsonSerializer.h"
int main(int argc, char *argv[])
{
    TestJsonSerializer test;
    QStringList arguments = QCoreApplication::arguments();
    return QTest::qExec(&test, arguments);
}
```

有时我们有测试不同输入的需要，那么给xxx()输入数据的函数必须命名为xxx_data()，如下：

```c++
class TestTrack : public QObject
{
	Q_OBJECT
public:
	explicit TestTrack(QObject *parent = 0);
private slots:
    void addSoundEvent_data();
    void addSoundEvent();
};

void TestTrack::addSoundEvent_data()
{
    QTest::addColumn<int>("trackState");
    QTest::addColumn<int>("soundEventCount");
    QTest::newRow("STOPPED")
    << static_cast<int>(Track::State::STOPPED)
    << 0;
    QTest::newRow("PLAYING")
    << static_cast<int>(Track::State::PLAYING)
    << 0;
    QTest::newRow("RECORDING")
    << static_cast<int>(Track::State::RECORDING)
    << 1;
}

void TestTrack::addSoundEvent()
{
    QFETCH(int, trackState);
    QFETCH(int, soundEventCount);
    Track track;
    switch (static_cast<Track::State>(trackState)) {
        case Track::State::STOPPED:
            track.stop();
            break;
        case Track::State::PLAYING:
            track.play();
            break;
        case Track::State::RECORDING:
            track.record();
            break;
        default:
            break;
    }
    track.addSoundEvent(0);
    track.stop();
    QCOMPARE(track.soundEvents().size(),
    static_cast<size_t>(soundEventCount));
}


```

其中，QFETCH(int, trackState)做了两件事，第一件事是声明trackState的类型为int，第二件事是匹配当前列的数据并将其存储在trackState中。

QBENCHMARK这个宏定义用于基准测试，使用示例如下：

```c++
QBENCHMARK {
	mSerializer.save(track, FILENAME);
}
```

如果进行基准测试的时候不想迭代多次，可以使用QBENCHMARK_ONCE。

QTest还提供了对信号进行监视的测试，使用的是QSignalSpy这个类，使用如下：

```c++
void TestGui::playSound()
{
	SoundEffectWidget widget;
	QSignalSpy spy(&widget, &SoundEffectWidget::soundPlayed);
	widget.setId(2);
	widget.play();
	QCOMPARE(spy.count(), 1);
	QList<QVariant> arguments = spy.takeFirst();
	QCOMPARE(arguments.at(0).toInt(), 2);
}
```

需要使用要监视对象的指针和要监视的信号来初始化QSignalSpy这个类，发射信号的参数被储存在一个QVariantList中。