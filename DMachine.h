#ifndef DMACHINE_H
#define DMACHINE_H

#include "tablemodel.h"
#include <QMainWindow>
#include <QMap>

namespace Ui {
class MainWindow;
}

class MysqlQuery;

class QTimer;
class VlcInstance;
class VlcMedia;
class VlcMediaPlayer;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *);

private slots:
    void on_pushButton_addLogo_clicked();

    void on_pushButton_addVideo_clicked();

    void on_pushButton_addImage_clicked();

    void on_pushButton_deleLogo_clicked();

    void on_pushButton_deleVideo_clicked();

    void on_pushButton_upload_clicked();

//    void on_pushButton_play_clicked(bool checked);

private:
    void initWidget();
    void initTableView();
    void readAndSetStyleSheet();

    QString fileName(const QString &path);
    QString fileSize(const QString &path);
    QString imageExists(const QString &retStr, const QString &path);

    bool isEmpty();

    void insertSql(QMap<QString, QString> &value);
    void initValue();
    QString downImage(const QString &url);

    void writeLog(const QString &error);

    int isExistsFile(const QString &name);


private slots:
    void moveUp(const int &row);
    void moveDown(const int &row);
    void deleteCurrentRow(const int &row);
    void replace(const int &row);

    void timeOver();

    void on_pushButton_play_clicked();

    void on_pushButton_stop_clicked(bool checked);

    bool on_lineEdit_welcome_editingFinished();

private:
    Ui::MainWindow *ui;

//    QMap<QString, QString> json;
    QList< QStringList > rowList;
    QStringList pathList;
    TableModel *model;
    QString retSize;
    QTimer *timer;
    bool startValue;
    QString logoName, videPath;
    QString exePath, logPath;

    MysqlQuery    *_sql;


    VlcInstance *_instance;
    VlcMedia *_media;
    VlcMediaPlayer *_player;

};

#endif // DMACHINE_H
