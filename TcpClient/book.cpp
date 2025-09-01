#include "book.h" // 包含 Book 类的头文件
#include <QInputDialog> // 包含 QInputDialog，用于弹出输入对话框
#include "tcpclient.h" // 包含 TcpClient 类，用于处理网络通信
#include <QMessageBox> // 包含 QMessageBox，用于弹出消息提示框
#include <QListWidgetItem> // 包含 QListWidgetItem，用于列表项
#include "protocol.h" // 包含 protocol 头文件，定义了协议数据单元 PDU
#include <QFileDialog> // 包含 QFileDialog，用于文件选择对话框
#include <QStandardPaths> // 新增这个头文件，用于获取标准路径
// Book 类的构造函数
Book::Book(QWidget *parent)
    : QWidget{parent}
{
    m_bDownload = false;
    m_iRecved = 0;
    m_iTotal = 0;
    // 创建一个 QListWidget 控件，用于显示文件和文件夹列表
    m_pBookListw = new QListWidget(this);
    // 创建各种按钮
    m_pReturnPB = new QPushButton("返回", this);
    m_pCreateDirPB = new QPushButton("新建文件夹", this);
    m_pDelItemPB = new QPushButton("删除", this);
    m_pRenamePB = new QPushButton("重命名", this);
    m_pFlushFilePB = new QPushButton("刷新文件", this);

    // 创建一个垂直布局管理器，用于放置文件夹操作按钮
    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelItemPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);

    // 创建文件操作按钮
    m_pUploadPB = new QPushButton("上传", this);
    m_pDownLoadPB = new QPushButton("下载", this);
    m_pShareFilePB = new QPushButton("分享文件", this);

    // 创建另一个垂直布局管理器，用于放置文件操作按钮
    QVBoxLayout *pFileVBL = new QVBoxLayout;
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
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
    // 将删除按钮的 clicked 信号连接到 delItem 槽函数
    connect(m_pDelItemPB, &QPushButton::clicked, this, &Book::delItem);
    // 将重命名按钮的 clicked 信号连接到 reName 槽函数
    connect(m_pRenamePB, &QPushButton::clicked, this, &Book::reName);
    // 将文件列表的双击信号连接到 entryDir 槽函数
    connect(m_pBookListw, &QListWidget::doubleClicked, this, &Book::entryDir);
    // 将返回按钮的 clicked 信号连接到 returnDir 槽函数
    connect(m_pReturnPB, &QPushButton::clicked, this, &Book::returnDir);
    // 将上传按钮的 clicked 信号连接到 uploadFile 槽函数
    connect(m_pUploadPB, &QPushButton::clicked, this, &Book::uploadFile);
    // 将上传文件数据的槽函数连接到定时器的 timeout 信号
    connect(&m_pTimer, &QTimer::timeout, this, &Book::uploadFileData);
    // 将下载按钮的 clicked 信号连接到 downloadFile 槽函数
    connect(m_pDownLoadPB, &QPushButton::clicked, this, &Book::downloadFile);
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
        // <-- 新增：将文件类型，存储到Item的UserRole中
        pItem->setData(Qt::UserRole, pFileInfo->iFileType);
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

