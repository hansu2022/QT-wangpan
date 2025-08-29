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

// 更新消息显示区域，显示新接收到的消息
void PrivateChat::updateMsg(const PDU *pdu)
{
    if(pdu == NULL) return; // 检查 PDU 指针是否为空
    char caSendName[32] = {'\0'}; // 创建一个字符数组，用于存储发送者名称
    strncpy(caSendName,pdu->caData,32); // 从 PDU 的 caData 字段中复制发送者名称
    QString strMsg = QString("%1：%2").arg(QString(caSendName)).arg(QString(pdu->caMsg)); // 格式化消息字符串，格式为 "发送者：消息内容"
    ui->showMsg_te->append(strMsg); // 将格式化后的消息追加到 QTextEdit 控件中显示
}

// "发送消息"按钮的点击槽函数
void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg = ui->inputMsg_le->text(); // 获取用户在输入框中输入的文本
    ui->inputMsg_le->clear(); // 清空输入框中的文本
    if(!strMsg.isEmpty()){ // 检查消息内容是否为空
        // 如果不为空，则构造 PDU 数据包
        PDU *pdu = mkPDU(strMsg.size()+1); // 创建一个大小合适的 PDU，strMsg.size()+1 是为了容纳字符串结束符
        pdu->uiMsgType = ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST; // 设置消息类型为私聊请求
        // 将发送者、接收者和消息内容复制到 PDU 数据区
        strncpy(pdu->caData,m_strLoginName.toStdString().c_str(),32); // 复制发送者的登录名
        strncpy(pdu->caData+32,m_strChatName.toStdString().c_str(),32); // 复制接收者的名称
        strcpy(pdu->caMsg,strMsg.toStdString().c_str()); // 复制消息内容
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen); // 通过 TCP 套接字发送 PDU 数据
        free(pdu); // 释放动态分配的 PDU 内存
        pdu = NULL; // 将指针置为空，防止野指针
        // 显示自己发送的消息
        QString strShowMsg = QString("我：%1").arg(strMsg); // 格式化显示的消息字符串，格式为 "我：消息内容"
        ui->showMsg_te->append(strShowMsg); // 将发送的消息显示在消息区域
        ui->inputMsg_le->clear();
    }else{
        QMessageBox::critical(this,"发送消息","发送消息不能为空"); // 如果消息为空，则弹出警告框
    }
}
