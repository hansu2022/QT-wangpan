#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include "opewidget.h"
#include "protocol.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class TcpClient;
}
QT_END_NAMESPACE

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    TcpClient(QWidget *parent = nullptr);
    ~TcpClient();
    void sendPdu(std::unique_ptr<PDU> pdu);
    void loadConfig();
    static TcpClient &getInstance();
    QTcpSocket& getTcpSocket();
    QString getLoginName();
    QString curPath();
    void setEnterDirName(const QString& name);
    void setCurPath(QString setCurPath);
    QString getRootPath();

    // 【新增】获取服务器IP地址
    QString getServerIP() const;

    // 【新增】获取服务器端口号
    quint16 getServerPort() const;
public slots:
    void showConnect();
    void recvMsg();

private slots:
    // void on_pushButton_clicked();

    void on_login_pb_clicked();

    void on_cancel_pb_clicked();

    void on_regist_pb_clicked();

private:
    Ui::TcpClient *ui;
    QString m_strIP;
    quint16 m_usPort;

    QTcpSocket m_tcpSocket;
    QByteArray m_buffer; // <-- 【新增】用于处理TCP粘包、半包的缓冲区

    void handleRegistResponse(const PDU& pdu); // 处理注册响应
    void handleLoginResponse(const PDU& pdu);// 处理登录响应
    void handleSearchUsrResponse(const PDU& pdu); // 处理搜索用户响应
    void handleFriendRequest(const PDU& pdu); // 处理添加好友请求
    void handleDelFriendResponse(const PDU& pdu); // 处理删除好友响应
    void handleDelFriendNotice(const PDU& pdu); // 处理删除好友通知
    void handlePrivateChatRequest(const PDU& pdu);// 处理私聊请求
    void handleGroupChatRequest(const PDU& pdu); // 处理群聊请求
    void handleDownloadFileResponse(const PDU& pdu); // 处理下载文件响应
    void handleShareFileNotice(const PDU& pdu); // 处理分享文件通知


    QString m_strLoginName;
    // 当前路径
    QString m_strCurPath;
    QString m_strRootPath; // <-- 新增成员变量，用于存储根目录
    QString m_strEnterDirName; // 进入的文件夹名称

    QFile m_file;
};
#endif // TCPCLIENT_H
