#ifndef DBCONCONFDIALOG_H
#define DBCONCONFDIALOG_H

#include <QDialog>

namespace Ui {
class DBConConfDialog;
}

class DBConConfDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DBConConfDialog(QWidget *parent = nullptr);
    ~DBConConfDialog();

private slots:
    void on_pushButton_Set_clicked();

    void on_pushButton_Cancel_clicked();

private:
    Ui::DBConConfDialog *ui;
};

#endif // DBCONCONFDIALOG_H
