#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QImage>
#include "hkplaywindow.h"
#include <QNetworkAccessManager>
#include <QtConcurrent>
#include <QThreadPool>
#include <QUrl>
#include <QCryptographicHash>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QHostInfo>
#include <QHostAddress>
#include <QFileDialog>
#include <QTimerEvent>
#include <QHeaderView>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //read ini config,for db connect
    /*
    QImage img;
    img.load("F:\\qtProject\\HKQt\\icon\\WinSelBar.png");
    img.save("F:\\qtProject\\HKQt\\icon\\WinSelBar.png");
    */

    for(int i=1 ; i<301 ; i++){
        ui->comboBox->addItem(QString::number(i));
    }
    QSettings* m_IniFile = new QSettings("./conf.ini", QSettings::IniFormat);
    QString str_dbip = m_IniFile->value("dbconf/dbip").toString();
    QString str_dbport = m_IniFile->value("dbconf/dbport").toString();
    QString str_dbname = m_IniFile->value("dbconf/dbname").toString();
    QString str_dbuser = m_IniFile->value("dbconf/dbuser").toString();
    QString str_dbpass = m_IniFile->value("dbconf/dbpass").toString();
    //内外网设置
    Is_InNet = static_cast<quint8>(m_IniFile->value("Netconf/InNet").toInt());
    //取码流类型设置
    StreamMark = static_cast<quint8>(m_IniFile->value("Netconf/StreamType").toInt());
    if(Is_InNet != 0 && Is_InNet != 1){
        Is_InNet = 1;
    }
    if(StreamMark != 0 && StreamMark != 1){
        StreamMark = 0;
    }
    //报警最大ID
    alarm_MaxID = 0;
    //语音对讲句柄
    i_AutoHandler = -1;
    if(!str_dbip.isEmpty() && !str_dbport.isEmpty() && !str_dbname.isEmpty()
             && !str_dbuser.isEmpty() && !str_dbpass.isEmpty()){

        db = QSqlDatabase::addDatabase("QOCI");//QOCI
        //db.setConnectOptions("SQL_ATTR_CONNECTION_TIMEOUT=2");
        db.setPort(str_dbport.toInt());
        db.setHostName(str_dbip);
        db.setDatabaseName(str_dbname);
        db.setUserName(str_dbuser);
        db.setPassword(str_dbpass);
        if (!db.open()) {
            qDebug() << "connect db failed!";
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("数据库连接失败，请重新配置连接信息!"));
        }
        else{
            //qDebug() << "connect db successful!";
            qRegisterMetaType<TreeNodeData>("TreeNodeData");
            qRegisterMetaType<QVector<int>>("QVector<int>");
            qRegisterMetaType<Camera>("Camera");
            connect(this,&MainWindow::signal_CreateDeviceTreeOK,this,&MainWindow::slot_CreateDeviceTreeOK);
            connect(this,&MainWindow::signal_MyPlaySound,this,&MainWindow::slot_MyPlaySound);
            ui->treeWidget->setHeaderHidden(true);
            NET_DVR_Init();
            NET_DVR_SetConnectTime(2000, 1);
            NET_DVR_SetReconnect(10000, true);
            //设置报警表样式
            AlarmTable();
            //初始化语音系统
            textSpeech.setLocale(QLocale::Chinese);//设置语言环境
            textSpeech.setRate(0.0);
            textSpeech.setPitch(1.0);
            textSpeech.setVolume(1.0);
            textSpeech.stop();
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("获取配置文件信息失败!"));
    }
    m_IniFile->deleteLater();

    MessageManager = new QNetworkAccessManager(this);
    connect(MessageManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slot_finishedSlot(QNetworkReply*)));
    setWindowState(Qt::WindowMaximized);
    //界面布局
    ui->splitter_2->setStretchFactor(0,6);
    ui->splitter_2->setStretchFactor(1,1);

    ui->splitter_3->setStretchFactor(0,1);
    ui->splitter_3->setStretchFactor(1,5);

    ui->splitter_4->setStretchFactor(0,5);
    ui->splitter_4->setStretchFactor(1,1);

    ui->splitter->setStretchFactor(0,180);
    ui->splitter->setStretchFactor(1,1);
    on_toolButton_2_clicked();
    //隐藏第二个tab
    //ui->tabWidget->setTabEnabled(1,false);
    ui->tabWidget->tabBar()->hide();
    ui->tabWidget->removeTab(1);
}

MainWindow::~MainWindow()
{
    delete ui;
    NET_DVR_Cleanup();
    CleanPlayWin();
}
/**
 * @brief slot_finishedSlot
 * @func 短信发送，收到返回码
 */
void MainWindow::slot_finishedSlot(QNetworkReply* m_Reply)
{
    m_Reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    m_Reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (m_Reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = m_Reply->readAll();
        QString stringRet = QString::fromUtf8(bytes);
        //qDebug() << QString::fromLocal8Bit("短信发送收到返回码:") << stringRet;
        int i_ret = stringRet.toInt();
        if(i_ret == 0){
            QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("短信已发送!"));
        }
        else{
            QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("短信发送失败:") + stringRet);
        }
    }
    else
    {
        qDebug()<< m_Reply->errorString();
    }

    m_Reply->deleteLater();
}
void MainWindow::closeEvent(QCloseEvent *)
{
    ClientloginOut();
}

void MainWindow::timerEvent( QTimerEvent *event)
{
    int i_timeout = event->timerId();
    if(i_RefrushAlarm_TimeOut == i_timeout){
        //刷新报警信息
        QtConcurrent::run(QThreadPool::globalInstance(),this,&MainWindow::RefrushAlarm);
        //killTimer(i_RefrushAlarm_TimeOut);
        //RefrushAlarm();
    }
}
/**
 * @brief MainWindow::AlarmTable
 * @func 报警表布局
 */
void MainWindow::AlarmTable()
{
    qDebug() <<"set table style";
    ui->tableWidget->setColumnCount(9);
    QStringList header;
    header << QString::fromLocal8Bit("报警编号") << QString::fromLocal8Bit("系统编号") << QString::fromLocal8Bit("报警点") << QString::fromLocal8Bit("报警时间") <<
              QString::fromLocal8Bit("报警类型编号") << QString::fromLocal8Bit("报警类型") << QString::fromLocal8Bit("设备编号") << QString::fromLocal8Bit("报警序号") <<
              QString::fromLocal8Bit("报警距离");
    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);                    //列宽自适应
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);        //列宽根据内容调整
    ui->tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows);                              //设置选择方式为行
    ui->tableWidget->setSelectionMode ( QAbstractItemView::SingleSelection);                            //设置选择模式，选择单行
    ui->tableWidget->setFrameShape(QFrame::NoFrame);                                                    //设置无边框
    ui->tableWidget->setShowGrid(false);                                                                //设置不显示格子线
    ui->tableWidget->setStyleSheet("selection-background-color:lightblue;");                            //设置选中背景色
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);                                //设置不可编辑
    ui->tableWidget->setAlternatingRowColors(true);                                                     //设置交替行颜色
    ui->tableWidget->setColumnHidden(0,true);                                                           //隐藏第0列
    ui->tableWidget->setColumnHidden(1,true);                                                           //隐藏第1列
    ui->tableWidget->setColumnHidden(4,true);                                                           //隐藏第4列
    ui->tableWidget->setColumnHidden(6,true);                                                           //隐藏第6列
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:rgb(0,101,105);}");    //设置表头背景色
    ui->tableWidget->horizontalHeader()->setHighlightSections(false);
}
/**
 * @brief MainWindow::RefrushAlarm
 * @func 刷新报警条目
 * @time 2018.12.17
 * @author zxl
 */
