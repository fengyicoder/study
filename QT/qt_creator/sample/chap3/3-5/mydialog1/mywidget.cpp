#include "mywidget.h"
#include "ui_mywidget.h"
#include "mydialog.h"
#include <QDialog>

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
//    connect(ui->showChildButton, &QPushButton::clicked,
//            this, &MyWidget::showChildDialog);
//    QDialog *dialog = new QDialog(this);
//    dialog->setModal(true);
//    dialog->show();
}

MyWidget::~MyWidget()
{
    delete ui;
}

void MyWidget::on_showChildButton_clicked()
{
    QDialog *dialog = new QDialog(this);
    dialog->show();
}

void MyWidget::on_pushButton_clicked()
{
    close();
    MyDialog dlg;
    if(dlg.exec() == QDialog::Accepted) show();
}
