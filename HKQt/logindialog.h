#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString username;
    QString password;
private slots:
    void on_pushButton_Login_clicked();

    void on_pushButton_Cancel_clicked();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
