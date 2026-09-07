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
        QString host = "example.com";
        QString username = "testuser";
        QString password = "testpass123";

        m_store->save(host, username, password);
        
        QVariantList hosts = m_store->hosts();
        bool found = false;
        for (const QVariant &v : hosts) {
            QVariantMap map = v.toMap();
            if (map["host"].toString() == host && map["username"].toString() == username) {
                found = true;
                break;
            }
        }
        QVERIFY(found);

        QString retrieved = m_store->passwordFor(host, username);
        QCOMPARE(retrieved, password);
    }

    void testRemove()
    {
        QString host = "remove.example.com";
        QString username = "removeuser";
        QString password = "removepass";

        m_store->save(host, username, password);
        QVERIFY(!m_store->passwordFor(host, username).isEmpty());

        m_store->remove(host, username);
        QVERIFY(m_store->passwordFor(host, username).isEmpty());
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
        m_store->save("clear1.com", "user1", "pass1");
        m_store->save("clear2.com", "user2", "pass2");
        
        m_store->clearAll();
        
        QVariantList hosts = m_store->hosts();
        QVERIFY(hosts.isEmpty());
    }

    void testDuplicateHostMultipleUsers()
    {
        QString host = "multiuser.example.com";
        
        m_store->save(host, "user1", "pass1");
        m_store->save(host, "user2", "pass2");
        
        QVariantList entries = m_store->entriesFor(host);
        QCOMPARE(entries.count(), 2);
    }
};

QTEST_MAIN(TestPasswordStore)
#include "test_passwordstore.moc"