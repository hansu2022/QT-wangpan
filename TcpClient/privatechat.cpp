#include "privatechat.h"
#include "ui_privatechat.h"
#include <QMessageBox>
PrivateChat::PrivateChat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

PrivateChat &PrivateChat::getInstance()
{
    static PrivateChat instance;
    return instance;
}

void PrivateChat::setChatName(QString strName)
{
    m_strChatName = strName;
    m_strLoginName = TcpClient::getInstance().getLoginName();
}

void PrivateChat::clearMsg()
{
    ui->showMsg_te->clear();
}

void PrivateChat::updateMsg(const PDU *pdu)
{
    if(pdu == NULL) return;
    char caSendName[32] = {'\0'};
    strncpy(caSendName,pdu->caData,32);
    QString strMsg = QString("%1：%2").arg(QString(caSendName)).arg(QString(pdu->caMsg));
    ui->showMsg_te->append(strMsg);
}

void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg = ui->inputMsg_le->text();
    ui->inputMsg_le->clear();
    if(!strMsg.isEmpty()){
        PDU *pdu = mkPDU(strMsg.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;
        // 将发送者、接收者和消息内容复制到 PDU 数据区
        strncpy(pdu->caData,m_strLoginName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,m_strChatName.toStdString().c_str(),32);
        strcpy(pdu->caMsg,strMsg.toStdString().c_str());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }else{
        QMessageBox::critical(this,"发送消息","发送消息不能为空");
    }
}

