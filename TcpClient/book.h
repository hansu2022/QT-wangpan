#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "protocol.h"
class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);
    void flushFile(const PDU *pdu);


signals:

public slots:
    void createDirSlot();
    void flushFileSlot();
private:
    QListWidget *m_pBookListw;
    QPushButton *m_pReturnPB;
    QPushButton *m_pCreateDirPB;
    QPushButton *m_pDelDirPB;
    QPushButton *m_pRenamePB;
    QPushButton *m_pFlushFilePB;
    QPushButton *m_pUploadPB;
    QPushButton *m_pDownLoadPB;
    QPushButton *m_pDelFilePB;
    QPushButton *m_pShareFilePB;
};

#endif // BOOK_H
