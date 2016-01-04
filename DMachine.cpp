#include "DMachine.h"
#include "ui_DMachine.h"

#include "tablemodel.h"
#include "yqcdelegate.h"
#include "curlupload.h"
#include "mysqlquery.h"
#include <QTimer>
#include <QImage>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QSettings>
#include <QProcessEnvironment>

#include <Common.h>
#include "Instance.h"
#include "Media.h"
#include "MediaPlayer.h"

#include <QPicture>
#include <QDebug>
#include <QTextCodec>

#define DATABASE_NAME "yiqiding_ktv" //yiqiding_ktv
#define PROGRESS_TIME 500

#define STARTSTR "播放"
#define STOPSTR  "暂停"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initWidget();
    initTableView();
    readAndSetStyleSheet();
    this->setWindowIcon(QIcon(":/Resources/img/logo.ico"));

    timer = NULL;
    retSize = "";
    startValue = true;
    timer = new QTimer(this);

    _sql = new MysqlQuery();
    _sql->openMysql(DATABASE_NAME);

    exePath = QCoreApplication::applicationDirPath();
    QString imagePath = exePath;
    logPath = exePath;
    logPath.append("/yqclog.log");
    imagePath.append("/image/" );
    QDir dir(imagePath);
    if(!dir.exists())
        dir.mkpath(imagePath);

    initValue();
//    connect(timer, &QTimer::timeout, this, &MainWindow::timeOver);

    _instance = new VlcInstance(VlcCommon::args(), this);
    _player = new VlcMediaPlayer(_instance);
    _player->setVideoWidget(ui->widget_mp4Video);

    ui->widget_mp4Video->setMediaPlayer(_player);
    ui->widget_mp4Volume->setMediaPlayer(_player);
    ui->widget_mp4Volume->setVolume(50);
    ui->widget_mp4Seek->setMediaPlayer(_player);
    ui->pushButton_stop->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

}

void MainWindow::closeEvent(QCloseEvent *)
{
    if(_player)
        _player->stop();
}

void MainWindow::initWidget()
{
    ui->label_ktvLogo->setText("KTV Logo：");
    ui->pushButton_addLogo->setText("添加图片");
    ui->pushButton_deleLogo->setText("删除");

    ui->label_videoPlay->setText("选择播放形式：");
    ui->radioButton_image->setText("图片连播");
    ui->radioButton_video->setText("视频播放");
    ui->label_video->setText("视频：");

    ui->pushButton_addVideo->setText("添加视频");
    ui->pushButton_deleVideo->setText("删除");
    ui->pushButton_upload->setText("上传");

    ui->label_image->setText("连播图片：");
    ui->pushButton_addImage->setText("新增图片");

    ui->label_welcome->setText("欢迎语：");
    ui->lineEdit_welcome->setPlaceholderText("欢迎光临xxxxxx(KTV点名)");
    ui->lineEdit_logo->setPlaceholderText("LOGO图片的绝对路径");
    ui->lineEdit_video->setPlaceholderText("视频文件的绝对路径");

    ui->radioButton_image->setChecked(true);

    ui->label_ktvLogo->setMinimumWidth(100);
    ui->label_image->setMinimumWidth(100);
    ui->label_video->setMinimumWidth(100);
    ui->lineEdit_welcome->setMinimumWidth(600);
    ui->pushButton_upload->setMinimumSize(120, 50);

    ui->pushButton_addImage->setMinimumSize(90, 36);
    ui->pushButton_addLogo->setMinimumSize(90, 36);
    ui->pushButton_addVideo->setMinimumSize(90, 36);
    ui->pushButton_deleLogo->setMinimumSize(60, 36);
    ui->pushButton_deleVideo->setMinimumSize(60, 36);
    ui->pushButton_play->setMinimumSize(60, 30);
    ui->pushButton_stop->setMinimumSize(60, 30);

    ui->lineEdit_logo->setMinimumHeight(36);
    ui->lineEdit_video->setMinimumHeight(36);
    ui->lineEdit_welcome->setMinimumHeight(36);

    ui->label_ktvlogo_preview->clear();
    ui->progressBar->setHidden(true);

    ui->label_ktvlogo_preview->setScaledContents(true);
    ui->label_ktvlogo_preview->setAlignment(Qt::AlignCenter);
}

