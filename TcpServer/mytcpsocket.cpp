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
        case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
            handleAddFriendRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
            handleAddFriendAgree(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
            handleAddFriendRefuse(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
            handleFlushFriendRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DEL_FRIEND_REQUEST:
            handleDelFriendRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
            handlePrivateChatRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
            handleGroupChatRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
            handleCreateDirRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
            handleFlushFileRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DEL_ITEM_REQUEST:
            handleDelItemRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_RENAME_DIR_REQUEST:
            handleRenameDirRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ENTRY_DIR_REQUEST:
            handleEntryDirRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
            handleUploadFileRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
            handleDownloadFileRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
            handleShareFileRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_SHARE_FILE_NOTICE_RESPOND:
            handleShareFileNoticeRespond(*pdu);
            break;
        // case MsgType::ENUM_MSG_TYPE_RECEIVE_FILE_RESULT:
        //     handleReceiveFileResult(*pdu);
        //     break;
        case MsgType::ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
            handleMoveFileRequest(*pdu);
            break;
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
        QString userDirPath = QString("./user_data/%1").arg(caName);

        QDir dir;
        // 检查并创建 user_data 父目录 (如果它不存在的话)，这让程序更健壮
        if (!dir.exists("./user_data")) {
            dir.mkdir("./user_data");
        }

        // 在指定路径下创建用户的专属文件夹
        bool success = dir.mkdir(userDirPath);

        // 【强烈建议】检查文件夹是否创建成功，并打印日志
        if (success) {
            strncpy(respdu->caData, REGIST_OK, sizeof(respdu->caData) - 1);
            qDebug() << "为新用户创建目录成功:" << userDirPath;
        } else {
            // 如果创建失败，这是一个严重的服务器端问题，需要记录下来
            qWarning() << "！！！为新用户创建目录失败:" << userDirPath;
        }
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

    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid login request format";
        return;
    }
    QString caName = QString::fromUtf8(parts[0]);
    QString caPwd = QString::fromUtf8(parts[1]);

    qDebug() << "登录请求 -> 用户名:" << caName << "密码:" << caPwd;

    bool ret = OpeDB::getInstance().handleLogin(caName.toStdString().c_str(), caPwd.toStdString().c_str());


    if (ret) {
        QString rootPath = QString("./user_data/%1").arg(caName);
        QByteArray pathData = rootPath.toUtf8();
                auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_LOGIN_RESPOND, pathData.size());
        strncpy(respdu->caData, LOGIN_OK, sizeof(respdu->caData) - 1);
        memcpy(respdu->vMsg.data(), pathData.constData(), pathData.size());
        m_strName = caName; // 登录成功，记录用户名
        emit loggedIn(this, m_strName); // 通知服务器该用户已登录
            sendPdu(std::move(respdu));
    } else {
        auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_LOGIN_RESPOND);
        strncpy(respdu->caData, LOGIN_FAILED, sizeof(respdu->caData) - 1);
        sendPdu(std::move(respdu));
    }
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
    // 从 pdu->caData 中提取请求者A和被请求者B的名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid add friend request format";
        return;
    }
    QString caPerName = QString::fromUtf8(parts[0]); // 被请求者B
    QString caName = QString::fromUtf8(parts[1]);    // 请求者A
    qDebug() << "添加好友请求 -> 请求者:" << caName << "被请求者:" << caPerName;
    // 调用数据库操作类处理添加好友请求
    int res = OpeDB::getInstance().handleAddFriend(caName.toStdString().c_str(), caPerName.toStdString().c_str());
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

/**
 * @brief 处理同意添加好友的请求 (B -> 服务器 -> A)
 * @param pdu 包含同意信息的PDU。caData格式: "请求者A\0同意者B"
 */
void MyTcpSocket::handleAddFriendAgree(const PDU& pdu){
    // 1. 解析名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid add friend agree format";
        return;
    }
    QString caName = QString::fromUtf8(parts[0]);    // 请求者A
    QString caPerName = QString::fromUtf8(parts[1]); // 同意者B
    qDebug() << "同意添加好友 -> 请求者:" << caName << "同意者:" << caPerName;
    // 调用数据库操作类处理添加好友请求
    OpeDB::getInstance().handleAddFriendAgree(caName.toStdString().c_str(), caPerName.toStdString().c_str());

    // 2. 创建响应PDU，通知 A
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_ADD_FRIEND_RESPOND);

    // 3. 准备要转发给 A 的消息
    QString successMsgg = QString("%1 已同意您的好友请求").arg(caPerName);
    strncpy(respdu->caData, successMsgg.toStdString().c_str(), sizeof(respdu->caData) - 1);
    // 将响应发送给请求者A
    MyTcpServer::getInstance().resend(caName, *respdu);
}

