#include "tcpclient.h"
#include <QApplication>
#include "sharefile.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TcpClient::getInstance().show();



    return a.exec();
}
