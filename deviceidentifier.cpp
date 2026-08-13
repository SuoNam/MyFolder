#include "deviceidentifier.h"
#include <QNetworkInterface>
#include <QStringList>
DeviceIdentifier::DeviceIdentifier(QObject *parent)
    : QObject{parent}
{



}
QString DeviceIdentifier::getLinuxDeviceId() {
    QByteArray rawId = QSysInfo::machineUniqueId();

#ifdef Q_OS_LINUX
    if (rawId.isEmpty()) {
        QFile file("/etc/machine-id");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            rawId = file.readAll().trimmed();
        }
    }
    if (rawId.isEmpty()) {
        // 2. 备选路径：针对较旧的系统或特定的 D-Bus 配置
        QFile backupFile("/var/lib/dbus/machine-id");
        if (backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            rawId = backupFile.readAll().trimmed();
        }
    }
#endif

    // 如果获取失败（极少见），可以拿主机名兜底
    if (rawId.isEmpty()) {

        rawId = QSysInfo::machineHostName().toUtf8();
    }

    // 3. 为了安全性与格式统一，建议进行哈希处理
    QByteArray hash = QCryptographicHash::hash(rawId, QCryptographicHash::Sha256);
    return hash.toHex().toUpper();
}
QString DeviceIdentifier::getHostName(){
      return QSysInfo::machineHostName();
}
QString DeviceIdentifier::setLocalFolder(const QString &folderpath){
    QString actualPath = folderpath;
    if (actualPath.isEmpty()) {
        actualPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QDir dir;
    if(dir.mkpath(actualPath)){
        return actualPath;
    }else{
        return "";
    }
}