void MainWindow::RefrushAlarm()
{
    //获取报警之前先把报警恢复得删除掉
    //qDebug() << QString::fromLocal8Bit("获取新的报警信息");
    int i_PlaySountTimes = 0;                       //一轮报警查询播放报警得次数
    int i_rowCount = ui->tableWidget->rowCount();
    for(int i=i_rowCount ; i>0 ; --i){
        QTableWidgetItem* TableitemTypeID = ui->tableWidget->item(i-1,4);
        int i_ID = TableitemTypeID->text().toInt();
        //qDebug() << QString::fromLocal8Bit("打印报警类型:") << i_ID;
        if(i_ID == -1){
            ui->tableWidget->removeRow(i-1);
        }
    }
    //获取新的报警信息
    QSqlQuery query(db);
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString Yesterday_date =current_date_time.addDays(-1).toString("yyyy-MM-dd hh:mm:ss");
    QString str_nodeid = logininfo.RootNodeID;
    /*
    QString str_Sql = "select a.ID,a.ANTIID,a.timestamp,a.ALARMUNIT_TYPE,a.ALARMUNIT_NO,a.ALARMNO,a.ACQUISITION_DISTANCE,a.ALARM_PICPATH,b.nodename from antihisdata a,maingroup b where a.timestamp>to_date('"+Yesterday_date+"','yyyy-mm-dd hh24:mi:ss') "
                      "and a.end_timestamp is null and a.antiid=b.nodeid and a.antiid in (select nodeid from maingroup start with "
                      "nodeid='"+str_nodeid+"' connect by prior nodeid=pid) order by id";*/
    QString str_Sql = "select a.id,a.ANTIID,a.timestamp,a.ALARMUNIT_TYPE,a.ALARMUNIT_NO,a.ALARMNO,a.ACQUISITION_DISTANCE,a.ALARM_PICPATH,b.NODENAME from ANTIHISDATA a,maingroup b where "
                            "a.antiid=b.NODEID and a.id in (select max(id) from ANTIHISDATA where ANTIID in "
                            "(select nodeid from maingroup start with nodeid=(select rootid from login where username=?) "
                            "connect by PRIOR nodeid=pid) and end_timestamp is null and "
                            "timestamp>to_date('"+Yesterday_date+"','yyyy-mm-dd hh24:mi:ss') group by ANTIID,ALARMUNIT_type) order by a.id";
    query.prepare(str_Sql);
    query.bindValue(0,logdlg.username);
    //qDebug() << "print refrush alarm sql:" << str_Sql;
    query.exec();
    //qDebug() << db.lastError().text();
    QList<int> list_ID;
    //int alarm_MaxID_BackUp = alarm_MaxID;
    int i_SoundTimes = ui->lineEdit_SoundTimes->text().toInt();
    if(i_SoundTimes < 1 || i_SoundTimes > 20){
        i_SoundTimes = 10;
    }
    while(query.next()){
        int i_ID = query.value(0).toInt();
        list_ID.append(i_ID);
        QString str_AntiID = query.value(1).toString();
        QString str_AlarmTime = query.value(2).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        int i_AlarmType = query.value(3).toInt();   //报警类型
        int i_AlarmUNo = query.value(4).toInt();    //报警设备编号
        int i_AlarmNo = query.value(5).toInt();     //报警序号
        int i_AlarmJL = query.value(6).toInt();     //报警距离
        QString str_AlarmPicPath = query.value(7).toString();   //报警图片路径
        QString str_AlarmAddr = query.value(8).toString();      //报警地点
        //qDebug() << "布置报警:" << i_ID << " ---- " << alarm_MaxID;
        if(i_ID > alarm_MaxID){
            //新来的报警.添加到表中
            alarm_MaxID = i_ID;
            ui->tableWidget->insertRow(0);
            ui->tableWidget->setItem(0,0,new QTableWidgetItem(QString::number(i_ID)));
            ui->tableWidget->setItem(0,1,new QTableWidgetItem(str_AntiID));
            ui->tableWidget->setItem(0,2,new QTableWidgetItem(str_AlarmAddr));
            ui->tableWidget->setItem(0,3,new QTableWidgetItem(str_AlarmTime));
            ui->tableWidget->setItem(0,4,new QTableWidgetItem(QString::number(i_AlarmType)));
            QTableWidgetItem* item = nullptr;
            QString str_BJ;
            switch(i_AlarmType){
            case 0:
                str_BJ = QString::fromLocal8Bit("水平扫描");
                item = new QTableWidgetItem(str_BJ);
                item->setTextColor(QColor(139,76,57));
                ui->tableWidget->setItem(0,5,item);
                break;
            case 1:
                str_BJ = QString::fromLocal8Bit("垂直扫描");
                item = new QTableWidgetItem(str_BJ);
                item->setTextColor(QColor(139,76,57));
                ui->tableWidget->setItem(0,5,item);
                break;
            case 2:
                str_BJ = QString::fromLocal8Bit("高压防触碰");
                item = new QTableWidgetItem(str_BJ);
                item->setTextColor(QColor(255,0,0));
                ui->tableWidget->setItem(0,5,item);
                break;
            case 3:
                str_BJ = QString::fromLocal8Bit("激光探测");
                item = new QTableWidgetItem(str_BJ);
                item->setTextColor(QColor(255,246,143));
                ui->tableWidget->setItem(0,5,item);
                break;
            default:
                str_BJ = QString::fromLocal8Bit("其他类型");
                item = new QTableWidgetItem(str_BJ);
                item->setTextColor(QColor(255,246,143));
                ui->tableWidget->setItem(0,5,item);
                break;
            }
            ui->tableWidget->setItem(0,6,new QTableWidgetItem(QString::number(i_AlarmUNo)));
            ui->tableWidget->setItem(0,7,new QTableWidgetItem(QString::number(i_AlarmNo)));
            ui->tableWidget->setItem(0,8,new QTableWidgetItem(QString::number(i_AlarmJL)));
            //MyPlaySound(QString::fromLocal8Bit("你有新的报警!"));
            if(ui->comboBox_2->currentIndex() == 0){
                QString str_Sound = str_AlarmAddr + QString::fromLocal8Bit("第") + QString::number(i_AlarmNo) + QString::fromLocal8Bit("次") + QString::fromLocal8Bit("发出") + str_BJ + QString::fromLocal8Bit("报警");
                if(i_PlaySountTimes < i_SoundTimes){
                    //防止系统刚启动得时候有好多报警，会导致一个劲得喊话
                    emit(signal_MyPlaySound(str_Sound));
                    i_PlaySountTimes ++;
                }
            }
        }
        else{
            //qDebug() << QString::fromLocal8Bit("i_ID < alarm_MaxID");
        }
    }
    //检查报警恢复
    for(int i=0;i<ui->tableWidget->rowCount();i++){
        QTableWidgetItem* Tableitem = ui->tableWidget->item(i,0);
        int i_Id = Tableitem->text().toInt();
        bool b_getID = false;
        for(int j=0 ; j<list_ID.size() ; j++){
            if(list_ID.at(j) == i_Id){
                b_getID = true;
                break;
            }
        }
        if(!b_getID){
            //原来得报警现在没有了，说明已经报警恢复了。
            //删除该行，并且播放报警恢复语音
            QTableWidgetItem* TableitemTypeText = ui->tableWidget->item(i,5);
            QTableWidgetItem* TableitemTypeID = ui->tableWidget->item(i,4);
            TableitemTypeID->setText("-1");
            QString str_AlarmType = TableitemTypeText->text();
            TableitemTypeText->setText(QString::fromLocal8Bit("报警恢复"));
            TableitemTypeText->setTextColor(QColor(0,101,105));
            if(ui->comboBox_2->currentIndex() == 0){
                QString str_Sound = ui->tableWidget->item(i,2)->text() + QString::fromLocal8Bit("第") + ui->tableWidget->item(i,7)->text() + QString::fromLocal8Bit("次") + str_AlarmType + QString::fromLocal8Bit("报警恢复");
                if(i_PlaySountTimes < i_SoundTimes){
                    //防止系统刚启动得时候有好多报警，会导致一个劲得喊话
                    emit(signal_MyPlaySound(str_Sound));
                    i_PlaySountTimes ++;
                }
            }
        }
    }
}
/**
 * @brief MainWindow::MyPlaySound
 * @func 播放语音
 */