void MainWindow::initTableView()
{
    model = new TableModel(this);
    ui->widget_tableView->setModel(model);
    ui->widget_tableView->setAlternatingRowColors(true);

    ui->widget_tableView->setItemDelegate(new NoFocusDelegate());
    ui->widget_tableView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->widget_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->widget_tableView->horizontalHeader()->setHighlightSections(false);
    ui->widget_tableView->verticalHeader()->setVisible(false);
    ui->widget_tableView->setShowGrid(false);
    ui->widget_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->widget_tableView->verticalHeader()->setDefaultSectionSize(70);
    QHeaderView *headerView = ui->widget_tableView->horizontalHeader();
//    headerView->setStretchLastSection(true);  ////最后一行适应空余部分
    headerView->setSectionResizeMode(QHeaderView::Stretch); //平均列宽

    ui->widget_tableView->show();
    ui->widget_tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    QStringList headerList;
    headerList << "编号顺序" << "文件名" << "缩略图" << "图片尺寸" << "图片大小" << "调整顺序" << "修改上传" << "删除";

    ImageDelegate  *m_imageDelegate = new ImageDelegate(this);
    ButtonDelegate *modifyDelegate = new ButtonDelegate(this);
    ButtonDelegate *deleteDelegate = new ButtonDelegate(this);
    MoveDelegate *m_moveDelegate = new MoveDelegate(this);;
    deleteDelegate->setTextColor(254, 107, 107);
    deleteDelegate->setButtonText("删除");
    modifyDelegate->setButtonText("替换");

    ui->widget_tableView->setItemDelegateForColumn(2, m_imageDelegate);
    ui->widget_tableView->setItemDelegateForColumn(5, m_moveDelegate);
    ui->widget_tableView->setItemDelegateForColumn(6, modifyDelegate);
    ui->widget_tableView->setItemDelegateForColumn(7, deleteDelegate);

    connect(m_moveDelegate, &MoveDelegate::left, this, &MainWindow::moveUp);
    connect(m_moveDelegate, &MoveDelegate::right, this, &MainWindow::moveDown);
    connect(deleteDelegate, &ButtonDelegate::currentRow, this, &MainWindow::deleteCurrentRow);
    connect(modifyDelegate, &ButtonDelegate::currentRow, this, &MainWindow::replace);

    model->setHorizontalHeaderList(headerList);
    model->refrushModel();
}

void MainWindow::readAndSetStyleSheet()
{
    QFile qss(":/Resources/title.qss");
    qss.open(QFile::ReadOnly);
    this->setStyleSheet(qss.readAll());
    qss.close();

    ui->pushButton_addLogo->setObjectName("Button");
    ui->pushButton_addImage->setObjectName("Button");
    ui->pushButton_addVideo->setObjectName("Button");
    ui->pushButton_deleLogo->setObjectName("Button");
    ui->pushButton_deleVideo->setObjectName("Button");
    ui->pushButton_upload->setObjectName("Button");

    ui->widget_video->setObjectName("CenterWidget");
}

QString MainWindow::fileName(const QString &path)
{
   QStringList lists = path.split("/");
   QString name = lists.last();
   return name.left(name.indexOf("."));
}

QString MainWindow::fileSize(const QString &path)
{
    QFile file(path);
    qint64 size = file.size();
    QString retSize;
    if(size/1024/1024 < 1){
        retSize.append(QString::number(size/1024));
        retSize.append(".");
        retSize.append(QString::number(size%1024));
        retSize.append("KB");
    }else{
        retSize.append(QString::number(size/1024/1024));
        retSize.append(".");
        retSize.append(QString::number(size%1024/1024));
        retSize.append("MB");
    }

    return retSize;
}

QString MainWindow::imageExists(const QString &retStr, const QString &path)
{
    QString url;
    if(retStr.compare("1") == 0)
    {
        url.append("/image/");
        QStringList tempList = path.split("/");
        url.append(tempList.last());
    }else if(retStr.compare("0") == 0){
        ///写log文件
        QString error;
        error.append(QString("上传图片 %1失败，返回%2").arg(path).arg(retStr));
        writeLog(error);
        url = "";
    }else{
        url.append("/");
        url.append(retStr);
    }


    return url;
}

