#include "mytcpsocket.h" // 包含自定义的头文件，通常包含 MyTcpSocket 类的声明
#include "mytcpserver.h" // 包含 MyTcpServer 类的头文件，用于服务器的转发逻辑
#include <QDebug>        // 提供 qDebug() 函数，用于调试输出
#include <QTimer>
// MyTcpSocket 类的构造函数
MyTcpSocket::MyTcpSocket()
{
    // 连接信号和槽：当 socket 接收到新数据时，触发 recvMsg() 槽函数
    connect(this, &MyTcpSocket::readyRead, this, &MyTcpSocket::recvMsg);
    // 连接信号和槽：当客户端断开连接时，触发 clientOffline() 槽函数
    connect(this, &MyTcpSocket::disconnected, this, &MyTcpSocket::clientOffline);

    m_bUpload = false; // 初始化上传状态为 false
    m_pTimer = new QTimer(this); // 创建一个新的定时器对象
    connect(m_pTimer, &QTimer::timeout, this, &MyTcpSocket::sendFileToClient); // 连接定时器的超时信号到发送文件槽函数
}

// 获取当前客户端的用户名
QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString srcDir, QString destDir)
{
    QDir dest_dir(destDir);
    if (dest_dir.exists()) {
        //目标文件夹已存在
        //将新文件夹重命名，例如 "docs_1"
        destDir = destDir + "_1";
    }
    QDir dir;
    dir.mkdir(destDir);
    dir.setPath(srcDir);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (fileInfo.isDir()) {
            copyDir(fileInfo.filePath(), destDir + "/" + fileInfo.fileName());
        } else {
            QFile::copy(fileInfo.filePath(), destDir + "/" + fileInfo.fileName());
        }
    }
}

// 槽函数：处理接收到的数据
void MyTcpSocket::recvMsg()
{
    if(m_bUpload){
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();

        if(m_iRecved == m_iTotal){
            m_file.close();
            m_bUpload = false;

            auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND);
            strncpy(respdu->caData, "文件上传成功", sizeof(respdu->caData) - 1);
            sendPdu(std::move(respdu));
        }else if(m_iRecved > m_iTotal){
            m_file.close();
            m_bUpload = false;

            auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND);
            strncpy(respdu->caData, "文件上传失败", sizeof(respdu->caData) - 1);
            sendPdu(std::move(respdu));
        }
        return;
    }
    // 将socket中所有可读数据追加到缓冲区
    m_buffer.append(readAll());

    // 循环处理缓冲区中的数据，直到数据不足一个完整的包
    while(true){
        // 首先判断缓冲区数据是否足够读取一个PDU的头部长度
        if(m_buffer.size() < sizeof(uint)){
            break;
        }
        uint uiPDULen = 0;
        memcpy(&uiPDULen,m_buffer.constData(),sizeof(uint));

        // 判断缓冲区数据是否足够一个完整的PDU
        if (m_buffer.size() < uiPDULen) {
            break; // 数据不完整，等待下一次readyRead信号
        }

        // ---- 数据包完整，开始安全的反序列化 ----

        // 定义固定头部的大小
        auto pdu = PDU::deserialize(m_buffer.constData(),uiPDULen);

        // 将处理完的数据从缓冲区头部移除
        m_buffer.remove(0, uiPDULen);

        if (!pdu) {
            qWarning() << "反序列化失败，可能是一个损坏的数据包。已丢弃。" << uiPDULen;
            continue; // PDU::deserialize 内部已经做了校验，如果返回nullptr说明包有问题
        }

        // 根据消息类型调用相应的处理函数
        switch(pdu->uiMsgType){
        case MsgType::ENUM_MSG_TYPE_REGIST_REQUEST:
            handleRegistRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_LOGIN_REQUEST:
            handleLoginRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
            handleAllOnlineRequest();
            break;
        case MsgType::ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
            handleSearchUsrRequest(*pdu);
            break;
        // case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        //     handleAddFriendRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        //     handleAddFriendAgree(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        //     handleAddFriendRefuse(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        //     handleFlushFriendRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_DEL_FRIEND_REQUEST:
        //     handleDelFriendRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        //     handlePrivateChatRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        //     handleGroupChatRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
        //     handleCreateDirRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        //     handleFlushFileRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_DEL_ITEM_REQUEST:
        //     handleDelItemRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_RENAME_DIR_REQUEST:
        //     handleRenameDirRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_ENTRY_DIR_REQUEST:
        //     handleEntryDirRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        //     handleUploadFileRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        //     handleDownloadFileRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        //     handleShareFileRequest(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_SHARE_FILE_NOTICE_RESPOND:
        //     handleShareFileNoticeRespond(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_RECEIVE_FILE_RESULT:
        //     handleReceiveFileResult(*pdu);
        //     break;
        // case MsgType::ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
        //     handleMoveFileRequest(*pdu);
        //     break;
        default:
            qWarning() << "未知的消息类型:" << static_cast<uint>(pdu->uiMsgType);
            break;
        }
    }
}

