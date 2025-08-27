#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
TcpClient::TcpClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpClient)
{
    ui->setupUi(this);
    resize(500,200);
    loadConfig();
    connect(&m_tcpSocket,&QTcpSocket::connected,this,&TcpClient::showConnect);
    connect(&m_tcpSocket,&QTcpSocket::readyRead,this,&TcpClient::recvMsg);
    m_tcpSocket.connectToHost(QHostAddress(m_strIP),m_usPort);
}

TcpClient::~TcpClient()
{
    delete ui;
}

void TcpClient::loadConfig()
{
    QFile file(":/client.config");
    if(file.open(QIODevice::ReadOnly)){
        QByteArray baData = file.readAll();
        QString strData = baData.toStdString().c_str();
        file.close();
        strData.replace("\n"," ");
        QStringList strList = strData.split(" ");
        m_strIP = strList.at(0);
        m_usPort = strList.at(1).toUShort();
        qDebug() << "ip:" << m_strIP<<"port: " << m_usPort;
    }else{
        QMessageBox::critical(this,"open config","open filed");
    }

}

TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()
{
    return m_tcpSocket;
}

QString TcpClient::getLoginName()
{
    return m_strLoginName;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this, "连接服务器", "连接服务器成功");
}

void TcpClient::recvMsg()
{
    qDebug() << m_tcpSocket.bytesAvailable();
    uint uiPDULen = 0;
    m_tcpSocket.read((char*)&uiPDULen,sizeof(uint));
    uint uiMsgLen = uiPDULen-sizeof(PDU);
    PDU *pdu = mkPDU(uiMsgLen);
    m_tcpSocket.read((char*)pdu + sizeof(uint),uiPDULen-sizeof(uint));
    // qDebug() << pdu->uiMsgType << pdu->caMsg;
    switch(pdu->uiMsgType){
    case ENUM_MSG_TYPE_REGIST_RESPOND:{
        if(strcmp(pdu->caData,REGIST_OK) == 0){
            QMessageBox::information(this,"注册",REGIST_OK);
        }else if(strcmp(pdu->caData,REGIST_FAILED) == 0){
            QMessageBox::warning(this,"注册",REGIST_FAILED);
        }
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_RESPOND:{
        if(strcmp(pdu->caData,LOGIN_OK) == 0){
            QMessageBox::information(this,"登录",LOGIN_OK);
            OpeWidget::getInstance().show();
            hide();
        }else if(strcmp(pdu->caData,LOGIN_FAILED) == 0){
            QMessageBox::warning(this,"登录",LOGIN_FAILED);
        }
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:{
        OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:{
        if(strcmp(SEARCH_USR_NO,pdu->caData) == 0){
            QMessageBox::warning(this,"搜索用户","没有此用户");
        }else if(strcmp(SEARCH_USR_YES,pdu->caData) == 0){
            QMessageBox::information(this,"搜索用户","用户在线");
        }else if(strcmp(SEARCH_USR_OFFLINE,pdu->caData) == 0){
            QMessageBox::information(this,"搜索用户","用户不在线");
        break;
        }}
    /*A 发起请求 -> 服务器
    服务器 检查 -> B (如果B在线)
    B 同意/拒绝 -> 服务器
    服务器 处理(更新数据库) -> A (通知A最终结果)*/
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:{
        char caPerName[32] = {'\0'}; // B 的名字 (被请求者)
        char caName[32] = {'\0'};    // A 的名字 (请求者)
        strncpy(caPerName, pdu->caData, 32);
        strncpy(caName, pdu->caData + 32, 32);

        int ret = QMessageBox::information(this, "添加好友", QString("%1 请求添加你为好友").arg(caName), QMessageBox::Yes, QMessageBox::No);

        // 响应 PDU 需要 64 字节来存放两个名字
        PDU *respdu = mkPDU(64);

        // 把 A 的名字（请求者）放在前面
        memcpy(respdu->caData, caName, 32);
        // 把 B 的名字（被请求者，也就是自己）放在后面
        memcpy(respdu->caData + 32, caPerName, 32);

        if (QMessageBox::Yes == ret) {
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE;
        } else {
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
        }
        m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:{
        QMessageBox::information(this,"添加好友",pdu->caData);
        break;
    }
    default:
        break;
    }
    free(pdu);
    pdu = NULL;
}

// void TcpClient::on_pushButton_clicked()
// {
//     QString strMsg = ui->lineEdit->text();
//     if(!strMsg.isEmpty()){
//         PDU *pdu = mkPDU(strMsg.size()+1);
//         pdu->uiMsgType = 8888;

//         memcpy(pdu->caMsg,strMsg.toStdString().c_str(),strMsg.size()+1);
//         m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
//         free(pdu);
//         pdu = NULL;
//     }else{
//         QMessageBox::warning(this,"信息发送","发送的信息不能为空");
//     }
// }


void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();

    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty()){
        m_strLoginName = strName;
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }else{
        QMessageBox::critical(this,"注册","登录失败：用户名或密码为空");
    }
}


void TcpClient::on_cancel_pb_clicked()
{

}


void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty()){
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }else{
        QMessageBox::critical(this,"注册","注册失败：用户名或密码为空");
    }
}

