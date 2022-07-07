# QT Creator快速入门

## 窗口部件

Qt Creator提供的默认基类有QMainWIndow、QWidget和QDialog这三种，其中QMainWindow是带有菜单栏和工具栏的主窗口类，QDialog是各种对话框的基类，它们全部继承自QWidget，不仅如此，其实所有的窗口部件都继承自QWidget。

### 基础窗口部件QWidget

QWidget类是所有用户界面对象的基类，被称为基础窗口部件。QWidget继承自QObject类和QPaintDevice类，其中QObject类是所有支持Qt对象模型的基类，QPaintDevice类是所有可以绘制的对象的基类

窗口部件（Widget）简称部件，是Qt中建立用户界面的主要元素，像主窗口、对话框、标签以及按钮、文本输入框等都是窗口部件，这些部件可以接收用户输入、显示数据和状态信息并且在屏幕上绘制自己，有些也可以作为一个容器放置其他部件。Qt中把没有嵌入到其他部件中的部件称为窗口，一般窗口都有标题栏。

调试示例：

```qt

#include <QApplication>
#include <QWidget>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QWidget widget;
    widget.resize(400, 300);
    widget.move(200, 100);
    widget.show();
    int x = widget.x();
    qDebug("x: %d", x);
    int y = widget.y();
    qDebug("y: %d", y);
    QRect geometry = widget.geometry();
    QRect frame = widget.frameGeometry();
    
    qDebug() << "geometry: " << geometry << "frame: " << frame;
    return a.exec();
}

```

### 对话框QDialog

QDialog类是所有对话框窗口类的基类，对话框常被分成两类：模态的和非模态的。

模态对话框是没有关闭的话就不能与同一个应用程序的其他窗口进行交互，对于非模态对话框，既可以与它交互，也可以和同一程序中的其他窗口交互。以下是模态对话框的示例：

```
MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    QDialog dialog(this);
    dialog.exec();
    dialog.show();
}
```

通过使用exec函数使其成为模态对话框；

对于非模态对话框可以使用new操作来创建对象，通过show函数来显示，如下所示：

```
MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    QDialog *dialog = new QDialog(this);
    dialog->show();
}
```

上示代码也可建立模态对话框，如下：

```
MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    QDialog *dialog = new QDialog(this);
    dialog->setModal(true);
    dialog->show();
}
```

其与exec不一样，exec要等到对话框关闭时才会返回，而上述代码调用show后会立即返回。

Qt中使用信号和槽机制来完成对象之间的协作，使用示例如下：

```
......
public slots:
    void showChildDialog();
}
void MyWidget::showChildDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->show();
}
```

**标准对话框**：

颜色对话框示例：

```
void MyWidget::on_pushButton_clicked()
{
//    QColor color = QColorDialog::getColor(Qt::red, this, tr("颜色对话框"));
    QColorDialog dialog(Qt::red, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    dialog.exec();
    QColor color = dialog.currentColor();
    qDebug() << "color: " << color;
}
```

文本对话框示例：

```
void MyWidget::on_pushButton_5_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("文件对话框"),
                                                    "E:", tr("图片文件(*png *jpg)"));
    qDebug() << "fileName: " << fileName;
}
```

字体对话框示例：

```
void MyWidget::on_pushButton_2_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (ok) ui->pushButton_2->setFont(font);
    else qDebug() << tr("没有选择字体！");
}
```

输入对话框示例：

```
void MyWidget::on_pushButton_6_clicked()
{
    bool ok;
    QString string = QInputDialog::getText(this, tr("输入字符串对话框"), tr("请输入用户名："), QLineEdit::Normal, tr("admin"), &ok);
    if(ok) qDebug() << "string: " << string;
    int value1 = QInputDialog::getInt(this, tr("输入整数对话框"), tr("请输入-1000到1000之间的数值"), 100, -1000, 1000, 10, &ok);
    if (ok) qDebug() << "value1:" << value1;

    double value2 = QInputDialog::getDouble(this, tr("输入浮点数对话框"), tr("请输入-1000到1000之间的数值"), 0.00, -1000, 1000, 2, &ok);
    if (ok) qDebug() << "value2:" << value2;

    QStringList items;
    items << tr("条目1") << tr("条目2");
    QString item = QInputDialog::getItem(this, tr("输入条目对话框"), tr("请选择或输入一个条目"), items, 0, true, &ok);
    if (ok) qDebug() << "item:" << item;
}
```

消息对话框示例：

