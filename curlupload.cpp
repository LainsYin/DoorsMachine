#include "curlupload.h"
//#include "MD5.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QMessageBox>
#include <stdlib.h>
using namespace std;
extern "C"{
#include "curl/curl.h"
}

#define FM_TYPE_IMAGE "fm"
#define ACTOR_TYPE_IMAGE "avatar"

//#define HOST "http://192.168.1.199/"
//#define HOSTNAME "http://192.168.1.199//avatar/"
//#define FMHOSTNAME "http://192.168.1.199//fm/"
//#define POSTURL "http://192.168.1.199//upload.php"
#define IMAGEFORMAT ".jpg"
#define IMAGE_TYPE "avatar"
#define MP4_TYPE "mp4"
#define LYRIC_TYPE "lyric"
#define IMAGE_TYPE "image"
#define FM_TYPE "fm"
#define POSTFIELDS "?action=upload&method=post"

const int CURL_LOW_SPEED_LIMIT = 64 * 1024;
const int CURL_LOW_SPEED_TIME = 8;

QString receiveData;
size_t write_console(void *buffer, size_t size, size_t count, void *userp)
{
    std::string str((const char*)buffer, size*count);
    QString qstr(str.data());
    receiveData = qstr;

    qDebug() << "buffer data : " << qstr << " userp : " << userp;
    return count*size;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *user_p)
{
    std::string str((const char*)buffer, size*nmemb);
    QString qstr(str.data());
    if (qstr.indexOf("404 Not Found") != -1 || qstr.indexOf("400 Bad Request") != -1){
        qDebug() << "404 not found";
        return 0;
    }

    FILE *ffp = (FILE *)user_p;
    size_t return_size = fwrite(buffer, size, nmemb, ffp);
    return return_size;
}

struct WriteThis {
  char *readptr;
  int sizeleft;
};
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;

  if(size*nmemb < 1)
    return 0;

  if(pooh->sizeleft) {
    *(char *)ptr = pooh->readptr[0]; /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    pooh->sizeleft--;                /* less data left */
    return 1;                        /* we return 1 byte at a time! */
  }

  return -1;                         /* no more data left to deliver */
}

