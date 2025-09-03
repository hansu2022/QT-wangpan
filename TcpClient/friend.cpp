#include "friend.h"  // 包含自定义的头文件，通常包含 Friend 类的声明
#include "tcpclient.h" // 包含 TCP 客户端类的头文件，用于网络通信
#include <QInputDialog> // 包含 Qt 输入对话框的头文件，用于获取用户输入
#include <qmessagebox.h> // 包含 Qt 消息框的头文件，用于显示消息
#include "privatechat.h"
// Friend 类的构造函数，QWidget *parent 参数指定了父窗口

Friend::Friend(QWidget *parent)
    : QWidget{parent}
{
    // 实例化UI控件
    // 显示消息的文本编辑框
    m_pShowMsgTE = new QTextEdit;
    // 好友列表
    m_pFriendListWidget = new QListWidget;
    // 消息输入框
    m_pInputMsgLE = new QLineEdit;

    // 实例化操作按钮
    // 删除好友按钮
    m_pDelFriendPB = new QPushButton("删除好友");
    // 刷新好友列表按钮
    m_pFlushFriendPB = new QPushButton("显示好友列表");
    // 显示在线用户按钮
    m_pShowOnlineUsrPB = new QPushButton("显示在线用户");
    // 查找用户按钮
    m_pSearchUsrPB = new QPushButton("查找用户");
    // 发送消息按钮
    m_pMsgSendPB = new QPushButton("群发信息");
    // 私聊按钮
    m_pPrivateChatPB = new QPushButton("私聊");

    // 创建右侧按钮的垂直布局
    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUsrPB);
    pRightPBVBL->addWidget(m_pSearchUsrPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);

    // 创建顶部的水平布局，包含消息显示区、好友列表和右侧按钮区
    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    // 创建消息输入和发送按钮的水平布局
    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    // 实例化一个在线用户列表窗口
    m_pOnline = new Online;

    // 创建主垂直布局，用于组织整个窗口的UI
    QVBoxLayout *pMainVBL = new QVBoxLayout;
    pMainVBL->addLayout(pTopHBL);  // 添加顶部布局
    pMainVBL->addLayout(pMsgHBL);  // 添加消息输入布局
    pMainVBL->addWidget(m_pOnline); // 添加在线用户列表窗口
    m_pOnline->hide(); // 默认隐藏在线用户列表窗口

    // 将主布局设置给当前窗口
    setLayout(pMainVBL);

    // 连接信号和槽：当“显示在线用户”按钮被点击时，触发 showOnline() 槽函数
    connect(m_pShowOnlineUsrPB, &QPushButton::clicked, this, &Friend::showOnline); // 新写法
    // 连接信号和槽：当“查找用户”按钮被点击时，触发 searchUsr() 槽函数
    connect(m_pSearchUsrPB, &QPushButton::clicked, this, &Friend::searchUsr); // 新写法
    // 连接信号和槽：当“刷新好友列表”按钮被点击时，触发 flushFriend() 槽函数
    connect(m_pFlushFriendPB, &QPushButton::clicked, this, &Friend::flushFriend); // 新写法
    // 连接信号和槽：当“删除好友”按钮被点击时，触发 delFriend() 槽函数
    connect(m_pDelFriendPB, &QPushButton::clicked, this, &Friend::delFriend); // 新写法
    // 连接信号和槽：当“私聊”按钮被点击时，触发 privateChat() 槽函数
    connect(m_pPrivateChatPB, &QPushButton::clicked, this, &Friend::privateChat); // 新写法
    // 连接信号和槽：当“发送消息”按钮被点击时，触发 groupMsgSend() 槽函数
    connect(m_pMsgSendPB, &QPushButton::clicked, this, &Friend::groupMsgSend); // 新写法
}

// 槽函数：显示所有在线用户
// PDU* pdu 参数是来自服务器的数据单元，包含了在线用户信息
void Friend::showAllOnlineUsr(const PDU &pdu)
{
    m_pOnline->showUsr(pdu); // 调用 Online 类的方法显示在线用户
}

void Friend::showAllFriend(const PDU &pdu)
{
    qDebug() << "客户端 -> 收到服务器的好友列表响应。";
    m_pFriendListWidget->clear();

    // 1. 使用 QDataStream 反序列化服务器发来的好友列表
    QByteArray data(pdu.vMsg.data(), pdu.vMsg.size());
    QDataStream stream(&data, QIODevice::ReadOnly);
    QStringList friendList;
    stream >> friendList; // 从数据流中读出 QStringList
    qDebug() << "客户端 -> 反序列化完成，获得的好友列表为:" << friendList;

    // 2. 将好友列表显示在 QListWidget 中
    if(!friendList.isEmpty()){
        m_pFriendListWidget->addItems(friendList);
        qDebug() << "客户端 -> 好友列表已成功添加到 QListWidget。";
    } else {
        qDebug() << "客户端 -> 好友列表为空，QListWidget 被清空。";
    }

    // 3. 发出好友列表更新信号
    emit friendListUpdated(); // 发出好友列表更新信号
    qDebug() << "客户端 -> 已发出 friendListUpdated 信号。";
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

// 槽函数：处理“显示在线用户”按钮的点击事件
void Friend::showOnline()
{
    if (m_pOnline->isHidden()) {
        m_pOnline->show();
        // 使用工厂函数创建PDU，并用新接口发送
        auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_ALL_ONLINE_REQUEST);
        TcpClient::getInstance().sendPdu(std::move(pdu));
    } else {
        m_pOnline->hide();
    }
}

