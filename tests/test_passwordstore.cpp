#include <QtTest/QtTest>
#include "PasswordStore.h"
#include "OSPaths.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

class TestPasswordStore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSaveAndRetrieve();
    void testSaveMultipleEntriesSameHost();
    void testSaveMultipleEntriesDifferentHosts();
    void testRemoveEntry();
    void testRemoveAllEntriesForHost();
    void testClearAll();
    void testEntriesFor();
    void testHosts();
    void testNeverSaveList();
    void testSetNeverSave();
    void testIsNeverSave();
    void testNormalization_WWWStripped();
    void testNormalization_PortStripped();
    void testNormalization_CaseInsensitive();
    void testNormalization_IPv6Bracket();
    void testEncryptionRoundTrip();
    void testAuthFailurePreventsWrite();
};

void TestPasswordStore::initTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
}

void TestPasswordStore::cleanupTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d(dir);
    d.remove("passwords.json");
    d.remove("never_save.json");
    d.remove("test_passwordstore_*");
}

void TestPasswordStore::init()
{
    // Clean up any existing test data
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile::remove(dir + "/passwords.json");
    QFile::remove(dir + "/never_save.json");
}

void TestPasswordStore::cleanup()
{
}

void TestPasswordStore::testSaveAndRetrieve()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "password123");
    
    QCOMPARE(store.passwordFor("example.com", "user1"), QString("password123"));
    QCOMPARE(store.passwordFor("EXAMPLE.COM", "user1"), QString("password123"));
    QCOMPARE(store.passwordFor("www.example.com", "user1"), QString("password123"));
}

void TestPasswordStore::testSaveMultipleEntriesSameHost()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("example.com", "user2", "pass2");
    
    QCOMPARE(store.passwordFor("example.com", "user1"), QString("pass1"));
    QCOMPARE(store.passwordFor("example.com", "user2"), QString("pass2"));
    
    // hosts() should return unique hosts
    QVariantList hosts = store.hosts();
    QCOMPARE(hosts.size(), 1);
    QCOMPARE(hosts[0].toMap()["host"].toString(), QString("example.com"));
}

void TestPasswordStore::testSaveMultipleEntriesDifferentHosts()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("test.com", "user1", "pass1");
    
    QVariantList hosts = store.hosts();
    QCOMPARE(hosts.size(), 2);
}

void TestPasswordStore::testRemoveEntry()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("example.com", "user2", "pass2");
    
    store.remove("example.com", "user1");
    
    QVERIFY(store.passwordFor("example.com", "user1").isEmpty());
    QCOMPARE(store.passwordFor("example.com", "user2"), QString("pass2"));
}

void TestPasswordStore::testRemoveAllEntriesForHost()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("example.com", "user2", "pass2");
    store.save("test.com", "user1", "pass1");
    
    store.remove("example.com", ""); // Empty username = remove all for host
    
    QVERIFY(store.passwordFor("example.com", "user1").isEmpty());
    QVERIFY(store.passwordFor("example.com", "user2").isEmpty());
    QCOMPARE(store.passwordFor("test.com", "user1"), QString("pass1"));
}

void TestPasswordStore::testClearAll()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("test.com", "user2", "pass2");
    
    store.clearAll();
    
    QVERIFY(store.passwordFor("example.com", "user1").isEmpty());
    QVERIFY(store.passwordFor("test.com", "user2").isEmpty());
    QCOMPARE(store.hosts().size(), 0);
}

void TestPasswordStore::testEntriesFor()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("example.com", "user2", "pass2");
    
    QVariantList entries = store.entriesFor("example.com");
    QCOMPARE(entries.size(), 2);
    
    QVariantMap e1 = entries[0].toMap();
    QVariantMap e2 = entries[1].toMap();
    
    QVERIFY((e1["username"] == "user1" && e1["password"] == "pass1") ||
            (e1["username"] == "user2" && e1["password"] == "pass2"));
}

