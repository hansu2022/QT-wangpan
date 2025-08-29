#include "mytcpsocket.h" // 包含自定义的头文件，通常包含 MyTcpSocket 类的声明
#include "mytcpserver.h" // 包含 MyTcpServer 类的头文件，用于服务器的转发逻辑
#include <QDebug>        // 提供 qDebug() 函数，用于调试输出

// MyTcpSocket 类的构造函数
MyTcpSocket::MyTcpSocket()
{
    // 连接信号和槽：当 socket 接收到新数据时，触发 recvMsg() 槽函数
    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    // 连接信号和槽：当客户端断开连接时，触发 clientOffline() 槽函数
    connect(this,SIGNAL(disconnected()),this,SLOT(clientOffline()));
}

// 获取当前客户端的用户名
QString MyTcpSocket::getName()
{
    return m_strName;
}

// 槽函数：处理接收到的数据
void MyTcpSocket::recvMsg()
{
    qDebug() << this ->bytesAvailable(); // 调试输出当前 socket 可读的字节数

    uint uiPDULen = 0;
    // 首先读取数据包的总长度
    this->read((char*)&uiPDULen,sizeof(uint));

    // 计算实际消息内容的长度
    uint uiMsgLen = uiPDULen-sizeof(PDU);
    // 根据消息长度创建 PDU 数据包
    PDU *pdu = mkPDU(uiMsgLen);
    // 读取剩余的数据到 PDU 结构体中
    this ->read((char*)pdu + sizeof(uint),uiPDULen-sizeof(uint));

    // 根据消息类型处理不同的逻辑
    switch(pdu->uiMsgType){
    // 处理注册请求
    case ENUM_MSG_TYPE_REGIST_REQUEST:{
        char caName[32] = {"\0"}; // 用户名
        char caPwd[32] = {"\0"};  // 密码
        strncpy(caName,pdu->caData,32);       // 复制用户名
        strncpy(caPwd,pdu->caData+32,32);     // 复制密码
        qDebug() << "zhanghu:" <<caName << "mima"<< caPwd; // 调试输出账号密码

        // 调用数据库操作类处理注册请求
        bool ret = OpeDB::getInstance().handleRegist(caName,caPwd);
        PDU *respdu = mkPDU(0); // 创建响应数据包
        respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND; // 设置响应消息类型
        if(ret){
            qDebug() << "success"; // 调试输出注册成功
            strcpy(respdu->caData,REGIST_OK); // 设置响应数据为注册成功
        }else{
            qDebug() << "failed"; // 调试输出注册失败
            strcpy(respdu->caData,REGIST_FAILED); // 设置响应数据为注册失败
        }
        write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 处理登录请求
    case ENUM_MSG_TYPE_LOGIN_REQUEST:{
        char caName[32] = {"\0"}; // 用户名
        char caPwd[32] = {"\0"};  // 密码
        strncpy(caName,pdu->caData,32);       // 复制用户名
        strncpy(caPwd,pdu->caData+32,32);     // 复制密码

        // 调用数据库操作类处理登录请求
        bool ret = OpeDB::getInstance().handleLogin(caName,caPwd);
        PDU *respdu = mkPDU(0); // 创建响应数据包
        respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND; // 设置响应消息类型
        if(ret){
            strcpy(respdu->caData,LOGIN_OK); // 设置响应数据为登录成功
            m_strName = caName; // 登录成功后保存当前客户端的用户名
        }else{
            strcpy(respdu->caData,LOGIN_FAILED); // 设置响应数据为登录失败
        }
        write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 处理请求所有在线用户列表
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:{
        // 调用数据库操作类获取所有在线用户的列表
        QStringList ret = OpeDB::getInstance().handleAllOnline();
        uint uiMsgLen = ret.size()*32; // 计算消息内容的长度（每个用户名32字节）
        PDU *respdu = mkPDU(uiMsgLen); // 创建响应数据包
        respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND; // 设置响应消息类型
        // 将在线用户名复制到 PDU 的消息数据区
        for(int i = 0; i < ret.size(); i++){
            strncpy(respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),32);
        }
        write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 处理搜索用户请求
    case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:{
        // 调用数据库操作类处理搜索用户请求，返回用户状态
        int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
        PDU *respdu = mkPDU(0); // 创建响应数据包
        respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND; // 设置响应消息类型
        if(ret == -1){ // 用户不存在
            strcpy(respdu->caData,SEARCH_USR_NO);
        }else if(ret == 1){ // 用户在线
            strcpy(respdu->caData,SEARCH_USR_YES);
        }else{ // 用户不在线 (0)
            strcpy(respdu->caData,SEARCH_USR_OFFLINE);
        }
        write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 处理添加好友请求 (A -> 服务器)
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
    {
        char caPerName[32] = {'\0'}; // 被请求者 (B) 的名字
        char caName[32] = {'\0'};    // 请求者 (A) 的名字
        strncpy(caPerName,pdu->caData,32);         // 从 PDU 中复制被请求者名字
        strncpy(caName,pdu->caData+32,32);         // 从 PDU 中复制请求者名字
        qDebug() << "caPerName:" << caPerName << "caName:" << caName;

        // 调用数据库操作类处理添加好友请求，返回结果码
        int res = OpeDB::getInstance().handleAddFriend(caPerName,caName);
        qDebug() << "handleAddFriend res:" << res;
        PDU *respdu = NULL;

        if(-1 == res){ // 未知错误
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,UNKONW_ERROR);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(0 == res){ // 已经是好友
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,EXISTED_FRIEND);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(1 == res){ // 请求成功，且被请求者在线，需要转发请求
            // 将 PDU 原样转发给被请求者 (B)，让 B 决定是否同意
            MyTcpServer::getInstance().resend(caPerName,pdu);
        }else if(2 == res){ // 请求成功，但被请求者不在线
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(3 == res){ // 被请求者不存在
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,ADD_FRIEND_NO_EXIST);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(4 == res){ // 处理不能添加自己的情况
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,"不能添加自己为好友"); // 定义一个明确的错误信息
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else{ // 其他未知错误
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData,UNKONW_ERROR);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        break;
    }
    // 处理添加好友同意请求 (B -> 服务器)
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
    {
        char caName[32] = {'\0'};     // 请求者 (A) 的名字
        char caPerName[32] = {'\0'};  // 同意者 (B) 的名字
        strncpy(caName, pdu->caData, 32);           // 复制请求者名字
        strncpy(caPerName, pdu->caData + 32, 32);   // 复制同意者名字

        // 1. 在数据库中正式添加好友关系
        OpeDB::getInstance().handleAddFriendAgree(caName, caPerName);

        // 2. 通知 A，B 已经同意了你的好友请求
        PDU* respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        QString successMsg = QString("%1 已同意您的好友请求").arg(caPerName);
        strncpy(respdu->caData, successMsg.toStdString().c_str(), successMsg.size());
        // 找到 A 的 socket 并发送消息
        MyTcpServer::getInstance().resend(caName, respdu);

        free(respdu);
        respdu = NULL;
        break;
    }
    // 处理添加好友拒绝请求 (B -> 服务器)
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
    {
        char caName[32] = {'\0'};     // 请求者 (A) 的名字
        char caPerName[32] = {'\0'};  // 拒绝者 (B) 的名字
        strncpy(caName, pdu->caData, 32);           // 复制请求者名字
        strncpy(caPerName, pdu->caData + 32, 32);   // 复制拒绝者名字

        // 直接通知 A，B 拒绝了你的好友请求
        PDU* respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        QString refuseMsg = QString("%1 拒绝了您的好友请求").arg(caPerName);
        strncpy(respdu->caData, refuseMsg.toStdString().c_str(), refuseMsg.size());
        // 找到 A 的 socket 并发送消息
        MyTcpServer::getInstance().resend(caName, respdu);

        free(respdu);
        respdu = NULL;
        break;
    }
    // 处理刷新好友列表请求
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:{
        char caName[32] = {'\0'};
        strncpy(caName,pdu->caData,32); // 复制用户名
        QStringList ret = OpeDB::getInstance().handleFlushFriend(caName); // 调用数据库操作类处理刷新好友请求
        uint uiMsgLen = ret.size()*32; // 计算消息内容的长度（每个好友32字节）
        PDU *respdu = mkPDU(uiMsgLen); // 创建响应数据包
        respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND; // 设置响应消息类型
        for(int i = 0;i<ret.size();i++){
            memcpy((char*)(respdu->caMsg)+i*32,ret.at(i).toStdString().c_str(),32); // 复制好友列表到 PDU 的消息数据区
        }
        write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
        free(respdu); // 释放内存
        respdu = NULL;
        break;
    }
    // 处理删除好友请求
    case ENUM_MSG_TYPE_DEL_FRIEND_REQUEST:
    {
        // 1. 正确解析请求者和被删除者
        char caMyName[32] = {'\0'};     // 请求删除的人 (自己)
        char caFriendName[32] = {'\0'}; // 被删除的人 (好友)
        strncpy(caMyName, pdu->caData, 32);
        strncpy(caFriendName, pdu->caData + 32, 32);

        // 调用数据库执行删除操作
        bool ret = OpeDB::getInstance().handleDelFriend(caMyName, caFriendName);

        // 2. 准备给请求者的响应
        PDU* respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_RESPOND;

        if (ret) {
            // 如果数据库操作成功
            strcpy(respdu->caData, "删除好友成功");

            // 3. 关键：创建并发送通知给被删除者
            PDU* noticePdu = mkPDU(0);
            // 4. 使用正确的通知消息类型！
            noticePdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_NOTICE;
            strncpy(noticePdu->caData, caMyName, 32); // 把删除者的名字放入通知中

            // 通过 resend 函数将通知转发给被删除的好友
            MyTcpServer::getInstance().resend(caFriendName, noticePdu);

            free(noticePdu);
            noticePdu = NULL;
        } else {
            // 如果数据库操作失败
            strcpy(respdu->caData, "删除好友失败");
        }

        // 将操作结果响应给发起删除请求的客户端
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:{
        char caPerName[32] = {'\0'}; // 接收者名字
        strncpy(caPerName,pdu->caData+32,32); // 复制接收者名字
        // 通过 MyTcpServer 的 resend 方法转发私聊消息给接收者
        MyTcpServer::getInstance().resend(caPerName,pdu);
        break;
    }
    default:
        break;
    }
    free(pdu); // 释放 PDU 内存
    pdu = NULL;
}

// 槽函数：客户端下线处理
void MyTcpSocket::clientOffline()
{
    // 调用数据库操作类处理用户下线
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    // 发射 offline 信号，通知 MyTcpServer 该客户端已下线
    emit offline(this);
}
