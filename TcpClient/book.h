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
    QString getSaveFilePath();
    qint64 m_iTotal;
    qint64 m_iRecved;
signals:

public slots:
    void createDirSlot();
    void flushFileSlot();
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

    QString m_strUploadFilePath;
    QString m_strSaveFilePath;   //用于下载保存路径
    QTimer m_pTimer;
    bool m_bDownload;
};

#endif // BOOK_H