// 槽函数：处理“查找用户”按钮的点击事件
void Friend::searchUsr()
{
    m_strSearchName = QInputDialog::getText(this,"搜索", "用户名:");
    if(!m_strSearchName.isEmpty()){
        qDebug() << "Searching for:" << m_strSearchName;
        auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_SEARCH_USR_REQUEST);
        
        // 将输入的用户名复制到 PDU 的数据区
        strncpy(pdu->caData, m_strSearchName.toStdString().c_str(), sizeof(pdu->caData) - 1);
        TcpClient::getInstance().sendPdu(std::move(pdu));
    }
}

// 槽函数：处理“刷新好友列表”按钮的点击事件
void Friend::flushFriend()
{
    QString strName = TcpClient::getInstance().getLoginName();
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST);
    strncpy(pdu->caData, strName.toStdString().c_str(), sizeof(pdu->caData) - 1);
    TcpClient::getInstance().sendPdu(std::move(pdu));
}
//
void Friend::delFriend()
{
    if (m_pFriendListWidget->currentItem() == NULL) {
        QMessageBox::warning(this, "删除好友", "请先在列表中选择要删除的好友。");
        return;
    }
    // 获取完整的字符串，例如 "jack(在线)"
    QString strfriendInfo = m_pFriendListWidget->currentItem()->text();

    // 提取纯粹的用户名，通过查找左括号 '(' 来分割
    int pos = strfriendInfo.indexOf('(');
    QString strfriendName;
    if (pos != -1) {
        strfriendName = strfriendInfo.left(pos);
    } else {
        // 如果没有括号，则说明是纯用户名，直接使用
        strfriendName = strfriendInfo;
    }
    QString questionText = QString("您确定要删除好友 %1 吗？").arg(strfriendName);
    if (QMessageBox::question(this, "删除好友确认", questionText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_DEL_FRIEND_REQUEST);
        QString strMyName = TcpClient::getInstance().getLoginName();

        // 按照服务器新格式打包数据 ("自己\0好友")
        QByteArray data;
        data.append(strMyName.toUtf8());
        data.append('\0');
        data.append(strfriendName.toUtf8());
        memcpy(pdu->caData, data.constData(), data.size());

        TcpClient::getInstance().sendPdu(std::move(pdu));
    }// 如果用户点击了“否”，则什么也不做，函数直接结束。
}

// 槽函数：处理“私聊”按钮的点击事件
void Friend::privateChat()
{
    if(m_pFriendListWidget->currentItem() == NULL){
        QMessageBox::warning(this,"私聊","请先在列表中选择一个好友");
        return;
    }
    QString strName = m_pFriendListWidget->currentItem()->text();
    int pos = strName.indexOf('(');
    QString strfriendName;
    if (pos != -1) {
        strfriendName = strName.left(pos);
    } else {
        // 如果没有括号，则说明是纯用户名，直接使用
        strfriendName = strName;
    }
    PrivateChat &privateChat = PrivateChat::getInstance();

    if(PrivateChat::getInstance().isHidden()){
        privateChat.show();
    }
    privateChat.clearMsg();
    privateChat.setChatName(strfriendName); // 设置聊天对象的名字
    privateChat.setWindowTitle(QString("与 %1 的私聊").arg(strfriendName)); // 设置窗口标题

    if(privateChat.isHidden()){
        privateChat.show();
    }
}

void Friend::groupMsgSend()
{
    QString strMsg = m_pInputMsgLE->text();

    if (strMsg.isEmpty()) {
        QMessageBox::warning(this, "发送消息", "发送消息不能为空");
        return;
    }

    QString strLoginName = TcpClient::getInstance().getLoginName();
    QByteArray msgBytes = strMsg.toUtf8(); // 将消息转为UTF-8字节数组

    // 1. 创建PDU，消息长度为字节数组的长度
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_GROUP_CHAT_REQUEST, msgBytes.size());

    // 2. 发送者名字放入 caData
    strncpy(pdu->caData, strLoginName.toStdString().c_str(), sizeof(pdu->caData) - 1);

    // 3. 聊天内容放入 vMsg
    memcpy(pdu->vMsg.data(), msgBytes.constData(), msgBytes.size());

    // 4. 发送
    TcpClient::getInstance().sendPdu(std::move(pdu));

    // 5. 更新本地显示
    QString strShowMsg = QString("我：%1").arg(strMsg);
    m_pShowMsgTE->append(strShowMsg);
    m_pInputMsgLE->clear();
}

void Friend::updateGroupMsg(const PDU &pdu)
{
    // 从 pdu 中解析出发送者和消息内容
    QString senderName = QString::fromUtf8(pdu.caData);
    QString msgContent = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    QString strShowMsg = QString("%1：%2").arg(senderName).arg(msgContent);

    if (m_pShowMsgTE != nullptr) {
        m_pShowMsgTE->append(strShowMsg);
    }
}
