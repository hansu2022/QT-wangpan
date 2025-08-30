#include "tcpclient.h" // 包含自定义的头文件，通常包含 TcpClient 类的声明
#include "ui_tcpclient.h" // 包含由 Qt Designer 生成的 UI 头文件
#include <QByteArray>     // 提供 QByteArray 类，用于处理字节数组数据
#include <QDebug>         // 提供 qDebug() 函数，用于调试输出
#include <QMessageBox>    // 提供 QMessageBox 类，用于显示消息框
#include <QHostAddress>   // 提供 QHostAddress 类，用于处理 IP 地址
#include "privatechat.h"
// TcpClient 类的构造函数
TcpClient::TcpClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpClient) // 初始化 UI 对象
{
    ui->setupUi(this);  // 设置 UI 界面
    resize(500,200);    // 调整窗口大小
    loadConfig();       // 加载配置文件（服务器IP和端口）

    // 连接信号和槽：当 m_tcpSocket 连接成功时，触发 showConnect() 槽函数
    connect(&m_tcpSocket,&QTcpSocket::connected,this,&TcpClient::showConnect);
    // 连接信号和槽：当 m_tcpSocket 接收到新数据时，触发 recvMsg() 槽函数
    connect(&m_tcpSocket,&QTcpSocket::readyRead,this,&TcpClient::recvMsg);

    // 尝试连接到服务器，使用配置文件中加载的IP和端口
    m_tcpSocket.connectToHost(QHostAddress(m_strIP),m_usPort);
}

// 析构函数，用于释放 UI 对象
TcpClient::~TcpClient()
{
    delete ui;
}

// 加载配置文件的方法
void TcpClient::loadConfig()
{
    // QFile file(":/client.config"); // 从资源文件中读取配置文件
    QFile file(":/client.config");

    // 尝试以只读模式打开文件
    if(file.open(QIODevice::ReadOnly)){
        QByteArray baData = file.readAll(); // 读取所有数据到 QByteArray
        QString strData = baData.toStdString().c_str(); // 转换为 QString
        file.close(); // 关闭文件

        strData.replace("\n"," "); // 将换行符替换为空格
        QStringList strList = strData.split(" "); // 按空格分割字符串
        m_strIP = strList.at(0); // 第一个元素是 IP 地址
        m_usPort = strList.at(1).toUShort(); // 第二个元素是端口号
        qDebug() << "ip:" << m_strIP<<"port: " << m_usPort; // 调试输出IP和端口
    }else{
        // 如果文件打开失败，弹出错误消息框
        QMessageBox::critical(this,"open config","open filed");
    }

}

// 单例模式：获取 TcpClient 类的唯一实例
TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

// 获取 TCP socket 对象的引用
QTcpSocket &TcpClient::getTcpSocket()
{
    return m_tcpSocket;
}

// 获取已登录的用户名
QString TcpClient::getLoginName()
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

// 槽函数：连接成功时显示消息框
void TcpClient::showConnect()
{
    QMessageBox::information(this, "连接服务器", "连接服务器成功");
}

