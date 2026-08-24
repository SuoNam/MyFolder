#include "../transfermanager.h"

#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

class TransferManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void createsOneTaskForMultipleSelectedFiles();
};

void TransferManagerTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("MyFolderTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("TransferManager-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    QStandardPaths::setTestModeEnabled(true);
}

void TransferManagerTest::createsOneTaskForMultipleSelectedFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstPath = directory.filePath(QStringLiteral("first.txt"));
    const QString secondPath = directory.filePath(QStringLiteral("second.txt"));
    QFile first(firstPath);
    QVERIFY(first.open(QIODevice::WriteOnly));
    QCOMPARE(first.write("first payload"), qint64(13));
    first.close();
    QFile second(secondPath);
    QVERIFY(second.open(QIODevice::WriteOnly));
    QCOMPARE(second.write("second payload"), qint64(14));
    second.close();

    TransferManager manager;
    const QString taskId = manager.startScopedFilesUpload(
        QVariantList{firstPath, secondPath}, QStringLiteral("inbox"),
        QStringLiteral("PRIVATE"), QString());
    QVERIFY(!taskId.isEmpty());

    const QVariantList tasks = manager.getTaskList();
    QCOMPARE(tasks.size(), 1);
    const QVariantMap task = tasks.constFirst().toMap();
    QCOMPARE(task.value(QStringLiteral("totalFiles")).toInt(), 2);
    QCOMPARE(task.value(QStringLiteral("files")).toList().size(), 2);
    QCOMPARE(task.value(QStringLiteral("state")).toString(), QStringLiteral("PENDING"));
    QCOMPARE(task.value(QStringLiteral("targetPath")).toString(), QStringLiteral("inbox"));
}

QTEST_MAIN(TransferManagerTest)
#include "transfermanager_test.moc"
