#ifndef FILEUPLOADWORKER_H
#define FILEUPLOADWORKER_H

#include <QObject>
#include <QString>
#include <QTcpSocket> // Worker 需要自己管理网络连接

class FileUploadWorker : public QObject
{
    Q_OBJECT
public:
    explicit FileUploadWorker(QObject *parent = nullptr);

public slots:
    /**
     * @brief 开始读取文件并分块发送的任务
     * @param filePath 要上传的本地文件路径
     */
    void startUpload(const QString &filePath, const QString &serverIP, quint16 serverPort, const QByteArray& pduData);

signals:
    /**
     * @brief 报告上传进度
     * @param sentSize 已发送（读取）的大小
     * @param totalSize 文件总大小
     */
    void progress(qint64 sentSize, qint64 totalSize);

    /**
     * @brief 当一个文件数据块准备好时发出此信号
     * @param data 文件数据块
     */
    void dataChunkReady(const QByteArray& data);

    /**
     * @brief 当任务完成（无论成功与否）时发出此信号
     */
    void finished();

    /**
     * @brief 报告错误信息
     * @param err 错误描述
     */
    void error(const QString &err);
private:
    QTcpSocket *m_pSocket;
};

#endif // FILEUPLOADWORKER_H