// 删除的槽函数
void Book::delItem()
{
    QListWidgetItem *pItem = m_pBookListw->currentItem();
    if (pItem == NULL) {
        QMessageBox::warning(this, "删除", "请选择要删除的项目");
        return;
    }

    // 从Item中获取我们之前存储的类型信息
    int itemType = pItem->data(Qt::UserRole).toInt();
    QString typeText = (itemType == 0) ? "文件夹" : "文件";
    QString itemName = pItem->text();

    // 增加删除前的确认对话框，提升用户体验
    QString question; // 先声明一个空的 question 字符串
    //使用 if-else 为不同类型生成专属提示**
    if (itemType == 0) { // 如果是文件夹
        question = QString("您确定要删除文件夹 “%1” 吗？\n\n<font color='red'>此操作将删除该文件夹下的所有内容，且不可恢复！</font>").arg(itemName);
    } else { // 如果是文件
        question = QString("您确定要删除文件 “%1” 吗？\n\n此操作不可恢复。").arg(itemName);
    }

    // 使用 QMessageBox::question 显示确认对话框
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("确认删除");
    msgBox.setTextFormat(Qt::RichText); // 设置文本格式为富文本，让HTML标签生效
    msgBox.setText(question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel); // 默认选中“取消”

    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        // 用户确认后，才发送请求
        QString strCurPath = TcpClient::getInstance().curPath();
        PDU *pdu = mkPDU(strCurPath.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_ITEM_REQUEST;
        strncpy(pdu->caData, itemName.toStdString().c_str(), 32);
        pdu->caData[31] = '\0';
        strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

// 重命名文件夹的槽函数
void Book::reName()
{
    // 获取当前客户端的路径
    QString strCurPath = TcpClient::getInstance().curPath();
    // 获取用户在列表视图中当前选中的项目
    QListWidgetItem *pItem =  m_pBookListw->currentItem();
    // 如果没有选中任何项目，则弹出警告框并返回
    if(pItem == NULL){
        QMessageBox::warning(this,"重命名","请选择要重命名的文件");
        return;
    }
    // 获取要重命名的旧文件/文件夹名称
    QString strOldName = pItem->text();
    // 弹出一个输入对话框，让用户输入新的文件/文件夹名称
    QString strNewName = QInputDialog::getText(this, "重命名", "请输入新的文件名称:");
    // 检查用户输入的新名称是否为空
    if(!strNewName.isEmpty()){
        // 检查新名称的长度是否超过32个字符
        if(strNewName.size()>32){
            QMessageBox::warning(this,"重命名","文件名称过长，不能超过32个字符");
        }else{
            // 创建一个PDU，用于封装重命名请求
            PDU *pdu = mkPDU(strCurPath.size()+1);
            // 设置消息类型为“重命名请求”
            pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIR_REQUEST;
            // 将旧名称拷贝到PDU的caData字段的前32字节
            strncpy(pdu->caData,strOldName.toStdString().c_str(),32);
            // 确保旧名称字符串以空字符结尾
            pdu->caData[31] = '\0';
            // 将新名称拷贝到caData字段的后32字节
            strncpy(pdu->caData+32,strNewName.toStdString().c_str(),32);
            // 确保新名称字符串以空字符结尾
            pdu->caData[63] = '\0';
            // 将当前路径拷贝到 PDU 的 caMsg 字段中
            strncpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
            // 通过TCP socket发送PDU数据
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            // 释放动态分配的PDU内存
            free(pdu);
            pdu = NULL;
        }
    }else{
        // 如果新名称为空，弹出警告框
        QMessageBox::warning(this,"重命名","文件名称不能为空");
    }
}

// 进入文件夹的槽函数
void Book::entryDir(const QModelIndex &index)
{
    // 获取被双击的 QListWidgetItem
    QListWidgetItem *item = m_pBookListw->item(index.row());
    // 如果获取失败，直接返回
    if (!item) return;
    // 从项目数据中取出之前存储的类型信息（Qt::UserRole 是一个自定义角色）
    int itemType = item->data(Qt::UserRole).toInt();
    // 如果类型不为0（0代表文件夹），则表示双击的不是文件夹，直接返回，不发送请求
    if (itemType != 0) { // 0 代表文件夹
        return;
    }
    // 获取被双击的文件夹名称
    QString strDirName = item->text();
    // 将要进入的文件夹名称存储到客户端实例中
    TcpClient::getInstance().setEnterDirName(strDirName);
    // 获取当前客户端的路径
    QString strCurPath = TcpClient::getInstance().curPath();
    // 创建一个PDU，用于封装“进入文件夹”请求
    PDU *pdu = mkPDU(strCurPath.size()+1);
    // 设置消息类型为“进入文件夹请求”
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTRY_DIR_REQUEST;
    // 将要进入的文件夹名称拷贝到PDU的caData字段中，最多32字节
    strncpy(pdu->caData,strDirName.toStdString().c_str(),32);
    // 确保字符串以空字符结尾
    pdu->caData[31] = '\0';
    // 将当前路径拷贝到PDU的caMsg字段中
    strncpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
    // 通过TCP socket发送PDU数据
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    // 释放动态分配的PDU内存
    free(pdu);
    pdu = NULL;
}

// 返回上一级目录的槽函数
void Book::returnDir()
{
    // 获取当前客户端的路径
    QString strCurPath = TcpClient::getInstance().curPath();
    // 获取根目录
    QString strRootPath = QString("./%1").arg(TcpClient::getInstance().getLoginName());
    //
    if(strCurPath == strRootPath){
        QMessageBox::warning(this,"返回","已经是根目录，无法返回");
        return;
    }
    // 找到最后一个 '/' 的位置
    int index = strCurPath.lastIndexOf('/');
    // 截取字符串，得到上一级目录路径
    strCurPath = strCurPath.left(index);
    // 更新客户端的当前路径
    TcpClient::getInstance().setEnterDirName(""); // 清空进入目录名称，表示返回上一级
    TcpClient::getInstance().setCurPath(strCurPath);
    // 主动请求刷新文件列表
    flushFileSlot();

}

void Book::uploadFile()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName(this, "选择要上传的文件");
    if(!m_strUploadFilePath.isEmpty()){
        int index = m_strUploadFilePath.lastIndexOf('/');
        QString strFileName = m_strUploadFilePath.mid(index+1);
        QFile file(m_strUploadFilePath);
        qint64 fileSize = file.size();

        // 创建一个PDU，用于封装上传文件请求
        QString strCurPath = TcpClient::getInstance().curPath();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        strncpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
        sprintf(pdu->caData,"%s#%lld",strFileName.toStdString().c_str(),fileSize);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

        //m_pTimer.start(1000); 不再需要定时器。

    }else{
        QMessageBox::warning(this,"上传文件","请选择要上传的文件");
    }

}

void Book::uploadFileData()
{
    QFile file(m_strUploadFilePath);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::warning(this,"上传文件","文件打开失败");
        return;
    }
    char *caBuf = new char[4096];
    qint64 readSize = 0;
    while(!file.atEnd()){
        readSize = file.read(caBuf,4096);
        if(readSize > 0){
            if(TcpClient::getInstance().getTcpSocket().write(caBuf,readSize) == -1){
                QMessageBox::warning(this,"上传文件","文件上传失败");
                break;
            }
        }else{
            QMessageBox::warning(this,"上传文件","文件读取出错，上传中断.");
            break;
        }
    }
    file.close();
    delete[] caBuf;
    caBuf = NULL;
}