// 槽函数：处理接收到的数据
void TcpClient::recvMsg()
{
    qDebug() << m_tcpSocket.bytesAvailable(); // 调试输出可读字节数
    uint uiPDULen = 0;
    // 先读取数据包的总长度
    m_tcpSocket.read((char*)&uiPDULen,sizeof(uint));
    uint uiMsgLen = uiPDULen-sizeof(PDU);
    // 根据总长度创建 PDU 数据包
    PDU *pdu = mkPDU(uiMsgLen);
    // 读取剩余的数据到 PDU 结构体中
    m_tcpSocket.read((char*)pdu + sizeof(uint),uiPDULen-sizeof(uint));

    // 根据消息类型处理不同的逻辑
    switch(pdu->uiMsgType){
    // 注册响应
    case ENUM_MSG_TYPE_REGIST_RESPOND:{
        if(strcmp(pdu->caData,REGIST_OK) == 0){
            QMessageBox::information(this,"注册",REGIST_OK);
        }else if(strcmp(pdu->caData,REGIST_FAILED) == 0){
            QMessageBox::warning(this,"注册",REGIST_FAILED);
        }
        break;
    }
    // 登录响应
    case ENUM_MSG_TYPE_LOGIN_RESPOND:{
        if(strcmp(pdu->caData,LOGIN_OK) == 0){
            m_strLoginName = ui->name_le->text(); // 登录成功后保存当前客户端的用户名
            m_strCurPath = QString("./%1").arg(m_strLoginName); // 设置当前路径为用户根目录
            QMessageBox::information(this,"登录",LOGIN_OK);
            OpeWidget::getInstance().setUsrName(m_strLoginName);
            OpeWidget::getInstance().show(); // 登录成功后显示主操作窗口
            hide(); // 隐藏当前登录窗口
        }else if(strcmp(pdu->caData,LOGIN_FAILED) == 0){
            QMessageBox::warning(this,"登录",LOGIN_FAILED);
        }
        break;
    }
    // 在线用户列表响应
    case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:{
        OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu); // 调用 Friend 类方法显示在线用户
        break;
    }
    // 搜索用户响应
    case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:{
        if(strcmp(SEARCH_USR_NO,pdu->caData) == 0){
            QMessageBox::warning(this,"搜索用户","没有此用户");
        }else if(strcmp(SEARCH_USR_YES,pdu->caData) == 0){
            QMessageBox::information(this,"搜索用户","用户在线");
        }else if(strcmp(SEARCH_USR_OFFLINE,pdu->caData) == 0){
            QMessageBox::information(this,"搜索用户","用户不在线");
        }
        break;
    }
    // 添加好友请求（从服务器接收）
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:{
        char caPerName[32] = {'\0'}; // 被请求者（我）
        char caName[32] = {'\0'};    // 请求者（对方）
        strncpy(caPerName, pdu->caData, 32);     // 复制被请求者名字
        strncpy(caName, pdu->caData + 32, 32);   // 复制请求者名字

        // 弹出消息框询问是否同意添加好友
        int ret = QMessageBox::information(this, "添加好友", QString("%1 请求添加你为好友").arg(caName), QMessageBox::Yes, QMessageBox::No);

        // 创建响应数据包
        PDU *respdu = mkPDU(64);
        // 填充响应数据包中的名字信息
        memcpy(respdu->caData, caName, 32);
        memcpy(respdu->caData + 32, caPerName, 32);

        // 根据用户选择设置响应消息类型
        if (QMessageBox::Yes == ret) {
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE;
        } else {
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
        }
        // 发送响应数据包给服务器
        m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 添加好友响应（从服务器接收）
    case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:{
        // 显示服务器返回的添加好友结果
        QMessageBox::information(this,"添加好友",pdu->caData);
        break;
    }
    // 刷新好友列表响应
    case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:{
        OpeWidget::getInstance().getFriend()->showAllFriend(pdu); // 调用 Friend 类方法显示好友列表
        break;
    }
    // 删除好友通知
    case ENUM_MSG_TYPE_DEL_FRIEND_NOTICE:{
        char caName[32] = {'\0'};
        strncpy(caName, pdu->caData, 32);
        QMessageBox::information(this, "好友关系", QString("%1 删除你为好友").arg(caName));

        break;
    }
    // 删除好友响应
    case ENUM_MSG_TYPE_DEL_FRIEND_RESPOND:{
        QMessageBox::information(this,"删除好友",pdu->caData);
        if (strcmp(pdu->caData, "删除好友成功") == 0) {
            // 主动请求刷新好友列表
            OpeWidget::getInstance().getFriend()->showAllFriend(pdu);
        }
        break;
    }
    // 私聊好友请求
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:{
        // 声明一个字符数组用于存储发送者的名字，并初始化为全0
        char caSendName[32] = {'\0'};
        // 从PDU（协议数据单元）的caData字段中复制发送者名字
        strncpy(caSendName,pdu->caData,32);
        // 将字符数组转换为QString，便于后续使用
        QString strSendName = QString(caSendName);
        // 获取 PrivateChat 单例的引用
        PrivateChat &privateChat = PrivateChat::getInstance();
        // 如果私聊窗口当前是隐藏状态
        if(privateChat.isHidden()){
            // 显示私聊窗口
            privateChat.show();
            // 设置聊天对象的名称
            privateChat.setChatName(strSendName);
            // 设置窗口的标题，显示“与[发送者名字]的私聊”
            privateChat.setWindowTitle(QString("与 %1 的私聊").arg(strSendName));
        }
        // 调用 updateMsg 方法，更新私聊窗口中的消息内容
        privateChat.updateMsg(pdu);
        break;
    }
    // 群聊请求
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:{
        // 声明一个字符数组用于存储发送者名字，并初始化为全0
        char caSendName[32] = {'\0'};
        // 从PDU的caData字段中复制发送者名字
        strncpy(caSendName,pdu->caData,32);
        // 格式化消息内容，将发送者名字和消息内容组合成“发送者: 消息”的格式
        QString strMsg = QString("%1: %2").arg(caSendName).arg(pdu->caMsg);
        // 获取 OpeWidget 单例，并调用其内部的 getFriend() 方法获取 Friend 对象，然后更新群聊消息显示
        OpeWidget::getInstance().getFriend()->updateGroupMsg(strMsg);
        break;
    }
    // 创建文件夹
    case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:{
        // 弹出消息框，显示服务器返回的创建文件夹结果（成功或失败信息）
        QMessageBox::information(this,"创建文件夹",pdu->caData);
        break;
    }
    // 刷新文件
    case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:{
        // 获取 OpeWidget 单例，并调用其内部的 getBook() 方法获取 Book 对象，然后调用其 flushFile() 方法刷新文件列表
        OpeWidget::getInstance().getBook()->flushFile(pdu);
        break;
    }
    default:
        break;
    }
    free(pdu); // 释放 PDU 内存
    pdu = NULL;
}


// 登录按钮的槽函数
void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty()){
        m_strLoginName = strName; // 保存登录名
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
        // 将用户名和密码复制到 PDU 数据区
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen); // 发送数据包
        free(pdu);
        pdu = NULL;
    }else{
        QMessageBox::critical(this,"注册","登录失败：用户名或密码为空");
    }
}

// 取消按钮的槽函数，此处为空实现
void TcpClient::on_cancel_pb_clicked()
{
}

// 注册按钮的槽函数
void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty()){
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
        // 将用户名和密码复制到 PDU 数据区
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen); // 发送数据包
        free(pdu);
        pdu = NULL;
    }else{
        QMessageBox::critical(this,"注册","注册失败：用户名或密码为空");
    }
}
