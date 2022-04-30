#include "mywidget.h"
#include <QApplication>
#include "mydialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyDialog dialog;
    if (dialog.exec() == QDialog::Accepted)
    {
        MyWidget w;
        w.show();
        return a.exec();
    }
    else return 0;
}
