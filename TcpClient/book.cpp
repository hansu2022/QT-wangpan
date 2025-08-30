#include "book.h"
#include <QInputDialog>
#include "tcpclient.h"
#include <QMessageBox>
Book::Book(QWidget *parent)
    : QWidget{parent}
{
    m_pBookListw = new QListWidget(this);
    m_pReturnPB = new QPushButton("返回", this);
    m_pCreateDirPB = new QPushButton("新建文件夹", this);
    m_pDelDirPB = new QPushButton("删除文件夹", this);
    m_pRenamePB = new QPushButton("重命名", this);
    m_pFlushFilePB = new QPushButton("刷新文件", this);

    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);

    m_pUploadPB = new QPushButton("上传", this);
    m_pDownLoadPB = new QPushButton("下载", this);
    m_pDelFilePB = new QPushButton("删除文件", this);
    m_pShareFilePB = new QPushButton("分享文件", this);

    QVBoxLayout *pFileVBL = new QVBoxLayout;
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);

    QHBoxLayout *pMainHBL = new QHBoxLayout(this);
    pMainHBL->addWidget(m_pBookListw);
    pMainHBL->addLayout(pDirVBL);
    pMainHBL->addLayout(pFileVBL);
    setLayout(pMainHBL);

    // 点击创建文件夹按钮，弹出新建文件夹对话框
    connect(m_pCreateDirPB, &QPushButton::clicked, this, &Book::createDirSlot);
}

void Book::createDirSlot()
{
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名称:");
    if(!strNewDir.isEmpty()){
        if(strNewDir.size()>32){
            QMessageBox::warning(this,"新建文件夹","文件夹名称过长，不能超过32个字符");
        }else{
            QString strName = TcpClient::getInstance().getLoginName();
            QString strCurPath = TcpClient::getInstance().curPath();
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType = EMUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData,strName.toStdString().c_str(),32);
            pdu->caData[31] = '\0';
            strncpy(pdu->caData+32,strNewDir.toStdString().c_str(),32);
            pdu->caData[63] = '\0';
            strcpy(pdu->caMsg,strCurPath.toStdString().c_str());
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
    }else{
        QMessageBox::warning(this,"新建文件夹","文件夹名称不能为空");
    }
}
