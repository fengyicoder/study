#include <QApplication>
#include <QDialog>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDialog w;
    w.resize(400, 300);
    QLabel lable(&w);
    lable.move(120, 120);
    lable.setText(QObject::tr("Hello World! 你好 QT!"));
    w.show();
    return a.exec();
}
