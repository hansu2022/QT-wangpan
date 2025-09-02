#include "mytcpsocket.h" // 包含自定义的头文件，通常包含 MyTcpSocket 类的声明
#include "mytcpserver.h" // 包含 MyTcpServer 类的头文件，用于服务器的转发逻辑
#include <QDebug>        // 提供 qDebug() 函数，用于调试输出
#include <QTimer>
// MyTcpSocket 类的构造函数
MyTcpSocket::MyTcpSocket()
{
    // 连接信号和槽：当 socket 接收到新数据时，触发 recvMsg() 槽函数
    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    // 连接信号和槽：当客户端断开连接时，触发 clientOffline() 槽函数
    connect(this,SIGNAL(disconnected()),this,SLOT(clientOffline()));

    m_bUpload = false; // 初始化上传状态为 false
    m_pTimer = new QTimer(this); // 创建一个新的定时器对象
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sendFileToClient())); // 连接定时器的超时信号到发送文件槽函数
}

// 获取当前客户端的用户名
QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString srcDir, QString destDir)
{
    QDir dest_dir(destDir);
    if (dest_dir.exists()) {
        //目标文件夹已存在
        //将新文件夹重命名，例如 "docs_1"
        destDir = destDir + "_1";
    }
    QDir dir;
    dir.mkdir(destDir);
    dir.setPath(srcDir);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (fileInfo.isDir()) {
            copyDir(fileInfo.filePath(), destDir + "/" + fileInfo.fileName());
        } else {
            QFile::copy(fileInfo.filePath(), destDir + "/" + fileInfo.fileName());
        }
    }
}

