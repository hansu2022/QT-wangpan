#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef unsigned int uint;

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
    ENUM_MSG_TYPE_MAX = 0x00ffffff,
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
