#include "protocol.h"
#include <iostream>

std::unique_ptr<PDU> make_pdu(MsgType type,size_t msg_len){
    // 使用 std::make_unique 创建对象，这是创建 unique_ptr 的首选方式
    // 它安全且高效，能处理构造过程中可能出现的异常
    auto pdu = std::make_unique<PDU>();

    if(msg_len > 0){
        pdu->vMsg.resize(msg_len);
    }
    pdu->uiMsgType = type;
    // 更新PDU的总长度信息
    pdu->uiPDULen = pdu->calculatePDULen();
    return pdu;
}
