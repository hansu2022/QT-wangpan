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
void Friend::showAllOnlineUsr(PDU *pdu)
{
    // 如果传入的 PDU 为空，则直接返回
    if(pdu == NULL){
        return;
    }
    // 调用 m_pOnline 对象的 showUsr() 方法来显示用户列表
    m_pOnline->showUsr(pdu);
}

void Friend::showAllFriend(PDU *pdu)
{
    if(pdu == NULL){
        return;
    }
    m_pFriendListWidget->clear();
    uint uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    for(uint i = 0;i<uiSize;i++){
        memcpy(caTmp,(char*)(pdu->caMsg)+i*32,32);
        m_pFriendListWidget->addItem(caTmp);
    }
    emit friendListUpdated(); // 发出好友列表更新信号
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

// 槽函数：处理“显示在线用户”按钮的点击事件
void Friend::showOnline()
{
    // 检查 m_pOnline 窗口是否被隐藏
    if(m_pOnline->isHidden()){
        // 如果隐藏，则显示窗口
        m_pOnline->show();
        // 创建一个 PDU 数据包
        PDU *pdu = mkPDU(0);
        // 设置消息类型为“所有在线用户请求”
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        // 获取 TCP 客户端实例并发送数据包
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        // 释放 PDU 内存
        free(pdu);
        pdu = NULL;
    }else{
        // 如果已显示，则隐藏窗口
        m_pOnline->hide();
    }
}

// 槽函数：处理“查找用户”按钮的点击事件
void Friend::searchUsr()
{
    // 弹出输入对话框，让用户输入要搜索的用户名
    m_strSearchName =QInputDialog::getText(this,"搜索","用户名:");
    // 检查用户是否输入了内容
    if(!m_strSearchName.isEmpty()){
        // 在调试控制台输出搜索的用户名
        qDebug() << m_strSearchName;
        // 创建一个 PDU 数据包
        PDU *pdu = mkPDU(0);
        // 设置消息类型为“查找用户请求”
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_REQUEST;
        // 将输入的用户名复制到 PDU 的数据区
        strncpy(pdu->caData,m_strSearchName.toStdString().c_str(),32);
        // 获取 TCP 客户端实例并发送数据包
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        // 释放 PDU 内存
        free(pdu);
        pdu = NULL;
    }
}

void Friend::flushFriend()
{
    QString strName = TcpClient::getInstance().getLoginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    strncpy(pdu->caData,strName.toStdString().c_str(),32);
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

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
    QString questionText = QString("您确定要删除好友 %1 吗？\n此操作不可恢复。").arg(strfriendName);
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, // 父窗口
                                  "删除好友确认", // 对话框标题
                                  questionText, // 对话框内容
                                  QMessageBox::Yes | QMessageBox::No, // 显示“是”和“否”两个按钮
                                  QMessageBox::No); // 默认选中的按钮是“否”

    if (reply == QMessageBox::Yes) {
        // 如果用户点击了“是”，才执行真正的删除逻辑（即发送请求给服务器）
        // 这部分代码就是您原来函数里的网络请求代码
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_REQUEST;
        QString strMyName = TcpClient::getInstance().getLoginName();

        // 确保数据顺序正确：自己的名字在前，好友的名字在后
        strncpy(pdu->caData, strMyName.toStdString().c_str(), 32);
        pdu->caData[31] = '\0';

        strncpy(pdu->caData + 32, strfriendName.toStdString().c_str(), 32);
        pdu->caData[63] = '\0';

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
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

    if(!strMsg.isEmpty()){
        PDU *pdu = mkPDU(strMsg.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
        QString strLoginName = TcpClient::getInstance().getLoginName();
        strncpy(pdu->caData,strLoginName.toStdString().c_str(),32);
        strcpy(pdu->caMsg,strMsg.toStdString().c_str());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
        QString strShowMsg = QString("我：%1").arg(strMsg);
        m_pShowMsgTE->append(strShowMsg);
        m_pInputMsgLE->clear();
    }else{
        QMessageBox::critical(this,"发送消息","发送消息不能为空");
    }
}

void Friend::updateGroupMsg(QString strMsg)
{
    if (m_pShowMsgTE != NULL) {
        m_pShowMsgTE->append(strMsg);
    }
}
