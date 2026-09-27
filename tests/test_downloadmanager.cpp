#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include "../DownloadManager.h"
#include "../BrowserSettings.h"

class MockDownloadRequest : public QWebEngineDownloadRequest
{
    Q_OBJECT
public:
    MockDownloadRequest(const QString &fileName, const QString &url, qint64 totalBytes = 1000, QObject *parent = nullptr)
        : QWebEngineDownloadRequest(parent), m_fileName(fileName), m_url(url), m_totalBytes(totalBytes), m_receivedBytes(0), m_state(DownloadInProgress) {}

    QString downloadFileName() const override { return m_fileName; }
    QString downloadDirectory() const override { return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation); }
    QUrl url() const override { return QUrl(m_url); }
    qint64 receivedBytes() const override { return m_receivedBytes; }
    qint64 totalBytes() const override { return m_totalBytes; }
    DownloadState state() const override { return m_state; }
    void setReceivedBytes(qint64 bytes) { m_receivedBytes = bytes; emit receivedBytesChanged(); }
    void setState(DownloadState state) { m_state = state; emit stateChanged(state); }
    void accept() override {}
    void pause() override { if (m_state == DownloadInProgress) { m_state = DownloadPaused; emit stateChanged(m_state); } }
    void resume() override { if (m_state == DownloadPaused) { m_state = DownloadInProgress; emit stateChanged(m_state); } }
    void cancel() override { m_state = DownloadCancelled; emit stateChanged(m_state); }

private:
    QString m_fileName;
    QString m_url;
    qint64 m_totalBytes;
    qint64 m_receivedBytes;
    DownloadState m_state;
};

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
    }

    void testClearCompleted()
    {
        m_manager->clearCompleted();
        QCOMPARE(m_manager->completedCount(), 0);
    }

    void testClearAll()
    {
        m_manager->clearAll();
        QCOMPARE(m_manager->completedCount(), 0);
    }

    void testPauseResumeCancelRetry()
    {
        // Test with invalid indices - should not crash
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

    void testDownloadProgressSignal()
    {
        QSignalSpy spy(m_manager, &DownloadManager::downloadProgress);
        QVERIFY(spy.isValid());
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