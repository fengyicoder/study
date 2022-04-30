//#include <QApplication>
//#include <QDialog>
//#include <QLabel>

#include "ui_hellodialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDialog w;
//    w.resize(400, 300);
//    QLabel label(&w);
//    label.move(120, 120);
//    label.setText(QObject::tr("Hello World! 你好 Qt!"));
    Ui::HelloDialog ui;
    ui.setupUi(&w);
    w.show();
    return a.exec();
}
