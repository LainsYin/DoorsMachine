#include "thread.h"
#include "songinfowidget.h"
#include "singerinfowdiget.h"
#include "defistruct.h"
#include "mysqlquery.h"
#include "curlupload.h"
#include "ConvertionMP4.h"
#include "releasetech.h"
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QMessageBox>
Thread::Thread(QObject *parent, int _isSong)
    :QThread(parent),
     isSong(_isSong)
{

}

Thread::~Thread()
{

}

void Thread::run()
{
    if(isSong == 1)
    {
       uploadMedia();
    }
    else if(isSong == 2)
    {
        uploadActor();
    }
    else if(isSong == 3)
    {
        ///mp3
    }
    else if(isSong == 4)
    {
        uploadYun();
    }
}

void Thread::uploadMedia()
{
    if(song == NULL || sql == NULL)
    {
        emit result(-1);
        return;
    }

    for (int i=0; i<medias.count(); i++)
    {
        Media media = medias.at(i);
        QFile file(media.path);/*c:/test/12345678.mp4*/

        if(!media.path.isEmpty())
        {  

//            ///修改文件名为serial_id后六位
//            int before = path.lastIndexOf("/");
//            int after = path.lastIndexOf(".");

//            QString mediaName = media.serial_id.right(6);
//            path = path.replace(before+1, after-before-1, mediaName);
//            file.rename(path);

            ConvertionMP4 mp4;
#ifndef YQC_TECH
            QString path = mp4.convertionFilePS(media.path, media.mid);
#else
            QString path = mp4.convertionFile(media.path, media.serial_id);
#endif

            qDebug() << " dest path : " << path;
//            QTime time;
//            time.start();
            if(!path.isEmpty())
            {
                file.setFileName(path);
                if(file.exists())
                {
#ifndef YQC_TECH
                    emit start_upload(media.mid.toLongLong());
#else
                    emit start_upload(media.serial_id.toLongLong());
#endif
//                    emit start_upload(media.serial_id.toLongLong());
                    QString retPath = song->uploadV(path, retSize);
                    if(retPath.compare("0") == 0)
                    {
                        ///类型不合法
                        emit result(0);
                        return;
                    }
                    else if(retPath.compare("1") == 0)
                    {
                        ///上传的⽂文件已经存在于服务器,并不可覆盖
//                        QMessageBox::information(NULL, "提示", "上传的视频已经存在于服务器,并不可覆盖\n数据上传失败");
                         emit result(1);
                        qDebug() << "重复";
                        return;
                    }
                    retPath.insert(0, "/");
                    media.path = retPath;
                    qDebug() << "path: " << retPath;
                }
                //    file.rename(media.path);
//                qDebug()<< time.elapsed()/1000.0<<"s";
            }
        }

        if(!media.lyric.isEmpty())
        {
            file.setFileName(media.lyric);
            if(file.exists())
                media.lyric = song->uploadL(media.lyric);
        }

#ifndef YQC_TECH
        if(sql->insertMedia_PS(media))
        {
            emit result(media.mid.toLongLong());
        }
#else
        if(sql->insertMedia(media))
        {
            emit result(media.serial_id.toLongLong());
        }
#endif
        else
        {
            emit result(-1);
        }
    }
}

void Thread::uploadActor()
{
    if(singer == NULL || sql == NULL)
    {
        emit result(-1);
        return;
    }

    for(int i=0; i<actors.count(); i++)
    {
        Actor actor = actors.at(i);
        QString path = actor.image;
        if(!path.isEmpty())
        {
            QFile file(path);
            if(file.exists())
            {
                emit start_upload(actor.serial_id.toLongLong());
                QString retStr = "";
                CurlUpload *curlUpload = new CurlUpload();
                retStr = curlUpload->uploadActorImage(path, retSize);
                if(retStr.compare("0") == 0){///类型不合法
                    emit result(0);
                }
                else if(retStr.compare("1") == 0){///上传的⽂文件已经存在于服务器,并不可覆盖
                   emit result(1);
                }
                singer->setImageOnlineRetPath(retStr);

                delete curlUpload;
            }
        }

#ifndef YQC_TECH
        if(sql->insertActor_PS(actor))
#else
        if(sql->insertActor(actor))
#endif
//        if(sql->insertActor(actor))
        {
            emit result(actor.serial_id.toLongLong());
        }
        else
        {
            emit result(-1);
        }
    }
}

void Thread::uploadYun()
{
    QString url;
    emit start_upload(-1);
    CurlUpload *curlUpload = new CurlUpload();
    bool ok = curlUpload->uploadYQDyun(_filename, _localpath, url, retSize);

    emit resultYun(url, ok);
}

void Thread::setUploadYun(const QString &filename, const QString &localpath, QString *_retSize)
{
    _filename = filename;
    _localpath = localpath;
    retSize = _retSize;
}

void Thread::setSongInfoWidgetPointer(SongInfoWidget *_song, MysqlQuery *_sql, QString *_retSize)
{
    song = _song;
    sql = _sql;
    retSize = _retSize;
}

void Thread::setSingerInfoWidgetPointer(SingerInfoWdiget *_singer, MysqlQuery *_sql, QString *_retSize)
{
    singer = _singer;
    sql = _sql;
    retSize = _retSize;
}

void Thread::setSongValue(const QList<Media> _medias)
{
    if(!medias.isEmpty())
        medias.clear();
    medias = _medias;
}

void Thread::setSingerValue(const QList<Actor> _actors)
{
    if(!actors.isEmpty())
        actors.clear();
    actors = _actors;
}



