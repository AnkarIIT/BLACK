#include <QtTest/QtTest>
#include "ShelfStore.h"
#include "OSPaths.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

class TestShelfStore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testAddAndRetrieve();
    void testAddDuplicateUrl();
    void testRemove();
    void testClearAll();
    void testImportBookmarks();
    void testImportBookmarksDedupe();
    void testNormalizedUrl();
    void testNormalizedUrl_Invalid();
    void testNormalizedUrl_CaseInsensitive();
    void testNormalizedUrl_TrailingSlash();
    void testNormalizedUrl_QueryFragment();
    void testNormalizedUrl_BareDomain();
    void testRetentionDays();
    void testPruneArray();
    void testMaxEntries();
    void testJsonOutput();
};

void TestShelfStore::initTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
}

void TestShelfStore::cleanupTestCase()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d(dir);
    d.remove("test_*.json");
    d.remove("bookmarks.json");
    d.remove("history.json");
}

void TestShelfStore::init()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile::remove(dir + "/test_shelfstore.json");
    QFile::remove(dir + "/test_bookmarks.json");
    QFile::remove(dir + "/test_history.json");
}

void TestShelfStore::cleanup()
{
}

void TestShelfStore::testAddAndRetrieve()
{
    ShelfStore store("test_shelfstore.json");
    
    store.add("Test Title", "https://example.com/page1");
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 1);
    
    QJsonObject item = array[0].toObject();
    QCOMPARE(item["title"].toString(), QString("Test Title"));
    QCOMPARE(item["url"].toString(), QString("https://example.com/page1"));
    QVERIFY(!item["timestamp"].toString().isEmpty());
}

void TestShelfStore::testAddDuplicateUrl()
{
    ShelfStore store("test_shelfstore.json");
    
    store.add("Title 1", "https://example.com/page1");
    store.add("Title 2", "https://example.com/page1"); // Same URL
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 1);
    QCOMPARE(array[0].toObject()["title"].toString(), QString("Title 2")); // Latest first
}

void TestShelfStore::testRemove()
{
    ShelfStore store("test_shelfstore.json");
    
    store.add("Title 1", "https://example.com/page1");
    store.add("Title 2", "https://example.com/page2");
    
    store.remove("https://example.com/page1");
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 1);
    QCOMPARE(array[0].toObject()["url"].toString(), QString("https://example.com/page2"));
}

void TestShelfStore::testClearAll()
{
    ShelfStore store("test_shelfstore.json");
    
    store.add("Title 1", "https://example.com/page1");
    store.add("Title 2", "https://example.com/page2");
    
    store.clearAll();
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 0);
}

void TestShelfStore::testImportBookmarks()
{
    ShelfStore store("test_bookmarks.json");
    
    QJsonArray items;
    QJsonObject item1;
    item1["title"] = "Imported 1";
    item1["url"] = "https://imported.com/page1";
    items.append(item1);
    
    QJsonObject item2;
    item2["title"] = "Imported 2";
    item2["url"] = "https://imported.com/page2";
    items.append(item2);
    
    int count = store.importBookmarks(items);
    QCOMPARE(count, 2);
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 2);
}

void TestShelfStore::testImportBookmarksDedupe()
{
    ShelfStore store("test_bookmarks.json");
    
    // Add existing
    store.add("Existing", "https://example.com/page1");
    
    // Import with duplicate
    QJsonArray items;
    QJsonObject item;
    item["title"] = "Imported";
    item["url"] = "https://example.com/page1"; // Same URL
    items.append(item);
    
    int count = store.importBookmarks(items);
    QCOMPARE(count, 0); // Should not import duplicate
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 1);
}

void TestShelfStore::testNormalizedUrl()
{
    QString normalized = ShelfStore::normalizedUrl("https://www.Example.com/Path/");
    QCOMPARE(normalized, QString("https://example.com/Path"));
    
    normalized = ShelfStore::normalizedUrl("https://example.com/path?query=value#fragment");
    QCOMPARE(normalized, QString("https://example.com/path?query=value#fragment"));
}

void TestShelfStore::testNormalizedUrl_Invalid()
{
    QString normalized = ShelfStore::normalizedUrl("");
    QVERIFY(normalized.isEmpty());
    
    normalized = ShelfStore::normalizedUrl("not a url");
    QVERIFY(normalized.isEmpty());
    
    normalized = ShelfStore::normalizedUrl("javascript:alert(1)");
    QVERIFY(normalized.isEmpty());
}

void TestShelfStore::testNormalizedUrl_CaseInsensitive()
{
    QString normalized = ShelfStore::normalizedUrl("https://EXAMPLE.COM/Path");
    QCOMPARE(normalized, QString("https://example.com/Path"));
}

void TestShelfStore::testNormalizedUrl_TrailingSlash()
{
    QString normalized = ShelfStore::normalizedUrl("https://example.com/path/");
    QCOMPARE(normalized, QString("https://example.com/path"));
    
    // Root should keep trailing slash
    normalized = ShelfStore::normalizedUrl("https://example.com/");
    QCOMPARE(normalized, QString("https://example.com/"));
}

void TestShelfStore::testNormalizedUrl_QueryFragment()
{
    QString normalized = ShelfStore::normalizedUrl("https://example.com/path?query=value#fragment");
    QCOMPARE(normalized, QString("https://example.com/path?query=value#fragment"));
}

void TestShelfStore::testNormalizedUrl_BareDomain()
{
    QString normalized = ShelfStore::normalizedUrl("example.com");
    QCOMPARE(normalized, QString("https://example.com"));
}

void TestShelfStore::testRetentionDays()
{
    ShelfStore store("test_shelfstore.json");
    
    store.setRetentionDays(7);
    QVERIFY(true); // Just verify no crash
}

void TestShelfStore::testPruneArray()
{
    ShelfStore store("test_shelfstore.json");
    
    const QString oldDate = QDateTime::currentDateTime().addDays(-10).toString(Qt::ISODate);
    const QString recentDate = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonArray array;
    QJsonObject oldItem;
    oldItem["url"] = "https://old.com";
    oldItem["timestamp"] = oldDate;
    array.append(oldItem);
    
    QJsonObject recentItem;
    recentItem["url"] = "https://recent.com";
    recentItem["timestamp"] = recentDate;
    array.append(recentItem);
    
    store.setRetentionDays(7);
    store.saveArray(array); // This will prune
    
    QJsonArray result = store.loadArray();
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].toObject()["url"].toString(), QString("https://recent.com"));
}

void TestShelfStore::testMaxEntries()
{
    ShelfStore store("test_shelfstore.json");
    
    // Add more than kMaxEntries (500)
    for (int i = 0; i < 510; ++i) {
        store.add(QString("Title %1").arg(i), QString("https://example.com/page%1").arg(i));
    }
    
    QJsonArray array = store.loadArray();
    QCOMPARE(array.size(), 500);
    
    // Should keep newest (highest i)
    QCOMPARE(array[0].toObject()["url"].toString(), QString("https://example.com/page509"));
}

void TestShelfStore::testJsonOutput()
{
    ShelfStore store("test_shelfstore.json");
    
    store.add("Title", "https://example.com");
    
    QString json = store.json();
    QVERIFY(!json.isEmpty());
    QVERIFY(json.startsWith('['));
    QVERIFY(json.endsWith(']'));
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(), 1);
}

QTEST_MAIN(TestShelfStore)
#include "test_shelffstore.moc"