/**
 * @brief 处理拒绝添加好友的请求 (B -> 服务器 -> A)
 * @param pdu 包含拒绝信息的PDU。caData格式: "请求者A\0拒绝者B"
 */

void MyTcpSocket::handleAddFriendRefuse(const PDU& pdu){
    // 1. 解析名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid add friend refuse format";
        return;
    }
    QString caName = QString::fromUtf8(parts[0]);    // 请求者A
    QString caPerName = QString::fromUtf8(parts[1]); // 拒绝者B
    qDebug() << "拒绝添加好友 -> 请求者:" << caName << "拒绝者:" << caPerName;

    // 2. 创建响应PDU，通知 A
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_ADD_FRIEND_RESPOND);

    // 3. 准备要转发给 A 的消息
    QString refuseMsg = QString("%1 拒绝了您的好友请求").arg(caPerName);
    strncpy(respdu->caData, refuseMsg.toStdString().c_str(), sizeof(respdu->caData) - 1);
    // 将响应发送给请求者A
    MyTcpServer::getInstance().resend(caName, *respdu);
}

/**
 * @brief 处理刷新好友列表的请求
 * @param pdu 包含请求者用户名的PDU
 */
void MyTcpSocket::handleFlushFriendRequest(const PDU& pdu){
    // 1. 提取请求者用户名
    QString caName = QString::fromUtf8(pdu.caData);
    qDebug() << "刷新好友列表请求 -> 用户名:" << caName;

    // 2. 调用数据库操作类获取好友列表
    qDebug() << "调用数据库操作，查询用户 " << caName << " 的好友列表...";
    QStringList friendList = OpeDB::getInstance().handleFlushFriend(caName.toStdString().c_str());
    qDebug() << "数据库返回的好友列表:" << friendList;

    // 3.使用 QDataStream 序列化好友列表
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << friendList;
    qDebug() << "序列化好友列表完成，数据大小为:" << buffer.size();

    // 4. 创建响应PDU并填充数据
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND, buffer.size());
    qDebug() << "创建响应PDU，消息类型: ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND, 数据大小:" << respdu->vMsg.size();

    // 5. 将序列化后的数据拷贝到PDU的可变消息区
    if (!buffer.isEmpty()) {
        memcpy(respdu->vMsg.data(), buffer.constData(), buffer.size());
        qDebug() << "好友列表数据成功拷贝到PDU中。";
    } else {
        qWarning() << "好友列表为空，没有数据需要拷贝。";
    }

    // 6. 发送PDU给客户端
    sendPdu(std::move(respdu));
    qDebug() << "响应PDU已发送给客户端。";
}

/**
 * @brief 处理删除好友的请求
 * @param pdu 包含请求者和被删除者信息的PDU。caData格式: "请求者\0被删除的好友"
 */
void MyTcpSocket::handleDelFriendRequest(const PDU& pdu){
    // 1. 解析名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid delete friend request format";
        return;
    }

    QString caName = QString::fromUtf8(parts[0]);    // 请求者
    QString caPerName = QString::fromUtf8(parts[1]); // 被删除的好友
    qDebug() << "删除好友请求 -> 请求者:" << caName << "被删除的好友:" << caPerName;

    // 2. 调用数据库操作类处理删除好友请求
    int ret = OpeDB::getInstance().handleDelFriend(caName.toStdString().c_str(), caPerName.toStdString().c_str());

    // 3. 创建响应PDU
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_DEL_FRIEND_RESPOND);
    if(ret){
        strncpy(respdu->caData, "删除好友成功", sizeof(respdu->caData) - 1);

        //4. 通知被删除的好友
        auto noticePdu = make_pdu(MsgType::ENUM_MSG_TYPE_DEL_FRIEND_NOTICE);
        strncpy(noticePdu->caData, caName.toStdString().c_str(), sizeof(noticePdu->caData) - 1);
        MyTcpServer::getInstance().resend(caPerName, *noticePdu);
    }else{
        strncpy(respdu->caData, "删除好友失败", sizeof(respdu->caData) - 1);
    }
    // 5. 发送响应PDU给请求者
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理私聊请求并转发
 * @param pdu 包含私聊信息的PDU。caData格式: "发送者\0接收者", vMsg: 聊天内容
 */
