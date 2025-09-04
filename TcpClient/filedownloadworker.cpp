#include "filedownloadworker.h"
#include <QFile>
#include <QHostAddress>
#include <QThread>
#include <QDebug>
#include "protocol.h" // 需要PDU的定义和反序列化方法

FileDownloadWorker::FileDownloadWorker(QObject *parent)
    : QObject{parent}
{
    m_pSocket = nullptr;
    m_pFile = new QFile(this); // 初始化文件对象
    m_totalSize = 0;
    m_receivedSize = 0;
}

// startDownload 现在只负责发起连接和设置
void FileDownloadWorker::startDownload(const QString &savePath, const QString &serverIP, quint16 serverPort, const QByteArray &pduData)
{
    qDebug() << "下载工作线程 " << QThread::currentThreadId() << ": 任务开始，保存至" << savePath;

    m_pFile->setFileName(savePath);
    if (!m_pFile->open(QIODevice::WriteOnly)) {
        emit error("无法创建本地文件: " + m_pFile->errorString());
        // 由于任务无法开始，需要发出finished信号以允许主线程清理
        emit finished("下载失败");
        return;
    }

    m_pSocket = new QTcpSocket();
    connect(m_pSocket, &QTcpSocket::connected, this, &FileDownloadWorker::onConnected);
    connect(m_pSocket, &QTcpSocket::readyRead, this, &FileDownloadWorker::onReadyRead);
    connect(m_pSocket, &QTcpSocket::disconnected, this, &FileDownloadWorker::onDisconnected);
    connect(m_pSocket, &QTcpSocket::errorOccurred, this, &FileDownloadWorker::onErrorOccurred);

    // 将pduData暂存起来，在连接成功后再发送
    m_pSocket->setProperty("pduData", pduData);

    m_pSocket->connectToHost(QHostAddress(serverIP), serverPort);
}

// 连接成功后，发送下载请求
void FileDownloadWorker::onConnected()
{
    qDebug() << "下载工作线程: 连接成功，发送请求PDU";
    QByteArray pduData = m_pSocket->property("pduData").toByteArray();
    m_pSocket->write(pduData);
}

// 收到数据时，进行处理
void FileDownloadWorker::onReadyRead()
{
    // 1. 将所有新数据读入内部缓冲区
    m_buffer.append(m_pSocket->readAll());

    // --- 阶段一: 解析服务器响应PDU ---
    if (m_totalSize == 0) {
        // 使用循环，以防PDU和部分文件数据在同一个数据包里到达
        while(true) {
            if (m_buffer.size() < sizeof(uint)) {
                return; // PDU头都收不全，等待更多数据
            }

            uint uiPDULen = 0;
            // 直接从缓冲区“窥探”
            memcpy(&uiPDULen, m_buffer.constData(), sizeof(uint));

            if (m_buffer.size() < uiPDULen) {
                return; // PDU不完整，等待更多数据
            }

            // PDU完整，可以反序列化
            auto pdu = PDU::deserialize(m_buffer.constData(), uiPDULen);
            m_buffer.remove(0, uiPDULen); // 从缓冲区移除已处理的PDU数据

            if (!pdu || pdu->uiMsgType != MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND) {
                emit error("服务器响应无效或类型错误");
                m_pSocket->disconnectFromHost();
                return;
            }
            // ... (解析 m_totalSize 的逻辑不变) ...

            // 解析成功后，m_totalSize > 0，跳出PDU解析阶段
            break;
        }
    }

    // --- 阶段二: 写入文件数据 ---
    // (缓冲区中剩余的数据就是文件数据)
    if (m_totalSize > 0 && !m_buffer.isEmpty()) {
        qint64 bytesToWrite = qMin((qint64)m_buffer.size(), m_totalSize - m_receivedSize);
        if (bytesToWrite <= 0) return;

        if (m_pFile->write(m_buffer.constData(), bytesToWrite) == -1) {
            emit error("写入本地文件失败: " + m_pFile->errorString());
            m_pSocket->disconnectFromHost();
            return;
        }

        m_buffer.remove(0, bytesToWrite);
        m_receivedSize += bytesToWrite;
        emit progress(m_receivedSize, m_totalSize);

        if (m_receivedSize >= m_totalSize) {
            m_pSocket->disconnectFromHost();
        }
    }
}

// 连接断开时，判断结果并收尾
void FileDownloadWorker::onDisconnected()
{
    qDebug() << "下载工作线程: 连接已断开";
    m_pFile->close();

    // 只有当接收大小和总大小相等且大于0时，才算成功
    if (m_totalSize > 0 && m_receivedSize == m_totalSize) {
        emit finished("文件下载成功！");
    } else {
        // 其他所有情况（如提前断开、解析失败等）都算作错误
        // onErrorOccurred 或 onReadyRead 中已经发过具体的error信号了
        // 这里可以发一个通用的完成信号
        emit finished("下载未完成");
    }

    if(m_pSocket) {
        m_pSocket->deleteLater();
        m_pSocket = nullptr;
    }
}

// 处理网络错误
void FileDownloadWorker::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError); // 避免编译器警告
    // 只有在socket还存在时才获取错误信息
    if (m_pSocket) {
        emit error("网络错误: " + m_pSocket->errorString());
        m_pSocket->disconnectFromHost();
    }
}
