#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>

class Command : public QObject
{
    Q_OBJECT
public:
    QString filePath;
    explicit Command(QObject *parent = nullptr);
    Q_INVOKABLE QStringList excel(QString type,QStringList args);


signals:
};

#endif // COMMAND_H
