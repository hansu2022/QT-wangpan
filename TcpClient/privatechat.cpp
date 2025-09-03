#include "privatechat.h" // 包含 PrivateChat 类的头文件
#include "ui_privatechat.h" // 包含由 Qt Designer 生成的 UI 头文件
#include <QMessageBox> // 包含 QMessageBox，用于显示弹窗消息
#include "tcpclient.h" // 包含 TcpClient 类，用于处理网络通信

// 构造函数
// : QWidget(parent) 是初始化列表，调用父类 QWidget 的构造函数
// , ui(new Ui::PrivateChat) 是初始化列表，创建一个 Ui::PrivateChat 对象，用于访问 UI 控件
PrivateChat::PrivateChat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PrivateChat)
{
    ui->setupUi(this); // 初始化 UI 界面，将 UI 控件与代码关联起来
}

// 析构函数
// 负责清理动态分配的资源，防止内存泄漏
PrivateChat::~PrivateChat()
{
    delete ui; // 释放 ui 对象占用的内存
}

// 单例模式的实现
// 确保 PrivateChat 类在程序运行期间只有一个实例
PrivateChat &PrivateChat::getInstance()
{
    static PrivateChat instance; // 静态局部变量，只在第一次调用时创建
    return instance; // 返回该实例的引用
}

// 设置聊天对象的名称
void PrivateChat::setChatName(QString strName)
{
    m_strChatName = strName; // 设置对方（聊天对象）的名称
    m_strLoginName = TcpClient::getInstance().getLoginName(); // 获取当前用户的登录名
}

// 清空消息显示区域
void PrivateChat::clearMsg()
{
    ui->showMsg_te->clear(); // 调用 QTextEdit 控件的 clear() 方法，清空显示内容
}

// // 更新消息显示区域，显示新接收到的消息
// void PrivateChat::updateMsg(const PDU *pdu)
// {
//     if(pdu == NULL) return; // 检查 PDU 指针是否为空
//     char caSendName[32] = {'\0'}; // 创建一个字符数组，用于存储发送者名称
//     strncpy(caSendName,pdu->caData,32); // 从 PDU 的 caData 字段中复制发送者名称
//     QString strMsg = QString("%1：%2").arg(QString(caSendName)).arg(QString(pdu->caMsg)); // 格式化消息字符串，格式为 "发送者：消息内容"
//     ui->showMsg_te->append(strMsg); // 将格式化后的消息追加到 QTextEdit 控件中显示
// }

/**
 * @brief 【重构】显示接收到的私聊消息
 * @param pdu 包含消息内容的PDU
 */
void PrivateChat::showMsg(const PDU &pdu)
{
    // 1. 从 caData 中解析出发送者名称
    // caData 格式: "发送者\0接收者(我)"
    QByteArray nameData(pdu.caData, sizeof(pdu.caData));
    QString senderName = QString::fromUtf8(nameData.split('\0').first());

    // 2. 从 vMsg 中解析出消息内容
    QString msgContent = QString::fromUtf8(pdu.vMsg.data(), pdu.vMsg.size());

    // 3. 将格式化后的消息追加到显示区域
    QString strMsg = QString("%1：%2").arg(senderName).arg(msgContent);
    ui->showMsg_te->append(strMsg);
}

/**
 * @brief 【重构】"发送消息"按钮的点击槽函数
 */
void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg = ui->inputMsg_le->text();
    if (strMsg.isEmpty()) {
        QMessageBox::warning(this, "发送消息", "发送消息不能为空");
        return;
    }
    ui->inputMsg_le->clear();

    QByteArray msgBytes = strMsg.toUtf8();

    // 1. 使用工厂函数创建PDU，消息长度为实际内容的字节数
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST, msgBytes.size());

    // 2. 按照服务器的新协议打包发送者和接收者名称 ("发送者(我)\0接收者")
    QByteArray nameData;
    nameData.append(m_strLoginName.toUtf8());
    nameData.append('\0');
    nameData.append(m_strChatName.toUtf8());
    memcpy(pdu->caData, nameData.constData(), nameData.size());

    // 3. 将消息内容拷贝到可变数据区 vMsg
    memcpy(pdu->vMsg.data(), msgBytes.constData(), msgBytes.size());

    // 4. 使用 TcpClient 的新接口发送PDU
    TcpClient::getInstance().sendPdu(std::move(pdu));

    // 5. 在本地显示自己发送的消息
    QString strShowMsg = QString("我：%1").arg(strMsg);
    ui->showMsg_te->append(strShowMsg);
}
