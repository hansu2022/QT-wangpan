#include "book.h" // 包含 Book 类的头文件
#include <QInputDialog> // 包含 QInputDialog，用于弹出输入对话框
#include "tcpclient.h" // 包含 TcpClient 类，用于处理网络通信
#include <QMessageBox> // 包含 QMessageBox，用于弹出消息提示框
#include <QListWidgetItem> // 包含 QListWidgetItem，用于列表项
#include "protocol.h" // 包含 protocol 头文件，定义了协议数据单元 PDU
#include <QFileDialog> // 包含 QFileDialog，用于文件选择对话框
#include <QStandardPaths> // 新增这个头文件，用于获取标准路径
#include "openwidget.h"
#include "sharefile.h"
#include <QListWidget>
#include <QThread>
#include "fileuploadworker.h"
#include "filedownloadworker.h"
// Book 类的构造函数
Book::Book(QWidget *parent)
    : QWidget{parent}
{

    m_bDownload = false;
    m_iRecved = 0;
    m_iTotal = 0;
    m_isDownloading = false; // 初始化
    m_bInSharingProcess = false; // 初始化为不处于分享文件过程
    // 创建一个 QListWidget 控件，用于显示文件和文件夹列表
    m_pBookListw = new QListWidget(this);
    // 创建各种按钮
    m_pReturnPB = new QPushButton("返回", this);
    m_pCreateDirPB = new QPushButton("新建文件夹", this);
    m_pDelItemPB = new QPushButton("删除", this);
    m_pRenamePB = new QPushButton("重命名", this);
    m_pFlushFilePB = new QPushButton("刷新文件", this);
    m_pMoveFilePB = new QPushButton("移动文件", this);
    m_pSelectDirPB = new QPushButton("目标目录", this);
    m_pSelectDirPB->setEnabled(false); // 初始状态下禁用目标目录按钮

    // --- 新增进度条 ---
    m_pProgressBar = new QProgressBar(this);
    m_pProgressBar->setRange(0, 100);
    m_pProgressBar->setValue(0);
    m_pProgressBar->setVisible(false); // 初始时隐藏

    // 创建一个垂直布局管理器，用于放置文件夹操作按钮
    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelItemPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);
    pDirVBL->addWidget(m_pMoveFilePB);
    pDirVBL->addWidget(m_pSelectDirPB);
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
    QHBoxLayout *pMainHBL = new QHBoxLayout;
    pMainHBL->addWidget(m_pBookListw); // 将文件列表添加到主布局
    pMainHBL->addLayout(pDirVBL);      // 将文件夹操作布局添加到主布局
    pMainHBL->addLayout(pFileVBL);     // 将文件操作布局添加到主布局

    // 将主布局和进度条放入一个新的垂直布局
    QVBoxLayout* pFinalLayout = new QVBoxLayout;
    pFinalLayout->addLayout(pMainHBL);
    pFinalLayout->addWidget(m_pProgressBar); // 将进度条添加到布局底部
    setLayout(pFinalLayout);

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
    //将下载按钮的 clicked 信号连接到 downloadFile 槽函数
    connect(m_pDownLoadPB, &QPushButton::clicked, this, &Book::downloadFile);

    // 将分享文件按钮的 clicked 信号连接到 shareFile 槽函数
    connect(m_pShareFilePB, &QPushButton::clicked, this, &Book::shareFile);
    // 将移动文件按钮的 clicked 信号连接到 moveFile 槽函数
    connect(m_pMoveFilePB, &QPushButton::clicked, this, &Book::moveFile);
    // 将目标目录按钮的 clicked 信号连接到 selectDir 槽函数
    connect(m_pSelectDirPB, &QPushButton::clicked, this, &Book::selectDir);
}

