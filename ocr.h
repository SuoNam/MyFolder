#ifndef OCR_H
#define OCR_H

#include <QObject>

class OCR : public QObject
{
    Q_OBJECT
public:
    explicit OCR(QObject *parent = nullptr);
    Q_INVOKABLE void test(const QString &imagePath);

signals:
};

#endif // OCR_H