void MyTcpSocket::handlePrivateChatRequest(const PDU& pdu){
    // 1. 解析名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid private chat request format";
        return;
    }

    QString caName = QString::fromUtf8(parts[0]);    // 发送者
    QString caPerName = QString::fromUtf8(parts[1]); // 接收者
    qDebug() << "私聊请求 -> 发送者:" << caName << "接收者:" << caPerName;

    // 2. 直接转发给接收者
    MyTcpServer::getInstance().resend(caPerName, pdu);
}

/**
 * @brief 处理群聊请求并转发给所有在线好友
 * @param pdu 包含群聊信息的PDU。caData格式: "发送者", vMsg: 聊天内容
 */
void MyTcpSocket::handleGroupChatRequest(const PDU& pdu){
    // 1. 解析发送者名字
    QString senderName = QString::fromUtf8(pdu.caData);
    qDebug() << "群聊请求 -> 发送者:" << senderName;
    // 2. 获取发送者的好友列表
    QStringList friendListWithStatus = OpeDB::getInstance().handleFlushFriend(senderName.toStdString().c_str());
    // 3. 遍历好友列表，逐个转发消息
    for(const QString& friendInfo : friendListWithStatus){
        if(friendInfo.endsWith("(在线)")){
            QString friendName = friendInfo.left(friendInfo.lastIndexOf('('));
            MyTcpServer::getInstance().resend(friendName, pdu);
        }
    }
}

/**
 * @brief 处理客户端创建文件夹的请求
 * @param pdu 包含文件夹信息的请求PDU
 */
void MyTcpSocket::handleCreateDirRequest(const PDU& pdu)
{
    // 1. 解析请求数据
    QString newDirName = QString::fromUtf8(pdu.caData);
    QString currentPath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    if (newDirName.isEmpty() || currentPath.isEmpty()) {
        qWarning() << "Create dir request failed: Invalid parameters.";
        return;
    }

    qDebug() << "创建文件夹请求 -> 路径:" << currentPath << "新文件夹名:" << newDirName;

    // 2. 准备响应PDU
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_CREATE_DIR_RESPOND);
    const char* responseMsg = nullptr;

    // 3. 执行核心逻辑
    QDir dir(currentPath);
    if (!dir.exists()) {
        responseMsg = "当前路径不存在，创建失败";
    } else {
        QString newPath = dir.filePath(newDirName);
        if (dir.exists(newPath)) {
            responseMsg = "文件夹已存在，创建失败";
        } else {
            if (dir.mkdir(newDirName)) {
                responseMsg = "创建文件夹成功";
            } else {
                responseMsg = "创建文件夹失败，可能是权限不足";
            }
        }
    }

    // 4. 填充并发送响应
    strncpy(respdu->caData, responseMsg, sizeof(respdu->caData) - 1);
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理刷新文件列表的请求 (实际执行者)
 * @param path 需要刷新文件列表的目录路径
 */
void MyTcpSocket::handleFlushFileRequest(const QString& path)
{
    qDebug() << "刷新文件列表 -> 路径:" << path;

    QFileInfoList fileInfoList = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    QList<FileInfo> fileList;
    for (const QFileInfo& fileInfo : fileInfoList) {
        FileInfo info;
        info.sFileName = fileInfo.fileName().toStdString();
        info.iFileType = fileInfo.isDir() ? 0 : 1;
        fileList.append(info);
    }

    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << fileList;

    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_FLUSH_FILE_RESPOND, buffer.size());
    if (!buffer.isEmpty()) {
        memcpy(respdu->vMsg.data(), buffer.constData(), buffer.size());
    }
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理客户端刷新文件列表的请求 (协议接口)
 * @param pdu 包含目录路径的请求PDU
 */