void Book::flushFileSlot(){
    // 1.获取当前路径
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();
    // 2. 使用工厂函数创建 PDU，vMsg 的大小为路径的长度
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_FLUSH_FILE_REQUEST, pathData.size());

    // 3. 将路径数据拷贝到 vMsg 中
    memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

    // 4. 通过 TcpClient 发送 PDU
    TcpClient::getInstance().sendPdu(std::move(pdu));
}


// 刷新文件列表的槽函数，根据服务器返回的数据（pdu）更新显示
void Book::flushFile(const PDU &pdu)
{
    // 1. 从 pdu.vMsg 中获取序列化的数据
    QByteArray buffer(pdu.vMsg.data(), pdu.vMsg.size());
    QDataStream stream(&buffer, QIODevice::ReadOnly);

    // 2. 反序列化数据到 QList<FileInfo>
    QList<FileInfo> fileList;
    stream >> fileList;

    if (stream.status() != QDataStream::Ok) {
        qWarning() << "Failed to deserialize file list.";
        return;
    }

    // 3. 清空并用新数据填充UI列表
    m_pBookListw->clear();
    for (const FileInfo& info : fileList) {
        QString fileName = QString::fromStdString(info.sFileName);
        QListWidgetItem *pItem = new QListWidgetItem;

        if (info.iFileType == 0) { // 0 代表文件夹
            pItem->setIcon(QIcon(QPixmap(":/dir.png")));
        } else if (info.iFileType == 1) { // 1 代表文件
            pItem->setIcon(QIcon(QPixmap(":/file.jpg")));
        }

        pItem->setText(fileName);
        // 将文件类型存储到Item中，以便后续操作（如删除时判断）
        pItem->setData(Qt::UserRole, info.iFileType);
        m_pBookListw->addItem(pItem);
    }
}


// 新建文件夹的槽函数
void Book::createDirSlot()
{
    // 弹出输入对话框，获取用户输入的文件夹名称
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名称:");

    if (strNewDir.isEmpty()) {
        QMessageBox::warning(this, "新建文件夹", "文件夹名称不能为空");
        return;
    }
    if (strNewDir.size() > 32) { // 协议限制 caData 中的文件名长度
        QMessageBox::warning(this, "新建文件夹", "文件夹名称过长，不能超过32个字符");
        return;
    }
    // 获取当前路径
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();

    // 创建 PDU，vMsg 的大小为路径长度
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_CREATE_DIR_REQUEST, pathData.size());
    // 将新文件夹名称拷贝到 caData 中
    strncpy(pdu->caData, strNewDir.toStdString().c_str(), sizeof(pdu->caData)-1);
    memcpy(pdu->vMsg.data(),pathData.constData(),pathData.size());

    // 通过 TcpClient 发送 PDU
    TcpClient::getInstance().sendPdu(std::move(pdu));
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
        // 1. 获取项目名和当前路径
        QString itemName = pItem->text();
        QString strCurPath = TcpClient::getInstance().curPath();
        QByteArray pathData = strCurPath.toUtf8();

        // 2. 创建 PDU
        auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_DEL_ITEM_REQUEST, pathData.size());

        // 3. 填充数据 (项目名放入 caData，当前路径放入 vMsg)
        strncpy(pdu->caData, itemName.toStdString().c_str(), sizeof(pdu->caData) - 1);
        memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

        // 4. 发送
        TcpClient::getInstance().sendPdu(std::move(pdu));
    }
}

