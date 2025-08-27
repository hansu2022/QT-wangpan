/********************************************************************************
** Form generated from reading UI file 'tcpclient.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TCPCLIENT_H
#define UI_TCPCLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TcpClient
{
public:
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QLabel *name_lan;
    QLabel *pwd_lab;
    QLineEdit *pwd_le;
    QPushButton *login_pb;
    QLineEdit *name_le;
    QHBoxLayout *horizontalLayout;
    QPushButton *cancel_pb;
    QSpacerItem *horizontalSpacer;
    QPushButton *regist_pb;

    void setupUi(QWidget *TcpClient)
    {
        if (TcpClient->objectName().isEmpty())
            TcpClient->setObjectName("TcpClient");
        TcpClient->resize(320, 176);
        horizontalLayout_2 = new QHBoxLayout(TcpClient);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        name_lan = new QLabel(TcpClient);
        name_lan->setObjectName("name_lan");
        QFont font;
        font.setPointSize(24);
        name_lan->setFont(font);

        gridLayout->addWidget(name_lan, 0, 0, 1, 1);

        pwd_lab = new QLabel(TcpClient);
        pwd_lab->setObjectName("pwd_lab");
        pwd_lab->setFont(font);

        gridLayout->addWidget(pwd_lab, 1, 0, 1, 1);

        pwd_le = new QLineEdit(TcpClient);
        pwd_le->setObjectName("pwd_le");
        pwd_le->setEchoMode(QLineEdit::EchoMode::Password);

        gridLayout->addWidget(pwd_le, 1, 1, 1, 2);

        login_pb = new QPushButton(TcpClient);
        login_pb->setObjectName("login_pb");
        login_pb->setFont(font);

        gridLayout->addWidget(login_pb, 2, 0, 1, 3);

        name_le = new QLineEdit(TcpClient);
        name_le->setObjectName("name_le");

        gridLayout->addWidget(name_le, 0, 1, 1, 2);


        verticalLayout->addLayout(gridLayout);


        verticalLayout_2->addLayout(verticalLayout);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        cancel_pb = new QPushButton(TcpClient);
        cancel_pb->setObjectName("cancel_pb");
        QFont font1;
        font1.setPointSize(16);
        cancel_pb->setFont(font1);

        horizontalLayout->addWidget(cancel_pb);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        regist_pb = new QPushButton(TcpClient);
        regist_pb->setObjectName("regist_pb");
        regist_pb->setFont(font1);

        horizontalLayout->addWidget(regist_pb);


        verticalLayout_2->addLayout(horizontalLayout);


        horizontalLayout_2->addLayout(verticalLayout_2);


        retranslateUi(TcpClient);

        QMetaObject::connectSlotsByName(TcpClient);
    } // setupUi

    void retranslateUi(QWidget *TcpClient)
    {
        TcpClient->setWindowTitle(QCoreApplication::translate("TcpClient", "TcpClient", nullptr));
        name_lan->setText(QCoreApplication::translate("TcpClient", "\347\224\250\346\210\267\345\220\215", nullptr));
        pwd_lab->setText(QCoreApplication::translate("TcpClient", "\345\257\206   \347\240\201", nullptr));
        login_pb->setText(QCoreApplication::translate("TcpClient", "\347\231\273\345\275\225", nullptr));
        cancel_pb->setText(QCoreApplication::translate("TcpClient", "\346\263\250\351\224\200", nullptr));
        regist_pb->setText(QCoreApplication::translate("TcpClient", "\346\263\250\345\206\214", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TcpClient: public Ui_TcpClient {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TCPCLIENT_H
