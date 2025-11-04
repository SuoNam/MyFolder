#include "ocr.h"
#include<QDebug>
#include <opencv2/opencv.hpp>
OCR::OCR(QObject *parent)
    : QObject{parent}
{}



void OCR::test(const QString &imagePath)
{
    qDebug() << "开始 OCR 识别:" << imagePath;
    QString path=imagePath;
    path.replace("file:///","");

    cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (image.empty()) {
        qDebug() << "❌ 无法加载图片:" << imagePath;
        return;
    }
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

}