void MainWindow::MyPlaySound(QString str_Text)
{
    //QMutexLocker lock(&PlaySound_Lock);
    if(textSpeech.state() == QTextToSpeech::Ready){
        //qDebug() << "textSpeech state is yes!";
        textSpeech.say(str_Text);
        //textSpeech.stop();
    }
    else{
        qDebug() << "textSpeech state is no!";
        textSpeech.stop();
        textSpeech.say(QString::fromLocal8Bit("语音通道被占用!"));
    }
}
/**
 * @brief MainWindow::slot_MyPlaySound
 * @func 播放语音只能在进程中进行，线程中无法播放
 */
void MainWindow::slot_MyPlaySound(QString str_text)
{
    MyPlaySound(str_text);
}
/**
 * @brief MainWindow::Clientlogin
 * @param user
 * @param pass
 * @return 登陆信息
 * @author zxl
 * @time 2018.11.27
 * @func 客户端登陆
 */
bool MainWindow::Clientlogin(QString user,QString pass,bool login_un)
{
    if(logininfo.username.isEmpty()){
        db.transaction();
        QSqlQuery query(db);
        query.prepare("update login set loginstate=? where username=? and password=?");
        query.bindValue(0,login_un?1:0);
        query.bindValue(1,user);
        query.bindValue(2,pass);
        if(query.exec()){
            query.prepare("insert into loginhis values (SEQLOGINHISID.NEXTVAL,?,sysdate,?,1)");
            query.bindValue(0,user);
            query.bindValue(1,get_local_ip());
            query.exec();

            query.prepare("select username,userlimit,realname,telphone,address,rootid from login where username=? and password=?");
            query.bindValue(0,user);
            query.bindValue(1,pass);
            query.exec();
            if(query.next()){
                logininfo.username = query.value(0).toString();
                logininfo.Limit = query.value(1).toInt();
                logininfo.RealName = query.value(2).toString();
                logininfo.PhoneNumber = query.value(3).toString();
                logininfo.Addr = query.value(4).toString();
                logininfo.RootNodeID = query.value(5).toString();
                db.commit();
                return true;
            }
            else{
                qDebug() << QString::fromLocal8Bit("查询用户信息失败!") << db.lastError().text();
                db.rollback();
                return false;
            }
        }
        else{
            QSqlError err = db.lastError();
            qDebug() << QString::fromLocal8Bit("更新用户名的在线状态失败!") << err.text();
            return false;
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("已经登陆过了，无需重复登陆!"));
        return false;
    }
}
/**
 * @brief MainWindow::get_local_ip
 * @return
 * @author zxl
 * @time 2018.11.30
 * @func 获取本机IP
 */
QString MainWindow::get_local_ip()
{
    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());

   // 找出一个IPv4地址即返回
   foreach(QHostAddress address,info.addresses())
   {
       if(address.protocol() == QAbstractSocket::IPv4Protocol)
       {
           return address.toString();
       }
   }

   return "0.0.0.0";
}
/**
 * @brief MainWindow::ClientloginOut
 * @author zxl
 * @time 2018.11.30
 * @func 退出登陆
 */
void MainWindow::ClientloginOut()
{
    if(!logininfo.username.isEmpty()){
        //需要退出登陆
        QSqlQuery query(db);
        query.prepare("update login set loginstate=? where username=?");
        query.bindValue(0,0);
        query.bindValue(1,logininfo.username);
        if(query.exec())
        {
            query.prepare("insert into loginhis values (SEQLOGINHISID.NEXTVAL,?,sysdate,?,1)");
            query.bindValue(0,logininfo.username);
            query.bindValue(1,get_local_ip());
            query.exec();
        }
    }
}
void MainWindow::on_actionlogin_triggered()
{

}

void MainWindow::on_actionplya_triggered()
{

}

void MainWindow::on_actionstop_triggered()
{

}
/**
 * @brief MainWindow::on_actionDBConnectConf_triggered
 * @author zxl
 * @time 2018.11.27
 * @func  数据库连接配置
 */
void MainWindow::on_actionDBConnectConf_triggered()
{
    dbconf.exec();
}

void MainWindow::on_actionlogin_2_triggered()
{
    logdlg.exec();
    if(!logdlg.username.isEmpty() && !logdlg.password.isEmpty()){
        if(!Clientlogin(logdlg.username,logdlg.password,true)){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("登陆失败!"));
        }
        else{
            //查询设备树
            QtConcurrent::run(QThreadPool::globalInstance(),this,&MainWindow::CreateDeviceTree);
            //CreateDeviceTree();
            //开启刷新报警定时器
            //RefrushAlarm();
            QtConcurrent::run(QThreadPool::globalInstance(),this,&MainWindow::RefrushAlarm);
            i_RefrushAlarm_TimeOut = startTimer(10000);//10s刷新一次
            QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("登陆成功!"));
        }
    }
}
/**
 * @brief MainWindow::CreateDeviceTree
 * @author zxl
 * @time 2018.11.30
 * @func 创建设备树,配合oracle的树形查询，一定可以保证在布置子节点之前父节点肯定已经存在与设备树上了，否则下面的while循环组成设备树是不对的
 */