// 重命名文件夹的槽函数
void Book::reName()
{
    QListWidgetItem *pItem = m_pBookListw->currentItem();
    if (pItem == NULL) {
        QMessageBox::warning(this, "重命名", "请选择要重命名的项目");
        return;
    }

    QString strOldName = pItem->text();
    QString strNewName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, strOldName);

    if (strNewName.isEmpty()) {
        QMessageBox::warning(this, "重命名", "名称不能为空");
        return;
    }
    if (strNewName.size() > 32) {
        QMessageBox::warning(this, "重命名", "名称过长，不能超过32个字符");
        return;
    }
    if (strNewName == strOldName) {
        return; // 名称未改变，直接返回
    }

    // 1. 获取当前路径
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();

    // 2. 创建 PDU
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_RENAME_DIR_REQUEST, pathData.size());

    // 3. 填充数据 (将旧名称和新名称用 '\0' 分隔后放入 caData)
    //    这里我们直接用 QByteArray 来处理，更安全
    QByteArray namesData;
    namesData.append(strOldName.toUtf8()).append('\0').append(strNewName.toUtf8());
    memcpy(pdu->caData, namesData.constData(), qMin((int)sizeof(pdu->caData), namesData.size()));

    memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

    // 4. 发送
    TcpClient::getInstance().sendPdu(std::move(pdu));
}

// 进入目录的槽函数
void Book::entryDir(const QModelIndex &index){
    QListWidgetItem *item = m_pBookListw->item(index.row());
    if (!item || item->data(Qt::UserRole).toInt() != 0) { // 0 代表文件夹
        return; // 如果不是文件夹，则不处理
    }

    QString strDirName = item->text();
    TcpClient::getInstance().setEnterDirName(strDirName); // 记录进入的目录名，用于更新路径

    // 1. 获取当前路径
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();

    // 2. 创建 PDU
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_ENTRY_DIR_REQUEST, pathData.size());

    // 3. 填充数据 (目标文件夹名放入 caData，当前路径放入 vMsg)
    strncpy(pdu->caData, strDirName.toStdString().c_str(), sizeof(pdu->caData) - 1);
    memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

    // 4. 发送
    TcpClient::getInstance().sendPdu(std::move(pdu));
}

// 返回上一级目录的槽函数
void Book::returnDir()
{
    // 获取当前客户端的路径
    QString strCurPath = TcpClient::getInstance().curPath();
    // 获取真实的、由服务器授予的根目录
    QString strRootPath = TcpClient::getInstance().getRootPath();

    // 为了防止路径末尾有'/'导致判断失败，先进行规范化
    if (strCurPath.endsWith('/')) {
        strCurPath.chop(1);
    }

    // 现在这个判断是可靠的了
    if(strCurPath == strRootPath){
        QMessageBox::warning(this,"返回","已经是根目录，无法返回");
        return;
    }

    // 找到最后一个 '/' 的位置
    int index = strCurPath.lastIndexOf('/');
    // 截取字符串，得到上一级目录路径
    QString parentPath = strCurPath.left(index);

    // 更新客户端的当前路径
    TcpClient::getInstance().setEnterDirName(""); // 清空进入目录名称
    TcpClient::getInstance().setCurPath(parentPath);
    // 主动请求刷新文件列表
    flushFileSlot();
}
// 上传文件的槽函数
void Book::uploadFile()
{
    QString uploadFilePath = QFileDialog::getOpenFileName(this, "选择要上传的文件");
    if (uploadFilePath.isEmpty()) return;

    // --- 1. 准备 PDU 并序列化 ---
    QFileInfo fileInfo(uploadFilePath);
    QString strFileName = fileInfo.fileName();
    qint64 fileSize = fileInfo.size();
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST, pathData.size());
    QString fileHeader = QString("%1#%2").arg(strFileName).arg(fileSize);
    strncpy(pdu->caData, fileHeader.toStdString().c_str(), sizeof(pdu->caData) - 1);
    memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

    std::vector<char> serialized_pdu_vec = pdu->serialize();
    QByteArray serialized_pdu = QByteArray(serialized_pdu_vec.data(), serialized_pdu_vec.size());

    // --- 2. 设置多线程 ---
    QThread* thread = new QThread;
    FileUploadWorker* worker = new FileUploadWorker;
    worker->moveToThread(thread);

    // a. 触发工作
    //    注意信号签名要改
    connect(this, &Book::startUploadTask, worker, &FileUploadWorker::startUpload);
    // b. 处理反馈
    connect(worker, &FileUploadWorker::progress, this, &Book::updateUploadProgress);
    connect(worker, &FileUploadWorker::error, this, &Book::onUploadError);
    // c. 清理
    connect(worker, &FileUploadWorker::finished, this, &Book::onUploadFinished);
    connect(worker, &FileUploadWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();

    // 5. 发出信号，触发任务开始
    QString serverIP = TcpClient::getInstance().getServerIP(); // 假设TcpClient提供这些接口
    quint16 serverPort = TcpClient::getInstance().getServerPort();
    emit startUploadTask(uploadFilePath, serverIP, serverPort, serialized_pdu);

    // UI提示
    m_pProgressBar->setValue(0);
    m_pProgressBar->setVisible(true);
}