void Book::downloadFile()
{
    QListWidgetItem *pItem = m_pBookListw->currentItem();
    if (pItem == NULL) {
        QMessageBox::warning(this, "下载", "请选择要下载的文件");
        qDebug() << "警告: 未选中任何项，下载操作中止。";
        return;
    }

    // 从Item中获取我们之前存储的类型信息
    int itemType = pItem->data(Qt::UserRole).toInt();


    if (itemType != 1) { // 1 代表文件
        QMessageBox::warning(this, "下载", "只能下载文件，不能下载文件夹");
        return;
    }

    // 1. 先从列表项获取要下载的文件名
    QString fileName = pItem->text();

    // 2. 获取系统的标准“下载”文件夹路径
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    // 3. 将路径和文件名拼接成一个完整的建议路径
    QString fullDefaultPath = defaultPath + "/" + fileName;

    // 4. 将这个建议路径作为第三个参数传入文件对话框
    QString strSaveFilePath = QFileDialog::getSaveFileName(this, "保存文件", fullDefaultPath);

    // 弹窗让用户选择保存位置，如果用户取消则中止,不继续下载
    if (strSaveFilePath.isEmpty()) {
        return;
    }

    m_strSaveFilePath = strSaveFilePath; // <-- 使用新的成员变量保存路径

    QString strCurPath = TcpClient::getInstance().curPath();

    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
    strncpy(pdu->caData, fileName.toStdString().c_str(), 32);
    strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;

}

void Book::setDownloadStatus(bool status)
{
    m_bDownload = status;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

