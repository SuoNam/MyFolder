#include "ftp.h"
#include <cstdlib>
#include <cstring>
#include <QProcessEnvironment>

FTP::FTP(QObject *parent)
    : FTP(QProcessEnvironment::systemEnvironment().value("MYFOLDER_FTP_HOST"),
          QProcessEnvironment::systemEnvironment().value("MYFOLDER_FTP_AUTH"), parent)
{
}

FTP::FTP(const QString &server, const QString &password, QObject *parent)
    : QObject{parent}
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    this->curl = curl_easy_init();
    this->server = strdup(server.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_USERPWD, password.toUtf8().constData());
}

FTP::FTP(const char* sever, const char* password, QObject *parent)
    : QObject{parent}
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    this->curl = curl_easy_init();
    this->server = strdup(sever);
    curl_easy_setopt(curl, CURLOPT_USERPWD, password);
}

FTP::~FTP()
{
    if (this->server) {
        free(this->server);
        this->server = nullptr;
    }
    if (this->curl) {
        curl_easy_cleanup(this->curl);
        this->curl = nullptr;
    }
}

size_t FTP::writeListCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    QList<QString>* list = static_cast<QList<QString>*>(userdata);
    size_t totalSize = size * nmemb;
    QString data = QString::fromUtf8(static_cast<char*>(ptr), totalSize);
    QStringList lines = data.split("\n", Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        list->append(line.trimmed());
    }
    return totalSize;
}

size_t FTP::writeCallback(void* Ptr, size_t Size, size_t Nmemb, void* Userdata) {
    FILE* fp = static_cast<FILE*>(Userdata);
    return fwrite(Ptr, Size, Nmemb, fp);
}

size_t FTP::readCallback(void* Ptr, size_t Size, size_t Nmemb, void* Userdata) {
    FILE* fp = static_cast<FILE*>(Userdata);
    return fread(Ptr, Size, Nmemb, fp);
}

bool FTP::download(const QString &filePath, const QString &localFilePath) {
    if (!curl || !server) return false;
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_READDATA, NULL);

    std::string filePathString = filePath.toStdString();
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &FTP::writeCallback);

    QByteArray urlBytes;
    urlBytes.append("ftp://");
    urlBytes.append(this->server);
    urlBytes.append(filePathString.c_str());
    const char* url = urlBytes.constData();
    curl_easy_setopt(this->curl, CURLOPT_URL, url);

    QByteArray localBytes = localFilePath.toLocal8Bit();

    FILE* fp = fopen(localBytes.data(), "wb");
    if (!fp) return false;
    curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    return (res == CURLE_OK);
}

bool FTP::upload(const QString &filePath, const QString &localFilePath) {
    if (!curl || !server) return false;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

    QByteArray localBytes = localFilePath.toLocal8Bit();
    FILE* fp = fopen(localBytes.data(), "rb");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string filePathString = filePath.toStdString();

    QByteArray urlBytes;
    urlBytes.append("ftp://");
    urlBytes.append(this->server);
    urlBytes.append(filePathString.c_str());
    const char* url = urlBytes.constData();
    curl_easy_setopt(this->curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, &FTP::readCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fileSize);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    return (res == CURLE_OK);
}

QList<QString> FTP::list(const QString &path) {
    QList<QString> fileList;
    if (!curl || !server) return fileList;

    std::string pathString = path.toStdString();

    QByteArray urlBytes;
    urlBytes.append("ftp://");
    urlBytes.append(this->server);
    urlBytes.append(pathString.c_str());
    const char* url = urlBytes.constData();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &FTP::writeListCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileList);

    curl_easy_perform(curl);
    return fileList;
}
