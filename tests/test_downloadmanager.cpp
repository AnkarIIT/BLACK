#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWebEngineProfile>
#include "../DownloadManager.h"

// NOTE: QWebEngineDownloadRequest has no public constructor and none of its
// methods are virtual, so it cannot be subclassed or mocked. This suite
// therefore covers everything testable without a live download: construction,
// invalid-index safety, list management, and the signal surface.
class TestDownloadManager : public QObject
{
    Q_OBJECT

private:
    DownloadManager *m_manager = nullptr;
    QWebEngineProfile *m_profile = nullptr;
    QTemporaryDir *m_tempDir = nullptr;

private slots:
    void initTestCase()
    {
        m_tempDir = new QTemporaryDir();
        QVERIFY(m_tempDir->isValid());
        qputenv("QT_TESTING", "1");

        m_profile = QWebEngineProfile::defaultProfile();
        QVERIFY(m_profile);
    }

    void cleanupTestCase()
    {
        delete m_manager;
        delete m_tempDir;
    }

    void init()
    {
        m_manager = new DownloadManager(m_profile, nullptr, nullptr, this);
        QVERIFY(m_manager);
    }

    void cleanup()
    {
        delete m_manager;
        m_manager = nullptr;
    }

    void testInitialState()
    {
        QVERIFY(m_manager);
        QCOMPARE(m_manager->completedCount(), 0);
        QVERIFY(m_manager->items().isEmpty());
    }

    void testClearCompleted()
    {
        QSignalSpy spy(m_manager, &DownloadManager::itemsChanged);
        QVERIFY(spy.isValid());
        m_manager->clearCompleted();
        QCOMPARE(m_manager->completedCount(), 0);
        // clearCompleted() always emits itemsChanged().
        QCOMPARE(spy.count(), 1);
    }

    void testClearAll()
    {
        QSignalSpy spy(m_manager, &DownloadManager::itemsChanged);
        QVERIFY(spy.isValid());
        m_manager->clearAll();
        QCOMPARE(m_manager->completedCount(), 0);
        // clearAll() always emits itemsChanged().
        QCOMPARE(spy.count(), 1);
    }

    void testInvalidIndicesDoNotCrash()
    {
        // Out-of-range indices must be ignored, never crash.
        m_manager->pauseDownload(-1);
        m_manager->pauseDownload(999);
        m_manager->resumeDownload(-1);
        m_manager->resumeDownload(999);
        m_manager->cancelDownload(-1);
        m_manager->cancelDownload(999);
        m_manager->retryDownload(-1);
        m_manager->retryDownload(999);
        m_manager->showInFolder(-1);
        m_manager->showInFolder(999);

        QVERIFY(true); // No crash
    }

    void testItemsChangedSignal()
    {
        QSignalSpy spy(m_manager, &DownloadManager::itemsChanged);
        QVERIFY(spy.isValid());
    }

    void testButtonTooltipSignal()
    {
        QSignalSpy spy(m_manager, &DownloadManager::buttonTooltipChanged);
        QVERIFY(spy.isValid());
    }

    void testOpenInNewTabSignal()
    {
        QSignalSpy spy(m_manager, &DownloadManager::openInNewTab);
        QVERIFY(spy.isValid());
    }
};

QTEST_MAIN(TestDownloadManager)
#include "test_downloadmanager.moc"
