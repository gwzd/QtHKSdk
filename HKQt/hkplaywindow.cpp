#include "hkplaywindow.h"
#include <QDebug>
#include "HCNetSDK.h"

HKPlayWindow::HKPlayWindow(QWidget *parent) : QLabel(parent)
{
    setStyleSheet("QLabel{border-image: url(:/new/icon/gwzd.png)}");
    i_playHandler = -1;
    i_RealHandler = -1;
    isSelected = false;
}
HKPlayWindow::~HKPlayWindow(){
    //播放窗口析构
    //qDebug() << QString::fromLocal8Bit("播放窗口析构");
    if(i_playHandler >= 0){
        NET_DVR_StopRealPlay(i_playHandler);
    }
}
void HKPlayWindow::SetSelected(bool sel)
{
    if(sel== true && isSelected == sel){
        isSelected = !sel;
    }
    else{
        isSelected = sel;
    }
    if(isSelected){
        setStyleSheet("QLabel{border-image: url(:/new/icon/videoSip.png)}");
    }
    else{
        setStyleSheet("QLabel{border-image: url(:/new/icon/gwzd.png)}");
    }
}
void HKPlayWindow::SetPlaying(bool play)
{
    if(play){
        setStyleSheet("QLabel{border-image: url(:/new/icon/getVideo.png)}");
    }
    else{
        setStyleSheet("QLabel{border-image: url(:/new/icon/gwzd.png)}");
    }
}
void HKPlayWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    emit(signals_PlayWin_IsDBClicked(this));
}