void MainWindow::CreateDeviceTree()
{
    QSqlQuery query(db);
    query.prepare("select * from maingroup start with nodeid=? connect by prior nodeid=PID");
    query.bindValue(0,logininfo.RootNodeID);
    query.exec();
    while(query.next()){
        QTreeWidgetItem* item = nullptr;
        TreeNodeData nodeData;
        nodeData.Code = query.value(0).toString();
        QString str_NodeName = query.value(1).toString();
        QString str_PID = query.value(2).toString();
        nodeData.NodeTable = query.value(3).toString();
        if(str_PID == nullptr || str_PID.isEmpty() || nodeData.Code.compare(logininfo.RootNodeID) == 0){
            //根节点
            item = new QTreeWidgetItem(ui->treeWidget,QStringList(str_NodeName));
        }
        else{
            //叶子节点
            QTreeWidgetItem* Pitem = GetNodeByCode(str_PID);
            if(Pitem)
            {
                item = new QTreeWidgetItem(Pitem,QStringList(str_NodeName));
            }
        }
        if(item){
            QVariant variant = QVariant::fromValue(nodeData);
            item->setData(0,Qt::UserRole,variant);
            if(nodeData.NodeTable.compare("AREACTRL") == 0){
                item->setIcon(0,QIcon(":new/icon/TreeHome.png"));
            }
            else if(nodeData.NodeTable.compare("LINECTRL") == 0){
                item->setIcon(0,QIcon(":new/icon/lineNode.png"));
            }
            else if(nodeData.NodeTable.compare("TOWER") == 0){
                item->setIcon(0,QIcon(":new/icon/towerNode.png"));
            }
            else if(nodeData.NodeTable.compare("ANTICTRL") == 0){
                item->setIcon(0,QIcon(":new/icon/breachNode1.png"));
            }
            else if(nodeData.NodeTable.compare("CAMERA") == 0){
                item->setIcon(0,QIcon(":new/icon/CameraOnline.png"));
            }
            else{
                item->setIcon(0,QIcon(":new/icon/TreeHome.png"));
            }
        }
        else{
            qDebug() << QString::fromLocal8Bit("节点生成失败!");
        }
    }
    emit(signal_CreateDeviceTreeOK());
}
/**
 * @brief MainWindow::slot_CreateDeviceTreeOK
 * @author zxl
 * @time 2018.11.30
 * @func 设备树组成函数运行在线程中，布置完设备树后发送信号告诉主进程
 */
void MainWindow::slot_CreateDeviceTreeOK()
{
    ui->treeWidget->expandAll();
}
/**
 * @brief MainWindow::GetNodeByCode
 * @return
 * @author zxl
 * @time 2018.11.30
 * @func 通过节点CODE 获取设备树节点
 */
QTreeWidgetItem* MainWindow::GetNodeByCode(QString code)
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it) {
        QVariant variant = (*it)->data(0,Qt::UserRole);
        TreeNodeData nodeData = variant.value<TreeNodeData>();
        if(code.compare(nodeData.Code) == 0){
            return *it;
        }
        ++it;
    }
    return nullptr;
}
/**
 * @brief MainWindow::CleanPlayWin
 * @author zxl
 * @time 2018.11.28
 * @func 清理播放窗口
 */
void MainWindow::CleanPlayWin()
{
    foreach(HKPlayWindow *w,playwindowlist)
    {
        if(w)
        {
            w->deleteLater();
            w = nullptr;
        }
    }
    playwindowlist.clear();
}
/**
 * @brief MainWindow::GetAFreePlayWindow
 * @return
 * @author zxl
 * @time 2018.11.29
 * @func 获取一个空闲的播放窗口
 */
HKPlayWindow* MainWindow::GetAFreePlayWindow()
{
    HKPlayWindow* freeWin = nullptr;
    for(int i=0 ; i<playwindowlist.size() ; i++){
        HKPlayWindow* ListWin = playwindowlist.at(i);
        if(ListWin->i_playHandler < 0){
            if(ListWin->isSelected){
                freeWin = ListWin;
                break;
            }
            else{
                if(freeWin == nullptr){
                    freeWin = ListWin;
                }
            }
        }
    }
    return freeWin;
}
/**
 * @brief MainWindow::GetPlayingHandler
 * @return
 * @author zxl
 * @time 2018.12.1
 * @func 获取播放句柄
 */
HKPlayWindow* MainWindow::GetPlayingHandler()
{
    HKPlayWindow* freeWin = nullptr;
    for(int i=0 ; i<playwindowlist.size() ; i++){
        HKPlayWindow* ListWin = playwindowlist.at(i);
        if(ListWin->i_playHandler >= 0){
            if(ListWin->isSelected){
                freeWin = ListWin;
                break;
            }
            else{
                if(freeWin == nullptr && ListWin->i_playHandler >= 0){
                    freeWin = ListWin;
                }
            }
        }
    }
    return freeWin;
}
/**
 * @brief MainWindow::slots_PlayWin_IsDBClicked
 * @param label
 * @author zxl
 * @time 2018.11.29
 * @func 槽函数
 */
