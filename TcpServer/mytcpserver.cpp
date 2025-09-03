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
        connect(pTcpSocket, &MyTcpSocket::offline, this, &MyTcpServer::deleteSocket);
    // 连接信号和槽：当客户端成功登录时，触发 registerClient() 槽函数
    connect(pTcpSocket, &MyTcpSocket::loggedIn, this, &MyTcpServer::registerClient);
}

// 转发消息的函数 升级为了基于哈希表的快速查找（O(log n) 复杂度）
// 参数：pername - 目标用户的名字，pdu - 要发送的数据包
void MyTcpServer::resend(const QString &pername, const PDU &pdu)
{
    if(pername.isEmpty()){
        return;
    }


    // 使用QMap进行 O(log n) 的快速查找
    MyTcpSocket *targetSocket = m_clientsMap.value(pername, nullptr);
    if (targetSocket) {
        std::vector<char> block = pdu.serialize();
        targetSocket->write(block.data(), block.size());
        qDebug() << "Resent message to" << pername;
    } else {
        qDebug() << "User" << pername << "not online or not found for resend.";
    }
}

//【新增】槽函数，用于注册已登录的客户端
void MyTcpServer::registerClient(MyTcpSocket *socket, const QString &name)
{
    if (socket && !name.isEmpty()) {
        m_clientsMap.insert(name, socket);
        qDebug() << "Client registered:" << name;
    }
}

// 槽函数：安全地删除已下线的客户端 socket
void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    if (!mysocket) return;

    qDebug() << "Client disconnected, preparing to delete socket for user:" << mysocket->getName();

    // 从快速查找映射中移除
    if (!mysocket->getName().isEmpty()) {
        m_clientsMap.remove(mysocket->getName());
    }

    // 从列表中移除
    m_tcpSocketList.removeOne(mysocket);

    // 安全地销毁对象
    mysocket->deleteLater();

    qDebug() << "Socket scheduled for deletion. Remaining clients in map:" << m_clientsMap.keys();
}