void Book::sendDataChunk(const QByteArray &data)
{
    // 这个槽在主线程中执行，安全地写入主线程的socket
    TcpClient::getInstance().getTcpSocket().write(data);
}

void Book::updateUploadProgress(qint64 sentSize, qint64 totalSize)
{
    if (totalSize > 0) {
        int percentage = (sentSize * 100) / totalSize;
        m_pProgressBar->setValue(percentage);
    }
}

void Book::onUploadError(const QString &err)
{
    QMessageBox::critical(this, "上传失败", err);
    m_pProgressBar->setVisible(false); // 隐藏进度条
}

void Book::onUploadFinished()
{
    qDebug() << "主线程：收到工作线程 finished 信号";
    m_pProgressBar->setVisible(false);
    // 注意：这里的完成只是文件“读取”完成，
    // 最终的“上传成功”消息需要等待服务器的PDU响应
    // 主动请求刷新文件列表
    flushFileSlot();
}


void Book::startFileUpload()
{
    m_pTimer.start(1); // 立即开始，尽可能快地发送
}

void Book::uploadFileData(){
    // 这个函数现在是 private slot，并且需要连接到 m_pTimer.timeout()
    // connect(&m_pTimer, &QTimer::timeout, this, &Book::uploadFileData);

    QFile file(m_strUploadFilePath);
    // 每次都重新打开文件并seek，虽然效率稍低，但实现简单。
    // 更高效的方式是将 QFile 对象作为成员变量。
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "上传文件", "本地文件打开失败");
        m_pTimer.stop();
        return;
    }

    // 计算已发送大小，并移动文件指针
    static qint64 sentSize = 0; // 使用 static 或成员变量来跟踪进度
    file.seek(sentSize);

    char caBuf[4096] = {0};
    qint64 readSize = file.read(caBuf, sizeof(caBuf));

    if (readSize > 0) {
        if (TcpClient::getInstance().getTcpSocket().write(caBuf, readSize) == -1) {
            QMessageBox::warning(this, "上传文件", "文件上传失败：网络写入错误");
            m_pTimer.stop();
            sentSize = 0; // 重置进度
        }
        sentSize += readSize;
    }

    if (file.atEnd() || readSize == 0) {
        // 文件发送完毕或读取完毕
        m_pTimer.stop();
        sentSize = 0; // 重置进度
    }

    file.close();
}

