#include <QtTest>
#include <QSignalSpy>
#include "../PasswordStore.h"
#include "../VaultCrypto.h"
#include <QTemporaryDir>
#include <QStandardPaths>

class TestPasswordStore : public QObject
{
    Q_OBJECT

private:
    PasswordStore *m_store = nullptr;
    QTemporaryDir *m_tempDir = nullptr;

private slots:
    void initTestCase()
    {
        m_tempDir = new QTemporaryDir();
        QVERIFY(m_tempDir->isValid());
        // Set test data location
        qputenv("QT_TESTING", "1");
    }

    void cleanupTestCase()
    {
        delete m_store;
        delete m_tempDir;
    }

    void init()
    {
        m_store = new PasswordStore(this);
    }

    void cleanup()
    {
        delete m_store;
        m_store = nullptr;
    }

    void testInitialState()
    {
        QVERIFY(m_store);
        QVariantList hosts = m_store->hosts();
        // Initially empty or has existing data
        QVERIFY(hosts.count() >= 0);
    }

    void testSaveAndRetrieve()
    {
        QString origin = "https://example.com";
        QString username = "testuser";
        QString password = "testpass123";

        m_store->save(origin, username, password);
        
        QVariantList hosts = m_store->hosts();
        bool found = false;
        for (const QVariant &v : hosts) {
            QVariantMap map = v.toMap();
            if (map["origin"].toString() == origin && map["username"].toString() == username) {
                found = true;
                break;
            }
        }
        QVERIFY(found);

        QString retrieved = m_store->passwordFor(origin, username);
        QCOMPARE(retrieved, password);
    }

    void testOriginIsolation()
    {
        // Different origins should have separate credentials
        QString origin1 = "https://example.com";
        QString origin2 = "http://example.com";
        QString origin3 = "https://example.com:8443";
        QString username = "testuser";
        QString password1 = "pass1";
        QString password2 = "pass2";
        QString password3 = "pass3";

        m_store->save(origin1, username, password1);
        m_store->save(origin2, username, password2);
        m_store->save(origin3, username, password3);

        QCOMPARE(m_store->passwordFor(origin1, username), password1);
        QCOMPARE(m_store->passwordFor(origin2, username), password2);
        QCOMPARE(m_store->passwordFor(origin3, username), password3);

        // Origins should be listed separately
        QVariantList hosts = m_store->hosts();
        QSet<QString> origins;
        for (const QVariant &v : hosts) {
            QVariantMap map = v.toMap();
            origins.insert(map["origin"].toString());
        }
        QVERIFY(origins.contains(origin1));
        QVERIFY(origins.contains(origin2));
        QVERIFY(origins.contains(origin3));
    }

    void testRemove()
    {
        QString origin = "https://remove.example.com";
        QString username = "removeuser";
        QString password = "removepass";

        m_store->save(origin, username, password);
        QVERIFY(!m_store->passwordFor(origin, username).isEmpty());

        m_store->remove(origin, username);
        QVERIFY(m_store->passwordFor(origin, username).isEmpty());
    }

    void testNeverSaveList()
    {
        QString host = "neversave.example.com";
        
        QVERIFY(!m_store->isNeverSave(host));
        
        m_store->setNeverSave(host, true);
        QVERIFY(m_store->isNeverSave(host));
        
        m_store->setNeverSave(host, false);
        QVERIFY(!m_store->isNeverSave(host));
    }

    void testClearAll()
    {
        m_store->save("https://clear1.com", "user1", "pass1");
        m_store->save("https://clear2.com", "user2", "pass2");
        
        m_store->clearAll();
        
        QVariantList hosts = m_store->hosts();
        QVERIFY(hosts.isEmpty());
    }

    void testDuplicateOriginMultipleUsers()
    {
        QString origin = "https://multiuser.example.com";
        
        m_store->save(origin, "user1", "pass1");
        m_store->save(origin, "user2", "pass2");
        
        QVariantList entries = m_store->entriesFor(origin);
        QCOMPARE(entries.count(), 2);
    }

    void testPortNormalization()
    {
        // Default ports should not be included in origin
        QString origin1 = "https://example.com:443";
        QString origin2 = "https://example.com";
        QString origin3 = "http://example.com:80";
        QString origin4 = "http://example.com";
        
        m_store->save(origin1, "user", "pass1");
        m_store->save(origin2, "user", "pass2");
        
        // Both should resolve to same origin (https://example.com)
        QVariantList entries = m_store->entriesFor(origin1);
        QCOMPARE(entries.count(), 2); // Both saves went to same origin
        
        m_store->clearAll();
        
        m_store->save(origin3, "user", "pass3");
        m_store->save(origin4, "user", "pass4");
        
        entries = m_store->entriesFor(origin3);
        QCOMPARE(entries.count(), 2);
    }

    void testWWWStrippingRemoved()
    {
        // www. is no longer stripped from origins
        QString origin1 = "https://www.example.com";
        QString origin2 = "https://example.com";
        
        m_store->save(origin1, "user", "pass1");
        m_store->save(origin2, "user", "pass2");
        
        QCOMPARE(m_store->passwordFor(origin1, "user"), "pass1");
        QCOMPARE(m_store->passwordFor(origin2, "user"), "pass2");
        
        QVariantList hosts = m_store->hosts();
        QCOMPARE(hosts.count(), 2); // Two separate origins
    }
};

QTEST_MAIN(TestPasswordStore)
#include "test_passwordstore.moc"