/********************************************************************************
** Form generated from reading UI file 'online.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ONLINE_H
#define UI_ONLINE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Online
{
public:
    QHBoxLayout *horizontalLayout_2;
    QHBoxLayout *horizontalLayout;
    QListWidget *Online_lw;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QPushButton *addFriend;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *Online)
    {
        if (Online->objectName().isEmpty())
            Online->setObjectName("Online");
        Online->resize(385, 352);
        horizontalLayout_2 = new QHBoxLayout(Online);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Online_lw = new QListWidget(Online);
        new QListWidgetItem(Online_lw);
        new QListWidgetItem(Online_lw);
        new QListWidgetItem(Online_lw);
        Online_lw->setObjectName("Online_lw");
        QFont font;
        font.setPointSize(16);
        Online_lw->setFont(font);

        horizontalLayout->addWidget(Online_lw);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        addFriend = new QPushButton(Online);
        addFriend->setObjectName("addFriend");
        QFont font1;
        font1.setPointSize(24);
        addFriend->setFont(font1);

        verticalLayout->addWidget(addFriend);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);


        horizontalLayout->addLayout(verticalLayout);


        horizontalLayout_2->addLayout(horizontalLayout);


        retranslateUi(Online);

        QMetaObject::connectSlotsByName(Online);
    } // setupUi

    void retranslateUi(QWidget *Online)
    {
        Online->setWindowTitle(QCoreApplication::translate("Online", "Form", nullptr));

        const bool __sortingEnabled = Online_lw->isSortingEnabled();
        Online_lw->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = Online_lw->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("Online", "1234", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = Online_lw->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("Online", "1234", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = Online_lw->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("Online", "124", nullptr));
        Online_lw->setSortingEnabled(__sortingEnabled);

        addFriend->setText(QCoreApplication::translate("Online", "\345\212\240\345\245\275\345\217\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Online: public Ui_Online {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ONLINE_H
