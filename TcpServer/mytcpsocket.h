#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include "opedb.h"
#include <QDir>        // 提供 QDir 类，用于处理目录和文件路径
class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString getName();
    void copyDir(QString srcDir, QString destDir);
signals:
    void offline(MyTcpSocket *mysocket);
    void loggedIn(MyTcpSocket *socket, const QString &name); // 新增信号，用于通知服务器客户端已登录
public slots:
    void recvMsg();
    void clientOffline();
    void sendFileToClient();
private:
    QString m_strName;
    void sendPdu(std::unique_ptr<PDU> pdu); // 发送PDU的辅助函数
    QByteArray m_buffer; // 用于处理TCP粘包、半包的缓冲区
    // 处理不同消息类型的私有函数
    void handleRegistRequest(const PDU& pdu);
    void handleLoginRequest(const PDU& pdu);
    void handleAllOnlineRequest();
    void handleSearchUsrRequest(const PDU& pdu);
    void handleAddFriendRequest(const PDU& pdu);
    void handleAddFriendAgree(const PDU& pdu);
    void handleAddFriendRefuse(const PDU& pdu);
    void handleFlushFriendRequest(const PDU& pdu);
    void handleDelFriendRequest(const PDU& pdu);
    void handlePrivateChatRequest(const PDU& pdu);
    void handleGroupChatRequest(const PDU& pdu);

    void handleCreateDirRequest(const PDU& pdu);
    void handleFlushFileRequest(const QString& path);
    void handleFlushFileRequest(const PDU& pdu);

    void handleDelItemRequest(const PDU& pdu);
    void handleRenameDirRequest(const PDU& pdu);
    void handleEntryDirRequest(const PDU& pdu);
    void handleUploadFileRequest(const PDU& pdu);
    void handleDownloadFileRequest(const PDU& pdu);
    void handleShareFileRequest(const PDU& pdu);
    void handleShareFileNoticeRespond(const PDU& pdu);
    void handleReceiveFileResult(const PDU& pdu);
    void handleMoveFileRequest(const PDU& pdu);


    QFile m_file;
    qint64 m_iTotal;
    qint64 m_iRecved;
    bool m_bUpload;
    QTimer *m_pTimer;
};

#endif // MYTCPSOCKET_H
