#include "mysqlquery.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSettings>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlField>
#include <QDateTime>

#include <QDir>
#include <QFile>
#include <QCoreApplication>

#include <QDebug>
#include <QTime>
#include <QMessageBox>
#define BLACK_YES 1
#define BLACK_NO  0

MysqlQuery::MysqlQuery()
{
    initConfig = new QSettings("config.ini", QSettings::IniFormat);
    initConfig->setIniCodec("UTF-8");

    _query = NULL;
    readConfigFile();   
}

MysqlQuery::~MysqlQuery()
{

}

void MysqlQuery::readConfigFile()
{
    hostName = initConfig->value("SQL/hostname").toString();
    port = initConfig->value("SQL/port").toString();
    userName = initConfig->value("SQL/username").toString();
    password = initConfig->value("SQL/password").toString();
    dataBase = initConfig->value("SQL/database").toString();
}

bool MysqlQuery::openMysql(const QString databaseName)
{    
    if (QSqlDatabase::contains(dataBase)) //"yqcdb"
        db = QSqlDatabase::database(dataBase);
    else
        db = QSqlDatabase::addDatabase("QMYSQL");

    db.setHostName(hostName);
    db.setPort(port.toInt());
    db.setDatabaseName(dataBase);
    db.setUserName(userName);
    db.setPassword(password);

    if (!db.open()){        
        QString error =  QString("数据库登录失败！\n%1").arg(db.lastError().text());
        QMessageBox::information(NULL, "错误提示", error, QMessageBox::Ok);
//        exit(0);
        return false;
    }else{
        qDebug() << "connection successful." << "  databaseName : " << databaseName;
    }

    _query = new QSqlQuery(db);
    return true;
}

void MysqlQuery::closeMysql(const QString databaseName)
{
    qDebug() << "databaseName : " << databaseName;
    if (db.isOpen()){

        QString connect = QSqlDatabase::database().connectionName();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connect);
    }
}

bool MysqlQuery::queryConfigResource(QString &detail)
{
    QString sqlStr = QString("select * from config_resource where name = 'portinfo';");
    QSqlQuery query;
    if(query.exec(sqlStr)){
        query.next();
        detail = query.value("detail").toString();
        return true;
    }

    return false;
}

bool MysqlQuery::updateConfigResource(const QString &detail)
{
    QString sqlStr = QString("select * from config_resource where name = 'portinfo';");
    QSqlQuery query;
    if(query.exec(sqlStr))
    {
        if(query.next()){
            int id = query.value("cid").toInt();
//            QString name = query.value("name").toString();


            sqlStr = QString("UPDATE `config_resource` SET `detail` = '%1' "
                             "WHERE cid = %2 and name = '%3';")
                    .arg(detail)
                    .arg(id)
                    .arg("portinfo");
            if(query.exec(sqlStr))
                return true;

        }
        else
        {
            sqlStr = QString("INSERT IGNORE INTO `config_resource`(`name`, `value`, `detail`) "
                             "VALUES('%1', NULL, '%2') ;").arg("portinfo").arg(detail);

            qDebug() << " sql str : " << sqlStr;
            if(query.exec(sqlStr))
                return true;
        }
    }

    return false;
}

//bool MysqlQuery::querySql(const QString &queryStr, QSqlQuery &query)
//{
//    QSqlQuery _query; //(db);
//   if(_query.exec(queryStr)){
//        query = _query;
//        return true;
//    }
//    else
//        return false;
//}

