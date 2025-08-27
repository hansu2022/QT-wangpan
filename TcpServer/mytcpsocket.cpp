#include "mytcpsocket.h"
#include "mytcpserver.h"
#include <QDebug>
MyTcpSocket::MyTcpSocket()
{
    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    connect(this,SIGNAL(disconnected()),this,SLOT(clientOffline()));
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::recvMsg()
{
    qDebug() << this ->bytesAvailable();
    uint uiPDULen = 0;
    this->read((char*)&uiPDULen,sizeof(uint));
    uint uiMsgLen = uiPDULen-sizeof(PDU);
    PDU *pdu = mkPDU(uiMsgLen);
    this ->read((char*)pdu + sizeof(uint),uiPDULen-sizeof(uint));
    // qDebug() << pdu->uiMsgType << pdu->caMsg;
    switch(pdu->uiMsgType){
    case ENUM_MSG_TYPE_REGIST_REQUEST:{
        char caName[32] = {"\0"};
        char caPwd[32] = {"\0"};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        qDebug() << "zhanghu:" <<caName << "mima"<< caPwd;
        bool ret = OpeDB::getInstance().handleRegist(caName,caPwd);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
        if(ret){
            qDebug() << "success";
            strcpy(respdu->caData,REGIST_OK);
        }else{
            qDebug() << "failed";
            strcpy(respdu->caData,REGIST_FAILED);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST:{
        char caName[32] = {"\0"};
        char caPwd[32] = {"\0"};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        bool ret = OpeDB::getInstance().handleLogin(caName,caPwd);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
        if(ret){
            strcpy(respdu->caData,LOGIN_OK);
            m_strName = caName;
        }else{
            strcpy(respdu->caData,LOGIN_FAILED);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:{
        QStringList ret = OpeDB::getInstance().handleAllOnline();
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
        for(int i = 0; i < ret.size(); i++){
            strncpy(respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),32);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:{
        int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
        if(ret == -1){
            strcpy(respdu->caData,SEARCH_USR_NO);
        }else if(ret == 1){
            strcpy(respdu->caData,SEARCH_USR_YES);
        }else{
            strcpy(respdu->caData,SEARCH_USR_OFFLINE);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
    {
        char caPerName[32] = {'\0'};
        char caName[32] = {'\0'};
        strncpy(caPerName,pdu->caData,32);
        strncpy(caName,pdu->caData+32,32);
        qDebug() << "caPerName:" << caPerName << "caName:" << caName;
        int res = OpeDB::getInstance().handleAddFriend(caPerName,caName);
        qDebug() << "handleAddFriend res:" << res;
        PDU *respdu = NULL;
        if(-1==res){
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,UNKONW_ERROR);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(0==res){
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,EXISTED_FRIEND);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(1==res){
            MyTcpServer::getInstance().resend(caPerName,pdu);
        }else if(2==res){
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(3==res){
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,ADD_FRIEND_NO_EXIST);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(4==res){ // 处理不能添加自己的情况
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,"不能添加自己为好友"); // 定义一个明确的错误信息
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else{
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,UNKONW_ERROR);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
    {
        char caName[32] = {'\0'};      // A 的名字 (请求者)
        char caPerName[32] = {'\0'};   // B 的名字 (同意者)
        strncpy(caName, pdu->caData, 32);
        strncpy(caPerName, pdu->caData + 32, 32);

        // 1. 在数据库中正式添加好友关系
        OpeDB::getInstance().handleAddFriendAgree(caName, caPerName);

        // 2. 通知 A，B 已经同意了你的好友请求
        //    需要通过 resend 找到 A 的socket并发送消息
        PDU* respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        QString successMsg = QString("%1 已同意您的好友请求").arg(caPerName);
        strncpy(respdu->caData, successMsg.toStdString().c_str(), successMsg.size());
        MyTcpServer::getInstance().resend(caName, respdu);

        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
    {
        char caName[32] = {'\0'};      // A 的名字 (请求者)
        char caPerName[32] = {'\0'};   // B 的名字 (拒绝者)
        strncpy(caName, pdu->caData, 32);
        strncpy(caPerName, pdu->caData + 32, 32);

        // 直接通知 A，B 拒绝了你的好友请求
        PDU* respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        QString refuseMsg = QString("%1 拒绝了您的好友请求").arg(caPerName);
        strncpy(respdu->caData, refuseMsg.toStdString().c_str(), refuseMsg.size());
        MyTcpServer::getInstance().resend(caName, respdu);

        free(respdu);
        respdu = NULL;
        break;
    }
    default:
        break;
    }
    free(pdu);
    pdu = NULL;
}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);

}
