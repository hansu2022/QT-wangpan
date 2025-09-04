#ifndef FILEDOWNLOADWORKER_H
#define FILEDOWNLOADWORKER_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QFile>
/**
 * @brief 文件下载工作者类
 *
 * 在独立线程中运行，负责处理单个文件的完整下载流程：
 * 1. 连接服务器
 * 2. 发送下载请求PDU
 * 3. 接收并解析服务器响应（获取文件大小）
 * 4. 接收文件数据流并写入本地磁盘
 * 5. 断开连接并报告任务完成
 */
class FileDownloadWorker : public QObject
{
    Q_OBJECT
public:
    explicit FileDownloadWorker(QObject *parent = nullptr);


public slots:
    /**
     * @brief 启动下载任务的槽函数
     * @param savePath 本地保存路径
     * @param serverIP 服务器IP
     * @param serverPort 服务器端口
     * @param pduData 序列化后的下载请求PDU
     */
    void startDownload(const QString &savePath, const QString &serverIP, quint16 serverPort, const QByteArray &pduData);
signals:
    void progress(qint64 receivedSize, qint64 totalSize);
    void finished(const QString& message); // 完成时可以附带一条消息
    void error(const QString &err);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
private:
private:
    QTcpSocket *m_pSocket;
    QFile *m_pFile;       // 需要一个文件对象的成员变量
    qint64 m_totalSize;   // 文件总大小
    qint64 m_receivedSize;// 已接收大小
    QByteArray m_buffer; // 内部数据缓冲区
};

#endif // FILEDOWNLOADWORKER_H