```
void MyWidget::on_pushButton_3_clicked()
{
    int ret1 = QMessageBox::question(this, tr("问题对话框"), tr("你了解Qt吗？"), QMessageBox::Yes, QMessageBox::No);
    if (ret1 == QMessageBox::Yes) qDebug() << tr("问题！");
    int ret2 = QMessageBox::information(this, tr("提示对话框"), tr("这是Qt书籍！"), QMessageBox::Ok);
    if (ret2 == QMessageBox::Ok) qDebug() << tr("提示！");
    int ret3 = QMessageBox::warning(this, tr("警告对话框"), tr("不能提前结束！"), QMessageBox::Abort);
    if (ret3 == QMessageBox::Abort) qDebug() << tr("警告！");
    int ret4 = QMessageBox::critical(this, tr("严重错误对话框"), tr("发现一个严重错误！现在要关闭所有文件"), QMessageBox::YesAll);
    if (ret4 == QMessageBox::YesAll) qDebug() << tr("错误！");
    QMessageBox::about(this, tr("关于对话框"), tr("Qt Creator"));
}
```

进度对话框示例：

```
void MyWidget::on_pushButton_7_clicked()
{
    QProgressDialog dialog(tr("文件复制进度"), tr("取消"), 0, 50000, this);
    dialog.setWindowTitle(tr("进度对话框"));
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();
    for (int i=0; i<50000; i++) {
        dialog.setValue(i);
        QCoreApplication::processEvents();
        if (dialog.wasCanceled()) break;
    }
    dialog.setValue(50000);
    qDebug() << tr("复制结束！");
}

```

错误消息对话框示例：

```
MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    errordlg = new QErrorMessage(this);
}


void MyWidget::on_pushButton_4_clicked()
{
    errordlg->setWindowTitle(tr("错误信息对话框"));
    errordlg->showMessage(tr("这里是出错信息"));
}
```

向导对话框示例：

```

QWizardPage* MyWidget::createPage1()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle(tr("介绍"));
    return page;
}

QWizardPage* MyWidget::createPage2()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle(tr("用户选择信息"));
    return page;
}

QWizardPage* MyWidget::createPage3()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle(tr("结束"));
    return page;
}

void MyWidget::on_pushButton_8_clicked()
{
    QWizard wizard(this);
    wizard.setWindowTitle(tr("向导对话框"));
    wizard.addPage(createPage1());
    wizard.addPage(createPage2());
    wizard.addPage(createPage3());
    wizard.exec();
}
```

### 其他窗口部件

QFrame类是带有边框的部件的基类，它的子类包括最常用的标签部件QLabel、QLCDNumber、QSplitter、QStackedWidget、QToolBox和QAbstractScrollArea类。QAbstractScrollArea类是所有带有滚动区域部件类的抽象基类。QFrame类的主要功能是用来实现不同的边框效果，由边框形状和边框阴影组合形成。

## 事件系统

在Qt中，事件作为一个对象，继承自QEvent类，常见的有键盘事件QKeyEvent、鼠标事件QMouseEvent和定时器事件QTimeEvent等。

### Qt中的事件

事件是应用程序需要知道的内部或外部发生的事情或者动作的统称。Qt中使用一个对象来表示一个事件，继承自QEvent类，任何QObject子类实例都可以接收和处理事件。

一个事件由一个特定的QEvent子类表示，但有时一个事件又包含多个类型，比如鼠标事件又包含鼠标按下、双击、移动等操作，这些事件类型可由QEvent::type来表示。有了事件还要知道如何处理事件，QCoreApplication类的notify函数的帮助文档给了五种处理事件的方法：

- 重新实现部件的paintEvent()、mousePressEvent()等事件处理函数，这是最常用的方法，但只能应用于特定部件的特定方法；
- 重新实现notify函数，可实现完全的控制，能在事件过滤器得到事件之前就获得事件，但一次只能处理一个事件；
- 向QApplication对象上安装事件过滤器，因为一个程序只有一个Application对象，所以这这样实现与实现notify的功能一致，优点是可以处理多个事件；
- 重新实现event函数，可以在事件到达默认的事件处理函数之前进行处理；
- 在对象上安装事件过滤器，使用事件过滤器可以在一个界面类中同时处理不同子部件的不同事件；

在实际的使用中，最常用的是方法一，其次是方法五，方法二需要继承QApplication类，方法三则要用一个全局的事件过滤器。

每个程序在调用QApplication类的exec函数后，会使得Qt应用程序进入事件循环，这样就可以接收各种各样的事件，一旦有事件发生，Qt就会构建一个QEvent子类的对象来表示它，然后将其传递给相应的QObject对象或其子对象。