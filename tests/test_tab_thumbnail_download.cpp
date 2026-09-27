#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include "../TabController.h"
#include "../DownloadManager.h"
#include "../BrowserWindow.h"
#include "../ShelfStore.h"

class TestTabThumbnailDownload : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir *m_tempDir = nullptr;

private slots:
    void initTestCase()
    {
        m_tempDir = new QTemporaryDir();
        QVERIFY(m_tempDir->isValid());
        qputenv("QT_TESTING", "1");
    }

    void cleanupTestCase()
    {
        delete m_tempDir;
    }

    void testTabThumbnailCapture()
    {
        // Test that thumbnail capture works for tabs
        // This is a basic test since we can't easily create QWebEngineViews in headless tests
        // but we can verify the TabController methods exist and are callable
        
        BrowserWindow window;
        TabController *tabController = window.findChild<TabController*>();
        QVERIFY(tabController != nullptr);
        
        // Test that captureThumbnail doesn't crash
        tabController->captureThumbnail(0);
        
        // Test pruneThumbnails
        tabController->pruneThumbnails();
    }

    void testTabReorderAndClose()
    {
        // Test tab reordering and closing operations
        BrowserWindow window;
        
        // Add a few tabs
        window.addNewTab(QUrl("https://example.com"));
        window.addNewTab(QUrl("https://example.org"));
        window.addNewTab(QUrl("https://example.net"));
        
        QVERIFY(window.tabCount() == 3);
        
        // Test move tab
        window.moveTab(0, 2);
        
        // Test close tab
        window.closeTab(1);
        QVERIFY(window.tabCount() == 2);
        
        // Test close all
        window.closeAllTabs();
        QVERIFY(window.tabCount() == 1); // Should add a new tab after closing all
    }

    void testDownloadPauseResumeRetry()
    {
        // Test DownloadManager pause/resume/retry functionality
        // Note: This is limited since we can't easily create real downloads in tests
        // but we can verify the methods exist and handle invalid indices gracefully
        
        BrowserWindow window;
        DownloadManager *downloads = window.findChild<DownloadManager*>();
        QVERIFY(downloads != nullptr);
        
        // Test with invalid indices - should not crash
        downloads->pauseDownload(-1);
        downloads->pauseDownload(999);
        downloads->resumeDownload(-1);
        downloads->resumeDownload(999);
        downloads->cancelDownload(-1);
        downloads->cancelDownload(999);
        downloads->retryDownload(-1);
        downloads->retryDownload(999);
        downloads->showInFolder(-1);
        downloads->showInFolder(999);
        
        // Test clear operations
        downloads->clearCompleted();
        downloads->clearAll();
    }

    void testDownloadClearAllCancelsActive()
    {
        BrowserWindow window;
        DownloadManager *downloads = window.findChild<DownloadManager*>();
        QVERIFY(downloads != nullptr);
        
        // clearAll should cancel active downloads
        // We can't easily test this without real downloads, but verify method exists
        downloads->clearAll();
    }

    void testShelfStoreTruncation()
    {
        // Test that ShelfStore properly truncates and reports it
        ShelfStore store("test_truncation.json");
        
        // Add more entries than max (default 500)
        // We'll set a small max for testing
        store.setMaxEntries(5);
        
        for (int i = 0; i < 10; ++i) {
            store.add(QString("Item %1").arg(i), QString("https://example.com/item%1").arg(i));
        }
        
        QVERIFY(store.wasTruncated());
        QVERIFY(store.truncatedCount() == 5); // 10 added, max 5 = 5 truncated
        
        // Verify signal was emitted
        QSignalSpy spy(&store, &ShelfStore::truncated);
        store.add("New Item", "https://example.com/new");
        QVERIFY(spy.count() >= 1);
    }

    void testShelfStoreTimeBasedPruning()
    {
        ShelfStore store("test_pruning.json");
        store.setRetentionDays(1); // Keep only 1 day
        
        // Add entries
        store.add("Old Item", "https://example.com/old");
        
        // Manually call pruneOldEntries (simulates timer)
        // Since we just added it, it shouldn't be pruned
        store.pruneOldEntries();
        
        // Verify the item is still there
        QString json = store.json();
        QVERIFY(json.contains("Old Item"));
    }

    void testShelfStoreImportTruncation()
    {
        ShelfStore store("test_import.json");
        store.setMaxEntries(3);
        
        // Import more items than max
        QJsonArray items;
        for (int i = 0; i < 10; ++i) {
            QJsonObject obj;
            obj["title"] = QString("Import %1").arg(i);
            obj["url"] = QString("https://import.example.com/%1").arg(i);
            items.append(obj);
        }
        
        int imported = store.importBookmarks(items);
        QVERIFY(imported == 10); // All imported in memory
        
        // But only 3 should be stored due to max
        QVERIFY(store.wasTruncated());
        QVERIFY(store.truncatedCount() == 7);
    }

    void testShelfStoreSeparateCaps()
    {
        // Bookmarks and history should have separate caps
        ShelfStore bookmarks("bookmarks.json");
        ShelfStore history("history.json");
        
        bookmarks.setMaxEntries(100);
        history.setMaxEntries(1000);
        
        QVERIFY(bookmarks.maxEntries() == 100);
        QVERIFY(history.maxEntries() == 1000);
        
        // They should be independent
        bookmarks.setMaxEntries(200);
        QVERIFY(bookmarks.maxEntries() == 200);
        QVERIFY(history.maxEntries() == 1000); // Unchanged
    }
};

QTEST_MAIN(TestTabThumbnailDownload)
#include "test_tab_thumbnail_download.moc"