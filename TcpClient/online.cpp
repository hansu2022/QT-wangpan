#include "online.h"
#include "ui_online.h"
#include "tcpclient.h"
#include <QMessageBox>
Online::Online(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUsr(PDU *pdu)
{
    if(pdu == NULL){
        return;
    }
    ui->Online_lw->clear();
    uint uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    for(uint i = 0;i<uiSize;i++){
        memcpy(caTmp,(char*)(pdu->caMsg)+i*32,32);
        ui->Online_lw->addItem(caTmp);
    }
}

void Online::on_addFriend_clicked()
{
    QListWidgetItem *pItem = ui->Online_lw->currentItem();
    QString strPerUsrName = pItem->text();
    QString strLoginName = TcpClient::getInstance().getLoginName();
    if (strPerUsrName == strLoginName) {
        QMessageBox::warning(this, "添加好友", "不能添加自己为好友。");
        return; // 如果是自己，则直接返回，不发送请求
    }
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    strncpy(pdu->caData,strPerUsrName.toStdString().c_str(),32);
    strncpy(pdu->caData+32,strLoginName.toStdString().c_str(),32);
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}




