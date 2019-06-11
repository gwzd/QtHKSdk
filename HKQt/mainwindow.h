#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSettings>
#include "dbconconfdialog.h"
#include "default.h"
#include "logindialog.h"
#include <QMutex>
#include <QTextToSpeech>
#include <QLocale>
#include <QNetworkReply>
#include <QTableWidgetItem>

namespace Ui {
class MainWindow;
}
class HKPlayWindow;
class QNetworkAccessManager;
class QTreeWidgetItem;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    virtual void timerEvent( QTimerEvent *event);
    int i_RefrushAlarm_TimeOut;
    QSqlDatabase db;
    DBConConfDialog dbconf;
    LoginDialog logdlg;
    loginInfo logininfo;
    QList<HKPlayWindow*> playwindowlist;
    quint8 Is_InNet;                        //内外网标记，如果该程序运行在外网，那么取摄像机的流媒体IP位112.54.80.211
    quint8 StreamMark;                      // 0:主码流  1:子码流
    qint32 i_AutoHandler;                   //语音对讲句柄

    bool Clientlogin(QString user,QString pass,bool login_un);
    void ClientloginOut();
    void CleanPlayWin();
    void SendMessageToPhone(QString phoneNum,QString Utf8text);
    void CreateDeviceTree();
    QString get_local_ip();
    QTreeWidgetItem* GetNodeByCode(QString code);
    HKPlayWindow* GetAFreePlayWindow();
    HKPlayWindow* GetPlayingHandler();
    QNetworkAccessManager* MessageManager;
private:
    void AlarmTable();
    void RefrushAlarm();
    void MyPlaySound(QString str_Text);
    int alarm_MaxID;
    QMutex PlaySound_Lock;
    QTextToSpeech textSpeech;
signals:
    void signal_CreateDeviceTreeOK();
    void signal_MyPlaySound(QString);
public slots:
    void slots_PlayWin_IsDBClicked(void* label);
    void slot_CreateDeviceTreeOK();
    void slot_MyPlaySound(QString);
    void slot_finishedSlot(QNetworkReply*);
private slots:
    void on_actionlogin_triggered();
    void on_actionplya_triggered();
    void on_actionstop_triggered();
    void on_actionDBConnectConf_triggered();
    void on_actionlogin_2_triggered();
    void on_toolButton_2_clicked();
    void on_toolButton_3_clicked();
    void on_toolButton_5_clicked();
    void on_toolButton_4_clicked();
    void on_toolButton_clicked();
    //void on_pushButton_SendMessage_clicked();
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_pushButton_CloseVideo_clicked();
    void on_pushButton_pressed();
    void on_pushButton_released();

    void on_pushButton_DJ_pressed();

    void on_pushButton_DJ_released();

    void on_pushButton_ZIM_clicked();

    void on_pushButton_ZHUM_clicked();

    void on_pushButton_PTZUp_pressed();

    void on_pushButton_PTZUp_released();

    void on_pushButton_Right_Up_pressed();

    void on_pushButton_Right_Up_released();

    void on_pushButton_Left_pressed();

    void on_pushButton_Left_released();

    void on_pushButton_Auto_pressed();

    void on_pushButton_Auto_released();

    void on_pushButton_Right_pressed();

    void on_pushButton_Right_released();

    void on_pushButton_8_pressed();

    void on_pushButton_8_released();

    void on_pushButton_PTZDown_pressed();

    void on_pushButton_PTZDown_released();

    void on_pushButton_PTZRightDown_pressed();

    void on_pushButton_PTZRightDown_released();

    void on_pushButton_10_pressed();

    void on_pushButton_10_released();

    void on_pushButton_11_pressed();

    void on_pushButton_11_released();

    void on_pushButton_13_pressed();

    void on_pushButton_13_released();

    void on_pushButton_12_pressed();

    void on_pushButton_12_released();

    void on_pushButton_15_clicked();

    void on_pushButton_14_clicked();

    void on_pushButton_17_clicked();

    void on_pushButton_16_clicked();

    void on_toolButton_13_clicked();

    void on_toolButton_14_clicked();

    void on_toolButton_15_clicked();

    void on_toolButton_11_clicked();

    void on_pushButton_Capture_clicked();

    void on_toolButton_12_clicked();

    void on_pushButton_LX_clicked();

    void on_pushButton_StopLX_clicked();

    void on_pushButton_MessClientRef_clicked();

    void on_pushButton_SendDX_clicked();

    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
