#ifndef CURLUPLOAD_H
#define CURLUPLOAD_H
#include <iostream>
extern "C"{
#include "curl/curl.h"
}
#include <QString>
#include <QObject>
#include <QStringList>

class QPixmap;
class QByteArray;
class QJsonObject;
class CurlUpload
{

public:
    CurlUpload();
    ~CurlUpload();

public:
    bool downloadImage( QString actorName, QString filePath);
    QString uploadImage(const QString &filePath, QString *ret = NULL);
    QString uploadMediaVideo(const QString videoPath, QString *ret = NULL);

private:
    QString getImgPath(QString filename);
    QString getUrl(QString filename);

    CURLcode download_image( QString url,  QString save_path);
    CURLcode download_mp4( QString url,  QString save_path);
    CURLcode download_image(const char *url, const char *save_path, const char *file_name);


    int uploadFile(const char *url,
                   const char *type,
                   const char *filename,
                   const char *filepath
                   );
    int uploadFile(QByteArray &url,
                   QByteArray &type,
                   QByteArray &fileName,
                   QByteArray &filePath
                   );

private:
    QStringList  resHostList;
    QString  resHost;
    QString  resUpload;

    QString *retSize;
};

#endif // CURLUPLOAD_H
