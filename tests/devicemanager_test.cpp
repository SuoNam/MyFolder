#include "../devicemanager.h"

#include <QHostAddress>
#include <QtTest>

class DeviceManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void filtersVirtualNetworkAddressRanges();
};

void DeviceManagerTest::filtersVirtualNetworkAddressRanges()
{
    QVERIFY(DeviceManager::isLanCandidateAddress(QHostAddress("192.168.0.106")));
    QVERIFY(DeviceManager::isLanCandidateAddress(QHostAddress("10.20.30.40")));
    QVERIFY(!DeviceManager::isLanCandidateAddress(QHostAddress("127.0.0.1")));
    QVERIFY(!DeviceManager::isLanCandidateAddress(QHostAddress("100.95.201.12")));
    QVERIFY(!DeviceManager::isLanCandidateAddress(QHostAddress("198.18.0.1")));
    QVERIFY(!DeviceManager::isLanCandidateAddress(QHostAddress("198.19.255.254")));
    QVERIFY(!DeviceManager::isLanCandidateAddress(QHostAddress("169.254.10.20")));
}

QTEST_MAIN(DeviceManagerTest)
#include "devicemanager_test.moc"