void Book::downloadFile()
{
    if (m_isDownloading) {
        QMessageBox::information(this, "下载", "当前已有文件正在下载，请稍后再试。");
        return;
    }
    QListWidgetItem *pItem = m_pBookListw->currentItem();
    if (!pItem || pItem->data(Qt::UserRole).toInt() != 1) { // 1 代表文件
        QMessageBox::warning(this, "下载", "请选择一个文件进行下载");
        return;
    }

    // 1. 获取本地保存路径
    QString fileName = pItem->text();
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件", QDir(defaultPath).filePath(fileName));
    if (saveFilePath.isEmpty()) {
        return; // 用户取消
    }

    // 2. 准备下载请求 PDU
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray pathData = strCurPath.toUtf8();
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST, pathData.size());
    strncpy(pdu->caData, fileName.toStdString().c_str(), sizeof(pdu->caData) - 1);
    memcpy(pdu->vMsg.data(), pathData.constData(), pathData.size());

    // 序列化PDU以便跨线程传递
    std::vector<char> serialized_pdu_vec = pdu->serialize();
    QByteArray serialized_pdu = QByteArray(serialized_pdu_vec.data(), serialized_pdu_vec.size());

    // 3. 设置多线程
    QThread* thread = new QThread;
    FileDownloadWorker* worker = new FileDownloadWorker;
    worker->moveToThread(thread);

    // a. 触发工作
    connect(this, &Book::startDownloadTask, worker, &FileDownloadWorker::startDownload);
    // b. 处理反馈
    connect(worker, &FileDownloadWorker::progress, this, &Book::updateDownloadProgress);
    connect(worker, &FileDownloadWorker::error, this, &Book::onDownloadError);
    connect(worker, &FileDownloadWorker::finished, this, &Book::onDownloadFinished);
    // c. 清理
    connect(worker, &FileDownloadWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();

    // 4. 发出信号，触发任务开始
    QString serverIP = TcpClient::getInstance().getServerIP();
    quint16 serverPort = TcpClient::getInstance().getServerPort();
    emit startDownloadTask(saveFilePath, serverIP, serverPort, serialized_pdu);
    m_isDownloading = true; // 【新增】上锁
    // 5. UI提示
    m_pProgressBar->setValue(0);
    m_pProgressBar->setVisible(true);
}

// --- 新增的槽函数实现 ---

void Book::updateDownloadProgress(qint64 receivedSize, qint64 totalSize)
{
    if (totalSize > 0) {
        int percentage = (receivedSize * 100) / totalSize;
        m_pProgressBar->setValue(percentage);
    }
}

void Book::onDownloadError(const QString &err)
{
    QMessageBox::critical(this, "下载失败", err);
    m_pProgressBar->setVisible(false);
     m_isDownloading = false; // 【新增】解锁
}

void Book::onDownloadFinished(const QString& message)
{
    m_pProgressBar->setVisible(false);
    QMessageBox::information(this, "下载文件", message);
    // 下载成功后可以刷新本地目录，如果需要的话

    m_isDownloading = false; // 【新增】解锁
}

void Book::setDownloadStatus(bool status)
{
    qDebug() << "设置下载状态 m_bDownload 为:" << status;
    m_bDownload = status;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}
/**
 * @brief 点击“分享”按钮的槽函数 (入口)
 *
 * 这是一个异步流程的起点。由于分享操作需要最新的好友列表，
 * 此函数不直接打开分享窗口，而是先触发一次好友列表的刷新。
 * 它通过设置一个状态标志 m_bInSharingProcess 来“预约”一个操作，
 * 当好友列表刷新完成后，会由 handleFriendListUpdated 接手处理。
 */
void Book::shareFile()
{
    // 1. 检查是否已选中一个文件或文件夹
    QListWidgetItem *pCurItem = m_pBookListw->currentItem();
    if (pCurItem == NULL) {
        QMessageBox::warning(this, "分享", "请选择要分享的文件或文件夹");
        return;
    }

    // 2. 设置状态标志，表明我们正处于分享流程中
    m_bInSharingProcess = true;

    // 3. 发送刷新好友列表的请求，这是一个异步操作
    OpeWidget::getInstance().getFriend()->flushFriend();
}

/**
 * @brief 处理好友列表更新完成的槽函数 (分享流程的第二步)
 *
 * 此函数由 Friend 模块在好友列表刷新成功后通过信号触发。
 * 它检查 m_bInSharingProcess 标志，如果为true，则继续执行分享流程：
 * 弹出好友选择窗口。
 */
