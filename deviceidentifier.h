#ifndef DEVICEIDENTIFIER_H
#define DEVICEIDENTIFIER_H
#include <QObject>
#include <QFile>
#include <QSysInfo>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QtQml/qqmlregistration.h>
class DeviceIdentifier : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit DeviceIdentifier(QObject *parent = nullptr);
    // Kept for QML/API compatibility; returns a stable machine identifier on all platforms.
    static QString getLinuxDeviceId();
    static QString getHostName();
    Q_INVOKABLE QString linuxDeviceId() const { return getLinuxDeviceId(); }
    Q_INVOKABLE QString hostName() const { return getHostName(); }
    Q_INVOKABLE QString setLocalFolder(const QString &folderpath = QString());

signals:
};

#endif // DEVICEIDENTIFIER_H
