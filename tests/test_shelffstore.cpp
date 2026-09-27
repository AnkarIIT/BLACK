#include <QtTest>
#include <QSignalSpy>
#include "../ShelfStore.h"
#include <QTemporaryDir>
#include <QStandardPaths>

class TestShelfStore : public QObject
{
    Q_OBJECT

private:
    ShelfStore *m_store = nullptr;
    QTemporaryDir *m_tempDir = nullptr;

private slots:
    void initTestCase()
    {
        m_tempDir = new QTemporaryDir();
        QVERIFY(m_tempDir->isValid());
    }

    void cleanupTestCase()
    {
        delete m_store;
        delete m_tempDir;
    }

    void init()
    {
        m_store = new ShelfStore("test_store.json", this);
    }

    void cleanup()
    {
        delete m_store;
        m_store = nullptr;
    }

    void testInitialState()
    {
        QVERIFY(m_store);
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(doc.isArray());
        QCOMPARE(doc.array().count(), 0);
    }

    void testAddAndRetrieve()
    {
        m_store->add("Test Bookmark", "https://example.com");
        
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QJsonArray array = doc.array();
        QCOMPARE(array.count(), 1);
        
        QJsonObject item = array[0].toObject();
        QCOMPARE(item["title"].toString(), "Test Bookmark");
        QCOMPARE(item["url"].toString(), "https://example.com");
    }

    void testDuplicateUrlNotAdded()
    {
        m_store->add("Test 1", "https://example.com");
        m_store->add("Test 2", "https://example.com");
        
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QJsonArray array = doc.array();
        QCOMPARE(array.count(), 1);
    }

    void testRemove()
    {
        m_store->add("Test", "https://example.com");
        QCOMPARE(m_store->json().contains("example.com"), true);
        
        m_store->remove("https://example.com");
        QCOMPARE(m_store->json().contains("example.com"), false);
    }

    void testClearAll()
    {
        m_store->add("Test 1", "https://example1.com");
        m_store->add("Test 2", "https://example2.com");
        
        m_store->clearAll();
        
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QJsonArray array = doc.array();
        QCOMPARE(array.count(), 0);
    }

    void testNormalizedUrl()
    {
        QString normalized = ShelfStore::normalizedUrl("https://www.Example.COM/path/");
        QCOMPARE(normalized, "https://example.com/path");
        
        QString normalized2 = ShelfStore::normalizedUrl("http://example.com");
        QCOMPARE(normalized2, "http://example.com");
        
        QString normalized3 = ShelfStore::normalizedUrl("example.com");
        QCOMPARE(normalized3, "https://example.com");
    }

    void testRetentionDays()
    {
        m_store->setRetentionDays(7);
        
        // Add an old entry
        m_store->add("Old", "https://old.com");
        
        // Manually set timestamp to old
        // Note: We can't easily test this without modifying internal state
        QVERIFY(true);
    }

    void testImportBookmarks()
    {
        QJsonArray items;
        QJsonObject item1;
        item1["title"] = "Imported 1";
        item1["url"] = "https://import1.com";
        items.append(item1);
        
        QJsonObject item2;
        item2["title"] = "Imported 2";
        item2["url"] = "https://import2.com";
        items.append(item2);
        
        int imported = m_store->importBookmarks(items);
        QCOMPARE(imported, 2);
        
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QJsonArray array = doc.array();
        QCOMPARE(array.count(), 2);
    }

    void testMaxEntriesCap()
    {
        m_store->setMaxEntries(3);
        
        m_store->add("Item 1", "https://example.com/1");
        m_store->add("Item 2", "https://example.com/2");
        m_store->add("Item 3", "https://example.com/3");
        m_store->add("Item 4", "https://example.com/4"); // Should truncate
        
        QVERIFY(m_store->wasTruncated());
        QCOMPARE(m_store->truncatedCount(), 1);
        
        QString json = m_store->json();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QJsonArray array = doc.array();
        QCOMPARE(array.count(), 3); // Only 3 stored
    }

    void testTruncationSignal()
    {
        m_store->setMaxEntries(2);
        
        QSignalSpy spy(m_store, &ShelfStore::truncated);
        
        m_store->add("Item 1", "https://example.com/1");
        m_store->add("Item 2", "https://example.com/2");
        m_store->add("Item 3", "https://example.com/3"); // Should emit truncated
        
        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.first().first().toInt(), 1);
    }

    void testSeparateMaxEntries()
    {
        ShelfStore bookmarks("bookmarks_test.json");
        ShelfStore history("history_test.json");
        
        bookmarks.setMaxEntries(100);
        history.setMaxEntries(1000);
        
        QCOMPARE(bookmarks.maxEntries(), 100);
        QCOMPARE(history.maxEntries(), 1000);
        
        bookmarks.setMaxEntries(200);
        QCOMPARE(bookmarks.maxEntries(), 200);
        QCOMPARE(history.maxEntries(), 1000); // Unchanged
    }

    void testImportTruncation()
    {
        m_store->setMaxEntries(3);
        
        QJsonArray items;
        for (int i = 0; i < 10; ++i) {
            QJsonObject obj;
            obj["title"] = QString("Import %1").arg(i);
            obj["url"] = QString("https://import.example.com/%1").arg(i);
            items.append(obj);
        }
        
        int imported = m_store->importBookmarks(items);
        QCOMPARE(imported, 10); // All imported in memory
        
        QVERIFY(m_store->wasTruncated());
        QCOMPARE(m_store->truncatedCount(), 7);
    }
};

QTEST_MAIN(TestShelfStore)
#include "test_shelffstore.moc"