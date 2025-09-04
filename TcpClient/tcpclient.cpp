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

void TcpClient::sendPdu(std::unique_ptr<PDU> pdu)
{
    if (!pdu || m_tcpSocket.state() != QAbstractSocket::ConnectedState) {
        return; // 如果pdu为空或socket未连接，则不执行任何操作
    }
    // 调用pdu的serialize方法，得到包含所有待发送数据的vector
    std::vector<char> serialized_data = pdu->serialize();

    // 将vector的数据写入socket
    m_tcpSocket.write(serialized_data.data(), serialized_data.size());
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

void TcpClient::setEnterDirName(const QString &name)
{
    m_strEnterDirName = name;
}

void TcpClient::setCurPath(QString setCurPath)
{
    m_strCurPath = setCurPath;
}

QString TcpClient::getRootPath()
{
    return m_strRootPath;
}

QString TcpClient::getServerIP() const
{
    return m_strIP;
}


quint16 TcpClient::getServerPort() const
{
    return m_usPort;
}

// 槽函数：连接成功时显示消息框
void TcpClient::showConnect()
{
    QMessageBox::information(this, "连接服务器", "连接服务器成功");
}

// 槽函数：处理接收到的数据
void TcpClient::recvMsg()
{

    // 1.将socket中所有可读数据追加到缓冲区
    m_buffer.append(m_tcpSocket.readAll());

    Book *pBook = OpeWidget::getInstance().getBook();

    // 2. 优先处理文件下载数据流
    if (pBook->getDownloadStatus()) {
        // 写入所有已接收的数据到文件
        qint64 bytesWritten = m_file.write(m_buffer);
        if (bytesWritten > 0) {
            m_buffer.remove(0, bytesWritten); // 从缓冲区移除已写入的部分
            pBook->m_iRecved += bytesWritten;
        } else if (bytesWritten == -1) {
            // 文件写入出错
            QMessageBox::warning(this, "下载失败", "写入本地文件失败: " + m_file.errorString());
            m_file.close();
            pBook->setDownloadStatus(false);
            // 清空缓冲区，因为不知道数据是否正确
            m_buffer.clear();
            return;
        }

        // 检查下载是否完成
        if (pBook->m_iRecved >= pBook->m_iTotal) {
            m_file.close();
            QMessageBox::information(this, "下载文件", "文件下载成功！");

            // 重置下载状态
            pBook->m_iRecved = 0;
            pBook->m_iTotal = 0;
            pBook->setDownloadStatus(false);

            // 重要：下载完成后，缓冲区里可能还有下一个PDU的数据，
            // 所以不能直接 return，需要继续执行下面的PDU解析逻辑。
        } else {
            // 文件还未下载完，直接返回，等待下一次 readyRead 信号
            return;
        }
    }

    // 2. 循环处理缓冲区中的数据，直到数据不足一个完整的包\(仅在非下载状态，或下载刚完成时) 按PDU格式循环处理数据
    while(true){
        // 2.1 首先判断缓冲区数据是否足够读取一个PDU的头部长度
        if (m_buffer.size() < sizeof(uint)) {
            break; // 数据不够，等待下一次readyRead信号
        }

        // 2.2 预览包头中的总长度信息
        uint uiPDULen = 0;
        memcpy(&uiPDULen, m_buffer.constData(), sizeof(uint));
        // 2.3 判断缓冲区数据是否足够一个完整的PDU
        if (m_buffer.size() < uiPDULen) {
            break; // 数据不够，等待下一次readyRead信号
        }
        // 3. 数据包完整，开始安全地反序列化
        std::unique_ptr<PDU> pdu = PDU::deserialize(m_buffer.constData(), uiPDULen);

        // 4. 将处理完的数据从缓冲区头部移除
        m_buffer.remove(0, uiPDULen);

        // 5. 如果反序列化失败，丢弃这个包
        if (!pdu) {
            qWarning() << "反序列化失败，可能是一个损坏的数据包。已丢弃";
            continue;
        }

        switch (pdu->uiMsgType) {
        case MsgType::ENUM_MSG_TYPE_REGIST_RESPOND:
            handleRegistResponse(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_LOGIN_RESPOND:
            handleLoginResponse(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:
            OpeWidget::getInstance().getFriend()->showAllOnlineUsr(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_SEARCH_USR_RESPOND:
            handleSearchUsrResponse(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
            handleFriendRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:
            // 逻辑简单，直接处理
            QMessageBox::information(this, "添加好友", pdu->caData);
            break;
        case MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:
            OpeWidget::getInstance().getFriend()->showAllFriend(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DEL_FRIEND_RESPOND:
            handleDelFriendResponse(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DEL_FRIEND_NOTICE:
            handleDelFriendNotice(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
            handlePrivateChatRequest(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
            OpeWidget::getInstance().getFriend()->updateGroupMsg(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_CREATE_DIR_RESPOND:
            QMessageBox::information(this, "新建文件夹", pdu->caData);
            // 新建文件夹成功后，刷新文件列表
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;
        case MsgType::ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:
            if (!m_strEnterDirName.isEmpty()) {
                // 拼接成新的当前路径
                m_strCurPath = m_strCurPath + "/" + m_strEnterDirName;
                // 重要：更新完路径后，立即清空 m_strEnterDirName
                // 防止下次普通刷新时也错误地拼接路径
                m_strEnterDirName.clear();
            }
            OpeWidget::getInstance().getBook()->flushFile(*pdu);
            break;
        case MsgType::ENUM_MSG_TYPE_DEL_ITEM_RESPOND:
            QMessageBox::information(this, "删除文件/文件夹", pdu->caData);
            // 删除文件/文件夹成功后，刷新文件列表
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;
        case MsgType::ENUM_MSG_TYPE_RENAME_DIR_RESPOND:
            QMessageBox::information(this, "重命名文件夹", pdu->caData);
            // 重命名文件夹成功后，刷新文件列表
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;
        case MsgType::ENUM_MSG_TYPE_ENTRY_DIR_RESPOND:
            QMessageBox::warning(this, "进入文件夹", pdu->caData);
            break;
        case MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:
            QMessageBox::information(this, "上传文件", pdu->caData);
            // 上传文件成功后，刷新文件列表
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;
        case MsgType::ENUM_MSG_TYPE_SHARE_FILE_NOTICE:
            handleShareFileNotice(*pdu); // 调用新的独立函数处理
            break;

        case MsgType::ENUM_MSG_TYPE_SHARE_FILE_RESPOND:
            QMessageBox::information(this, "文件分享", pdu->caData);
            break;

        case MsgType::ENUM_MSG_TYPE_RECEIVE_FILE_RESULT:
            QMessageBox::information(this, "文件接收", pdu->caData);
            // 接收成功或失败后，都刷新一下当前目录，以便看到新文件
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;

        case MsgType::ENUM_MSG_TYPE_MOVE_FILE_RESPOND:
            QMessageBox::information(this, "移动文件", pdu->caData);
            // 移动成功或失败后，刷新文件列表以反映变化
            OpeWidget::getInstance().getBook()->flushFileSlot();
            break;
        default:
            qWarning() << "Unhandled message type:" << static_cast<uint>(pdu->uiMsgType);
            break;
        }
    }
}

void TcpClient::handleRegistResponse(const PDU& pdu)
{
    if (strcmp(pdu.caData, REGIST_OK) == 0) {
        QMessageBox::information(this, "注册", REGIST_OK);
    } else if (strcmp(pdu.caData, REGIST_FAILED) == 0) {
        QMessageBox::warning(this, "注册", REGIST_FAILED);
    }
}

void TcpClient::handleLoginResponse(const PDU& pdu)
{
    if (strcmp(pdu.caData, LOGIN_OK) == 0) {
        m_strLoginName = ui->name_le->text();
        // 从vMsg中获取服务器返回的根目录路径
        QString rootPath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());
        if (rootPath.isEmpty()) {
            QMessageBox::critical(this, "登录失败", "服务器未返回正确的用户路径。");
            return;
        }
        setCurPath(rootPath);     // 设置当前路径
        m_strRootPath = rootPath; // 保存这个路径为权威的根目录
        OpeWidget::getInstance().setUsrName(m_strLoginName);
        OpeWidget::getInstance().show();

        this->hide();
    } else if (strcmp(pdu.caData, LOGIN_FAILED) == 0) {
        QMessageBox::warning(this, "登录", LOGIN_FAILED);
    }
}

void TcpClient::handleSearchUsrResponse(const PDU& pdu)
{
    // 这里的响应消息在重构时已经统一为字符串
    QMessageBox::information(this, "搜索用户", pdu.caData);
}

void TcpClient::handleFriendRequest(const PDU& pdu)
{
    // 1. 解析请求者名字
    QByteArray data(pdu.caData, sizeof(pdu.caData));
    QList<QByteArray> parts = data.split('\0');
    if (parts.size() < 2) {
        qWarning() << "Invalid friend request PDU received";
        return;
    }
    QString caName = QString::fromUtf8(parts[1]); // 请求者名字在第二部分

    // 2. 弹出对话框询问用户是否接受好友请求
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "好友请求", QString("%1 想添加您为好友，是否同意？").arg(caName),
                                  QMessageBox::Yes | QMessageBox::No);

    // 3. 根据用户选择，发送同意或拒绝的响应
    auto responsePdu = make_pdu(reply == QMessageBox::Yes ? MsgType::ENUM_MSG_TYPE_ADD_FRIEND_AGREE : MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REFUSE);

    // 准备要发送的数据 ("请求者\0回应者")
    QByteArray resData;
    resData.append(caName.toUtf8()); // 请求者A
    resData.append('\0');
    resData.append(m_strLoginName.toUtf8()); // 回应者B (自己)
    memcpy(responsePdu->caData, resData.constData(), resData.size());

    sendPdu(std::move(responsePdu));
}

void TcpClient::handleDelFriendResponse(const PDU& pdu)
{
    QMessageBox::information(this, "删除好友", pdu.caData);
    // 删除好友成功后，刷新好友列表
    auto pduFlush = make_pdu(MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST);
    QString strMyName = getLoginName();
    memcpy(pduFlush->caData, strMyName.toUtf8().constData(), strMyName.size());
    sendPdu(std::move(pduFlush));
}

void TcpClient::handleDelFriendNotice(const PDU& pdu){
    QString removerName = QString::fromUtf8(pdu.caData);
    QMessageBox::information(this, "好友关系", QString("%1 已将您从好友列表中移除。").arg(removerName));
    OpeWidget::getInstance().getFriend()->flushFriend();
}

void TcpClient::handlePrivateChatRequest(const PDU& pdu)
{
    QString senderName = QString::fromUtf8(pdu.caData);
    QString msgContent = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    // 显示私聊消息
    PrivateChat &privateChat = PrivateChat::getInstance();
    if (privateChat.isHidden()) {
        privateChat.show();
    }
    privateChat.setChatName(senderName); // 设置聊天对象的名字
    privateChat.setWindowTitle(QString("与 %1 的私聊").arg(senderName)); // 设置窗口标题
    privateChat.showMsg(pdu); // 显示消息
}

void TcpClient::handleDownloadFileResponse(const PDU &pdu)
{
    // 1. 使用 QString::section 安全地解析文件名和大小
    QString dataStr = QString::fromUtf8(pdu.caData);
    QString fileName = dataStr.section('#', 0, 0);
    qint64 fileSize = dataStr.section('#', 1, 1).toLongLong();

    Book *pBook = OpeWidget::getInstance().getBook();

    // 2. 根据服务器返回的文件大小判断是否有效
    if (fileSize == -1) { // -1 是我们约定的错误码
        QMessageBox::warning(this, "下载文件", "服务器未能找到该文件或文件无效。");
        return;
    }

    // 3. 准备接收文件
    pBook->m_iTotal = fileSize;
    pBook->m_iRecved = 0;
    pBook->setDownloadStatus(true);

    m_file.setFileName(pBook->getSaveFilePath());
    if (!m_file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "下载文件", "本地文件创建失败，请检查路径或权限。");
        pBook->setDownloadStatus(false); // 打开失败，重置状态
    }
}

/**
 * @brief 处理来自其他用户的文件分享通知
 * @param pdu pdu.caData: 分享者名字. pdu.vMsg: 分享的文件路径
 */
void TcpClient::handleShareFileNotice(const PDU &pdu)
{
    // 1. 从 PDU 中安全地解析出分享者名字和文件路径
    // 分享者名字在 caData
    QString sharerName = QString::fromUtf8(pdu.caData);
    // 文件路径在 vMsg
    QString filePath = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    // 2. 弹出对话框，询问用户是否接受
    QString strMessage = QString("%1 分享了文件给你：%2\n是否接受？")
                             .arg(sharerName)
                             .arg(filePath);
    QMessageBox::StandardButton choice = QMessageBox::question(this, "文件分享通知", strMessage);

    // 3. 如果用户同意，则向服务器发送回应
    if (choice == QMessageBox::Yes) {
        // a. 使用工厂函数创建回应PDU，vMsg大小为文件路径的长度
        auto resPdu = make_pdu(MsgType::ENUM_MSG_TYPE_SHARE_FILE_NOTICE_RESPOND, pdu.vMsg.size());

        // b. 在 caData 中放入自己的名字，告诉服务器文件要复制给谁
        strncpy(resPdu->caData, getLoginName().toStdString().c_str(), sizeof(resPdu->caData) - 1);

        // c. 在 vMsg 中回传要接收的文件路径 (从收到的pdu中直接拷贝)
        memcpy(resPdu->vMsg.data(), pdu.vMsg.data(), pdu.vMsg.size());

        // d. 使用统一的接口发送PDU
        sendPdu(std::move(resPdu));
    }
    // 如果用户选择 "No"，则不执行任何操作
}




// 登录按钮的槽函数
void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if (strName.isEmpty() || strPwd.isEmpty()) {
        QMessageBox::critical(this, "登录", "登录失败：用户名或密码为空");
        return;
    }
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_LOGIN_REQUEST);
    // 将用户名和密码复制到 PDU 数据区
    QByteArray data;
    data.append(strName.toUtf8());
    data.append('\0');
    data.append(strPwd.toUtf8());
    memcpy(pdu->caData, data.constData(), sizeof(pdu->caData) - 1);
    sendPdu(std::move(pdu));
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
    if (strName.isEmpty() || strPwd.isEmpty()) {
        QMessageBox::critical(this, "注册", "注册失败：用户名或密码为空");
        return;
    }
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_REGIST_REQUEST);
    // 将用户名和密码复制到 PDU 数据区
    QByteArray data;
    data.append(strName.toUtf8());
    data.append('\0');
    data.append(strPwd.toUtf8());
    memcpy(pdu->caData, data.constData(), sizeof(pdu->caData) - 1);
    sendPdu(std::move(pdu));
}
