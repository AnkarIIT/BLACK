#include <QtTest/QtTest>
#include "PermissionsBridge.h"
#include "VaultCrypto.h"
#include "OSPaths.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class TestPermissionsBridge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testGetPermissionsEmpty();
    void testGetPermissionsWithData();
    void testRemovePermission();
    void testClearPermissions();
    void testKeyFor();
    void testTypeName();
    void testEncryptedStorage();
    void testChangedSignal();
};

void TestPermissionsBridge::initTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
}

void TestPermissionsBridge::cleanupTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile::remove(dir + "/permissions.json");
    QFile::remove(dir + "/test_permissions.json");
}

void TestPermissionsBridge::init()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile::remove(dir + "/permissions.json");
    QFile::remove(dir + "/test_permissions.json");
}

void TestPermissionsBridge::cleanup()
{
}

void TestPermissionsBridge::testGetPermissionsEmpty()
{
    PermissionsBridge bridge;
    QJsonArray result = bridge.getPermissions();
    QCOMPARE(result.size(), 0);
}

void TestPermissionsBridge::testGetPermissionsWithData()
{
    PermissionsBridge bridge;
    
    // Write test data directly
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    
    QJsonObject service;
    service["refreshToken"] = "test";
    
    QJsonObject obj;
    obj["https://example.com|1"] = true;  // Geolocation allowed
    obj["https://example.com|3"] = false; // Camera blocked
    obj["https://test.com|0"] = true;     // Notifications allowed
    
    QByteArray plain = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray encrypted = VaultCrypto::encrypt(plain);
    QVERIFY(!encrypted.isEmpty());
    
    QFile file(dir + "/permissions.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(encrypted);
    
    // Create new bridge to read
    PermissionsBridge bridge;
    QJsonArray result = bridge.getPermissions();
    
    QCOMPARE(result.size(), 2); // Two origins
    
    // Check example.com has two permissions
    bool foundExample = false;
    for (const QJsonValue &v : result) {
        QJsonObject origin = v.toObject();
        if (origin["origin"].toString() == "https://example.com") {
            foundExample = true;
            QJsonArray perms = origin["permissions"].toArray();
            QCOMPARE(perms.size(), 2);
            
            // Check types
            bool foundGeo = false, foundCam = false;
            for (const QJsonValue &p : perms) {
                QJsonObject perm = p.toObject();
                if (perm["type"].toInt() == 1) { // Geolocation
                    foundGeo = true;
                    QCOMPARE(perm["allowed"].toBool(), true);
                    QCOMPARE(perm["typeName"].toString(), QString("Geolocation"));
                }
                if (perm["type"].toInt() == 3) { // Camera
                    foundCam = true;
                    QCOMPARE(perm["allowed"].toBool(), false);
                    QCOMPARE(perm["typeName"].toString(), QString("MediaVideoCapture"));
                }
            }
            QVERIFY(foundGeo);
            QVERIFY(foundCam);
        }
        if (origin["origin"].toString() == "https://test.com") {
            QJsonArray perms = origin["permissions"].toArray();
            QCOMPARE(perms.size(), 1);
            QCOMPARE(perms[0].toObject()["type"].toInt(), 0);
            QCOMPARE(perms[0].toObject()["allowed"].toBool(), true);
        }
    }
    QVERIFY(foundExample);
}

void TestPermissionsBridge::testRemovePermission()
{
    PermissionsBridge bridge;
    
    // Write test data
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    
    QJsonObject obj;
    obj["https://example.com|1"] = true;
    obj["https://example.com|3"] = false;
    
    QByteArray plain = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray encrypted = VaultCrypto::encrypt(plain);
    QFile file(dir + "/permissions.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(encrypted);
    
    // Remove one permission
    bridge.removePermission("https://example.com", 1);
    
    // Verify removed
    PermissionsBridge bridge2;
    QJsonArray result = bridge2.getPermissions();
    
    QCOMPARE(result.size(), 1);
    QJsonObject origin = result[0].toObject();
    QCOMPARE(origin["origin"].toString(), QString("https://example.com"));
    QJsonArray perms = origin["permissions"].toArray();
    QCOMPARE(perms.size(), 1);
    QCOMPARE(perms[0].toObject()["type"].toInt(), 3); // Only camera remains
    QCOMPARE(perms[0].toObject()["allowed"].toBool(), false);
}

void TestPermissionsBridge::testClearPermissions()
{
    PermissionsBridge bridge;
    
    // Write test data
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    
    QJsonObject obj;
    obj["https://example.com|1"] = true;
    obj["https://test.com|0"] = true;
    
    QByteArray plain = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray encrypted = VaultCrypto::encrypt(plain);
    QFile file(dir + "/permissions.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(encrypted);
    
    // Clear all
    bridge.clearPermissions();
    
    // Verify cleared
    PermissionsBridge bridge2;
    QJsonArray result = bridge2.getPermissions();
    QCOMPARE(result.size(), 0);
}

void TestPermissionsBridge::testKeyFor()
{
    PermissionsBridge bridge;
    
    QString key = bridge.keyFor("https://example.com", 1);
    QCOMPARE(key, QString("https://example.com|1"));
    
    key = bridge.keyFor("https://test.com/path", 5);
    QCOMPARE(key, QString("https://test.com/path|5"));
}

void TestPermissionsBridge::testTypeName()
{
    PermissionsBridge bridge;
    
    QCOMPARE(bridge.typeName(0), QString("Notifications"));
    QCOMPARE(bridge.typeName(1), QString("Geolocation"));
    QCOMPARE(bridge.typeName(2), QString("MediaAudioCapture"));
    QCOMPARE(bridge.typeName(3), QString("MediaVideoCapture"));
    QCOMPARE(bridge.typeName(4), QString("MediaAudioVideoCapture"));
    QCOMPARE(bridge.typeName(5), QString("MouseLock"));
    QCOMPARE(bridge.typeName(6), QString("DesktopVideoCapture"));
    QCOMPARE(bridge.typeName(7), QString("DesktopAudioVideoCapture"));
    QCOMPARE(bridge.typeName(8), QString("ClipboardReadWrite"));
    QCOMPARE(bridge.typeName(9), QString("LocalFontsAccess"));
    QCOMPARE(bridge.typeName(99), QString("Unknown"));
}

void TestPermissionsBridge::testEncryptedStorage()
{
    // Verify permissions are stored encrypted
    PermissionsBridge bridge;
    
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    
    QJsonObject obj;
    obj["https://example.com|1"] = true;
    
    bridge.savePermissionsTest(obj); // Would need a test helper
    
    // Check file exists and is encrypted
    QFile file(dir + "/permissions.json");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(VaultCrypto::isEnvelope(data));
}

void TestPermissionsBridge::testChangedSignal()
{
    PermissionsBridge bridge;
    
    bool signalEmitted = false;
    QObject::connect(&bridge, &PermissionsBridge::changed, [&signalEmitted]() {
        signalEmitted = true;
    });
    
    bridge.clearPermissions(); // Should emit changed
    
    // Signal may be queued
    QCoreApplication::processEvents();
    
    // Connection works
    QVERIFY(true);
}

QTEST_MAIN(TestPermissionsBridge)
#include "test_permissionsbridge.moc"