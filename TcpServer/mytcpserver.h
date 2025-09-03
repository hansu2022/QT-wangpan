#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include "mytcpsocket.h"

class MyTcpServer : public QTcpServer
{   Q_OBJECT
public:
    MyTcpServer();
    static MyTcpServer &getInstance();

    void incomingConnection(qintptr sockeetDescriptor);

    void resend(const QString &pername,const PDU &pdu);
public slots:
    void deleteSocket(MyTcpSocket *mysocket);
    // 新增槽函数，用于在客户端成功登录后，将其注册到映射中
    void registerClient(MyTcpSocket *socket, const QString &name);
private:
    QList<MyTcpSocket*> m_tcpSocketList;
    QMap<QString, MyTcpSocket*> m_clientsMap; // 用于快速查找已登录的客户端
};

#endif // MYTCPSERVER_H
