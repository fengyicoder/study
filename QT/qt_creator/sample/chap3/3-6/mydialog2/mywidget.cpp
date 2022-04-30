#include "mywidget.h"
#include "ui_mywidget.h"
#include <QDebug>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QErrorMessage>

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    errordlg = new QErrorMessage(this);
}

MyWidget::~MyWidget()
{
    delete ui;
}

void MyWidget::on_pushButton_clicked()
{
//    QColor color = QColorDialog::getColor(Qt::red, this, tr("颜色对话框"));
    QColorDialog dialog(Qt::red, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    dialog.exec();
    QColor color = dialog.currentColor();
    qDebug() << "color: " << color;
}

void MyWidget::on_pushButton_5_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("文件对话框"),
                                                    "E:", tr("图片文件(*png *jpg)"));
    qDebug() << "fileName: " << fileName;
}

void MyWidget::on_pushButton_2_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (ok) ui->pushButton_2->setFont(font);
    else qDebug() << tr("没有选择字体！");
}

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

void MyWidget::on_pushButton_4_clicked()
{
    errordlg->setWindowTitle(tr("错误信息对话框"));
    errordlg->showMessage(tr("这里是出错信息"));
}

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
