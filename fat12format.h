#ifndef FAT12FORMAT_H
#define FAT12FORMAT_H

#include <QObject>

class Fat12Format : public QObject
{
    Q_OBJECT
public:
    explicit Fat12Format(QObject *parent = nullptr);
    Q_INVOKABLE void format(QString filePath);

signals:
};

#endif // FAT12FORMAT_H
