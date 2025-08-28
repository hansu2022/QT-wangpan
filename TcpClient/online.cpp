#include "online.h"     // 包含自定义头文件，通常包含 Online 类的声明
#include "ui_online.h"  // 包含由 Qt Designer 生成的 UI 头文件
#include "tcpclient.h"  // 包含 TCP 客户端类的头文件，用于获取登录名和发送数据
#include <QMessageBox>  // 提供 QMessageBox 类，用于显示消息框

// Online 类的构造函数
Online::Online(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Online) // 初始化 UI 对象
{
    ui->setupUi(this); // 设置 UI 界面
}

// 析构函数，用于释放 UI 对象
Online::~Online()
{
    delete ui;
}

// 显示在线用户列表的槽函数
// PDU* pdu 参数包含了从服务器接收到的在线用户信息
void Online::showUsr(PDU *pdu)
{
    // 如果传入的 PDU 为空，则直接返回
    if(pdu == NULL){
        return;
    }
    ui->Online_lw->clear(); // 清空列表控件中的所有项
    // 计算用户数量，每个用户名占用 32 字节
    uint uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    // 循环遍历 PDU 数据，将每个用户名添加到列表中
    for(uint i = 0;i<uiSize;i++){
        // 从 PDU 的消息数据区复制一个用户名
        memcpy(caTmp,(char*)(pdu->caMsg)+i*32,32);
        // 将用户名添加到在线用户列表控件
        ui->Online_lw->addItem(caTmp);
    }
}

// 槽函数：处理“添加好友”按钮的点击事件
void Online::on_addFriend_clicked()
{
    // 获取用户在列表中当前选中的项
    QListWidgetItem *pItem = ui->Online_lw->currentItem();
    // 获取选中项的文本，即要添加的好友名
    QString strPerUsrName = pItem->text();
    // 获取当前登录的用户名
    QString strLoginName = TcpClient::getInstance().getLoginName();

    // 检查是否选择了自己
    if (strPerUsrName == strLoginName) {
        QMessageBox::warning(this, "添加好友", "不能添加自己为好友。");
        return; // 如果是自己，则直接返回，不发送请求
    }

    // 创建一个 PDU 数据包
    PDU *pdu = mkPDU(0);
    // 设置消息类型为“添加好友请求”
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    // 将要添加的好友名（被请求者）复制到 PDU 的数据区
    strncpy(pdu->caData,strPerUsrName.toStdString().c_str(),32);
    // 将自己的用户名（请求者）复制到 PDU 的数据区
    strncpy(pdu->caData+32,strLoginName.toStdString().c_str(),32);
    // 发送数据包给服务器
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    // 释放 PDU 内存
    free(pdu);
    pdu = NULL;
}