void Book::handleFriendListUpdated()
{
    // 如果当前不是分享流程，则直接返回
    if (!m_bInSharingProcess){
        return;
    }
    // 重置标志，防止后续的刷新操作错误地触发此逻辑
    m_bInSharingProcess = false;

    QListWidgetItem *pCurItem = m_pBookListw->currentItem();
    // 再次检查，防止在刷新过程中用户取消了选择
    if (pCurItem == NULL) {
        return;
    }
    m_strShareFileName = pCurItem->text();

    Friend *pFriend = OpeWidget::getInstance().getFriend();
    QListWidget *pFriendList = pFriend->getFriendList();

    if (pFriendList->count() == 0){
        QMessageBox::information(this, "分享文件", "好友列表为空，无法分享");
        return;
    }

    // 使用最新的好友列表更新分享窗口的UI
    ShareFile::getInstance().updateFriend(pFriendList);

    // 显示分享窗口
    if(ShareFile::getInstance().isHidden()){
        ShareFile::getInstance().show();
    }
}

// --- 文件移动流程 (重构) ---

/**
 * @brief 点击“移动文件”按钮的槽函数
 *
 * 这是移动文件操作的第一步。记录下要移动的文件或文件夹，
 * 并启用“目标目录”按钮，等待用户选择目标位置。
 */
void Book::moveFile()
{
    QListWidgetItem* pItem = m_pBookListw->currentItem();
    if (!pItem) {
        QMessageBox::warning(this, "移动文件", "请选择要移动的文件或文件夹。");
        return;
    }

    // 记录源文件的完整路径
    m_strMoveFilePath = TcpClient::getInstance().curPath() + "/" + pItem->text();

    // 启用目标目录按钮，并提示用户进行下一步操作
    m_pSelectDirPB->setEnabled(true);
    QMessageBox::information(this, "移动文件",
                             QString("已选择“%1”，请在列表中选择一个目标文件夹，然后点击“目标目录”按钮。").arg(pItem->text()));
}

/**
 * @brief 点击“目标目录”按钮的槽函数 (原 selectDir 函数重构)
 *
 * 这是移动文件操作的第二步。获取用户选择的目标文件夹，
 * 构建并发送移动文件的请求。完成后，重置状态。
 */
void Book::selectDir()
{
    QListWidgetItem *pItem = m_pBookListw->currentItem();
    if (!pItem || pItem->data(Qt::UserRole).toInt() != 0) { // 必须选择一个文件夹
        QMessageBox::warning(this, "选择目标", "请选择一个文件夹作为目标目录！");
        return;
    }

    QString strDestDirName = pItem->text();
    QString strCurPath = TcpClient::getInstance().curPath();
    m_strDestDirPath = strCurPath + '/' + strDestDirName;

    // 安全检查：不能移动到自身或其子目录
    if (m_strMoveFilePath == m_strDestDirPath || m_strDestDirPath.startsWith(m_strMoveFilePath + "/")) {
        QMessageBox::warning(this, "移动文件", "不能将项目移动到其自身或其子目录中！");
        return;
    }

    // 1. 使用 QByteArray 构建 vMsg，格式：[源路径]\0[目标路径]
    QByteArray vMsgData;
    vMsgData.append(m_strMoveFilePath.toUtf8());
    vMsgData.append('\0');
    vMsgData.append(m_strDestDirPath.toUtf8());

    // 2. 使用工厂函数创建 PDU
    auto pdu = make_pdu(MsgType::ENUM_MSG_TYPE_MOVE_FILE_REQUEST, vMsgData.size());

    // 3. 填充 vMsg
    memcpy(pdu->vMsg.data(), vMsgData.constData(), vMsgData.size());

    // 4. 发送 PDU
    TcpClient::getInstance().sendPdu(std::move(pdu));

    // 5. 重置状态
    m_pSelectDirPB->setEnabled(false);
    m_strMoveFilePath.clear();
    m_strDestDirPath.clear();
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

QString Book::getShareFileName()
{
    return m_strShareFileName;
}

