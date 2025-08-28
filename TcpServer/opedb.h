#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QStringList>
class OpeDB : public QObject
{
    Q_OBJECT
public:
    explicit OpeDB(QObject *parent = nullptr);
    static OpeDB& getInstance();
    void init();
    ~OpeDB();

    bool handleRegist(const char *name,const char *pwd);
    bool handleLogin(const char *name,const char *pwd);
    void handleOffline(const char *name);
    QStringList handleAllOnline();
    int handleSearchUsr(const char *name);
    int handleAddFriend(const char *pername,const char *name);
void handleAddFriendAgree(const char *name, const char *pername);
    QStringList handleFlushFriend(const char *name);
    bool handleDelFriend(const char *name, const char *pername);
signals:

public slots:
private:
    QSqlDatabase m_db;
};

#endif // OPEDB_H
