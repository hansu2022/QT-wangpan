#ifndef OPEWIDGET_H
#define OPEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "friend.h"
#include "book.h"
#include <QStackedWidget>
#include <QLabel>
class OpeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OpeWidget(QWidget *parent = nullptr);
    static OpeWidget &getInstance();
    Friend *getFriend();
    Book *getBook();
    void setUsrName(const QString &name);
signals:


private:
    QListWidget *m_pListW;
    Friend *m_pFriend;
    Book *m_pBook;

    QStackedWidget *m_pSW;
    QLabel *m_pUsrNameLbl; // 显示当前用户
};

#endif // OPEWIDGET_H
