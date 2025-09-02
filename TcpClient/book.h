#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "protocol.h"
#include <QTimer>

class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);
    void flushFile(const PDU *pdu);
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

signals:

public slots:
    void createDirSlot();
    void flushFileSlot();
    void handleFriendListUpdated(); // 处理好友列表更新的槽函数
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

};

#endif // BOOK_H