void MainWindow::slots_PlayWin_IsDBClicked(void* label)
{
    HKPlayWindow* playWin = reinterpret_cast<HKPlayWindow*>(label);
    foreach(HKPlayWindow* win,playwindowlist){
        if(playWin == win){
            win->SetSelected(true);
        }
        else{
            win->SetSelected(false);
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_2_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 窗口切换-1窗口
 */
void MainWindow::on_toolButton_2_clicked()
{
    if(playwindowlist.size() != 1){
        ui->toolButton_2->setStyleSheet("QToolButton{background-color:blue;border-style: outset;}"
                                               "QToolButton:hover{background-color:rgb(160,32,240);}"
                                               "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                            "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_3->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_4->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_5->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        CleanPlayWin();
        HKPlayWindow *playWin = new HKPlayWindow(this);
        connect(playWin,&HKPlayWindow::signals_PlayWin_IsDBClicked,this,&MainWindow::slots_PlayWin_IsDBClicked);
        playwindowlist.append(playWin);
        ui->gridLayout->addWidget(playWin);
    }
}
/**
 * @brief MainWindow::on_toolButton_3_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 窗口切换-4窗口
 */
void MainWindow::on_toolButton_3_clicked()
{
    if(playwindowlist.size() != 4){
        ui->toolButton_3->setStyleSheet("QToolButton{background-color:blue;border-style: outset;}"
                                               "QToolButton:hover{background-color:rgb(160,32,240);}"
                                               "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                            "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_2->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_4->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_5->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        CleanPlayWin();
        for(int i=0 ; i<4 ; i++){
            HKPlayWindow *playVideo = new HKPlayWindow(this);
            connect(playVideo,&HKPlayWindow::signals_PlayWin_IsDBClicked,this,&MainWindow::slots_PlayWin_IsDBClicked);
            ui->gridLayout->addWidget(playVideo,i/2,i%2);
            playwindowlist.append(playVideo);
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_5_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 窗口切换-6窗口
 */
void MainWindow::on_toolButton_5_clicked()
{
    if(playwindowlist.size() != 6){
        ui->toolButton_5->setStyleSheet("QToolButton{background-color:blue;border-style: outset;}"
                                               "QToolButton:hover{background-color:rgb(160,32,240);}"
                                               "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                            "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_2->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_4->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_3->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        CleanPlayWin();

        for(int i=0 ; i<6 ; i++){
            HKPlayWindow *playVideo = new HKPlayWindow(this);
            connect(playVideo,&HKPlayWindow::signals_PlayWin_IsDBClicked,this,&MainWindow::slots_PlayWin_IsDBClicked);
            if(i == 4){
                ui->gridLayout->addWidget(playVideo,i/3,i%3,2,2);
            }
            else if(i == 5){
                ui->gridLayout->addWidget(playVideo,2,0);
            }
            else{
                ui->gridLayout->addWidget(playVideo,i/3,i%3);
            }
            playwindowlist.append(playVideo);
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_4_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 窗口切换-9窗口
 */
void MainWindow::on_toolButton_4_clicked()
{
    if(playwindowlist.size() != 9){
        ui->toolButton_4->setStyleSheet("QToolButton{background-color:blue;border-style: outset;}"
                                               "QToolButton:hover{background-color:rgb(160,32,240);}"
                                               "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                            "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_2->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_3->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_5->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        CleanPlayWin();
        for(int i=0 ; i<9 ; i++){
            HKPlayWindow *playVideo = new HKPlayWindow(this);
            connect(playVideo,&HKPlayWindow::signals_PlayWin_IsDBClicked,this,&MainWindow::slots_PlayWin_IsDBClicked);
            ui->gridLayout->addWidget(playVideo,i/3,i%3);
            playwindowlist.append(playVideo);
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_4_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 窗口切换-13窗口
 */
void MainWindow::on_toolButton_clicked()
{
    if(playwindowlist.size() != 13){
        ui->toolButton->setStyleSheet("QToolButton{background-color:blue;border-style: outset;}"
                                               "QToolButton:hover{background-color:rgb(160,32,240);}"
                                               "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_3->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                            "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_2->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_4->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        ui->toolButton_5->setStyleSheet("QToolButton{background-color:green;border-style: outset;}"
                                        "QToolButton:hover{background-color:rgb(160,32,240);}"
                                            "QToolButton:pressed{background-color:blue;border-style: inset; }");
        CleanPlayWin();
        for(int i=0 ; i<13 ; i++){
            HKPlayWindow *playVideo = new HKPlayWindow(this);
            connect(playVideo,&HKPlayWindow::signals_PlayWin_IsDBClicked,this,&MainWindow::slots_PlayWin_IsDBClicked);
            if(i<5){
                ui->gridLayout->addWidget(playVideo,i/4,i%4);
            }
            else if(i == 5){
                ui->gridLayout->addWidget(playVideo,1,1,2,2);
            }
            else if(i == 6){
                ui->gridLayout->addWidget(playVideo,1,3);
            }
            else if(i==7){
                ui->gridLayout->addWidget(playVideo,2,0);
            }
            else{
                int a = i+3;
                ui->gridLayout->addWidget(playVideo,a/4,a%4);
            }
            playwindowlist.append(playVideo);
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_SendMessage_clicked
 * @author zxl
 * @time 2018.11.28
 * @func 发送短信
 */
/*
void MainWindow::on_pushButton_SendMessage_clicked()
{
    QString str_PhoneNumber = ui->lineEdit_PhoneNumber->text();
    QString str_Neirong = ui->textEdit->toPlainText();
    //qDebug() << "手机号:" << str_PhoneNumber;
    //qDebug() << "内容:" << str_Neirong;
    //QtConcurrent::run(QThreadPool::globalInstance(),this,&MainWindow::SendMessageToPhone,str_PhoneNumber,str_Neirong);
    SendMessageToPhone(str_PhoneNumber,str_Neirong);
}
*/
/**
 * @brief MainWindow::SendMessageToPhone
 * @param phoneNum
 * @param text
 * @author zxl
 * @time 2018.11.28
 * @func 发送短信 实现函数
 */
void MainWindow::SendMessageToPhone(QString phoneNum,QString text)
{
    qDebug() << "message info :" << text;
    QString str_Pass;
    QString pwd="700908";
    QByteArray bb;
    bb = QCryptographicHash::hash(pwd.toUtf8(),QCryptographicHash::Md5);
    str_Pass.append(bb.toHex());
    str_Pass = str_Pass.toUpper();
    QString str_Url_buf = "http://api.esms100.com:8080/mt/?cpid=2026276&cppwd=%1&phone=%2&msgtext=%3&encode=utf8";
    QString str_Url = str_Url_buf.arg(str_Pass).arg(phoneNum).arg(text);
    //qDebug() << "url:" <<str_Url;
    str_Url = QUrl::fromPercentEncoding(str_Url.toUtf8());
    //QUrl url("");
    //qDebug() << "url:" << str_Url;
    QUrl url(str_Url);
    QNetworkRequest request;
    request.setUrl(url);
    MessageManager->get(request);
    //QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("短信已发送!"));
}
/**
 * @brief MainWindow::on_treeWidget_itemDoubleClicked
 * @param item
 * @param column
 * @author zxl
 * @time 2018.11.30
 * @func 设备树节点双击触发，摄像机节点播放视频
 */
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int )
{
    HKPlayWindow* playWin = GetAFreePlayWindow();
    if(playWin){
        playWin->SetPlaying(true);
        QVariant variant = item->data(0,Qt::UserRole);
        TreeNodeData nodedata = variant.value<TreeNodeData>();
        QSqlQuery query(db);
        if(nodedata.NodeTable.compare("CAMERA") == 0){
            QVariant variantIn = item->data(1,Qt::UserRole);
            Camera camInfo = variantIn.value<Camera>();
            qDebug() << camInfo.UserName;
            if(camInfo.UserName.isNull() || camInfo.UserName.isEmpty()){
                qDebug() << QString::fromLocal8Bit("该摄像机没有绑定1号位信息，需要从数据库获取!");
                query.prepare("select * from camera where id=?");
                query.bindValue(0,nodedata.Code);
                query.exec();
                if(query.next()){
                    Camera camera;
                    camera.CameraCode = query.value(0).toString();
                    camera.Factory = query.value(1).toInt();
                    camera.UserName = query.value(2).toString();
                    camera.Password = query.value(3).toString();
                    camera.OutNetIPV4 = query.value(4).toString();
                    camera.OutNetIPV6 = query.value(5).toString();
                    camera.OutNetPort = static_cast<quint16>(query.value(6).toInt());
                    camera.InNetIPV4 = query.value(7).toString();
                    camera.InNetIPV6 = query.value(8).toString();
                    camera.InNetPort = static_cast<quint16>(query.value(9).toInt());
                    camera.LoginID = -1;
                    //camera.PlayID = -1;
                    QVariant variantBind = QVariant::fromValue(camera);
                    item->setData(1,Qt::UserRole,variantBind);
                    camInfo = camera;
                }
                else{
                    qWarning() << QString::fromLocal8Bit("数据库中没有查到该摄像机的信息");
                }
            }
            else{
                qDebug() << QString::fromLocal8Bit("该摄像机已经绑定1号位信息，可以登陆摄像机并且获取流媒体!");
            }
            if(!camInfo.UserName.isNull() && !camInfo.UserName.isEmpty()){
                if(camInfo.LoginID < 0)
                {
                    //摄像机未登录
                    if(Is_InNet == 1){
                        QByteArray baIP = camInfo.InNetIPV4.toLatin1();
                        char *CamIp = baIP.data();
                        QByteArray baUserName = camInfo.UserName.toLatin1();
                        char *UserName = baUserName.data();
                        QByteArray baPass = camInfo.Password.toLatin1();
                        char *PassWord = baPass.data();
                        camInfo.LoginID = NET_DVR_Login_V30(CamIp,camInfo.InNetPort,UserName,PassWord,&camInfo.CameraLoginInfo);
                    }
                    else{
                        QByteArray baIP = camInfo.OutNetIPV4.toLatin1();
                        char *CamIp = baIP.data();
                        QByteArray baUserName = camInfo.UserName.toLatin1();
                        char *UserName = baUserName.data();
                        QByteArray baPass = camInfo.Password.toLatin1();
                        char *PassWord = baPass.data();
                        camInfo.LoginID = NET_DVR_Login_V30(CamIp,camInfo.OutNetPort,UserName,PassWord,&camInfo.CameraLoginInfo);
                    }
                    QVariant variantBind = QVariant::fromValue(camInfo);
                    item->setData(1,Qt::UserRole,variantBind);
                    if(camInfo.LoginID < 0){
                        //qDebug() << "Camera login failed!";
                        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("摄像机登陆失败!"));
                    }
                    else{
                        qDebug() << "Camera login success!";
                    }
                }
                else{
                    qDebug() << QString::fromLocal8Bit("该摄像机已经登陆了,可以播放视频了!");
                }
                //播放
                if(camInfo.LoginID >= 0){

                    int lRealHandle = -1;
                    if(Is_InNet == 1){
                        //内网模式，该模式下取视频流会向554端口通信,端口映射很难做
                        NET_DVR_PREVIEWINFO tmpclientinfo;//ui->label->winId()
                        tmpclientinfo.hPlayWnd=reinterpret_cast<HWND>(playWin->winId());//需要 SDK 解码时句柄设为有效值，仅取流不解码时可设为空
                        tmpclientinfo.lChannel=1;//预览通道号
                        tmpclientinfo.dwStreamType=StreamMark;//0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
                        tmpclientinfo.dwLinkMode = 0; //0- TCP 方式，1- UDP 方式，2- 多播方式，3- RTP 方式，4-RTP/RTSP，5-RSTP/HTTP
                        QFuture<long> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_RealPlay_V40,camInfo.LoginID,&tmpclientinfo,nullptr,nullptr);
                        lRealHandle = future.result();
                        //lRealHandle = NET_DVR_RealPlay_V40(camInfo.LoginID,&tmpclientinfo,nullptr,nullptr);
                    }
                    else{
                        //外网模式,该模式下登陆与取视频流只用到了8000端口
                        NET_DVR_CLIENTINFO ClientInfo;
                        ClientInfo.hPlayWnd     = reinterpret_cast<HWND>(playWin->winId());
                        ClientInfo.lChannel     = 1;
                        ClientInfo.lLinkMode    = 0;
                        ClientInfo.sMultiCastIP = nullptr;
                        //lRealHandle = NET_DVR_RealPlay_V30(camInfo.LoginID,&ClientInfo,nullptr,nullptr,TRUE);
                        QFuture<long> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_RealPlay_V30,camInfo.LoginID,&ClientInfo,nullptr,nullptr,TRUE);
                        lRealHandle = future.result();
                    }
                    if(lRealHandle < 0){
                        qDebug() << "play video failed!" << NET_DVR_GetLastError();
                        playWin->SetPlaying(false);
                        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("视频播放失败!"));
                    }
                    else{
                        qDebug() << "plya video success!";
                        playWin->i_playHandler = lRealHandle;
                        playWin->i_LoginHandler = camInfo.LoginID;
                    }
                }
                else{
                    playWin->SetPlaying(false);
                    qWarning() << QString::fromLocal8Bit("摄像机没有登陆，无法播放视频");
                }
            }
            else{
                qWarning() << QString::fromLocal8Bit("双击摄像机节点播放视频操作失败，无法获取到摄像机的信息!");
            }
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("播放窗口已满!"));
    }
}
/**
 * @brief MainWindow::on_pushButton_CloseVideo_clicked
 * @author zxl
 * @time 2018.12.1
 * @func 关闭视频
 */
void MainWindow::on_pushButton_CloseVideo_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_StopRealPlay(playWin->i_playHandler);
        playWin->i_playHandler = -1;
        playWin->i_LoginHandler = -1;
        playWin->i_RealHandler = -1;
        playWin->clear();
        playWin->SetPlaying(false);
    }
}
/**
 * @brief MainWindow::on_pushButton_DJ_pressed
 * @author zxl
 * @time 2018.12.01
 * @func 语音对讲,开启
 */
void MainWindow::on_pushButton_DJ_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(i_AutoHandler == -1 && playWin){
        i_AutoHandler = NET_DVR_StartVoiceCom_V30(playWin->i_LoginHandler,1,false,nullptr,nullptr);
    }
}
/**
 * @brief MainWindow::on_pushButton_DJ_released
 * @author zxl
 * @time 2018.12.01
 * @func 语音对讲,停止
 */
void MainWindow::on_pushButton_DJ_released()
{
    NET_DVR_StopVoiceCom(i_AutoHandler);
    i_AutoHandler = -1;
}
/**
 * @brief MainWindow::on_pushButton_ZIM_clicked
 * @author zxl
 * @time 2018.12.01
 * @func 子码流播放
 */
void MainWindow::on_pushButton_ZIM_clicked()
{
    if(Is_InNet == 1){
        HKPlayWindow* playWin = GetPlayingHandler();
        if(playWin){
            if(NET_DVR_StopRealPlay(playWin->i_playHandler))
            {
                NET_DVR_PREVIEWINFO tmpclientinfo;//ui->label->winId()
                tmpclientinfo.hPlayWnd=reinterpret_cast<HWND>(playWin->winId());//需要 SDK 解码时句柄设为有效值，仅取流不解码时可设为空
                tmpclientinfo.lChannel=1;//预览通道号
                tmpclientinfo.dwStreamType=1;//0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
                tmpclientinfo.dwLinkMode = 1; //0- TCP 方式，1- UDP 方式，2- 多播方式，3- RTP 方式，4-RTP/RTSP，5-RSTP/HTTP

                //playWin->i_playHandler = NET_DVR_RealPlay_V40(playWin->i_LoginHandler,&tmpclientinfo,nullptr,nullptr);
                QFuture<long> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_RealPlay_V40,playWin->i_LoginHandler,&tmpclientinfo,nullptr,nullptr);
                playWin->i_playHandler = future.result();

                if(playWin->i_playHandler < 0){
                    QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("码流切换失败!"));
                }
            }
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("抱歉-限于外网端口映射问题，在外网环境下的客户端无法切换码流!"));
    }
}
/**
 * @brief MainWindow::on_pushButton_ZHUM_clicked
 * @author zxl
 * @time 2018.12.01
 * @func 主码流播放
 */
void MainWindow::on_pushButton_ZHUM_clicked()
{
    if(Is_InNet == 1){
        HKPlayWindow* playWin = GetPlayingHandler();
        if(playWin){
            if(NET_DVR_StopRealPlay(playWin->i_playHandler)){
                NET_DVR_PREVIEWINFO tmpclientinfo;//ui->label->winId()
                tmpclientinfo.hPlayWnd=reinterpret_cast<HWND>(playWin->winId());//需要 SDK 解码时句柄设为有效值，仅取流不解码时可设为空
                tmpclientinfo.lChannel=1;//预览通道号
                tmpclientinfo.dwStreamType=0;//0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
                tmpclientinfo.dwLinkMode = 1; //0- TCP 方式，1- UDP 方式，2- 多播方式，3- RTP 方式，4-RTP/RTSP，5-RSTP/HTTP

                //playWin->i_playHandler = NET_DVR_RealPlay_V40(playWin->i_LoginHandler,&tmpclientinfo,nullptr,nullptr);
                QFuture<long> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_RealPlay_V40,playWin->i_LoginHandler,&tmpclientinfo,nullptr,nullptr);
                playWin->i_playHandler = future.result();
                if(playWin->i_playHandler < 0){
                    QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("码流切换失败!"));
                }
            }
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("抱歉-限于外网端口映射问题，在外网环境下的客户端无法切换码流!"));
    }
}
/**
 * @brief MainWindow::on_pushButton_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制-左上-按下
 */
void MainWindow::on_pushButton_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,UP_LEFT,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制-左上-释放
 */
void MainWindow::on_pushButton_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,UP_LEFT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZUp_pressed
 * @author zxl
 * @time 2018.12.01
 * @func 云台控制 上 按下
 */
void MainWindow::on_pushButton_PTZUp_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,TILT_UP,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZUp_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 上 释放
 */
void MainWindow::on_pushButton_PTZUp_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,TILT_UP,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_Right_Up_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 右上 按下
 */
void MainWindow::on_pushButton_Right_Up_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,UP_RIGHT,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_Right_Up_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 右上 释放
 */
void MainWindow::on_pushButton_Right_Up_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,UP_RIGHT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_Left_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  左  按下
 */
void MainWindow::on_pushButton_Left_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_LEFT,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_Left_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  左  释放
 */
void MainWindow::on_pushButton_Left_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_LEFT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_Auto_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  左右自动扫描  按下
 */
void MainWindow::on_pushButton_Auto_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_AUTO,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_Auto_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  左右自动扫描  释放
 */
void MainWindow::on_pushButton_Auto_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_AUTO,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_Right_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  右 按下
 */
void MainWindow::on_pushButton_Right_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_RIGHT,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_Right_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制  右  释放
 */
void MainWindow::on_pushButton_Right_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,PAN_RIGHT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_8_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 左下 按下
 */
void MainWindow::on_pushButton_8_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,DOWN_LEFT,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_8_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 左下 释放
 */
void MainWindow::on_pushButton_8_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,DOWN_LEFT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZDown_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 下 释放
 */
void MainWindow::on_pushButton_PTZDown_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,TILT_DOWN,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZDown_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 下 释放
 */
void MainWindow::on_pushButton_PTZDown_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,TILT_DOWN,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZDown_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 右下 按下
 */
void MainWindow::on_pushButton_PTZRightDown_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,DOWN_RIGHT ,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_PTZDown_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 右下 释放
 */
void MainWindow::on_pushButton_PTZRightDown_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,DOWN_RIGHT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_10_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 焦距变大   倍率变大  按下
 */
void MainWindow::on_pushButton_10_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,ZOOM_IN ,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_10_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 焦距变大   倍率变大  释放
 */
void MainWindow::on_pushButton_10_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,ZOOM_IN,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_11_pressed
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 焦距变小   倍率变小  按下
 */
void MainWindow::on_pushButton_11_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,ZOOM_OUT ,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_11_released
 * @author zxl
 * @time 2018.12.1
 * @func 云台控制 焦距变小   倍率变小  释放
 */
void MainWindow::on_pushButton_11_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,ZOOM_OUT,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_13_pressed
 * @func 焦距远 开启
 */
void MainWindow::on_pushButton_13_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,FOCUS_NEAR  ,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_13_released
 * @func 焦距远 停止
 */
void MainWindow::on_pushButton_13_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,FOCUS_NEAR,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_12_pressed
 * @func 焦距近 开启
 */
void MainWindow::on_pushButton_12_pressed()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,FOCUS_FAR  ,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_12_released
 * @func 焦距近 停止
 */
void MainWindow::on_pushButton_12_released()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,FOCUS_FAR,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_15_clicked
 * @func 开灯
 */
void MainWindow::on_pushButton_15_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,AUX_PWRON1,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_14_clicked
 * @func 关灯
 */
void MainWindow::on_pushButton_14_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,AUX_PWRON1,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_pushButton_17_clicked
 * @func 开雨刷
 */
void MainWindow::on_pushButton_17_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        if(!NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,AUX_PWRON2,0,static_cast<quint32>(ui->verticalSlider->value()))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("云台控制失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_16_clicked
 * @func 关雨刷
 */
void MainWindow::on_pushButton_16_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        NET_DVR_PTZControlWithSpeed(playWin->i_playHandler,AUX_PWRON2,1,static_cast<quint32>(ui->verticalSlider->value()));
    }
}
/**
 * @brief MainWindow::on_toolButton_13_clicked
 * @func 设置预置位
 */
void MainWindow::on_toolButton_13_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        int i_PresetNo = ui->comboBox->currentIndex() + 1;
        if(!NET_DVR_PTZPreset(playWin->i_playHandler,SET_PRESET,static_cast<quint32>(i_PresetNo))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("设置失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_14_clicked
 * @func 调用预置位
 */
void MainWindow::on_toolButton_14_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        int i_PresetNo = ui->comboBox->currentIndex() + 1;
        if(!NET_DVR_PTZPreset(playWin->i_playHandler,GOTO_PRESET,static_cast<quint32>(i_PresetNo))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("调用失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_15_clicked
 * @func 删除预置位
 */
void MainWindow::on_toolButton_15_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin){
        int i_PresetNo = ui->comboBox->currentIndex() + 1;
        if(NET_DVR_PTZPreset(playWin->i_playHandler,CLE_PRESET,static_cast<quint32>(i_PresetNo))){
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("删除预置位失败!"));
        }
    }
}
/**
 * @brief MainWindow::on_toolButton_11_clicked
 * @func 抓图路径获取
 */
void MainWindow::on_toolButton_11_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,"");

    ui->lineEdit_2->setText(fileName);
}
/**
 * @brief MainWindow::on_pushButton_Capture_clicked
 * @func 抓图按钮触发
 */
void MainWindow::on_pushButton_Capture_clicked()
{
    QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
    QString str = time.toString("yyyyMMdd_hhmmss"); //设置显示格式
    QString FileName = "/Capture_" + str + ".jpg";
    QString path = ui->lineEdit_2->text();
    if(path.isEmpty()){
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("文件报文路径错误!"));
        return;
    }
    QString FilePath_Name = ui->lineEdit_2->text() + FileName;
    QTreeWidgetItem* currentItem = ui->treeWidget->currentItem();
    if(!currentItem){
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("请选择摄像机节点!"));
        return;
    }
    QVariant variant = currentItem->data(0,Qt::UserRole);
    TreeNodeData nodedata = variant.value<TreeNodeData>();
    if(nodedata.NodeTable.compare("CAMERA") == 0){
        QSqlQuery query(db);
        QVariant variantIn = currentItem->data(1,Qt::UserRole);
        Camera camInfo = variantIn.value<Camera>();
        qDebug() << camInfo.UserName;
        if(camInfo.UserName.isNull() || camInfo.UserName.isEmpty()){
            qDebug() << QString::fromLocal8Bit("该摄像机没有绑定1号位信息，需要从数据库获取!");
            query.prepare("select * from camera where id=?");
            query.bindValue(0,nodedata.Code);
            query.exec();
            if(query.next()){
                Camera camera;
                camera.CameraCode = query.value(0).toString();
                camera.Factory = query.value(1).toInt();
                camera.UserName = query.value(2).toString();
                camera.Password = query.value(3).toString();
                camera.OutNetIPV4 = query.value(4).toString();
                camera.OutNetIPV6 = query.value(5).toString();
                camera.OutNetPort = static_cast<quint16>(query.value(6).toInt());
                camera.InNetIPV4 = query.value(7).toString();
                camera.InNetIPV6 = query.value(8).toString();
                camera.InNetPort = static_cast<quint16>(query.value(9).toInt());
                camera.LoginID = -1;
                //camera.PlayID = -1;
                QVariant variantBind = QVariant::fromValue(camera);
                currentItem->setData(1,Qt::UserRole,variantBind);
                camInfo = camera;
            }
            else{
                qWarning() << QString::fromLocal8Bit("数据库中没有查到该摄像机的信息");
            }
            //开始登陆摄像机并且抓图
        }
        else{
            qDebug() << QString::fromLocal8Bit("该摄像机已经绑定1号位信息，可以登陆摄像机并且获取流媒体!");
        }

        if(camInfo.LoginID < 0)
        {
            //摄像机未登录
            if(Is_InNet == 1){
                QByteArray baIP = camInfo.InNetIPV4.toLatin1();
                char *CamIp = baIP.data();
                QByteArray baUserName = camInfo.UserName.toLatin1();
                char *UserName = baUserName.data();
                QByteArray baPass = camInfo.Password.toLatin1();
                char *PassWord = baPass.data();
                camInfo.LoginID = NET_DVR_Login_V30(CamIp,camInfo.InNetPort,UserName,PassWord,&camInfo.CameraLoginInfo);
            }
            else{
                QByteArray baIP = camInfo.OutNetIPV4.toLatin1();
                char *CamIp = baIP.data();
                QByteArray baUserName = camInfo.UserName.toLatin1();
                char *UserName = baUserName.data();
                QByteArray baPass = camInfo.Password.toLatin1();
                char *PassWord = baPass.data();
                camInfo.LoginID = NET_DVR_Login_V30(CamIp,camInfo.OutNetPort,UserName,PassWord,&camInfo.CameraLoginInfo);
            }
            QVariant variantBind = QVariant::fromValue(camInfo);
            currentItem->setData(1,Qt::UserRole,variantBind);
            if(camInfo.LoginID < 0){
                //qDebug() << "Camera login failed!";
                QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("摄像机登陆失败!"));
            }
            else{
                qDebug() << "Camera login success!";
            }
        }
        else{
            qDebug() << QString::fromLocal8Bit("该摄像机已经登陆了,可以播放视频了!");
        }
        //播放
        if(camInfo.LoginID >= 0){
            NET_DVR_JPEGPARA JpgPara;
            JpgPara.wPicSize = 0xff;
            JpgPara.wPicQuality = 2;
            QByteArray baPath = FilePath_Name.toLocal8Bit();
            char* cPath = baPath.data();
            QFuture<int> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_CaptureJPEGPicture,camInfo.LoginID,1,&JpgPara, cPath);
            int i_Capret = future.result();
            if(i_Capret < 0){
                qDebug() << cPath;
                QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("抓图失败!") + QString::number(NET_DVR_GetLastError()));
            }
            else{
                QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("抓图成功!"));
            }
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("只能对摄像机节点进行抓图操作!"));
    }
}
/**
 * @brief MainWindow::on_toolButton_12_clicked
 * @func 获取录像报文文件
 */
