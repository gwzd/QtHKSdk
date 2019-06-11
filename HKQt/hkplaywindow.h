#ifndef HKPLAYWINDOW_H
#define HKPLAYWINDOW_H

#include <QObject>
#include <QLabel>

class HKPlayWindow : public QLabel
{
    Q_OBJECT
public:
    explicit HKPlayWindow(QWidget *parent = nullptr);
    ~HKPlayWindow();
    int i_playHandler;   //播放句柄，小于0时该窗口空闲
    int i_LoginHandler;  //摄像机的登陆句柄
    int i_RealHandler;   //录像句柄
    bool isSelected;

    void mouseDoubleClickEvent(QMouseEvent *ev);
    void SetSelected(bool sel);
    void SetPlaying(bool play);
signals:
    void signals_PlayWin_IsDBClicked(void* handler);
public slots:
};

#endif // HKPLAYWINDOW_H
