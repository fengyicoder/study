#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QWidget *widget = new QWidget();
    widget->setWindowTitle(QObject::tr("我是widget"));
    QLabel *label = new QLabel();
    label->setWindowTitle(QObject::tr("我是label"));
    label->setText(QObject::tr("我是Label"));
    label->resize(180, 20);
    QLabel *label2 = new QLabel(widget);
    label2->setText(QObject::tr("label2:我不是独立窗口，只是widget的子部件"));
    label2->resize(250, 20);
    label->show();
    widget->show();
    int ret = a.exec();
    delete label;
    delete widget;
    return ret;
}