void MainWindow::on_toolButton_12_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,"");

    ui->lineEdit_3->setText(fileName);
}
/**
 * @brief MainWindow::on_pushButton_LX_clicked
 * @func 录像按钮触发函数
 */
void MainWindow::on_pushButton_LX_clicked()
{
    QString str_Path = ui->lineEdit_3->text();
    if(str_Path.isEmpty()){
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("文件报文路径错误!"));
        return;
    }
    QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
    QString str = time.toString("yyyyMMdd_hhmmss"); //设置显示格式
    QString FileName = "/RealPlay_" + str + ".mp4";
    QString FilePath_Name = str_Path + FileName;
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin && playWin->i_RealHandler == -1){
        QByteArray baPath = FilePath_Name.toLocal8Bit();
        char* cPath = baPath.data();
        QFuture<int> future = QtConcurrent::run(QThreadPool::globalInstance(),NET_DVR_SaveRealData,playWin->i_playHandler,cPath);
        playWin->i_RealHandler = future.result();
        //playWin->i_RealHandler = NET_DVR_SaveRealData(playWin->i_playHandler,cPath);
        QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("视频正在录制中.....!"));
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("请求录制视频失败!"));
    }
}
/**
 * @brief MainWindow::on_pushButton_StopLX_clicked
 * @func 停止录像按钮触发函数
 */