void MyTcpSocket::handleFlushFileRequest(const PDU& pdu)
{
    QString path = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
    handleFlushFileRequest(path);
}

/**
 * @brief 处理删除文件或文件夹的请求
 * @param pdu 包含项目信息的请求PDU
 */
void MyTcpSocket::handleDelItemRequest(const PDU& pdu){
    // 1. 解析请求数据
    QString itemName = QString::fromUtf8(pdu.caData);
    QString parentPath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
    QString fullPath = QDir(parentPath).filePath(itemName);
    qDebug() << "删除项目请求 -> 路径:" << fullPath;

    // 2. 准备响应PDU
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_DEL_ITEM_RESPOND);
    const char* responseMsg = nullptr;

    // 3. 执行删除操作
    QFileInfo fileInfo(fullPath);
    if(!fileInfo.exists()){
        responseMsg = "项目不存在，删除失败";
    }else{
        bool success = false;
        if(fileInfo.isDir()){
            QDir dir(fullPath);
            success = dir.removeRecursively();
        }else{
            success = QFile::remove(fullPath);
        }
        responseMsg = success ? "删除成功" : "删除失败，可能是权限不足";
    }
    // 4. 填充并发送响应
    strncpy(respdu->caData, responseMsg, sizeof(respdu->caData) - 1);
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理重命名文件或文件夹的请求
 * @param pdu 包含重命名信息的请求PDU
 */
void MyTcpSocket::handleRenameDirRequest(const PDU& pdu){
    // 1. 解析请求数据
    QList<QByteArray> parts = QByteArray(pdu.caData,sizeof(pdu.caData)).split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid rename request format";
        return;
    }
    QString oldName = QString::fromUtf8(parts[0]);
    QString newName = QString::fromUtf8(parts[1]);
    QString parentPath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    QString oldPath = QDir(parentPath).filePath(oldName);
    QString newPath = QDir(parentPath).filePath(newName);

    qDebug() << "重命名请求 -> 旧名称:" << oldPath << "新名称:" << newPath;

    // 2. 准备响应PDU
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_RENAME_DIR_RESPOND);
    const char* responseMsg = nullptr;

    // 3. 执行重命名操作
    if(QFile::exists(newPath)){
        responseMsg = "新名称已存在，重命名失败";
    }else{
        bool success = QFile::rename(oldPath, newPath);
        responseMsg = success ? "重命名成功" : "重命名失败";
    }
    // 4. 填充并发送响应
    strncpy(respdu->caData, responseMsg, sizeof(respdu->caData) - 1);
    sendPdu(std::move(respdu));
}

/**
 * @brief 处理进入文件夹的请求
 * @param pdu caData中是要进入的文件夹名, vMsg中是当前路径
 */
void MyTcpSocket::handleEntryDirRequest(const PDU &pdu)
{
    // 1. 解析请求数据
    QString dirName = QString::fromUtf8(pdu.caData);
    QString currentPath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
    QString enterPath = QDir(currentPath).filePath(dirName);

    qDebug() << "进入文件夹请求 -> 当前路径:" << currentPath << "目标文件夹:" << dirName;

    // 2. 检查路径有效性
    QFileInfo fileInfo(enterPath);
    if (!fileInfo.exists() || !fileInfo.isDir()) {
        // 目标不存在或是个文件，发送错误响应
        auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_ENTRY_DIR_RESPOND);
        const char* errorMsg = fileInfo.exists() ? "进入失败：目标不是文件夹" : "进入失败：目标不存在";
        strncpy(respdu->caData, errorMsg, sizeof(respdu->caData) - 1);
        sendPdu(std::move(respdu));
        return;
    }

    // 3. 目标是有效文件夹，直接调用刷新文件列表的逻辑
    // 注意：这里我们直接传递新的路径，而不是整个PDU
    handleFlushFileRequest(enterPath);
}

/**
 * @brief 处理上传文件的初始请求
 * @param pdu caData格式为"文件名#文件大小", vMsg为上传目录
 */
