#include "mytcpserver.h" // 包含自定义的头文件，通常包含 MyTcpServer 类的声明
#include <QDebug>        // 提供 qDebug() 函数，用于调试输出

// 构造函数，私有化以实现单例模式
MyTcpServer::MyTcpServer() {}

// 单例模式：获取 MyTcpServer 类的唯一实例
MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

// 槽函数：处理新的客户端连接
// 当有新的连接到来时，QTcpServer 会自动调用此虚函数
void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "new client connected"; // 调试输出新连接信息

    // 创建一个新的 MyTcpSocket 对象来处理这个连接
    MyTcpSocket *pTcpSocket = new MyTcpSocket();

    // 设置新 socket 的描述符，使其与新连接绑定
    pTcpSocket->setSocketDescriptor(socketDescriptor);

    // 将这个新的 socket 对象添加到列表中进行管理
    m_tcpSocketList.append(pTcpSocket);

    // 连接信号和槽：当这个客户端下线时，触发 deleteSocket() 槽函数来清理资源
    connect(pTcpSocket,SIGNAL(offline(MyTcpSocket*)),this,SLOT(deleteSocket(MyTcpSocket*)));
}

// 转发消息的函数
// 参数：pername - 目标用户的名字，pdu - 要发送的数据包
void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if (pername == NULL || pdu == NULL) {
        return;
    }

    QString strName = pername;
    // 遍历所有已连接的客户端
    for (int i = 0; i < m_tcpSocketList.size(); i++) {
        // 查找与目标用户名匹配的客户端
        if (m_tcpSocketList.at(i)->getName() == strName) {
            // 找到后，将数据包发送给该客户端
            m_tcpSocketList.at(i)->write((char*)pdu, pdu->uiPDULen);
            break; // 找到并发送后，退出循环
        }
    }
}

// 槽函数：安全地删除已下线的客户端 socket
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
    // 调试输出当前剩余的客户端
    for(int i = 0; i < m_tcpSocketList.size(); i++){
        // 确保列表中的指针有效才调用其成员函数
        if(m_tcpSocketList.at(i)) {
            qDebug() << m_tcpSocketList.at(i)->getName();
        }
    }
}