int progress_callback(QString *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{

//    qDebug() << " huidiao " << ulnow << "  total : " << ultotal << "   " << 100*ulnow/ultotal << "%";
    if(clientp == NULL)
        return 0;
    QString str = QString::number(ulnow);
    str.append(",");
    str.append(QString::number(ultotal));
    //QString tempValue = QString::number((100*ulnow/ultotal));
    *clientp =  QString(str);
    return 0;
}

size_t put_data(void *ptr,size_t size,size_t nmemb,void *userdata) //上传文件的回调函数
{
    return fread(ptr,size,nmemb,(FILE *)userdata);
}



CurlUpload::CurlUpload()//QObject *parent
//    :QObject(parent)
{
    QSettings *initConfig = new QSettings("config.ini", QSettings::IniFormat);
    initConfig->setIniCodec("UTF-8");
//    resHost = initConfig->value("SERVER/hostname", "http://192.168.1.199").toString();
//    resHost.append("/");

//    resUpload = resHost;
//    resUpload.append("upload.php");

    if(!resHostList.isEmpty())
        resHostList.clear();

    initConfig->beginGroup("SERVER");
    QStringList keyList=initConfig->childKeys();
    foreach(QString key, keyList)
    {
        QString value = initConfig->value(key).toString();
        value.append("/upload.php");
        resHostList.push_back(value);
    }
    initConfig->endGroup();

    if(resHostList.size() > 0)
    {///下载用
        resHost =  resHostList.at(0);//initConfig->value("SERVER/hostname", "http://192.168.1.199").toString();
        resHost = resHost.replace("/upload.php", "");
    }else{
        resHost = "http://192.168.1.199";
    }
    resHost.append("/");

    retSize = NULL;
}

CurlUpload::~CurlUpload()
{

}

bool CurlUpload::downloadImage( QString actorName, QString filePath)
{
    QString url = resHost;
    url.append(actorName);

    QFile file(filePath);
    if(file.exists())
        return true;

    CURLcode nRet;
    nRet = download_image(url, filePath);

    if(file.size() < 500)
        file.remove();

    if (CURLE_OK != nRet)
        return false;
    else
        return true;
}

QString CurlUpload::uploadImage(const QString &filePath, QString *ret)
{
    retSize = ret;
    QString retStr = "";
    if(filePath.isEmpty())
        return retStr;
    QStringList list = filePath.split("/");
    QString fileName = list.back();

    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QByteArray pathB = codec->fromUnicode(filePath);
    QByteArray fileB = fileName.toLocal8Bit();

    foreach (QString urlStr, resHostList) {

        QByteArray urlByte = urlStr.toLocal8Bit();  // resUpload.toLocal8Bit();
        const char *url = urlByte.data();
        const char type[] = IMAGE_TYPE;

        uploadFile(url, type, fileB.data(), pathB.data());
        if(!receiveData.isEmpty()){
            retStr = receiveData;
            receiveData.clear();
        }
    }

    return retStr;
}



QString CurlUpload::uploadMediaVideo(const QString videoPath, QString *ret)
{
    retSize = ret;
    QStringList list = videoPath.split("/");
    QString fileName = list.back();
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QByteArray pathB = codec->fromUnicode(videoPath);
    QByteArray fileB = fileName.toLocal8Bit();

    QString returnStr = "";
    foreach (QString urlStr, resHostList) {

        QByteArray urlByte = urlStr.toLocal8Bit();  ///resUpload.toLocal8Bit();
        const char type[] = MP4_TYPE;
        const char *url = urlByte.data();
        uploadFile(url, type, fileB.data(), pathB.data());
        if(!receiveData.isEmpty()){
            returnStr = receiveData;
            receiveData.clear();
        }

        qDebug() << " return str : " << returnStr;
    }

    QString temp("-1,-1");
    retSize = &temp;

    return returnStr;
}

//QString CurlUpload::uploadMedialyric(const QString lyricPath)
//{
//    QStringList list = lyricPath.split("/");
//    QString fileName = list.back();
//    QTextCodec *codec = QTextCodec::codecForName("GBK");
//    QByteArray pathB = codec->fromUnicode(lyricPath);
//    QByteArray fileB = fileName.toLocal8Bit();

//    QString returnStr = "";
//    foreach (QString urlStr, resHostList) {

//        QByteArray urlByte = urlStr.toLocal8Bit();  ///resUpload.toLocal8Bit();
//        const char type[] = LYRIC_TYPE;
//        const char *url = urlByte.data();
//        uploadFile(url, type, fileB.data(), pathB.data());
//        if(!receiveData.isEmpty()){
//            returnStr = receiveData;
//            receiveData.clear();
//        }
//    }
//    return returnStr;
//}

//QString CurlUpload::postJson(const QString &json)
//{
//    QSettings *initConfig = new QSettings("SongsMaintain.conf", QSettings::IniFormat);
//    initConfig->setIniCodec("UTF-8");
//    QString urlStr = initConfig->value("JSON/jshost", "http://testyun.17chang.com/upload.php").toString();
////    urlStr.append("/upload.php");
//    QByteArray array = urlStr.toLocal8Bit();
//    const char *url = array.data();

////    QString testss("name=muddsic&remark=sdfasf&time=2015-07-14&url=http://www.dd.se&version=1436863884");
////    std::string stdStr = testss.toStdString();
//    std::string stdStr = json.toStdString();
//    char szJsonData[1024];
//    memset(szJsonData, 0, sizeof(szJsonData));
//    strcpy(szJsonData, stdStr.c_str());

//    CURL *curl = NULL;
//    CURLcode res;

//    curl_global_init(CURL_GLOBAL_ALL);
//    curl = curl_easy_init();
//    if(curl == NULL)
//    {
//       fprintf(stderr, "curl_easy_init() error.\n");
//       return "";
//    }

//    qDebug() << " URL : " << url;
//    qDebug() << " data :  " << szJsonData;

//    curl_easy_setopt(curl, CURLOPT_URL, url);
//    curl_easy_setopt(curl, CURLOPT_POST, 1);
//    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szJsonData);
//    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_console);
//    res = curl_easy_perform(curl);
//    if(res != CURLE_OK)
//    {
//       qDebug() << " post failed : " << res;
//       return "";
//    }
//    curl_easy_cleanup(curl);


//    QString returnStr = "";
//    if(!receiveData.isEmpty()){
//        returnStr = receiveData;
//        receiveData.clear();
//    }
//    return returnStr;
//}

//bool CurlUpload::uploadYQDyun(const QString &filename , const QString &localpath,
//                              QString &url, QString *ret)
//{
//    retSize = ret;

//    QSettings *initConfig = new QSettings("SongsMaintain.conf", QSettings::IniFormat);
//    initConfig->setIniCodec("UTF-8");
//    QString urlStr = initConfig->value("JSON/yunhost", "http://v0.api.upyun.com").toString();

//    std::string username = "song";
//    std::string pwd = "yiqiding1314";
//    std::string buckname = "yiqichang-yun";
//    std::string api_url = urlStr.toStdString();   //"http://v0.api.upyun.com";

//    std::string uri = "/" + buckname + "/music/" + filename.toStdString() ;
//    url = QString(uri.c_str());
//    std::string date;
//    {
//        char mytime[100];
//        time_t lt =	::time(NULL);
//        struct tm *ptm;
//        const time_t *llt = &lt;
//        ptm = gmtime(llt);
//        strftime(mytime , 100 , "%a, %d %b %Y %H:%M:%S GMT" , ptm);
//        date = mytime;
//    }


//    bool flag = true;

//    QFile file(localpath);
//    qint64 len = file.size();
////    int len = localpath.length(); //文件本身大小
//    QString qlen = QString::number(len);
//    std::string strlen = qlen.toStdString();
//    MD5 md5("PUT&" + uri + "&" + date + "&" + strlen + "&" + MD5(pwd).hex());
//    std::string sign = md5.hex();
//    std::string hexpert = "Expect:";
//    std::string hdate = "Date: " + date;
//    std::string hauth = "Authorization: UpYun " + username + ":" + sign;
//    std::string hmdir = "mkdir: true";
//    struct curl_slist	 *headerlist = NULL;
//    CURL * curl = curl_easy_init();
//    headerlist = curl_slist_append(headerlist, hexpert.c_str());
//    headerlist = curl_slist_append(headerlist, hdate.c_str());
//    headerlist = curl_slist_append(headerlist, hauth.c_str());
//    headerlist = curl_slist_append(headerlist, hmdir.c_str());

//    QTextCodec *codec = QTextCodec::codecForName("GBK");
//    QByteArray pathB = codec->fromUnicode(localpath);
//    std::string localpath_std = pathB.toStdString();
////    std::string localpath_std = localpath.toStdString();
//    FILE *fp = fopen(localpath_std.c_str() , "rb");
//    if(!fp)
//    {
//        QMessageBox::information(NULL, "提示", "文件打开失败。", QMessageBox::Yes);
//        return false;
//    }
//    uri = api_url + uri;  //"http://v0.api.upyun.com" + uri;

//    curl_easy_setopt(curl, CURLOPT_URL,   uri.c_str());
//    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
//    curl_easy_setopt(curl, CURLOPT_PUT , true);
////    curl_easy_setopt(curl , CURLOPT_INFILE , fp);
//    curl_easy_setopt(curl, CURLOPT_READFUNCTION, put_data);
//    curl_easy_setopt(curl, CURLOPT_READDATA, fp);

//    curl_easy_setopt(curl , CURLOPT_INFILESIZE , len);//
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_console);