void MyTcpSocket::handleUploadFileRequest(const PDU& pdu)
{
    // 1. 解析文件名和大小
    QString strData = QString::fromUtf8(pdu.caData);
    QString fileName = strData.section('#', 0, 0);
    qint64 fileSize = strData.section('#', 1, 1).toLongLong();

    // 2. 解析保存路径并构建完整路径
    QString savePath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
    QString fullSavePath = QDir(savePath).filePath(fileName);

    qDebug() << "上传文件请求 ->" << fullSavePath << "大小:" << fileSize;

    // 3. 准备接收文件
    m_file.setFileName(fullSavePath);
    if (m_file.open(QIODevice::WriteOnly)) {
        // 文件创建成功，设置状态变量
        m_bUpload = true;
        m_iTotal = fileSize;
        m_iRecved = 0;

        // 回复客户端：准备就绪，可以发送文件数据
        auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_READY_RESPOND);
        sendPdu(std::move(respdu));
    } else {
        // 文件创建失败，直接回复最终的失败响应
        auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND);
        strncpy(respdu->caData, "服务器创建文件失败", sizeof(respdu->caData) - 1);
        sendPdu(std::move(respdu));
    }
}

/**
 * @brief 处理下载文件的请求
 * @param pdu caData为文件名, vMsg为文件所在目录
 */
void MyTcpSocket::handleDownloadFileRequest(const PDU& pdu)
{
    // 1. 解析文件名和路径
    QString fileName = QString::fromUtf8(pdu.caData);
    QString filePath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
    QString fullPath = QDir(filePath).filePath(fileName);

    qDebug() << "下载文件请求 ->" << fullPath;

    // 2. 检查文件有效性
    QFileInfo fileInfo(fullPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND);
        // 通过文件大小为-1来表示文件无效
        QString responseData = QString("%1#%2").arg(fileName).arg(-1);
        strncpy(respdu->caData, responseData.toStdString().c_str(), sizeof(respdu->caData) - 1);
        sendPdu(std::move(respdu));
        return;
    }

    // 3. 文件有效，准备发送
    qint64 fileSize = fileInfo.size();

    // 4. 先回复客户端，告知文件名和大小
    auto respdu = make_pdu(MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND);
    QString responseData = QString("%1#%2").arg(fileName).arg(fileSize);
    strncpy(respdu->caData, responseData.toStdString().c_str(), sizeof(respdu->caData) - 1);
    sendPdu(std::move(respdu));

    // 5. 准备通过定时器发送文件内容
    m_file.setFileName(fullPath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qWarning() << "打开文件失败，无法发送:" << fullPath;
        return;
    }

    // 启动定时器，sendFileToClient 会被循环调用
    m_pTimer->start(1);
}

/**
 * @brief 处理客户端分享文件/文件夹的请求 (A -> Server -> B, C, D...)
 * @param pdu pdu.caData: 分享者A的名字. pdu.vMsg: [文件路径]\0[好友B(32字节)][好友C(32字节)]...
 */
void MyTcpSocket::handleShareFileRequest(const PDU& pdu)
{
    // 1. 解析分享者名字
    QString sharerName = QString::fromUtf8(pdu.caData);

    // 2. 使用 QByteArray 安全地解析 vMsg
    QByteArray vMsgData(pdu.vMsg.data(), pdu.vMsg.size());
    int separatorIndex = vMsgData.indexOf('\0');
    if (separatorIndex == -1) {
        qWarning() << "Invalid share file request format: no separator found.";
        return;
    }

    // a. 提取文件路径
    QByteArray filePathData = vMsgData.left(separatorIndex);
    QString sharedFilePath = QString::fromUtf8(filePathData);

    // b. 提取好友列表数据块
    QByteArray friendListData = vMsgData.mid(separatorIndex + 1);
    int friendCount = friendListData.size() / 32;

    qDebug() << "分享请求 -> 分享者:" << sharerName
             << "文件:" << sharedFilePath
             << "分享给" << friendCount << "个好友";

    // 3. 为每个好友创建并转发分享通知
    for (int i = 0; i < friendCount; ++i) {
        // a. 从数据块中提取好友名字
        const char* friendNamePtr = friendListData.constData() + i * 32;
        QString friendName = QString::fromUtf8(friendNamePtr, strnlen(friendNamePtr, 32));

        // b. 创建通知PDU
        auto noticePdu = make_pdu(MsgType::ENUM_MSG_TYPE_SHARE_FILE_NOTICE, filePathData.size());

        // c. 填充通知PDU：caData放分享者名字，vMsg放文件路径
        strncpy(noticePdu->caData, sharerName.toStdString().c_str(), sizeof(noticePdu->caData) - 1);
        memcpy(noticePdu->vMsg.data(), filePathData.constData(), filePathData.size());

        // d. 转发给在线好友
        MyTcpServer::getInstance().resend(friendName, *noticePdu);
    }

    // 4. 回复分享者，告知请求已处理
    auto resPdu = make_pdu(MsgType::ENUM_MSG_TYPE_SHARE_FILE_RESPOND);
    strncpy(resPdu->caData, "文件分享请求已成功发送", sizeof(resPdu->caData) - 1);
    sendPdu(std::move(resPdu));
}

