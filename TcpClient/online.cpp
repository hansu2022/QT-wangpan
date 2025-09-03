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
void Online::showUsr(const PDU &pdu)
{
    ui->Online_lw->clear();

    // 1. 使用 QDataStream 反序列化服务器发来的用户列表
    QByteArray data(pdu.vMsg.data(), pdu.vMsg.size());
    QDataStream stream(&data, QIODevice::ReadOnly);
    QStringList onlineUsers;
    stream >> onlineUsers; // 从数据流中读出 QStringList

    // 2. 将 QStringList 的内容添加到 UI 列表
    ui->Online_lw->addItems(onlineUsers);
}

// 槽函数：处理“添加好友”按钮的点击事件
void Online::on_addFriend_clicked()
{
    QListWidgetItem *pItem = ui->Online_lw->currentItem();
    if (pItem == nullptr) {
        QMessageBox::warning(this, "添加好友", "请先选择要添加的好友。");
        return;
    }

    QString strPerUsrName = pItem->text();
    QString strLoginName = TcpClient::getInstance().getLoginName();

    if (strPerUsrName == strLoginName) {
        QMessageBox::warning(this, "添加好友", "不能添加自己为好友。");
        return;
    }

    // 1. 使用工厂函数创建 PDU，自动管理内存
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_ADD_FRIEND_REQUEST);

    // 2. 按照服务器端的新格式打包数据 ("被请求者\0请求者")
    QByteArray data;
    data.append(strPerUsrName.toUtf8());
    data.append('\0');
    data.append(strLoginName.toUtf8());

    // 将打包好的数据复制到 caData
    memcpy(pdu->caData, data.constData(), data.size());

    // 3. 使用新的辅助函数发送 PDU
    TcpClient::getInstance().sendPdu(std::move(pdu));
}
