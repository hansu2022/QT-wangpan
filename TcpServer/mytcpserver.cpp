#include "mytcpserver.h"
#include <QDebug>
MyTcpServer::MyTcpServer() {}

MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "new client connected";
    MyTcpSocket *pTcpSocket = new MyTcpSocket();
    pTcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(pTcpSocket);

    connect(pTcpSocket,SIGNAL(offline(MyTcpSocket*)),this,SLOT(deleteSocket(MyTcpSocket*)));
}

void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if(pername == NULL || pdu == NULL){
        return;
    }
    QString strName = pername;
    for(int i = 0; i < m_tcpSocketList.size(); i++){
        if(m_tcpSocketList.at(i)->getName() == strName){
            m_tcpSocketList.at(i)->write((char*)pdu,pdu->uiPDULen);
            break;
        }
    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    qDebug() << "Client disconnected, preparing to delete socket for user:" << mysocket->getName();
    // 1. 使用 removeOne 从列表中安全地移除该套接字的指针。
    // 这个函数会查找并移除第一个匹配的项，返回true如果成功
    bool removed = m_tcpSocketList.removeOne(mysocket);

    if (removed) {
        // 2. 使用 deleteLater() 来安全地销毁 QObject 对象。
        // 这会将删除操作排入事件队列，在当前函数调用堆栈返回到事件循环后执行，
        // 从而避免了在对象自身的方法仍在执行时删除它。
        mysocket->deleteLater();
        qDebug() << "Socket successfully scheduled for deletion.";
    }

    qDebug() << "Remaining clients:";
    for(int i = 0; i < m_tcpSocketList.size(); i++){
        // 确保列表中的指针有效才调用其成员函数
        if(m_tcpSocketList.at(i)) {
            qDebug() << m_tcpSocketList.at(i)->getName();
        }
    }
}