/**
 * @brief 处理好友同意接收分享文件的响应 (B -> Server)
 * @param pdu pdu.caData: 接收者B的名字. pdu.vMsg: 分享者A提供的原始文件路径
 */
void MyTcpSocket::handleShareFileNoticeRespond(const PDU& pdu)
{
    // 1. 解析接收者名字和原始分享路径
    QString receiverName = QString::fromUtf8(pdu.caData);
    QString sharedFilePath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    // 2. 从分享路径中提取文件名或文件夹名
    QFileInfo sharedFileInfo(sharedFilePath);
    if (!sharedFileInfo.exists()) {
        qWarning() << "Share file failed: source file does not exist ->" << sharedFilePath;
        // （可选）可以通知接收者分享失败
        return;
    }
    QString baseName = sharedFileInfo.fileName();

    // 3. 构建目标路径：./user_data/接收者名字/文件名
    QString destPath = QString("./user_data/%1/%2").arg(receiverName).arg(baseName);

    qDebug() << "接收分享文件 -> 接收者:" << receiverName << "源路径:" << sharedFilePath << "目标路径:" << destPath;

    // 4. 根据源是文件还是目录，执行不同的复制操作
    bool success = false;
    if (sharedFileInfo.isDir()) {
        // 使用我们之前有的 copyDir 函数
        copyDir(sharedFilePath, destPath);
        success = true; // copyDir 没有返回值，这里我们假设它总是成功
    } else if (sharedFileInfo.isFile()) {
        success = QFile::copy(sharedFilePath, destPath);
    }

    // 5. 回复接收者，告知操作结果
    auto resPdu = make_pdu(MsgType::ENUM_MSG_TYPE_RECEIVE_FILE_RESULT);
    const char* message = success ? "文件接收成功" : "文件接收失败";
    strncpy(resPdu->caData, message, sizeof(resPdu->caData) - 1);

    // 注意：这里是直接回复给当前socket的客户端，因为这个操作是由他触发的
    sendPdu(std::move(resPdu));
}

/**
 * @brief 处理客户端移动文件/文件夹的请求
 * @param pdu pdu.vMsg: [源路径]\0[目标文件夹路径]
 */
void MyTcpSocket::handleMoveFileRequest(const PDU& pdu)
{
    // 1. 使用 QByteArray::split 安全地解析源路径和目标路径
    QList<QByteArray> parts = QByteArray(pdu.vMsg.data(), pdu.vMsg.size()).split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid move file request format.";
        return;
    }
    QString srcPath = QString::fromUtf8(parts[0]);
    QString destDirPath = QString::fromUtf8(parts[1]);

    // 2. 从源路径提取文件名/文件夹名，并构建最终的目标路径
    QFileInfo srcFileInfo(srcPath);
    QString finalDestPath = QDir(destDirPath).filePath(srcFileInfo.fileName());

    qDebug() << "移动文件请求 -> 从:" << srcPath << "到:" << finalDestPath;

    // 3. 执行重命名操作 (在同一个文件系统中，rename即为移动)
    bool success = QFile::rename(srcPath, finalDestPath);

    // 4. 回复客户端操作结果
    auto resPdu = make_pdu(MsgType::ENUM_MSG_TYPE_MOVE_FILE_RESPOND);
    const char* message = success ? "移动成功" : "移动失败，请检查路径或权限";
    strncpy(resPdu->caData, message, sizeof(resPdu->caData) - 1);
    sendPdu(std::move(resPdu));
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