/**
 * @brief 处理客户端的注册请求
 * @param pdu 包含注册信息的请求PDU
 */
void MyTcpSocket::handleRegistRequest(const PDU &pdu)
{
    // 1. 从 pdu.caData 创建一个 QByteArray
    // sizeof(pdu.caData) 确保我们不会读到缓冲区外
    QByteArray data(pdu.caData, sizeof(pdu.caData));

    // 2. 使用 '\0' 分割
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid regist request format";
        return;
    }
    QString caName = QString::fromUtf8(parts[0]);
    QString caPwd = QString::fromUtf8(parts[1]);
    qDebug() << "注册请求 -> 用户名:" << caName << "密码:" << caPwd;

    bool ret = OpeDB::getInstance().handleRegist(caName.toStdString().c_str(), caPwd.toStdString().c_str());

    // 3. 使用工厂函数创建响应PDU
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_REGIST_RESPOND);

    // 4. 填充响应数据
    if (ret) {
        strncpy(respdu->caData, REGIST_OK, sizeof(respdu->caData) - 1);
        QDir dir;
        dir.mkdir(QString("./user_data/%1").arg(caName));
    } else {
        strncpy(respdu->caData, REGIST_FAILED, sizeof(respdu->caData) - 1);
    }

    // 5. 调用统一的发送函数
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理客户端的登录请求
 * @param pdu 包含登录信息的请求PDU
 */
void MyTcpSocket::handleLoginRequest(const PDU &pdu)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};

    strncpy(caName, pdu.caData, 32);
    strncpy(caPwd, pdu.caData + 32, 32);
    qDebug() << "登录请求 -> 用户名:" << caName << "密码:" << caPwd;

    bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);

    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_LOGIN_RESPOND);
    if (ret) {
        strncpy(respdu->caData, LOGIN_OK, sizeof(respdu->caData) - 1);
        m_strName = caName; // 登录成功，记录用户名
        emit loggedIn(this, m_strName); // 通知服务器该用户已登录
    } else {
        strncpy(respdu->caData, LOGIN_FAILED, sizeof(respdu->caData) - 1);
    }
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理获取所有在线用户的请求
 */
void MyTcpSocket::handleAllOnlineRequest()
{
    qDebug() << "请求所有在线用户";
    QStringList onlineUsers = OpeDB::getInstance().handleAllOnline();

    // 1. 使用 QByteArray 作为缓冲区 QDataStream是Qt中用于序列化各种数据类型的强大工具，它能自动处理字节序、长度等问题。
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);

    // 2. 直接将 QStringList 写入流
    stream << onlineUsers;

    // 3. 将序列化后的数据放入 vMsg
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_ALL_ONLINE_RESPOND, buffer.size());
    memcpy(respdu->vMsg.data(), buffer.constData(), buffer.size());

    sendPdu(std::move(respdu));
}

/**
 * @brief 处理搜索用户的请求
 * @param pdu 包含搜索用户名的请求PDU
 */