void MainWindow::on_pushButton_StopLX_clicked()
{
    HKPlayWindow* playWin = GetPlayingHandler();
    if(playWin && playWin->i_RealHandler != -1){
        if(!NET_DVR_StopSaveRealData(playWin->i_playHandler)){
            QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("停止录像失败!") + QString::number(NET_DVR_GetLastError()));
        }
        else{
            playWin->i_RealHandler = -1;
            QMessageBox::about(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("停止录像成功!"));
        }
    }
    else{
        QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("该窗口并没有在录像!"));
    }
}
/**
 * @brief MainWindow::on_pushButton_MessClientRef_clicked
 * @func 刷新手机号码客户
 */
void MainWindow::on_pushButton_MessClientRef_clicked()
{
    QSqlQuery query(db);
    query.prepare("select realname,telphone from login");

    if(query.exec()){
        ui->comboBox_3->clear();
        while(query.next()){
            QString str_RealName = query.value(0).toString();
            QString str_TelPhone = query.value(1).toString();
            QVariant variant = QVariant::fromValue(str_TelPhone);
            ui->comboBox_3->addItem(str_RealName,variant);
        }
    }
}
/**
 * @brief MainWindow::on_pushButton_SendDX_clicked
 * @func 发送短信
 */
void MainWindow::on_pushButton_SendDX_clicked()
{
    QVariant variant = ui->comboBox_3->currentData();
    QString str_Phone = variant.value<QString>();
    if(!str_Phone.isEmpty()){
        //qDebug() << "phone is :" << str_Phone;
        QString str_text = ui->textEdit_DXNR->toPlainText();
        SendMessageToPhone(str_Phone,str_text);
    }
    else{
        qDebug() << "the phone number is error,conn't send message to it!";
    }
}
/**
 * @brief MainWindow::on_tableWidget_itemDoubleClicked
 * @param item
 * @func 双击报警条目
 */
