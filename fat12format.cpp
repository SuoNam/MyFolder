#include "fat12format.h"
#include<QDebug>
#include <QProcess>
Fat12Format::Fat12Format(QObject *parent)
    : QObject{parent}
{}
void Fat12Format::format(QString filePath){
    //TODO::调用子程序
    QProcess process;
    QString program = "/home/suo/Desktop/大三上/操作系统/fat12/format/fat12";
    QStringList args;
    args << filePath;
    process.start(program, args);
    process.waitForFinished(); // 等待执行结束
}