void MyTcpSocket::handleSearchUsrRequest(const PDU &pdu)
{
    qDebug() << "搜索用户 ->" << pdu.caData;
    int ret = OpeDB::getInstance().handleSearchUsr(pdu.caData);

    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_SEARCH_USR_RESPOND);

    if (ret == -1) {
        strncpy(respdu->caData, SEARCH_USR_NO, sizeof(respdu->caData) - 1);
    } else if (ret == 1) {
        strncpy(respdu->caData, SEARCH_USR_YES, sizeof(respdu->caData) - 1);
    } else { // ret == 0
        strncpy(respdu->caData, SEARCH_USR_OFFLINE, sizeof(respdu->caData) - 1);
    }
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理添加好友请求 (A -> 服务器)
 * @param pdu 收到的请求PDU
 */
void MyTcpSocket::handleAddFriendRequest(const PDU& pdu)
{
    char caPerName[32] = {'\0'}; // 被请求者 (B) 的名字
    char caName[32] = {'\0'};    // 请求者 (A) 的名字
    strncpy(caPerName, pdu.caData, 31);
    strncpy(caName, pdu.caData + 32, 31);
    qDebug() << "handleAddFriendRequest: target=" << caPerName << ", requester=" << caName;

    int res = OpeDB::getInstance().handleAddFriend(caPerName, caName);
    qDebug() << "handleAddFriend DB result:" << res;

    // 情况1：请求成功，且B在线，需要将请求转发给B
    if (1 == res) {
        // 直接将收到的pdu转发给目标用户。
        // 注意：我们假设MyTcpServer::resend函数也被同步修改，
        // 以便能处理新的protocol::PDU结构体（进行序列化后发送）。
        MyTcpServer::getInstance().resend(caPerName, pdu);
        return; // 转发后，此函数任务完成
    }

    // 情况2：其他所有情况，都需要直接给请求者A一个响应
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_ADD_FRIEND_RESPOND);
    const char* responseMsg = nullptr;

    switch (res) {
    case -1: responseMsg = UNKONW_ERROR; break;
    case 0:  responseMsg = EXISTED_FRIEND; break;
    case 2:  responseMsg = ADD_FRIEND_OFFLINE; break;
    case 3:  responseMsg = ADD_FRIEND_NO_EXIST; break;
    case 4:  responseMsg = "不能添加自己为好友"; break;
    default: responseMsg = UNKONW_ERROR; break;
    }

    strncpy(respdu->caData, responseMsg, sizeof(respdu->caData) - 1);
    sendPdu(std::move(respdu));
}

void MyTcpSocket::sendPdu(std::unique_ptr<PDU> pdu)
{
    if (!pdu) return;
// 直接调用 pdu 的 serialize 方法，得到一个包含所有数据的 vector
    std::vector<char> serialized_data = pdu->serialize();

    // 将 vector 的数据写入 socket
    // QByteArray 可以很方便地从 char* 和长度构造
    write(QByteArray(serialized_data.data(), serialized_data.size()));
}

// 槽函数：客户端下线处理
void MyTcpSocket::clientOffline()
{
    // 调用数据库操作类处理用户下线
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    // 发射 offline 信号，通知 MyTcpServer 该客户端已下线
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    // 1. 每次只发送一个数据块，而不是循环
    std::unique_ptr<char[]> pData(new char[4096]); // 使用智能指针防止内存泄漏
    qint64 ret = 0;

    ret = m_file.read(pData.get(), 4096);
    if (ret > 0) {
        // 成功读取数据块，写入socket
        if (this->write(pData.get(), ret) == -1) {
            // 如果写入失败（例如客户端断开），则停止发送
            qDebug() << "发送文件数据失败";
            m_pTimer->stop();
            m_file.close();
        }
    } else { // ret == 0 (文件读完) or ret < 0 (读取出错)
        if (ret < 0) {
            qDebug() << "读取文件时出错";
        } else {
            qDebug() << "文件发送完毕";
        }
        m_pTimer->stop(); // 停止定时器
        m_file.close();   // 关闭文件
    }
}
