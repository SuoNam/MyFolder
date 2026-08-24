#include "filedownloader.h"
#include <QDebug>

FileDownloader::FileDownloader(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}