bool MainWindow::isEmpty()
{
    if(ui->radioButton_image->isChecked()){

        if(rowList.isEmpty())
        {
            QMessageBox::information(this, "提示", "必须要有轮播图片！");
            return true;
        }

    }else{

        if(ui->lineEdit_video->text().isEmpty())
        {
            QMessageBox::information(this, "提示", "必须要有视频文件！");
            return true;
        }
    }

    return false;
}

void MainWindow::insertSql(QMap<QString, QString> &value)
{
    QJsonArray array;
    QString images = value.value("image");
    QStringList imageList = images.split(",");
    foreach (QString image, imageList) {
        array.append(image);
    }

    QJsonObject  json;
    json.insert("logo", value.value("logo"));
    json.insert("video", value.value("video"));
    json.insert("image", QJsonValue(array));
    json.insert("message", value.value("message"));
    qDebug() << " value " << value.value("showImage");
    if(value.value("showImage").compare("true") == 0)
        json.insert("showImage", QJsonValue(true));
    else
        json.insert("showImage", QJsonValue(false));
    json.insert("localVideoName", value.value("localVideoName"));


    QJsonDocument document;
    document.setObject(json);
    QByteArray byteArray = document.toJson(QJsonDocument::Compact);

    if(_sql->updateConfigResource(QString(byteArray))){
        QMessageBox::information(this, "提示", "数据提交成功！");
    }else{
        QMessageBox::information(this, "提示", "数据提交失败-_-！");
    }
}

void MainWindow::initValue()
{
    QString json;
    if(_sql->queryConfigResource(json))
    {
        QByteArray byte_array = json.toLocal8Bit();
        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(byte_array, &json_error);
        if(json_error.error == QJsonParseError::NoError)
        {
            if(!parse_doucment.isObject())
                return;

            QJsonObject json = parse_doucment.object();
            if(json.contains("logo")){
                QJsonValue logo_value = json.take("logo");
                QString logo = logo_value.toString();
                logoName = logo.split("/").last();
                ui->lineEdit_logo->setText(logo);

                QString path = downImage(logo_value.toString());
                if(!path.isEmpty())
                {
                    QImage image;
                    image.load(path);
                    ui->label_ktvlogo_preview->setPixmap(QPixmap::fromImage(image));
                }else{
                    ui->label_ktvlogo_preview->setText("LOGO最佳尺寸210×210px");
                }
            }

            if(json.contains("video")){
                videPath = json.take("video").toString();
                ui->lineEdit_video->setText(videPath);
            }

//            if(json.contains("localVideoName")){
//                QJsonValue video_value = json.take("localVideoName");
//                ui->lineEdit_video->setText(video_value.toString());
//            }

            if(json.contains("showImage")){

                QJsonValue showImage_value = json.take("showImage");
                if(showImage_value.toBool())
                    ui->radioButton_image->setChecked(true);
                else
                    ui->radioButton_video->setChecked(true);
            }

            if(json.contains("message")){
                QJsonValue message_value = json.take("message");
                ui->lineEdit_welcome->setText(message_value.toString());
            }

            if(json.contains("image"))
            {
                rowList.clear();
                QJsonArray array = json["image"].toArray();
                for (int i=0; i<array.size(); i++)
                {
                    QJsonValue value = array.at(i);
                    QString url = value.toString();
                    QStringList rowValue;
                    QString path = downImage(url);

                    if(!path.isEmpty()){

                        rowValue.clear();
                        QImage image(path);

                        rowValue.append(QString::number(i + 1));
                        rowValue.append(fileName(path));
                        rowValue.append(path);
                        rowValue.append(QString("%1*%2").arg(image.width()).arg(image.height()));
                        rowValue.append(fileSize(path));

                        rowList.append(rowValue);
                    }
                }

                model->setModalDatas(&rowList);
            }
        }
    }else{
        ui->label_ktvlogo_preview->setText("LOGO最佳尺寸210×210px");
    }
}

QString MainWindow::downImage(const QString &url)
{
    if(url.isEmpty())
        return "";

    CurlUpload *curlDownlaod = new CurlUpload();

    QStringList lists = url.split("/");
    QString filePath;
    filePath = exePath + "/image/" + lists.last();

    QFile file(filePath);
    if (!file.exists()){
        curlDownlaod->downloadImage(url, filePath);
    }

    if (file.exists())
        return filePath;
    else
        return "";
}

