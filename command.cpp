#include "command.h"
#include<QProcess>
#include <QDebug>
Command::Command(QObject *parent)
    : QObject{parent}
{}
QStringList Command::excel(QString type,QStringList args){
    QProcess process;
    process.start(type,args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QStringList stringlist =output.split("\n");
    return stringlist;
}