// 槽函数：处理接收到的数据
void MyTcpSocket::recvMsg()
{
    if(!m_bUpload){
        qDebug() << this ->bytesAvailable(); // 调试输出当前 socket 可读的字节数
        uint uiPDULen = 0;
        // 首先读取数据包的总长度
        this->read((char*)&uiPDULen,sizeof(uint));
        // 计算实际消息内容的长度
        uint uiMsgLen = uiPDULen-sizeof(PDU);
        // 根据消息长度创建 PDU 数据包
        PDU *pdu = mkPDU(uiMsgLen);
        // 读取剩余的数据到 PDU 结构体中
        this ->read((char*)pdu + sizeof(uint),uiPDULen-sizeof(uint));
        // 根据消息类型处理不同的逻辑
        switch(pdu->uiMsgType){
        // 处理注册请求
        case ENUM_MSG_TYPE_REGIST_REQUEST:{
            char caName[32] = {"\0"}; // 用户名
            char caPwd[32] = {"\0"};  // 密码
            strncpy(caName,pdu->caData,32);       // 复制用户名
            strncpy(caPwd,pdu->caData+32,32);     // 复制密码
            qDebug() << "zhanghu:" <<caName << "mima"<< caPwd; // 调试输出账号密码

            // 调用数据库操作类处理注册请求
            bool ret = OpeDB::getInstance().handleRegist(caName,caPwd);
            PDU *respdu = mkPDU(0); // 创建响应数据包
            respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND; // 设置响应消息类型
            if(ret){
                strcpy(respdu->caData,REGIST_OK); // 设置响应数据为注册成功
                QDir dir;
                dir.mkdir(caName); // 为新注册用户创建一个目录
            }else{
                qDebug() << "failed"; // 调试输出注册失败
                strcpy(respdu->caData,REGIST_FAILED); // 设置响应数据为注册失败
            }
            write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
            free(respdu); // 释放内存
            respdu = NULL;
            break;
        }
        // 处理登录请求
        case ENUM_MSG_TYPE_LOGIN_REQUEST:{
            char caName[32] = {"\0"}; // 用户名
            char caPwd[32] = {"\0"};  // 密码
            strncpy(caName,pdu->caData,32);       // 复制用户名
            strncpy(caPwd,pdu->caData+32,32);     // 复制密码

            // 调用数据库操作类处理登录请求
            bool ret = OpeDB::getInstance().handleLogin(caName,caPwd);
            PDU *respdu = mkPDU(0); // 创建响应数据包
            respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND; // 设置响应消息类型
            if(ret){
                strcpy(respdu->caData,LOGIN_OK); // 设置响应数据为登录成功
                m_strName = caName; // 登录成功后保存当前客户端的用户名
            }else{
                strcpy(respdu->caData,LOGIN_FAILED); // 设置响应数据为登录失败
            }
            write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
            free(respdu); // 释放内存
            respdu = NULL;
            break;
        }
        // 处理请求所有在线用户列表
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:{
            // 调用数据库操作类获取所有在线用户的列表
            QStringList ret = OpeDB::getInstance().handleAllOnline();
            uint uiMsgLen = ret.size()*32; // 计算消息内容的长度（每个用户名32字节）
            PDU *respdu = mkPDU(uiMsgLen); // 创建响应数据包
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND; // 设置响应消息类型
            // 将在线用户名复制到 PDU 的消息数据区
            for(int i = 0; i < ret.size(); i++){
                strncpy(respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),32);
            }
            write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
            free(respdu); // 释放内存
            respdu = NULL;
            break;
        }
        // 处理搜索用户请求
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:{
            // 调用数据库操作类处理搜索用户请求，返回用户状态
            int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
            PDU *respdu = mkPDU(0); // 创建响应数据包
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND; // 设置响应消息类型
            if(ret == -1){ // 用户不存在
                strcpy(respdu->caData,SEARCH_USR_NO);
            }else if(ret == 1){ // 用户在线
                strcpy(respdu->caData,SEARCH_USR_YES);
            }else{ // 用户不在线 (0)
                strcpy(respdu->caData,SEARCH_USR_OFFLINE);
            }
            write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
            free(respdu); // 释放内存
            respdu = NULL;
            break;
        }
        // 处理添加好友请求 (A -> 服务器)
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'}; // 被请求者 (B) 的名字
            char caName[32] = {'\0'};    // 请求者 (A) 的名字
            strncpy(caPerName,pdu->caData,32);         // 从 PDU 中复制被请求者名字
            strncpy(caName,pdu->caData+32,32);         // 从 PDU 中复制请求者名字
            qDebug() << "caPerName:" << caPerName << "caName:" << caName;

            // 调用数据库操作类处理添加好友请求，返回结果码
            int res = OpeDB::getInstance().handleAddFriend(caPerName,caName);
            qDebug() << "handleAddFriend res:" << res;
            PDU *respdu = NULL;

            if(-1 == res){ // 未知错误
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,UNKONW_ERROR);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else if(0 == res){ // 已经是好友
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,EXISTED_FRIEND);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else if(1 == res){ // 请求成功，且被请求者在线，需要转发请求
                // 将 PDU 原样转发给被请求者 (B)，让 B 决定是否同意
                MyTcpServer::getInstance().resend(caPerName,pdu);
            }else if(2 == res){ // 请求成功，但被请求者不在线
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else if(3 == res){ // 被请求者不存在
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_NO_EXIST);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else if(4 == res){ // 处理不能添加自己的情况
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,"不能添加自己为好友"); // 定义一个明确的错误信息
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else{ // 其他未知错误
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,UNKONW_ERROR);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            break;
        }
        // 处理添加好友同意请求 (B -> 服务器)
        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char caName[32] = {'\0'};     // 请求者 (A) 的名字
            char caPerName[32] = {'\0'};  // 同意者 (B) 的名字
            strncpy(caName, pdu->caData, 32);           // 复制请求者名字
            strncpy(caPerName, pdu->caData + 32, 32);   // 复制同意者名字

            // 1. 在数据库中正式添加好友关系
            OpeDB::getInstance().handleAddFriendAgree(caName, caPerName);

            // 2. 通知 A，B 已经同意了你的好友请求
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            QString successMsg = QString("%1 已同意您的好友请求").arg(caPerName);
            strncpy(respdu->caData, successMsg.toStdString().c_str(), successMsg.size());
            // 找到 A 的 socket 并发送消息
            MyTcpServer::getInstance().resend(caName, respdu);

            free(respdu);
            respdu = NULL;
            break;
        }
        // 处理添加好友拒绝请求 (B -> 服务器)
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char caName[32] = {'\0'};     // 请求者 (A) 的名字
            char caPerName[32] = {'\0'};  // 拒绝者 (B) 的名字
            strncpy(caName, pdu->caData, 32);           // 复制请求者名字
            strncpy(caPerName, pdu->caData + 32, 32);   // 复制拒绝者名字

            // 直接通知 A，B 拒绝了你的好友请求
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            QString refuseMsg = QString("%1 拒绝了您的好友请求").arg(caPerName);
            strncpy(respdu->caData, refuseMsg.toStdString().c_str(), refuseMsg.size());
            // 找到 A 的 socket 并发送消息
            MyTcpServer::getInstance().resend(caName, respdu);

            free(respdu);
            respdu = NULL;
            break;
        }
        // 处理刷新好友列表请求
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:{
            char caName[32] = {'\0'};
            strncpy(caName,pdu->caData,32); // 复制用户名
            QStringList ret = OpeDB::getInstance().handleFlushFriend(caName); // 调用数据库操作类处理刷新好友请求
            uint uiMsgLen = ret.size()*32; // 计算消息内容的长度（每个好友32字节）
            PDU *respdu = mkPDU(uiMsgLen); // 创建响应数据包
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND; // 设置响应消息类型
            for(int i = 0;i<ret.size();i++){
                memcpy((char*)(respdu->caMsg)+i*32,ret.at(i).toStdString().c_str(),32); // 复制好友列表到 PDU 的消息数据区
            }
            write((char*)respdu,respdu->uiPDULen); // 将响应数据包发送给客户端
            free(respdu); // 释放内存
            respdu = NULL;
            break;
        }
        // 处理删除好友请求
        case ENUM_MSG_TYPE_DEL_FRIEND_REQUEST:
        {
            // 1. 正确解析请求者和被删除者
            char caMyName[32] = {'\0'};     // 请求删除的人 (自己)
            char caFriendName[32] = {'\0'}; // 被删除的人 (好友)
            strncpy(caMyName, pdu->caData, 32);
            strncpy(caFriendName, pdu->caData + 32, 32);

            // 调用数据库执行删除操作
            bool ret = OpeDB::getInstance().handleDelFriend(caMyName, caFriendName);

            // 2. 准备给请求者的响应
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_RESPOND;

            if (ret) {
                // 如果数据库操作成功
                strcpy(respdu->caData, "删除好友成功");

                // 3. 关键：创建并发送通知给被删除者
                PDU* noticePdu = mkPDU(0);
                // 4. 使用正确的通知消息类型！
                noticePdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_NOTICE;
                strncpy(noticePdu->caData, caMyName, 32); // 把删除者的名字放入通知中

                // 通过 resend 函数将通知转发给被删除的好友
                MyTcpServer::getInstance().resend(caFriendName, noticePdu);

                free(noticePdu);
                noticePdu = NULL;
            } else {
                // 如果数据库操作失败
                strcpy(respdu->caData, "删除好友失败");
            }

            // 将操作结果响应给发起删除请求的客户端
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        // 处理私聊请求
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:{
            // 声明一个字符数组用于存储消息接收者的名字，并初始化为全0
            char caPerName[32] = {'\0'};
            // 从PDU（协议数据单元）的caData字段中偏移32个字节处，复制消息接收者的名字
            strncpy(caPerName,pdu->caData+32,32);
            // 通过 MyTcpServer 的单例，调用 resend 方法将私聊消息转发给指定接收者
            MyTcpServer::getInstance().resend(caPerName,pdu);
            break;
        }
        // 处理群聊请求
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:{
            // 声明一个字符数组用于存储消息发送者的名字
            char caSendName[32] = {'\0'};
            // 从PDU的caData字段中复制发送者名字
            strncpy(caSendName, pdu->caData, 32);

            // 1. 调用数据库操作类 OpeDB 的 handleFlushFriend 方法，查询发送者的所有好友及其在线状态
            QStringList friendList = OpeDB::getInstance().handleFlushFriend(caSendName);

            // 2. 遍历好友列表
            for (const QString &friendInfo : friendList) {
                // friendInfo 的格式为 "friendName(在线)" 或 "friendName(不在线)"
                // 找到名字和状态之间的左括号位置
                int pos = friendInfo.indexOf('(');
                if (pos != -1) {
                    // 提取好友的名字
                    QString friendName = friendInfo.left(pos);
                    // 提取好友的在线状态
                    QString status = friendInfo.mid(pos + 1, 2);

                    // 3. 判断好友是否在线
                    if (status == "在线") {
                        // 4. 如果在线，则将群聊消息转发给该好友
                        MyTcpServer::getInstance().resend(friendName.toStdString().c_str(), pdu);
                    }
                }
            }
            break;
        }
        // 处理新建文件夹请求
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST: {
            // 创建一个 QDir 对象，用于文件系统操作
            QDir dir;
            // 从PDU的caMsg字段中获取当前路径
            QString strCurPath = QString(pdu->caMsg);
            qDebug() << strCurPath; // 打印当前路径用于调试

            // 检查当前路径是否存在
            bool ret = dir.exists(strCurPath);
            // 准备一个用于响应客户端的PDU
            PDU *respdu = mkPDU(0);

            if(ret){ // 如果当前路径存在
                // 声明一个字符数组用于存储新文件夹的名称
                char caNewDir[32] = {'\0'};
                // 从PDU的caData字段中偏移32个字节处，复制新文件夹的名称
                strncpy(caNewDir, pdu->caData + 32, 32);
                // 拼接出新文件夹的完整路径
                QString strNewPath = strCurPath + "/" + QString(caNewDir);
                qDebug() << strNewPath; // 打印新路径用于调试

                // 再次检查新文件夹是否已经存在
                ret = dir.exists(strNewPath);
                qDebug()<< "exists:"<< ret; // 打印检查结果用于调试

                if(ret){ // 如果新文件夹已存在
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND; // 设置响应消息类型
                    // 设置响应数据为“文件夹已存在，创建文件夹失败”
                    strcpy(respdu->caData,"文件夹已存在，创建文件夹失败");
                }else{ // 如果新文件夹不存在
                    dir.mkdir(strNewPath); // 创建新文件夹
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND; // 设置响应消息类型
                    // 设置响应数据为“创建文件夹成功”
                    strcpy(respdu->caData,"创建文件夹成功");
                }
            }else{ // 如果当前路径不存在
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND; // 设置响应消息类型
                // 设置响应数据为“当前路径不存在，创建文件夹失败”
                strcpy(respdu->caData,"当前路径不存在，创建文件夹失败");
            }
            // 将响应PDU发送给客户端
            write((char*)respdu, respdu->uiPDULen);
            // 释放PDU占用的内存
            free(respdu);
            respdu = NULL;
            break;
        }
        // 处理刷新文件请求
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:{
            // 创建一个字符数组，用于存储当前路径
            char *pCurPath = new char[pdu->uiMsgLen];
            // 从PDU的caMsg字段中复制当前路径
            strncpy(pCurPath,pdu->caMsg,pdu->uiMsgLen);
            // 确保字符串以 '\0' 结尾
            pCurPath[pdu->uiMsgLen - 1] = '\0';

            // 创建一个 QDir 对象，传入当前路径
            QDir dir(pCurPath);
            // 获取当前目录下的所有文件和目录信息
            QFileInfoList fileInfoList = dir.entryInfoList();

            // 1. 统计有效的文件和目录数量（排除 "." 和 ".."）
            int iFileCount = 0;
            for(int i=0; i<fileInfoList.size(); ++i) {
                if(fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == "..") {
                    continue;
                }
                iFileCount++;
            }

            // 2. 根据有效文件数量创建响应PDU
            PDU *respdu = mkPDU(iFileCount * sizeof(FileInfo));
            strcpy(respdu->caData,"success");
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;

            // 指向PDU消息内容的起始位置
            FileInfo *pFileInfo = (FileInfo*)(respdu->caMsg);
            int currentIndex = 0; // 用于跟踪已填充的文件信息数量

            // 3. 再次遍历文件信息列表并填充PDU
            for(int i=0; i<fileInfoList.size(); ++i){
                // 忽略 "." 和 ".." 目录
                if(fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == ".."){
                    continue;
                }

                // 根据当前索引计算当前 FileInfo 结构体的位置
                pFileInfo = (FileInfo*)(respdu->caMsg + currentIndex * sizeof(FileInfo));
                QString strFileName = fileInfoList.at(i).fileName();

                // 安全地将文件名拷贝到 FileInfo 结构体中，并确保以空字符结尾
                strncpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), 31);
                pFileInfo->caFileName[31] = '\0';

                // 根据文件类型设置 iFileType 字段（0为目录，1为文件）
                if(fileInfoList[i].isDir()){
                    pFileInfo->iFileType = 0;
                } else if (fileInfoList[i].isFile()){
                    pFileInfo->iFileType = 1;
                }
                currentIndex++; // 索引加一，准备填充下一个文件信息
            }

            // 将响应PDU发送给客户端
            write((char*)respdu,respdu->uiPDULen);
            // 释放PDU占用的内存
            free(respdu);
            respdu = NULL;
            // 释放用于存储路径的字符数组内存
            delete[] pCurPath;
            pCurPath = NULL;
            break;
        }

        case ENUM_MSG_TYPE_DEL_ITEM_REQUEST:{
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32); // 获取用户名
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            pPath[pdu->uiMsgLen-1] = '\0';
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if(fileInfo.isDir()){
                ret = QDir(strPath).removeRecursively();
            }else if(fileInfo.isFile()){
                ret = QFile::remove(strPath);
            }
            PDU *respdu = NULL;
            if(ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_ITEM_RESPOND;
                strcpy(respdu->caData,"删除成功");
            }else{
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_ITEM_RESPOND;
                // 提供失败原因
                if (!fileInfo.exists()) {
                    strcpy(respdu->caData, "删除失败：项目不存在");
                } else {
                    strcpy(respdu->caData, "删除失败：权限不足或未知错误");
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            delete[] pPath;
            pPath = NULL;
            break;
        }

        case ENUM_MSG_TYPE_RENAME_DIR_REQUEST:{
            char caOldName[32] = {'\0'};
            char caNewName[32] = {'\0'};
            strncpy(caOldName,pdu->caData,32);
            strncpy(caNewName,pdu->caData+32,32);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            pPath[pdu->uiMsgLen-1] = '\0';

            QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);
            QFileInfo fileInfo(strOldPath);
            QDir dir;
            bool ret = dir.rename(strOldPath,strNewPath);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIR_RESPOND;
            if(ret){
                strcpy(respdu->caData,"重命名成功");
            }else{
                strcpy(respdu->caData,"重命名失败");
            }
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            delete[] pPath;
            pPath = NULL;
            break;
        }

        // 处理进入文件夹请求
        case ENUM_MSG_TYPE_ENTRY_DIR_REQUEST: {
            // 从PDU的caData字段中获取要进入的文件夹名称
            char caEnterName[32] = {'\0'};
            strncpy(caEnterName, pdu->caData, 32);
            // 从PDU的caMsg字段中获取当前路径，并动态分配内存
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            pPath[pdu->uiMsgLen - 1] = '\0';
            // 拼接要进入的完整路径
            QString strEnterPath = QString("%1/%2").arg(pPath).arg(caEnterName);
            QFileInfo fileInfo(strEnterPath);
            PDU *respdu = NULL;
            // 如果是文件夹
            if (fileInfo.isDir()) {
                QDir dir(strEnterPath);
                // 获取文件夹下的所有文件和子文件夹信息
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount = 0;
                // 遍历列表，统计有效文件/文件夹数量（排除"."和".."）
                for (int i = 0; i < fileInfoList.size(); ++i) {
                    if (fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == "..") {
                        continue;
                    }
                    iFileCount++;
                }
                // 根据有效文件数量创建响应PDU
                respdu = mkPDU(iFileCount * sizeof(FileInfo));
                strcpy(respdu->caData,"success");
                // 设置响应消息类型为“刷新文件列表响应”
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
                FileInfo *pFileInfo = (FileInfo*)(respdu->caMsg);
                int currentIndex = 0;
                strcpy(respdu->caData,"success");
                // 再次遍历列表，填充PDU中的文件信息
                for (int i = 0; i < fileInfoList.size(); ++i) {
                    if (fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == "..") {
                        continue;
                    }
                    // 获取当前文件/文件夹的FileInfo结构体指针
                    pFileInfo = (FileInfo*)(respdu->caMsg + currentIndex * sizeof(FileInfo));
                    QString strFileName = fileInfoList.at(i).fileName();
                    // 拷贝文件名，并确保空终止
                    strncpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), 31);
                    pFileInfo->caFileName[31] = '\0';
                    // 设置文件类型（0为文件夹，1为文件）
                    if (fileInfoList[i].isDir()) {
                        pFileInfo->iFileType = 0;
                    } else if (fileInfoList[i].isFile()) {
                        pFileInfo->iFileType = 1;
                    }
                    currentIndex++;
                }
                // 发送响应PDU
                write((char*)respdu, respdu->uiPDULen);
                // 释放内存
                free(respdu);
                respdu = NULL;
            } else if (fileInfo.isFile()) { // 如果是文件，则不能进入
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTRY_DIR_RESPOND;
                strcpy(respdu->caData, "进入文件夹失败，不是文件夹");
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            } else { // 如果路径不存在
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTRY_DIR_RESPOND;
                strcpy(respdu->caData, "进入文件夹失败，文件夹不存在");
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            // 释放动态分配的路径内存
            delete[] pPath;
            pPath = NULL;
            break;
        }
        // 处理上传文件请求
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:{
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            sscanf(pdu->caData,"%[^#]#%lld",caFileName,&fileSize);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            pPath[pdu->uiMsgLen-1] = '\0';
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug() << strPath << fileSize;
            delete[] pPath;
            pPath = NULL;

            m_file.setFileName(strPath);
            // 只读模式打开文件
            if(m_file.open(QIODevice::WriteOnly)){
                m_bUpload = true;
                m_iTotal = fileSize;
                m_iRecved = 0;

                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_READY_RESPOND; // <-- 使用新消息类型
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }else{
                // 文件打开失败，通知客户端上传失败
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND; //可以直接复用最终响应
                strcpy(respdu->caData, "服务器创建文件失败，请检查路径或权限");
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            break;
        }

        // 处理下载文件请求
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:{
            char caFileName[32] = {'\0'};
            strncpy(caFileName,pdu->caData,32);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            pPath[pdu->uiMsgLen-1] = '\0';
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            delete[] pPath;
            pPath = NULL;

            QFileInfo fileInfo(strPath);
            qint64 fileSize = fileInfo.size();

            // 1. 先回复客户端，告知文件名和大小
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND; // <-- 发送这个响应
            sprintf(respdu->caData, "%s#%lld", caFileName, fileSize);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            // 2. 准备发送文件
            m_file.setFileName(strPath);
            if (!m_file.open(QIODevice::ReadOnly)) {
                qDebug() << "打开文件失败：" << strPath;
                break; // 文件打不开就不用发送了
            }

            // 3. 启动定时器，开始发送文件
            m_pTimer->start(1); // 立即开始，尽可能快地发送
            break;
        }
        // 处理分享文件请求
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:{
            char caSharerName[32] = {'\0'};
            strncpy(caSharerName, pdu->caData, 32); // 分享者名字在 caData

            // 从 caMsg 中分离文件路径和好友列表
            char* pFilePathEnd = (char*)memchr(pdu->caMsg, '\0', pdu->uiMsgLen);
            if (!pFilePathEnd) {
                // 格式错误，找不到路径结束符
                break;
            }
            int filePathLen = pFilePathEnd - pdu->caMsg;
            QString strFilePath = QString::fromUtf8(pdu->caMsg, filePathLen);

            // 好友列表在文件路径之后
            char* pFriendList = pFilePathEnd + 1;
            int friendListLen = pdu->uiMsgLen - filePathLen - 1;
            int friendCount = friendListLen / 32;

            // 为每个好友转发分享通知,精确计算转发PDU所需大小：仅包含文件路径的长度 + 1个'\0'
            PDU* noticePdu = mkPDU(filePathLen + 1);
            noticePdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTICE;
            // 在通知的caData中放入分享者的名字
            strncpy(noticePdu->caData, caSharerName, 32);
            // 在通知的caMsg中放入文件路径
            strncpy(noticePdu->caMsg,pdu->caMsg,filePathLen+1);

            for(int i = 0;i<friendCount;i++){
                char caFriendName[32] = {'\0'};
                strncpy(caFriendName,pFriendList+i*32,32);
                MyTcpServer::getInstance().resend(caFriendName,noticePdu);
            }
            free(noticePdu);

            PDU* resPdu = mkPDU(0);
            resPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(resPdu->caData,"文件分享请求已发送给好友");
            write((char*)resPdu,resPdu->uiPDULen);
            free(resPdu);
            resPdu = NULL;
            break;

        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTICE_RESPOND:{
            // pdu->caData 存放的是接收者的名字
            // pdu->caMsg 存放的是原始文件的完整路径 (例如 "sharer_name/path/to/file.txt")

            // 构造目标路径：./接收者名字/文件名
            QString strRecvPath = QString("./%1").arg(pdu->caData); // 例如 ./receiver_name
            QString strShareFilePath = QString(pdu->caMsg); // 例如 sharer_name/path/to/file.txt

            // 从完整路径中提取文件名
            int index = strShareFilePath.lastIndexOf('/');
            QString strFileName = strShareFilePath.mid(index + 1);

            // 拼接成最终的目标文件路径
            strRecvPath = strRecvPath + '/' + strFileName;

            // 构造源文件路径
            QString strFullSourcePath = QString("./%1").arg(strShareFilePath);

            QFileInfo fileInfo(strFullSourcePath);
            bool success = false;  // 用于标记操作是否成功
            if (fileInfo.exists() && fileInfo.isFile()) {
               success = QFile::copy(strFullSourcePath, strRecvPath);
            } else if (fileInfo.isDir()) {
                copyDir(strFullSourcePath,strRecvPath);
                success = true;
            }
            // 向接收文件的客户端发送操作结果
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RECEIVE_FILE_RESULT;
            if(success){
                strcpy(respdu->caData,"文件接收成功");
            }else{
                strcpy(respdu->caData,"文件接收失败");
            }
            MyTcpServer::getInstance().resend(pdu->caData,respdu);
            free(respdu);
            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:{
            // 1. 从 caMsg 解析源路径和目标路径
            char* pSrcPath_relative = pdu->caMsg;
            int srcLen = strlen(pSrcPath_relative);
            char* pDestPath_relative = pdu->caMsg + srcLen + 1;

            // 2. 客户端发来的已经是我们需要的完整相对路径，直接使用
            // m_strName 是当前socket关联的用户名
            QString strSrcPath_server = QString(pSrcPath_relative);
            QString strDestPath_server = QString(pDestPath_relative);

            // 3. 从源路径中提取文件名/文件夹名
            QFileInfo srcFileInfo(strSrcPath_server);
            QString strFileName = srcFileInfo.fileName();

            // 4. 构建最终的目标路径 (目标文件夹 + 源文件名)
            QString strFinalDestPath_server = strDestPath_server + "/" + strFileName;

            // 5. 执行移动操作
            bool ret = QFile::rename(strSrcPath_server, strFinalDestPath_server);

            // 6. 回复客户端
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
            if (ret) {
                strcpy(respdu->caData, "移动成功");
            } else {
                strcpy(respdu->caData, "移动失败：请检查路径或权限");
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        default:
            break;
        }
        free(pdu); // 释放 PDU 内存
        pdu = NULL;
    }
    else{
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();

        if(m_iRecved == m_iTotal){
            m_file.close();
            m_bUpload = false;
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(respdu->caData,"文件上传成功");

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }else if(m_iRecved > m_iTotal){
            // 如果接收到的数据超出预期，也视为失败
            m_file.close();
            m_bUpload = false; // 恢复状态

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(respdu->caData, "文件上传失败：传输数据异常");

            // 只在创建了 respdu 后才发送和释放
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

        }
    }
}

// 槽函数：客户端下线处理
void MyTcpSocket::clientOffline()
{
    // 调用数据库操作类处理用户下线
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    // 发射 offline 信号，通知 MyTcpServer 该客户端已下线
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    char *pData = new char[4096]; // 每次发送4KB
    qint64 ret = 0;
    while(true) {
        ret = m_file.read(pData, 4096);
        if (ret > 0) {
            // 成功读取数据，写入socket
            if (this->write(pData, ret) == -1) {
                // 如果写入失败（例如客户端断开），则停止发送
                qDebug() << "发送文件数据失败";
                m_pTimer->stop();
                break;
            }
        } else if (ret == 0) {
            // 文件已读完
            m_pTimer->stop(); // 停止定时器
            m_file.close();   // 关闭文件
            break;
        } else {
            // 读取出错
            qDebug() << "读取文件时出错";
            m_pTimer->stop();
            m_file.close();
            break;
        }
    }
    delete[] pData;
    pData = NULL;
}
