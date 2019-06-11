#include "logindialog.h"
#include "ui_logindialog.h"
#include "mainwindow.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    username = "zxl";
    password = "123456";
    ui->lineEdit_LoginUser->setText(username);
    ui->lineEdit_LoginPass->setText(password);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}
/**
 * @brief LoginDialog::on_pushButton_Login_clicked
 * @author zxl
 * @time 2018.11.27
 * @func 登陆
 */
void LoginDialog::on_pushButton_Login_clicked()
{
    username = ui->lineEdit_LoginUser->text();
    password = ui->lineEdit_LoginPass->text();
    close();
}
/**
 * @brief LoginDialog::on_pushButton_Cancel_clicked
 * @author zxl
 * @time 2018.11.27
 * @func 退出
 */
void LoginDialog::on_pushButton_Cancel_clicked()
{
    password = "";
    close();
}
