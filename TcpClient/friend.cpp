#include "friend.h"  // 包含自定义的头文件，通常包含 Friend 类的声明
#include "tcpclient.h" // 包含 TCP 客户端类的头文件，用于网络通信
#include <QInputDialog> // 包含 Qt 输入对话框的头文件，用于获取用户输入

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
    M_pInputMsgLE = new QLineEdit;

    // 实例化操作按钮
    // 删除好友按钮
    m_pDelFriendPB = new QPushButton("删除");
    // 刷新好友列表按钮
    m_pFlushFriendPB = new QPushButton("刷新");
    // 显示在线用户按钮
    m_pShowOnlineUsrPB = new QPushButton("显示在线用户");
    // 查找用户按钮
    m_pSearchUsrPB = new QPushButton("查找用户");
    // 发送消息按钮
    m_pMsgSendPB = new QPushButton("发送");
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
    pMsgHBL->addWidget(M_pInputMsgLE);
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
    connect(m_pShowOnlineUsrPB,SIGNAL(clicked(bool)),this,SLOT(showOnline()));
    // 连接信号和槽：当“查找用户”按钮被点击时，触发 searchUsr() 槽函数
    connect(m_pSearchUsrPB,SIGNAL(clicked(bool)),this,SLOT(searchUsr()));
    connect(m_pFlushFriendPB,SIGNAL(clicked(bool)),this,SLOT(flushFriend()));
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