void MainWindow::writeLog(const QString &error)
{
    QFile logfile(logPath);
    if(logfile.open(QIODevice::Append | QIODevice::WriteOnly))
    {
        QString text = error;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss  ");

        text.insert(0, time);
        text.append(QString(" \r\n"));
        logfile.write(text.toLatin1());

        logfile.close();
    }
}

int MainWindow::isExistsFile(const QString &name)
{
    for (int i=0; i<rowList.size(); i++)
    {
        QStringList row = rowList.at(i);
        if(row.at(1).compare(name) == 0)
            return i;
    }

    return -1;
}

void MainWindow::moveUp(const int &row)
{
    if(row == 0) return;

    QStringList list = rowList.at(row);
    QStringList list1 = rowList.at(row-1);
    int index = list.at(0).toInt();
    list.replace(0, list1.at(0));
    list1.replace(0, QString::number(index));

    rowList.replace(row, list);
    rowList.replace(row-1, list1);

    rowList.swap(row, row-1);
    model->setModalDatas(&rowList);
}

void MainWindow::moveDown(const int &row)
{
    if(row == rowList.count() - 1) return;

    QStringList list = rowList.at(row);
    QStringList list1 = rowList.at(row+1);
    int index = list.at(0).toInt();
    list.replace(0, list1.at(0));
    list1.replace(0, QString::number(index));


    rowList.replace(row, list);
    rowList.replace(row+1, list1);

    rowList.swap(row, row+1);
    model->setModalDatas(&rowList);
}

void MainWindow::deleteCurrentRow(const int &row)
{
    if(row < 0  || row > rowList.size())
        return;

    QString content = QString("确定删除图片:%1").arg(rowList.at(row).at(1));
    QMessageBox box(QMessageBox::Warning, "提示", content);
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setButtonText(QMessageBox::Ok, "确定");
    box.setButtonText(QMessageBox::Cancel, "取消");

    if(box.exec() == QMessageBox::Ok)
    {
        rowList.removeAt(row);

        for(int i=0; i<rowList.size(); i++)
        {
            QStringList list = rowList.at(i);
            list.replace(0, QString::number(i+1));
            rowList.replace(i, list);
        }
    }
}

void MainWindow::replace(const int &row)
{
    QString fileFormat("LOGO文件(*.jpg)");
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    QString path = QFileDialog::getOpenFileName(this,
                                                 "添加LOGO图片",
                                                 document,
                                                 fileFormat
                                                 );

    if(path.isEmpty())
        return;


    QStringList rowValue;
    rowValue.clear();
    QImage image(path);

    rowValue.append(rowList.at(row).at(0));
    rowValue.append(fileName(path));
    rowValue.append(path);
    rowValue.append(QString("%1*%2").arg(image.width()).arg(image.height()));
    rowValue.append(fileSize(path));

    rowList.replace(row, rowValue);

    model->setModalDatas(&rowList);
}

void MainWindow::timeOver()
{
     qDebug() << " isempty stop  ";
    if(retSize.isEmpty())
    {
         return;

    }

    QStringList list = retSize.split(",");
    if(list.last().compare("-2") == 0)
        return;

    if(!list.last().isEmpty() && list.last().compare("0") != 0)
    {
        if(startValue){

            QString valueS = list.last();
            ui->progressBar->setRange(0, (int)valueS.toDouble());
            startValue = false;
        }else{

            QString currVal = list.first();
            ui->progressBar->setValue((int)currVal.toDouble());
        }


        qDebug() << "fist : " <<  list.first() << " last : " << list.last();
        if(list.first().compare(list.last()) == 0)
        {
            qDebug() << " 100 stop";
        }
    }
}


