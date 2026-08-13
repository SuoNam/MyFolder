#ifndef FTP_H
#define FTP_H

#include <QObject>
#include <curl/curl.h>
#include <QString>
#include <QtQml/qqmlregistration.h>

class FTP : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit FTP(QObject *parent = nullptr);
    explicit FTP(const QString &server, const QString &password, QObject *parent = nullptr);
    explicit FTP(const char* server, const char* password, QObject *parent = nullptr);
    Q_INVOKABLE bool download(const QString &filePath, const QString &localFilePath);
    Q_INVOKABLE bool upload(const QString &filePath, const QString &localFilePath);
    Q_INVOKABLE QList<QString> list(const QString &path);
    ~FTP();

private:
    CURL* curl;
    char* server;
    static size_t writeListCallback(void* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t writeCallback(void* Ptr, size_t Size, size_t Nmemb, void* Userdata);
    static size_t readCallback(void* Ptr, size_t Size, size_t Nmemb, void* Userdata);

signals:
};

#endif // FTP_H
