#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define REGIST_OK "regist ok"
#define REGIST_FAILED "regist failed : name existed"

#define LOGIN_OK "login ok"
#define LOGIN_FAILED "LOGIN failed : name error or pwd error or relogin "

#define SEARCH_USR_NO "no this usr"
#define SEARCH_USR_YES "usr is online"
#define SEARCH_USR_OFFLINE "usr is offline"

#define UNKONW_ERROR "unkonw error"
#define EXISTED_FRIEND "existed friend"
#define ADD_FRIEND_OFFLINE "usr offline"
#define ADD_FRIEND_NO_EXIST "usr no exist"

typedef unsigned int uint;

enum ENUM_MSG_TYPE
{
    ENUM_MSG_TYPE_MIN = 0,
    ENUM_MSG_TYPE_REGIST_REQUEST, //注册请求
    ENUM_MSG_TYPE_REGIST_RESPOND, //注册回复

    ENUM_MSG_TYPE_LOGIN_REQUEST, //登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND, //登录回复

    ENUM_MSG_TYPE_ALL_ONLINE_REQUEST, //请求在线用户
    ENUM_MSG_TYPE_ALL_ONLINE_RESPOND, //在线用户回复

    ENUM_MSG_TYPE_SEARCH_USR_REQUEST, //查找用户请求
    ENUM_MSG_TYPE_SEARCH_USR_RESPOND, //查找用户回复

    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,// 添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,// 添加好友回复

    ENUM_MSG_TYPE_ADD_FRIEND_AGREE,//同意添加好友
    ENUM_MSG_TYPE_ADD_FRIEND_REFUSE,//拒绝添加好友

    ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST,//刷新好友请求
    ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND,//刷新好友回复

    ENUM_MSG_TYPE_DEL_FRIEND_REQUEST,//删除好友请求
    ENUM_MSG_TYPE_DEL_FRIEND_RESPOND,//删除好友回复
    ENUM_MSG_TYPE_DEL_FRIEND_NOTICE,//删除好友通知

    ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST,//私聊请求
    ENUM_MSG_TYPE_PRIVATE_CHAT_RESPOND,//私聊回复

    ENUM_MSG_TYPE_GROUP_CHAT_REQUEST,//群发在线好友请求
    ENUM_MSG_TYPE_GROUP_CHAT_RESPOND,//群发在线好友回复

    ENUM_MSG_TYPE_CREATE_DIR_REQUEST, // 创建文件夹请求
    ENUM_MSG_TYPE_CREATE_DIR_RESPOND, // 创建文件夹回复

    ENUM_MSG_TYPE_FLUSH_FILE_REQUEST, //刷新文件请求
    ENUM_MSG_TYPE_FLUSH_FILE_RESPOND, //刷新文件回复

    ENUM_MSG_TYPE_DEL_DIR_REQUEST, // 删除文件夹请求
    ENUM_MSG_TYPE_DEL_DIR_RESPOND, // 删除文件夹回复

    ENUM_MSG_TYPE_RENAME_DIR_REQUEST, // 重命名文件夹请求
    ENUM_MSG_TYPE_RENAME_DIR_RESPOND, // 重命名文件夹回复
    ENUM_MSG_TYPE_MAX = 0x00ffffff,
};

struct FileInfo{
    char caFileName[32];
    int iFileType;
};


struct PDU{
    uint uiPDULen;   // 总协议数据单元长度
    uint uiMsgType;  // 消息类型
    char caData[64]; // 固定数据
    uint uiMsgLen;   // 可变长度数据部分（caMsg）的字节数
    char caMsg[];    // 柔性数组，存放可变长度的消息内容
};

PDU *mkPDU(uint uiMsgLen);

#endif // PROTOCOL_H