void MainWindow::on_tableWidget_itemDoubleClicked(QTableWidgetItem *)
{
    int i_Row = ui->tableWidget->currentRow();
    QTableWidgetItem* itemNodeID = ui->tableWidget->item(i_Row,1);
    QString str_NodeID = itemNodeID->text();
    //qDebug() << QString::fromLocal8Bit("节点ID:") << str_NodeID;
    QTreeWidgetItem* getTreeItem = GetNodeByCode(str_NodeID);
    if(getTreeItem){
        //qDebug() << QString::fromLocal8Bit("对应设备树节点名称:") << getTreeItem->text(0);
        int i_childCount = getTreeItem->childCount();
        if(i_childCount > 0){
            for(int i=0 ; i<i_childCount ; i++)
            {
                QTreeWidgetItem* cItem = getTreeItem->child(i);
                if(cItem){
                    QVariant variant = cItem->data(0,Qt::UserRole);
                    TreeNodeData nodedata = variant.value<TreeNodeData>();
                    if(nodedata.NodeTable.compare("CAMERA") == 0){
                        on_treeWidget_itemDoubleClicked(cItem,0);
                    }
                    break;//只找一个摄像机就可以
                }
            }
        }
        else{
            QMessageBox::warning(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("没有找到对应的摄像机!"));
        }
    }
    else{
        qDebug() << QString::fromLocal8Bit("没有找到对应设备树节点");
    }
}