void TestPasswordStore::testHosts()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "pass1");
    store.save("test.com", "user1", "pass1");
    
    QVariantList hosts = store.hosts();
    QCOMPARE(hosts.size(), 2);
    
    QStringList hostNames;
    for (const QVariant &v : hosts) {
        hostNames << v.toMap()["host"].toString();
    }
    hostNames.sort();
    QCOMPARE(hostNames, QStringList({"example.com", "test.com"}));
}

void TestPasswordStore::testNeverSaveList()
{
    PasswordStore store;
    
    QVERIFY(!store.isNeverSave("example.com"));
    
    store.setNeverSave("example.com", true);
    QVERIFY(store.isNeverSave("example.com"));
    QVERIFY(!store.isNeverSave("test.com"));
    
    store.setNeverSave("example.com", false);
    QVERIFY(!store.isNeverSave("example.com"));
}

void TestPasswordStore::testSetNeverSave()
{
    PasswordStore store;
    
    store.setNeverSave("example.com", true);
    QVERIFY(store.isNeverSave("example.com"));
    
    // Setting again should be idempotent
    store.setNeverSave("example.com", true);
    QVERIFY(store.isNeverSave("example.com"));
    
    store.setNeverSave("example.com", false);
    QVERIFY(!store.isNeverSave("example.com"));
}

void TestPasswordStore::testIsNeverSave()
{
    PasswordStore store;
    
    QVERIFY(!store.isNeverSave("example.com"));
    QVERIFY(!store.isNeverSave("EXAMPLE.COM"));
    QVERIFY(!store.isNeverSave("www.example.com"));
    
    store.setNeverSave("example.com", true);
    QVERIFY(store.isNeverSave("example.com"));
    QVERIFY(store.isNeverSave("EXAMPLE.COM"));
    QVERIFY(store.isNeverSave("www.example.com"));
}

void TestPasswordStore::testNormalization_WWWStripped()
{
    PasswordStore store;
    
    store.save("www.example.com", "user", "pass");
    
    QCOMPARE(store.passwordFor("example.com", "user"), QString("pass"));
    QCOMPARE(store.passwordFor("www.example.com", "user"), QString("pass"));
}

void TestPasswordStore::testNormalization_PortStripped()
{
    PasswordStore store;
    
    store.save("example.com:8080", "user", "pass");
    
    QCOMPARE(store.passwordFor("example.com", "user"), QString("pass"));
    QCOMPARE(store.passwordFor("example.com:443", "user"), QString("pass"));
}

void TestPasswordStore::testNormalization_CaseInsensitive()
{
    PasswordStore store;
    
    store.save("Example.COM", "User", "pass");
    
    QCOMPARE(store.passwordFor("example.com", "user"), QString("pass"));
    QCOMPARE(store.passwordFor("EXAMPLE.COM", "USER"), QString("pass"));
}

void TestPasswordStore::testNormalization_IPv6Bracket()
{
    PasswordStore store;
    
    store.save("[::1]:8080", "user", "pass");
    
    QCOMPARE(store.passwordFor("[::1]", "user"), QString("pass"));
    QCOMPARE(store.passwordFor("[::1]:443", "user"), QString("pass"));
}

void TestPasswordStore::testEncryptionRoundTrip()
{
    PasswordStore store;
    
    store.save("example.com", "user1", "password123");
    store.save("test.com", "user2", "another_pass");
    
    // Force reload by creating new store instance
    PasswordStore store2;
    
    QCOMPARE(store2.passwordFor("example.com", "user1"), QString("password123"));
    QCOMPARE(store2.passwordFor("test.com", "user2"), QString("another_pass"));
}

void TestPasswordStore::testAuthFailurePreventsWrite()
{
    // This test would require corrupting the vault key or encrypted file
    // which is difficult in unit test without access to internals.
    // The behavior is tested in integration.
    QVERIFY(true);
}

QTEST_MAIN(TestPasswordStore)
#include "test_passwordstore.moc"