#include "fileuploadworker.h"
#include <QFile>
#include <QThread>
#include <QDebug>

FileUploadWorker::FileUploadWorker(QObject *parent) : QObject(parent)
{
    m_pSocket = nullptr;
}

void FileUploadWorker::startUpload(const QString &filePath, const QString &serverIP, quint16 serverPort, const QByteArray& pduData)
{
    qDebug() << "工作线程 " << QThread::currentThreadId() << ": 开始上传任务";

    // 1. 在工作线程中创建和使用socket
    m_pSocket = new QTcpSocket();
    m_pSocket->connectToHost(QHostAddress(serverIP), serverPort);
    if (!m_pSocket->waitForConnected(5000)) {
        emit error("无法连接到服务器");
        emit finished();
        delete m_pSocket; // 清理
        m_pSocket = nullptr;
        return;
    }

    // 2. 发送预请求PDU (现在是作为QByteArray传递进来的)
    m_pSocket->write(pduData);
    if (!m_pSocket->waitForBytesWritten(5000)) {
        emit error("发送请求失败");
        emit finished();
        m_pSocket->disconnectFromHost();
        delete m_pSocket;
        m_pSocket = nullptr;
        return;
    }

    // 3. 等待服务器的 "Ready" 响应 (非常重要的一步)
    if (!m_pSocket->waitForReadyRead(5000)) {
        emit error("等待服务器响应超时");
        emit finished();
        m_pSocket->disconnectFromHost();
        delete m_pSocket;
        m_pSocket = nullptr;
        return;
    }

    QByteArray responseData = m_pSocket->readAll();
    // 实际项目中需要解析这个responseData来确认服务器是否真的准备好了
    // 这里我们先简化，假设只要有回应就是准备好了

    // 4. 读取文件并直接写入socket
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("无法打开本地文件: " + file.errorString());
        emit finished();
        m_pSocket->disconnectFromHost();
        delete m_pSocket;
        m_pSocket = nullptr;
        return;
    }

    qint64 totalSize = file.size();
    qint64 sentSize = 0;

    while (!file.atEnd() && m_pSocket->state() == QAbstractSocket::ConnectedState) {
        QByteArray chunk = file.read(65536); // 可以尝试更大的块，例如64KB
        if (chunk.isEmpty() && !file.atEnd()) {
            emit error("读取文件时出错");
            break;
        }

        sentSize += chunk.size();

        // 直接在这里写入网络，这是在工作线程中执行的阻塞操作
        m_pSocket->write(chunk);
        if (!m_pSocket->waitForBytesWritten(-1)) { // -1 表示无限等待
            emit error("网络写入失败: " + m_pSocket->errorString());
            break;
        }

        emit progress(sentSize, totalSize);
    }

    // 5. 收尾
    file.close();
    m_pSocket->disconnectFromHost();
    delete m_pSocket;
    m_pSocket = nullptr;

    qDebug() << "工作线程 " << QThread::currentThreadId() << ": 上传任务完成";
    emit finished();
}
