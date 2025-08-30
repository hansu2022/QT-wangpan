#include "opewidget.h"

OpeWidget::OpeWidget(QWidget *parent)
    : QWidget{parent}
{
    m_pListW = new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("文件");
    m_pFriend = new Friend;
    m_pBook = new Book;
    m_pSW = new QStackedWidget;
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pBook);

    //实例化显示用户名的标签
    m_pUsrNameLbl = new QLabel(this);
    m_pUsrNameLbl->setAlignment(Qt::AlignCenter); // 设置文本居中

    //创建新的左侧垂直布局，用于放置用户名标签和列表
    QVBoxLayout *pLeftVBL = new QVBoxLayout;
    pLeftVBL->addWidget(m_pUsrNameLbl); // 用户名标签在上方
    pLeftVBL->addWidget(m_pListW);      // 功能列表在下方
    pLeftVBL->setSpacing(10); // 可以设置一些间距

    // 创建主水平布局
    QHBoxLayout *pMainHBL = new QHBoxLayout;
    pMainHBL->addLayout(pLeftVBL); // 将左侧布局添加到主布局
    pMainHBL->addWidget(m_pSW);    // 将右侧堆栈窗口添加到主布局
    setLayout(pMainHBL);

    connect(m_pListW,SIGNAL(currentRowChanged(int)),m_pSW,SLOT(setCurrentIndex(int)));

}

OpeWidget &OpeWidget::getInstance()
{
    static OpeWidget instance;
    return instance;
}



Friend *OpeWidget::getFriend()
{
    return m_pFriend;
}

Book *OpeWidget::getBook()
{
    return m_pBook;
}

// 实现设置用户名的方法
void OpeWidget::setUsrName(const QString &name)
{
    if (m_pUsrNameLbl != NULL) {
        m_pUsrNameLbl->setText(QString("欢迎, %1").arg(name));
    }
}
