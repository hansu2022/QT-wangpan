#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "protocol.h"
#include <QTimer>
#include <QProgressBar>
class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);
    void flushFile(const PDU &pdu);
    void delItem();// 新增一个统一删除的方法
    void reName();
    void entryDir(const QModelIndex &index);
    void returnDir();
    void uploadFile();
    void uploadFileData();
    void downloadFile();
    void setDownloadStatus(bool status);
    bool getDownloadStatus();
    void shareFile();
    void moveFile();
    void selectDir();
    QString getSaveFilePath();
    qint64 m_iTotal;
    qint64 m_iRecved;
    QString getShareFileName();
    void startFileUpload();
    void handleFriendListUpdated();
    void selectDestDir();
signals:
    // --- 多线程上传相关信号 ---
    /**
     * @brief 触发工作线程开始读取文件的信号
     * @param filePath 本地文件路径
     */
    void startUploadTask(const QString &filePath, const QString &serverIP, quint16 serverPort, const QByteArray &pduData);
    void startDownloadTask(const QString &savePath, const QString &serverIP, quint16 serverPort, const QByteArray &pduData);
public slots:
    void createDirSlot();
    void flushFileSlot();
    // --- 多线程上传相关槽函数 ---
    /**
     * @brief 接收工作线程发来的文件数据块并发送
     * @param data 文件数据块
     */
    void sendDataChunk(const QByteArray& data);

    /**
     * @brief 更新上传进度条
     * @param sentSize 已发送大小
     * @param totalSize 总大小
     */
    void updateUploadProgress(qint64 sentSize, qint64 totalSize);

    /**
     * @brief 处理上传过程中发生的错误
     * @param err 错误信息
     */
    void onUploadError(const QString &err);

    /**
     * @brief 上传完成后进行清理
     */
    void onUploadFinished();
    // --- 多线程下载相关槽函数 ---
    void updateDownloadProgress(qint64 receivedSize, qint64 totalSize);
    void onDownloadError(const QString &err);
    void onDownloadFinished(const QString& message);
    // void handleFriendListUpdated(); // 处理好友列表更新的槽函数
private:

    QListWidget *m_pBookListw;
    QPushButton *m_pReturnPB;
    QPushButton *m_pCreateDirPB;
    QPushButton *m_pDelItemPB;  // 新增一个统一的按钮
    QPushButton *m_pRenamePB;
    QPushButton *m_pFlushFilePB;
    QPushButton *m_pUploadPB;
    QPushButton *m_pDownLoadPB;
    QPushButton *m_pShareFilePB;
    QPushButton *m_pMoveFilePB;
    QPushButton *m_pSelectDirPB;

    QString m_strUploadFilePath;
    QString m_strSaveFilePath;   //用于下载保存路径
    QTimer m_pTimer;
    bool m_bDownload;

    bool m_bInSharingProcess; // 标志是否处于分享文件过程

    QString m_strShareFileName; // 用于存储要分享的文件名

    QString m_strMoveFileName; // 用于存储要移动的文件名
    QString m_strMoveFilePath; // 用于存储要移动的文件的当前路径
    QString m_strDestDirPath; // 用于存储目标目录路径
    // --- UI 相关 ---
    QProgressBar *m_pProgressBar; // 添加一个进度条成员变量
    bool m_isDownloading; // 标志当前是否在下载
};

#endif // BOOK_H
