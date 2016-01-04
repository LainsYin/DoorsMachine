#ifndef MYSQLQUERY_H
#define MYSQLQUERY_H

#include <QSqlDatabase>
#include <QString>
class MediaList;
class QSettings;

class MysqlQuery
{
public:
    MysqlQuery();
    ~MysqlQuery();

public:
    void readConfigFile();
    bool openMysql(const QString databaseName="yiqiding_ktv");
    void closeMysql(const QString databaseName="yiqiding_ktv");

    bool queryConfigResource(QString &detail);
    bool updateConfigResource(const QString &detail);

public:
    QString hostName;
    QString port;
    QString databaseName;
    QString userName;
    QString password;
    QString dataBase;

private:

public:
    QSqlDatabase db;
    QSettings *initConfig;
    QSqlQuery *_query;
};
#endif // MYSQLQUERY_H
