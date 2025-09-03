#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <memory>
#include <cstring>

namespace protocol{ // 使用命名空间避免全局污染

// 1. 使用 const char* 替代宏定义
constexpr const char* REGIST_OK = "regist ok";
constexpr const char* REGIST_FAILED = "regist failed : name exist";

constexpr const char* LOGIN_OK = "LOGIN ok";
constexpr const char* LOGIN_FAILED = "LOGIN failed : name error or pwd error or relogin ";

constexpr const char* SEARCH_USR_NO = "no this usr";
constexpr const char* SEARCH_USR_YES = "usr is online";
constexpr const char* SEARCH_USR_OFFLINE = "usr is offline";

constexpr const char* UNKONW_ERROR = "unkonw error";
constexpr const char* EXISTED_FRIEND = "existed friend";
constexpr const char* ADD_FRIEND_OFFLINE = "usr offline";
constexpr const char* ADD_FRIEND_NO_EXIST = "usr no exist";

typedef unsigned int uint;

enum class MsgType : unsigned int
{
    MIN = 0,
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

    ENUM_MSG_TYPE_DEL_ITEM_REQUEST, // 删除文件/文件夹请求
    ENUM_MSG_TYPE_DEL_ITEM_RESPOND, // 删除文件/文件夹回复

    ENUM_MSG_TYPE_RENAME_DIR_REQUEST, // 重命名文件夹请求
    ENUM_MSG_TYPE_RENAME_DIR_RESPOND, // 重命名文件夹回复

    ENUM_MSG_TYPE_ENTRY_DIR_REQUEST, // 进入文件夹请求
    ENUM_MSG_TYPE_ENTRY_DIR_RESPOND, // 进入文件夹回复

    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST, // 上传文件请求
    ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND, // 上传文件回复
    ENUM_MSG_TYPE_UPLOAD_FILE_READY_RESPOND, // 准备上传文件回复

    ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST, // 下载文件请求
    ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND, // 下载文件回复

    ENUM_MSG_TYPE_SHARE_FILE_REQUEST, // 分享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_RESPOND, // 分享文件回复
    ENUM_MSG_TYPE_SHARE_FILE_NOTICE, // 分享文件通知
    ENUM_MSG_TYPE_SHARE_FILE_NOTICE_RESPOND, // 分享文件通知回复
    ENUM_MSG_TYPE_RECEIVE_FILE_RESULT, // 接收文件结果的回复

    ENUM_MSG_TYPE_MOVE_FILE_REQUEST, // 移动文件请求
    ENUM_MSG_TYPE_MOVE_FILE_RESPOND, // 移动文件回复
    MAX = 0x00ffffff,
};

struct FileInfo{
    std::string sFileName;
    int iFileType;
};


struct PDU{
    uint uiPDULen;   // 总协议数据单元长度
    MsgType uiMsgType;  // 消息类型,使用强类型枚举
    char caData[64]; // 固定数据
    std::vector<char> vMsg;// 使用vector<char>管理可变长度的消息内容

    // 添加构造函数，方便初始化
    PDU() : uiPDULen(0), uiMsgType(MsgType::MIN), vMsg() {
        memset(caData, 0, sizeof(caData));
    }
    // 更新总长度的辅助函数
    void updatePDULen() {
        uiPDULen = sizeof(PDU) - sizeof(std::vector<char>) + vMsg.size();
    }
};

// 4. 创建PDU的工厂函数，返回智能指针
// 我们不再需要手动管理内存，因此函数返回一个 std::unique_ptr
// 参数是消息类型和消息体的大小
std::unique_ptr<PDU> make_pdu(MsgType type, size_t msg_len = 0);

}
#endif // PROTOCOL_H
