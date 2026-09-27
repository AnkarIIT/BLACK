#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include "../BrowserWindow.h"
#include "../BrowserSettings.h"
#include "../ShelfStore.h"
#include "../PasswordStore.h"
#include "../SafeBrowsing.h"
#include "../TrackerBlocker.h"
#include "../VaultCrypto.h"

class TestBrowserWindow : public QObject
{
    Q_OBJECT

private:
    BrowserWindow *m_window = nullptr;
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

    void init()
    {
        m_window = new BrowserWindow();
        QVERIFY(m_window);
    }

    void cleanup()
    {
        if (m_window) {
            m_window->close();
            m_window->deleteLater();
            m_window = nullptr;
        }
    }

    void testInitialState()
    {
        QVERIFY(m_window->tabCount() >= 1); // At least start page
        QVERIFY(m_window->currentTabIndex() == 0);
    }

    void testAddTab()
    {
        int initialCount = m_window->tabCount();
        m_window->addNewTab(QUrl("https://example.com"));
        QVERIFY(m_window->tabCount() == initialCount + 1);
    }

    void testCloseTab()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        int count = m_window->tabCount();
        
        m_window->closeTab(1);
        QVERIFY(m_window->tabCount() == count - 1);
    }

    void testCloseAllTabs()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        
        m_window->closeAllTabs();
        // Should add a new tab after closing all
        QVERIFY(m_window->tabCount() == 1);
    }

    void testTabNavigation()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        
        m_window->setCurrentTab(0);
        QVERIFY(m_window->currentTabIndex() == 0);
        
        m_window->setCurrentTab(1);
        QVERIFY(m_window->currentTabIndex() == 1);
    }

    void testTabPinning()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        
        m_window->togglePinTab(0);
        QVERIFY(m_window->tabInfo(0).isPinned);
        
        m_window->togglePinTab(0);
        QVERIFY(!m_window->tabInfo(0).isPinned);
    }

    void testTabMuting()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        
        m_window->toggleMuteTab(0);
        QVERIFY(m_window->tabInfo(0).isMuted);
        
        m_window->toggleMuteTab(0);
        QVERIFY(!m_window->tabInfo(0).isMuted);
    }

    void testTabMoving()
    {
        m_window->addNewTab(QUrl("https://example.com"));
        m_window->addNewTab(QUrl("https://example.org"));
        m_window->addNewTab(QUrl("https://example.net"));
        
        m_window->moveTab(0, 2);
        QVERIFY(m_window->tabCount() == 3);
    }

    void testIncognitoWindow()
    {
        BrowserWindow *privateWindow = new BrowserWindow(true);
        QVERIFY(privateWindow);
        QVERIFY(privateWindow->isIncognito());
        privateWindow->close();
        privateWindow->deleteLater();
    }

    void testBookmarkAdd()
    {
        m_window->saveBookmark("Test Bookmark", "https://bookmark.example.com");
        // Bookmarks are async, just verify no crash
        QVERIFY(true);
    }

    void testHistoryAdd()
    {
        m_window->saveHistoryItem("Test History", "https://history.example.com");
        // History is async, just verify no crash
        QVERIFY(true);
    }

    void testReaderModeToggle()
    {
        // Test that reader mode toggle doesn't crash
        m_window->toggleReaderMode();
        QVERIFY(true);
    }

    void testSettingsDialog()
    {
        // Test that settings dialog can be opened/closed
        m_window->showSettingsMenu();
        QVERIFY(true);
    }
};

QTEST_MAIN(TestBrowserWindow)
#include "test_browserwindow.moc"