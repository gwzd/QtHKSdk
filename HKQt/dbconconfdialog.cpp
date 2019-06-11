#include "dbconconfdialog.h"
#include "ui_dbconconfdialog.h"
#include <QSettings>
#include <QMessageBox>

DBConConfDialog::DBConConfDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DBConConfDialog)
{
    ui->setupUi(this);
    //读取配置文件中的信息
    QSettings *m_IniFile = new QSettings("./conf.ini", QSettings::IniFormat);
    ui->lineEdit_DBIP->setText(m_IniFile->value("dbconf/dbip").toString());
    ui->lineEdit_DBPort->setText(m_IniFile->value("dbconf/dbport").toString());
    ui->lineEdit_DBName->setText(m_IniFile->value("dbconf/dbname").toString());
    ui->lineEdit_DBUser->setText(m_IniFile->value("dbconf/dbuser").toString());
    ui->lineEdit_DBPass->setText(m_IniFile->value("dbconf/dbpass").toString());
    m_IniFile->deleteLater();
}

DBConConfDialog::~DBConConfDialog()
{
    delete ui;
}
/**
 * @brief DBConConfDialog::on_pushButton_Set_clicked
 * @author zxl
 * @time 2018.11.27
 * @func 设置数据库连接信息
 */
void DBConConfDialog::on_pushButton_Set_clicked()
{
    QSettings *m_IniFile = new QSettings("./conf.ini", QSettings::IniFormat);
    m_IniFile ->beginGroup("dbconf");     // 设置当前节名，代表以下的操作都是在这个节中
    m_IniFile->setValue( "dbip",ui->lineEdit_DBIP->text());
    m_IniFile->setValue( "dbport",ui->lineEdit_DBPort->text());
    m_IniFile->setValue( "dbname",ui->lineEdit_DBName->text());
    m_IniFile->setValue( "dbuser",ui->lineEdit_DBUser->text());
    m_IniFile->setValue( "dbpass",ui->lineEdit_DBPass->text());
    m_IniFile->endGroup();                   // 结束当前节的操作
    m_IniFile->deleteLater();
    QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("数据库连接信息配置成功，重启后生效!"));
    close();
}

void DBConConfDialog::on_pushButton_Cancel_clicked()
{
    close();
}