//    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
//    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
//    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, retSize);

//    curl_easy_setopt(curl , CURLOPT_LOW_SPEED_LIMIT , CURL_LOW_SPEED_LIMIT/1024);
//    curl_easy_setopt(curl , CURLOPT_LOW_SPEED_TIME , CURL_LOW_SPEED_TIME);
////    curl_easy_setopt(curl , CURLOPT_CONNECTTIMEOUT_MS , 1800*1000);
////    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1800);

//    CURLcode code =curl_easy_perform(curl);
//    if(code != CURLE_OK)
//    {
//        qDebug() << " curl 错误返回：  " << code;
//        flag = false;
//    }
//    if(code == CURLE_OK)
//    {
//        long retcode;
//        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
//        if(retcode != 200)
//        {
//            flag = false;
//        }
//    }

//    curl_slist_free_all(headerlist);
//    curl_easy_cleanup(curl);
//    fclose(fp);
//    return flag;
//}

QString CurlUpload::getImgPath(QString filename)
{
    QString path = QCoreApplication::applicationDirPath();
    path.append("/platforms/image/" );
    QDir dir(path);
    if(!dir.exists())
        dir.mkpath(path);
//    QStringList listPath= path.split("/");

//    int length = listPath.at(listPath.size()-1).size() + 6;
//    path = path.left(path.length() - length );
//    path += "/Resources/image/";
    path += filename;
    path += IMAGEFORMAT;

    return path;
}

QString CurlUpload::getUrl(QString filename)
{
    QString url = resHost;
    url.append("avatar/");
    url += filename;
    url += IMAGEFORMAT;

    return url;
}

