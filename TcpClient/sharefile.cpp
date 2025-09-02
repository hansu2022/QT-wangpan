#include "sharefile.h"
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QButtonGroup>
#include <QListWidget>

// ShareFile 类的构造函数
ShareFile::ShareFile(QWidget *parent)
    : QWidget{parent}
{
    // 创建“全选”和“取消选择”按钮
    m_pSelectAllPB = new QPushButton(tr("全选"));
    m_pCancelSelectPB = new QPushButton(tr("取消选择"));

    // 创建“确定”和“取消”按钮
    m_pOKPB = new QPushButton(tr("确定"));
    m_pCancelPB = new QPushButton(tr("取消"));

    // 创建滚动区域，用于显示好友列表
    m_pSA = new QScrollArea;
    // 创建一个QWidget作为滚动区域的内部容器
    m_pFriendW = new QWidget;
    // 为内部容器设置垂直布局，用于放置好友复选框
    m_pFriendWVBL = new QVBoxLayout(m_pFriendW);
    // 创建一个按钮组，用于管理好友复选框，以便批量操作
    m_pButtonGroup = new QButtonGroup(m_pFriendW);
    // 允许同时选中多个复选框（非互斥）
    m_pButtonGroup->setExclusive(false);

    // 将好友面板放入滚动区域
    m_pSA->setWidget(m_pFriendW);
    // 允许滚动区域的控件随之调整大小
    m_pSA->setWidgetResizable(true);

    // 创建顶部的水平布局，放置“全选”和“取消选择”按钮
    QHBoxLayout *pTopHL = new QHBoxLayout;
    pTopHL->addWidget(m_pSelectAllPB);
    pTopHL->addWidget(m_pCancelSelectPB);
    pTopHL->addStretch(); // 添加伸缩空间，将按钮推向左侧

    // 创建底部的水平布局，放置“确定”和“取消”按钮
    QHBoxLayout *pDownHBL = new QHBoxLayout;
    pDownHBL->addWidget(m_pOKPB);
    pDownHBL->addWidget(m_pCancelPB);

    // 创建主垂直布局，按顺序添加顶部布局、滚动区域和底部布局
    QVBoxLayout *pMainVL = new QVBoxLayout;
    pMainVL->addLayout(pTopHL);
    pMainVL->addWidget(m_pSA);
    pMainVL->addLayout(pDownHBL);
    // 将主布局设置给当前的ShareFile窗口
    setLayout(pMainVL);
}

// 获取 ShareFile 单例对象的静态函数
ShareFile &ShareFile::getInstance()
{
    // 使用局部静态变量实现单例模式
    static ShareFile instance;
    return instance;
}

// 根据好友列表更新 UI，动态创建复选框
void ShareFile::updateFriend(QListWidget *pFriendList)
{
    // 如果传入的好友列表为空，则直接返回
    if(pFriendList == NULL){
        return;
    }

    // --- 清除旧的好友复选框 ---
    QAbstractButton *tmp = NULL;
    // 获取按钮组中所有已有的复选框
    QList<QAbstractButton*>preFriendList = m_pButtonGroup->buttons();
    // 遍历并删除所有旧的复选框
    for(int i = 0;i<preFriendList.size();i++){
        tmp = preFriendList[i];
        m_pFriendWVBL->removeWidget(preFriendList[i]); // 从布局中移除
        m_pButtonGroup->removeButton(preFriendList[i]); // 从按钮组中移除
        preFriendList.removeOne(tmp); // 从临时列表中移除
        delete tmp; // 释放内存
        tmp = NULL;
    }

    // --- 创建新的好友复选框 ---
    QCheckBox *pCB = NULL;
    // 遍历传入的好友列表
    for(int i = 0;i<pFriendList->count();i++){
        // 获取好友信息字符串
        QString strFriendInfo = pFriendList->item(i)->text();
        // 为每个好友创建一个新的复选框
        pCB = new QCheckBox(strFriendInfo);
        m_pFriendWVBL->addWidget(pCB); // 将复选框添加到垂直布局
        m_pButtonGroup->addButton(pCB); // 将复选框添加到按钮组
        // 判断好友信息是否包含“(在线)”
        if(!strFriendInfo.contains("(在线)")){
            pCB->setEnabled(false); // 如果好友不在线，则禁用该复选框（灰色不可点击）
        }
    }
    // 重新设置布局，确保所有新添加的控件都正确显示
    m_pFriendW->setLayout(m_pFriendWVBL);
}