void MainWindow::on_pushButton_addLogo_clicked()
{
    QString fileFormat("LOGO文件(*.jpg)");
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    QString path = QFileDialog::getOpenFileName(this,
                                                 "添加LOGO图片",
                                                 document,
                                                 fileFormat
                                                 );

    if(path.isEmpty())
        return;
    ui->lineEdit_logo->setText(path);

    QImage image;
    image.load(path);
    ui->label_ktvlogo_preview->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::on_pushButton_addVideo_clicked()
{
    QString fileFormat("视频文件(*.mp4)");
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    QString path = QFileDialog::getOpenFileName(this,
                                                 "添加视频文件",
                                                 document,
                                                 fileFormat
                                                 );

    if(path.isEmpty())
        return;
    ui->lineEdit_video->setText(path);
}

void MainWindow::on_pushButton_addImage_clicked()
{
    pathList.clear();
    QString fileFormat("图片(*.jpg)");
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    pathList = QFileDialog::getOpenFileNames(this,
                                             "添加图片",
                                             document,
                                             fileFormat
                                             );
    if(pathList.isEmpty())
        return;

    QStringList rowValue;
    int index = rowList.count();
    for (int i=0; i<pathList.size(); i++)
    {
        rowValue.clear();
        QString imagePath = pathList.at(i);
        QImage image(imagePath);

        ////重复判断
        bool isCover = false;
        int rowNum = isExistsFile(fileName(imagePath));
        if( rowNum != -1)
        {
            QString content = QString("添加图片重复: %1 \n"
                                      "是否覆盖？").arg(fileName(imagePath));
            QMessageBox box(QMessageBox::Warning, "提示", content);
            box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            box.setButtonText(QMessageBox::Ok, "确定");
            box.setButtonText(QMessageBox::Cancel, "取消");

            if(box.exec() == QMessageBox::Cancel)
                continue;
            else
                isCover = true;
        }

        rowValue.append(QString::number(index + i + 1));
        rowValue.append(fileName(imagePath));
        rowValue.append(imagePath);
        rowValue.append(QString("%1*%2").arg(image.width()).arg(image.height()));
        rowValue.append(fileSize(imagePath));

        if(isCover)
            rowList.replace(rowNum, rowValue);
        else
            rowList.append(rowValue);
    }

    model->setModalDatas(&rowList);
}


void MainWindow::on_pushButton_deleLogo_clicked()
{
    ui->label_ktvlogo_preview->clear();
    ui->lineEdit_logo->clear();
    ui->label_ktvlogo_preview->setText("LOGO最佳尺寸210×210px");
}

void MainWindow::on_pushButton_deleVideo_clicked()
{
    ui->lineEdit_video->clear();

    if(_player){
        _player->pause();
        videPath.clear();
        ui->pushButton_stop->setEnabled(false);
    }
}

void MainWindow::on_pushButton_upload_clicked()
{
    if(on_lineEdit_welcome_editingFinished() || isEmpty())
    {
        return ;
    }

    retSize = "-2";
//    timer->start(PROGRESS_TIME);
    this->setCursor(Qt::WaitCursor);
//    ui->progressBar->setHidden(false);
    QMap<QString, QString> json;
    json.clear();
    QString retStr;
    CurlUpload *curlUpload = new CurlUpload();
    ////上传logo图片

    if(!ui->lineEdit_logo->text().isEmpty())
    {
        if( ui->lineEdit_logo->text().indexOf(".jpg") == -1){
            QMessageBox::information(this, "提示", "输入图片格式不对\n请选择jpg图片。");
            this->setCursor(Qt::ArrowCursor);
            return;
        }

        if(ui->lineEdit_logo->text().at(0) == '/')
        {
            retStr = ui->lineEdit_logo->text();
        }
        else
        {
            QFile logoFile(ui->lineEdit_logo->text());
            if(logoFile.exists()){
                retStr = curlUpload->uploadImage(ui->lineEdit_logo->text(), &retSize);
                retStr = imageExists(retStr, ui->lineEdit_logo->text());
            }else{
                QMessageBox::information(this, "提示", "LOGO图片文件不存在！");
                this->setCursor(Qt::ArrowCursor);
                return;
            }
        }
    }
    json.insert("logo", retStr);
    ui->lineEdit_logo->setText(retStr);
    ////上传图片

    QString imagesJson;
    retStr.clear();
    if(ui->radioButton_image->isChecked())
    {
        if(rowList.size() < 1){
            QMessageBox::warning(this, "提示", "必须要有轮播图片！");
            this->setCursor(Qt::ArrowCursor);
            return;
        }
    }
    foreach (QStringList images, rowList) {
        retStr = curlUpload->uploadImage(images.at(2), &retSize);

        QString url = imageExists(retStr, images.at(2));
        imagesJson.append(url);
        imagesJson.append(",");
    }
    json.insert("image", imagesJson);

    /// 上传视频
    retStr.clear();
    if(!ui->lineEdit_video->text().isEmpty())
    {
        if(ui->lineEdit_video->text().indexOf(".mp4") == -1)
        {
            QMessageBox::information(this, "提示", "输入视频格式不对\n请选择mp4视频。");
            this->setCursor(Qt::ArrowCursor);
            return;
        }
        else
        {
            if(ui->lineEdit_video->text().at(0) == '/')
            {
                retStr = ui->lineEdit_video->text();
            }
            else
            {
                QFile videoFile(ui->lineEdit_video->text());
                if(videoFile.exists())
                {

                    QString videoPath = ui->lineEdit_video->text();
                    //        localFileName = videoPath.split("/").last();
                    int before = videoPath.lastIndexOf("/");
                    int after = videoPath.lastIndexOf(".");
                    QString destName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh_mm_ss");
                    destName.append("_info");

                    videoPath = videoPath.replace(before+1, after-before-1, destName);
                    QFile file(ui->lineEdit_video->text());
                    file.rename(videoPath);
                    retStr =  curlUpload->uploadMediaVideo(videoPath, &retSize);

                    if(retStr.indexOf(".mp4") == -1)
                    {
                        QString error;
                        error.append(QString("上传视频 %1失败，返回%2").arg(videoPath).arg(retStr));
                        writeLog(error);
                        QMessageBox::information(this, "提示", "数据提交成功！");
                        this->setCursor(Qt::ArrowCursor);
                        return;
                    }else{
                        retStr.insert(0, "/");
                        ui->lineEdit_video->setText(retStr);
                        videPath = retStr;
                    }

                }else{
                    QMessageBox::information(this, "提示", "视频文件不存在！");
                    this->setCursor(Qt::ArrowCursor);
                    return;
                }
            }
        }
    }
    qDebug() << " upload video : " << retStr;
    json.insert("video", retStr);
//    json.insert("localVideoName", localFileName);

    retStr.clear();
    if(ui->lineEdit_welcome->text().isEmpty()){
        retStr = "欢迎光临！";
    }else{
        retStr = ui->lineEdit_welcome->text();
    }
    json.insert("message", retStr);

    retStr.clear();
    if(ui->radioButton_image->isChecked()){
         retStr = "true";
    }else{
        retStr = "false";
    }
    json.insert("showImage", retStr);

    insertSql(json);

//    ui->progressBar->setHidden(true);
//    timer->stop();

    this->setCursor(Qt::ArrowCursor);
}

void MainWindow::on_pushButton_play_clicked()
{
    QString path = ui->lineEdit_video->text();
    if(!path.isEmpty() && path.at(0) != '/'){
        if((path.at(0) >= 'A' && path.at(0) <= 'Z')
                || (path.at(0) >= 'a' && path.at(0) <= 'z') )
        {

            _media = new VlcMedia(path, true, _instance);
            _player->open(_media);
            ui->pushButton_stop->setEnabled(true);
        }
    }else{

        if(!videPath.isEmpty()){
            QSettings *initConfig = new QSettings("config.ini", QSettings::IniFormat);
            initConfig->setIniCodec("UTF-8");
            initConfig->beginGroup("SERVER");
            QStringList keyList=initConfig->childKeys();
            QStringList resHostList;
            foreach(QString key, keyList)
            {
                QString value = initConfig->value(key).toString();
                resHostList.push_back(value);
            }
            initConfig->endGroup();

            if(!resHostList.isEmpty())
            {
                QString url = resHostList.at(0);
                url.append(videPath);

                _media = new VlcMedia(url, _instance);
                _player->open(_media);
                ui->pushButton_stop->setEnabled(true);
            }
        }
    }
}


void MainWindow::on_pushButton_stop_clicked(bool checked)
{
    if(checked){
        _player->pause();
        ui->pushButton_stop->setText(STARTSTR);
    }else{

        _player->play();
        ui->pushButton_stop->setText(STOPSTR);
    }
}

bool MainWindow::on_lineEdit_welcome_editingFinished()
{
    QString str = ui->lineEdit_welcome->text();
    QTextCodec *gbk = QTextCodec::codecForName("GBK");
    QByteArray _src = gbk->fromUnicode(str);

    if(_src.length() >= 60){
        QMessageBox::information(this, "提示", "欢迎语输入字符超过60个字符！");
        return true;
    }

    return false;
}
