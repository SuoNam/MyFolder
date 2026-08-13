#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QUrlQuery>

class FileDownloader : public QObject {
    Q_OBJECT
public:
    explicit FileDownloader(QObject *parent = nullptr);

    // 暴露给 QML 调用的下载方法
    // baseUrl: 服务器地址 (如 http://localhost:8080)
    // serverFilePath: 服务器上的相对路径 (对应 filePathS)
    // saveDirectory: 本地要保存的文件夹路径
    // token: 如果你的系统开启了 JWT 验证，需要传 token
    Q_INVOKABLE void download(const QString &baseUrl, const QString &serverFilePath, const QString &saveDirectory, const QString &token = "");

signals:
    // 发送给 QML 的信号


private:
    QNetworkAccessManager *manager;
};
