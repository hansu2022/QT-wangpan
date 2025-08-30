#include "book.h" // 包含 Book 类的头文件
#include <QInputDialog> // 包含 QInputDialog，用于弹出输入对话框
#include "tcpclient.h" // 包含 TcpClient 类，用于处理网络通信
#include <QMessageBox> // 包含 QMessageBox，用于弹出消息提示框
#include <QListWidgetItem> // 包含 QListWidgetItem，用于列表项
#include "protocol.h" // 包含 protocol 头文件，定义了协议数据单元 PDU

// Book 类的构造函数
Book::Book(QWidget *parent)
    : QWidget{parent}
{
    // 创建一个 QListWidget 控件，用于显示文件和文件夹列表
    m_pBookListw = new QListWidget(this);
    // 创建各种按钮
    m_pReturnPB = new QPushButton("返回", this);
    m_pCreateDirPB = new QPushButton("新建文件夹", this);
    m_pDelDirPB = new QPushButton("删除文件夹", this);
    m_pRenamePB = new QPushButton("重命名", this);
    m_pFlushFilePB = new QPushButton("刷新文件", this);

    // 创建一个垂直布局管理器，用于放置文件夹操作按钮
    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);

    // 创建文件操作按钮
    m_pUploadPB = new QPushButton("上传", this);
    m_pDownLoadPB = new QPushButton("下载", this);
    m_pDelFilePB = new QPushButton("删除文件", this);
    m_pShareFilePB = new QPushButton("分享文件", this);

    // 创建另一个垂直布局管理器，用于放置文件操作按钮
    QVBoxLayout *pFileVBL = new QVBoxLayout;
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);

    // 创建一个主水平布局管理器，用于将列表和两个按钮布局管理器整合在一起
    QHBoxLayout *pMainHBL = new QHBoxLayout(this);
    pMainHBL->addWidget(m_pBookListw); // 将文件列表添加到主布局
    pMainHBL->addLayout(pDirVBL);      // 将文件夹操作布局添加到主布局
    pMainHBL->addLayout(pFileVBL);     // 将文件操作布局添加到主布局
    setLayout(pMainHBL); // 设置窗口的布局

    // 将新建文件夹按钮的 clicked 信号连接到 createDirSlot 槽函数
    connect(m_pCreateDirPB, &QPushButton::clicked, this, &Book::createDirSlot);

    // 将刷新文件按钮的 clicked 信号连接到 flushFileSlot 槽函数
    connect(m_pFlushFilePB, &QPushButton::clicked, this, &Book::flushFileSlot);
}

// 刷新文件列表的槽函数，根据服务器返回的数据（pdu）更新显示
void Book::flushFile(const PDU *pdu)
{
    // 检查 pdu 是否为空指针
    if(pdu == NULL){
        return;
    }
    // 清空当前的列表内容
    m_pBookListw->clear();
    FileInfo *pFileInfo = NULL;
    // 计算 pdu->caMsg 中包含的文件/文件夹信息数量
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    // 遍历所有文件/文件夹信息
    for (int i = 0;i<iCount;i++){
        // 将 PDU 数据块强制转换为 FileInfo 结构体指针
        pFileInfo = (FileInfo*)(pdu->caMsg+i*sizeof(FileInfo));
        // 将文件名从 char 数组转换为 QString
        QString strFileName = QString(pFileInfo->caFileName);
        // 创建一个新的列表项
        QListWidgetItem *pItem = new QListWidgetItem;
        // 根据文件类型设置列表项的图标
        if(pFileInfo->iFileType == 0){ // 0 代表文件夹
            pItem->setIcon(QIcon(QPixmap(":/dir.png")));
        }else if(pFileInfo->iFileType == 1){ // 1 代表文件
            pItem->setIcon(QIcon(QPixmap(":/file.jpg")));
        }
        // 设置列表项的显示文本为文件名
        pItem->setText(strFileName);
        // 将列表项添加到 QListWidget 中
        m_pBookListw->addItem(pItem);
    }
}

// 新建文件夹的槽函数
void Book::createDirSlot()
{
    // 弹出输入对话框，获取用户输入的文件夹名称
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名称:");
    // 检查用户输入是否为空
    if(!strNewDir.isEmpty()){
        // 检查文件夹名称是否过长
        if(strNewDir.size()>32){
            // 弹出警告消息框
            QMessageBox::warning(this,"新建文件夹","文件夹名称过长，不能超过32个字符");
        }else{
            // 获取当前登录用户名和当前目录路径
            QString strName = TcpClient::getInstance().getLoginName();
            QString strCurPath = TcpClient::getInstance().curPath();
            // 创建一个 PDU，用于向服务器发送请求
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST; // 设置消息类型为创建文件夹请求
            // 将用户名复制到 pdu->caData 的前32个字节
            strncpy(pdu->caData,strName.toStdString().c_str(),32);
            pdu->caData[31] = '\0'; // 确保字符串以空字符结尾
            // 将新文件夹名称复制到 pdu->caData 的后32个字节
            strncpy(pdu->caData+32,strNewDir.toStdString().c_str(),32);
            pdu->caData[63] = '\0'; // 确保字符串以空字符结尾
            // 将当前路径复制到 pdu->caMsg
            strcpy(pdu->caMsg,strCurPath.toStdString().c_str());
            // 通过 TcpClient 发送 PDU
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            // 释放 PDU 内存
            free(pdu);
            pdu = NULL;
        }
    }else{
        // 如果输入为空，弹出警告消息框
        QMessageBox::warning(this,"新建文件夹","文件夹名称不能为空");
    }
}

// 刷新文件的槽函数
void Book::flushFileSlot()
{
    // 获取当前目录路径
    QString strCurPath = TcpClient::getInstance().curPath();
    // 创建一个 PDU，用于向服务器发送刷新文件列表请求
    PDU *pdu = mkPDU(strCurPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST; // 设置消息类型为刷新文件请求
    // 将当前路径复制到 pdu->caMsg
    strncpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
    // 通过 TcpClient 发送 PDU
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    // 释放 PDU 内存
    free(pdu);
    pdu = NULL;
}
