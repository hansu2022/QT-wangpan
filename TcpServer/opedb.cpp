#include "opedb.h"

OpeDB::OpeDB(QObject *parent)
    : QObject{parent}
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setDatabaseName("/home/hansu/wangpan/TcpServer/cloud.db");
    if(m_db.open()){
        QSqlQuery query;
        query.exec("select * from usrInfo");
        while(query.next()){
            QString data = QString("%1,%2,%3").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
            qDebug() << data;
        }
    }else{
        QMessageBox::critical(NULL,"dakaishujukui","faild");
    }
}

OpeDB::~OpeDB()
{
    m_db.close();
}

bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    // 1. 检查输入参数是否为空，这是良好的编程习惯
    if (name == nullptr || pwd == nullptr) {
        qDebug() << "错误：用户名或密码为空。";
        return false;
    }

    // 2. 使用 QSqlQuery 准备预处理语句，防止 SQL 注入
    // 问号 '?' 是占位符，它们会稍后被绑定的数据替换
    QSqlQuery query;
    query.prepare("INSERT INTO usrInfo(name, pwd) VALUES(?, ?)");

    // 3. 绑定值到占位符
    // bindValue() 会自动处理字符串中的特殊字符，避免了手动转义和 SQL 注入
    query.bindValue(0, QString(name));
    query.bindValue(1, QString(pwd));

    // 4. 执行查询，并检查执行结果
    if (!query.exec()) {
        // 如果执行失败，打印详细的错误信息
        qDebug() << "注册失败：";
        return false;
    }

    // 5. 如果执行成功
    qDebug() << "注册成功。";
    return true;
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (name == nullptr || pwd == nullptr) {
        qDebug() << "错误：用户名或密码为空。";
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT * FROM usrInfo WHERE name = ? AND pwd = ? AND online = 0");
    query.addBindValue(QString(name));
    query.addBindValue(QString(pwd));

    if (!query.exec()) {
        qDebug() << "登录查询执行失败:";
        return false;
    }
    if(query.next()) {
        // 登录成功，更新online状态
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE usrInfo SET online = 1 WHERE name = ?");
        updateQuery.addBindValue(QString(name));
        if (!updateQuery.exec()) {
            qDebug() << "更新online状态失败:" ;
        }
        return true;
    } else {
        return false;
    }
}

void OpeDB::handleOffline(const char *name)
{
    if (name == nullptr ) {
        qDebug() << "错误：用户名为空。";
        return;
    }
    QSqlQuery query; // 变量名用 query 可能更符合习惯
    query.prepare("update usrInfo set online=0 where name=?");
    query.addBindValue(QString(name));

    // 检查exec的返回值
    if (!query.exec()) {
        // 如果执行失败，打印错误信息，便于调试
        qDebug() << "handleOffline FAILED:";
    }
}

QStringList OpeDB::handleAllOnline()
{
    QStringList onlineUsers;
    QSqlQuery query;
    query.prepare("SELECT name FROM usrInfo WHERE online = 1");
    if (!query.exec()) {
        qDebug() << "查询在线用户失败:";
        return onlineUsers; // 返回空列表
    }
    while (query.next()) {
        onlineUsers << query.value(0).toString();
    }
    return onlineUsers;
}

int OpeDB::handleSearchUsr(const char *name)
{
    if(name == NULL){
        return -1;
    }
    QSqlQuery query;
    query.prepare("select online from usrInfo where name=?");
    query.addBindValue(QString(name));
    if(!query.exec()){
        qDebug() << "查询用户失败:";
        return -1;
    }
    if(query.next()){
        return query.value(0).toInt();
    }
    return -1;
}

int OpeDB::handleAddFriend(const char *perName, const char *name)
{
    if (NULL == perName || name == NULL) {
        return -1;
    }
    // 判断用户是否在添加自己
    if (strcmp(perName, name) == 0) {
        return 4; // 返回一个新的错误码，例如 4 代表 "不能添加自己"
    }
    QString checkFriendSql = QString("SELECT * FROM friend WHERE (id = (SELECT id FROM usrInfo WHERE name = ?) AND friendId = (SELECT id FROM usrInfo WHERE name = ?)) OR (id = (SELECT id FROM usrInfo WHERE name = ?) AND friendId = (SELECT id FROM usrInfo WHERE name = ?))");
    QSqlQuery query;
    query.prepare(checkFriendSql);
    query.addBindValue(QString(perName));
    query.addBindValue(QString(name));
    query.addBindValue(QString(name));
    query.addBindValue(QString(perName));
    qDebug()<<query.lastQuery();
    // 检查查询是否成功执行
    if (!query.exec()) {
        qDebug() << "好友查询失败:";
        return -1; // 或者其他错误码
    }

    if (query.next()) {
        return 0; // 双方已是好友
    }

    // 如果不是好友，则查询被添加用户的在线状态
    QString checkUserSql = QString("SELECT online FROM usrInfo WHERE name = ?");
    query.prepare(checkUserSql);
    query.addBindValue(perName);

    // 检查查询是否成功执行
    if (!query.exec()) {
        qDebug() << "用户信息查询失败:";
        return -1; // 或者其他错误码
    }

    if (query.next()) {
        int res = query.value(0).toInt();
        if (1 == res) {
            return 1; // 在线
        } else if (0 == res) {
            return 2; // 不在线
        }
    }

    // 如果查询没有结果，则说明用户不存在
    return 3;
}

void OpeDB::handleAddFriendAgree(const char *name, const char *pername)
{
    if (NULL == name || pername == NULL) {
        return;
    }

    QString insertFriendSql = QString("INSERT INTO friend (id, friendId) VALUES ((SELECT id FROM usrInfo WHERE name = ?), (SELECT id FROM usrInfo WHERE name = ?))");
    QSqlQuery query;
    query.prepare(insertFriendSql);
    query.addBindValue(QString(name));
    query.addBindValue(QString(pername));

    // 检查插入是否成功执行
    if (!query.exec()) {
        qDebug() << "添加好友失败:";
    } else {
        qDebug() << "好友添加成功:" << name << "<->" << pername;
    }
}
