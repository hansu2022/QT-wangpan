#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H
#include "protocol.h"
#include "tcpclient.h"
#include <QWidget>

namespace Ui {
class PrivateChat;
}

class PrivateChat : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateChat(QWidget *parent = nullptr);
    ~PrivateChat();
    static PrivateChat &getInstance();
    void setChatName(QString strName);
    void clearMsg();
    // void updateMsg(const PDU *pdu);
    /**
     * @brief 显示接收到的消息。替代了旧的 updateMsg(const PDU* pdu)
     * @param pdu 包含消息内容的PDU
     */
    void showMsg(const PDU &pdu);
private slots:
    void on_sendMsg_pb_clicked();

private:
    Ui::PrivateChat *ui;
    QString m_strChatName;  // 对方的聊天名称
    QString m_strLoginName; // 我自己的登录名
};

#endif // PRIVATECHAT_H