CURLcode CurlUpload::download_image(QString url,  QString save_path)
{
    CURL *curl = NULL;
    FILE *fp;
    CURLcode res;    
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QByteArray path = codec->fromUnicode(save_path);
    QByteArray urlstr  = url.toLocal8Bit();

    fp = fopen(path.data(), "wb");
    if(fp != NULL)
    {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, urlstr.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
//        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        fclose(fp);
    }

    return res;
}

CURLcode CurlUpload::download_mp4(QString url, QString save_path)
{
    CURL *curl = NULL;
    FILE *fp;
    CURLcode res;
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QByteArray path = codec->fromUnicode(save_path);
    QByteArray urlstr  = url.toLocal8Bit();

    fp = fopen(path.data(), "wb");
    if(fp != NULL)
    {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, urlstr.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
//        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 150);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        fclose(fp);
    }

    return res;
}


CURLcode CurlUpload::download_image(const char *url, const char *save_path, const char *file_name)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();

    fp = fopen(save_path, "wb");
    char* encode_url = curl_easy_escape(curl, file_name, 0);
    std::string e_url = encode_url;
    //boost::algorithm::replace_first(url, file_name, e_url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_free(encode_url);
    fclose(fp);

    return res;
}

int CurlUpload::uploadFile(const char *url,
                           const char *type,
                           const char *filename,
                           const char *filepath
                          )
{
    CURL *curl = NULL;
    CURLcode res;

    qDebug() << " file Name : " << filename << " path " << filepath << "type : " << type << " url " << url;

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    headerlist = curl_slist_append(headerlist, buf);
    if(filepath == NULL || url == NULL || filename == NULL)
       return -1;

    curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "file",
              CURLFORM_FILE, filepath,
              CURLFORM_FILENAME, filename,
              CURLFORM_CONTENTTYPE, "application/octet-stream",
              CURLFORM_END);

//    if( curl_formadd(&post, &last, CURLFORM_COPYNAME, "file",
//                 CURLFORM_FILE, filepath, CURLFORM_END) != 0)
//       {
//          fprintf(stderr, "curl_formadd error.\n");
//          return -1;
//       }

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "sendfile",
              CURLFORM_FILE, filepath,
              CURLFORM_END);
     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "filename",
              CURLFORM_COPYCONTENTS, filename,
              CURLFORM_END);

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "type",
              CURLFORM_COPYCONTENTS, type,
              CURLFORM_END);

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "submit",
              CURLFORM_COPYCONTENTS, "Submit",
              CURLFORM_END);


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl == NULL)
    {
       fprintf(stderr, "curl_easy_init() error.\n");
       curl_formfree(post);
       return -1;
    }

    char *reData[512];
    curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
//    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
//    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, retSize);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reData);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_console);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
       fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
       curl_formfree(post);
       return -1;
    }
    qDebug() << " read data : " << reData;
    curl_easy_cleanup(curl);
    curl_formfree(post);
    curl_slist_free_all(headerlist);

    return 0;
}

int CurlUpload::uploadFile(QByteArray &url, QByteArray &type, QByteArray &fileName, QByteArray &filePath)
{
    CURL *curl = NULL;
    CURLcode res;

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    headerlist = curl_slist_append(headerlist, buf);
    if(filePath.isEmpty() || url.isEmpty() || type.isEmpty() || fileName.isEmpty())
       return -1;

    curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "file",
              CURLFORM_FILE, filePath.data(),
              CURLFORM_FILENAME, fileName.data(),
              CURLFORM_CONTENTTYPE, "application/octet-stream",
              CURLFORM_END);

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "sendfile",
              CURLFORM_FILE, filePath.data(),
              CURLFORM_END);
     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "filename",
              CURLFORM_COPYCONTENTS, fileName.data(),
              CURLFORM_END);

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "type",
              CURLFORM_COPYCONTENTS, type.data(),
              CURLFORM_END);

     curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "submit",
              CURLFORM_COPYCONTENTS, "Submit",
              CURLFORM_END);


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl == NULL)
    {
       fprintf(stderr, "curl_easy_init() error.\n");
       curl_formfree(post);
       return -1;
    }

    char *reData[512];
    curl_easy_setopt(curl, CURLOPT_URL, url.data()); /*Set URL*/
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reData);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_console);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
       fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
       curl_formfree(post);
       return -1;
    }
    qDebug() << " read data : " << reData;
    curl_easy_cleanup(curl);
    curl_formfree(post);
    curl_slist_free_all(headerlist);

    return 0;
}
