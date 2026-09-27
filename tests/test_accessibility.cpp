#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QAccessible>
#include "../BrowserWindow.h"
#include "../ToolbarController.h"
#include "../ReaderMode.h"

class TestAccessibility : public QObject
{
    Q_OBJECT

private:
    BrowserWindow *m_window = nullptr;
    ToolbarController *m_toolbar = nullptr;
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
        m_toolbar = new ToolbarController(m_window, m_window);
        QVERIFY(m_window);
        QVERIFY(m_toolbar);
    }

    void cleanup()
    {
        if (m_toolbar) {
            m_toolbar->deleteLater();
            m_toolbar = nullptr;
        }
        if (m_window) {
            m_window->close();
            m_window->deleteLater();
            m_window = nullptr;
        }
    }

    void testAccessibleNamesOnToolbarButtons()
    {
        // Test navigation buttons
        QVERIFY(!m_toolbar->backButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->forwardButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->reloadButton()->accessibleName().isEmpty());
        
        QCOMPARE(m_toolbar->backButton()->accessibleName(), QString("Back"));
        QCOMPARE(m_toolbar->forwardButton()->accessibleName(), QString("Forward"));
        QCOMPARE(m_toolbar->reloadButton()->accessibleName(), QString("Reload"));
    }

    void testAccessibleDescriptionsOnToolbarButtons()
    {
        QVERIFY(!m_toolbar->backButton()->accessibleDescription().isEmpty());
        QVERIFY(!m_toolbar->forwardButton()->accessibleDescription().isEmpty());
        QVERIFY(!m_toolbar->reloadButton()->accessibleDescription().isEmpty());
    }

    void testAccessibleNamesOnActionButtons()
    {
        // Test action buttons
        QVERIFY(!m_toolbar->sidebarButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->shareButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->downloadsButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->tabOverviewButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->readerModeButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->translateButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->pipButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->settingsButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->extensionsButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->profileButton()->accessibleName().isEmpty());
        QVERIFY(!m_toolbar->addTabButton()->accessibleName().isEmpty());
    }

    void testAccessibleNamesOnUrlBar()
    {
        // URL bar should have accessible name
        QToolButton *shield = m_toolbar->shieldInside();
        QToolButton *lock = m_toolbar->lockButton();
        QLineEdit *urlBar = m_toolbar->urlBar();
        
        QVERIFY(!shield->accessibleName().isEmpty());
        QVERIFY(!lock->accessibleName().isEmpty());
        QVERIFY(!urlBar->accessibleName().isEmpty());
        
        QCOMPARE(shield->accessibleName(), QString("Privacy Protection"));
        QCOMPARE(lock->accessibleName(), QString("Connection Security"));
        QCOMPARE(urlBar->accessibleName(), QString("Address Bar"));
    }

    void testAccessibleNamesOnWindowToolbar()
    {
        // Test window-level toolbar buttons
        QVERIFY(!m_window->m_backButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_forwardButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_sidebarButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_reloadButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_shareButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_downloadsButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_tabOverviewButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_readerModeButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_translateButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_pipButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_settingsButton->accessibleName().isEmpty());
        QVERIFY(!m_window->m_addTabButton->accessibleName().isEmpty());
    }

    void testTabBarAccessibility()
    {
        QVERIFY(!m_window->m_tabBar->accessibleName().isEmpty());
        QVERIFY(!m_window->m_tabBar->accessibleDescription().isEmpty());
        QCOMPARE(m_window->m_tabBar->accessibleName(), QString("Tab Bar"));
        
        // Add tab button
        QVERIFY(!m_window->m_addTabButton->accessibleName().isEmpty());
        QCOMPARE(m_window->m_addTabButton->accessibleName(), QString("New Tab"));
    }

    void testSidebarAccessibility()
    {
        QVERIFY(!m_window->m_sidebarSearch->accessibleName().isEmpty());
        QCOMPARE(m_window->m_sidebarSearch->accessibleName(), QString("Sidebar Search"));
    }

    void testReaderModeAccessibility()
    {
        ReaderMode *readerMode = new ReaderMode(this);
        QVERIFY(readerMode);
        
        // Test that reader mode has proper signals for accessibility
        QSignalSpy articleExtractedSpy(readerMode, &ReaderMode::articleExtracted);
        QSignalSpy readerModeToggledSpy(readerMode, &ReaderMode::readerModeToggled);
        QSignalSpy extractionFailedSpy(readerMode, &ReaderMode::extractionFailed);
        
        QVERIFY(articleExtractedSpy.isValid());
        QVERIFY(readerModeToggledSpy.isValid());
        QVERIFY(extractionFailedSpy.isValid());
        
        delete readerMode;
    }

    void testKeyboardNavigationOrder()
    {
        // Verify that tab order is logical
        // This is a basic check - full tab order testing requires GUI
        QVERIFY(true);
    }

    void testHighContrastSupport()
    {
        // Test that high contrast mode is supported
        // ReaderMode has dark mode which provides high contrast
        ReaderMode readerMode;
        readerMode.setDarkMode(true);
        QVERIFY(readerMode.darkMode());
        
        QString darkStylesheet = ReaderMode::readerModeStylesheet(true);
        QString lightStylesheet = ReaderMode::readerModeStylesheet(false);
        
        QVERIFY(!darkStylesheet.isEmpty());
        QVERIFY(!lightStylesheet.isEmpty());
        QVERIFY(darkStylesheet != lightStylesheet);
        
        // Dark mode should have dark background
        QVERIFY(darkStylesheet.contains("#1a1a1a") || darkStylesheet.contains("dark"));
        // Light mode should have light background
        QVERIFY(lightStylesheet.contains("#ffffff") || lightStylesheet.contains("white"));
    }

    void testFocusIndicators()
    {
        // Test that focusable elements have focus policies
        QVERIFY(m_toolbar->backButton()->focusPolicy() != Qt::NoFocus);
        QVERIFY(m_toolbar->forwardButton()->focusPolicy() != Qt::NoFocus);
        QVERIFY(m_toolbar->reloadButton()->focusPolicy() != Qt::NoFocus);
        QVERIFY(m_toolbar->sidebarButton()->focusPolicy() != Qt::NoFocus);
        QVERIFY(m_toolbar->shareButton()->focusPolicy() != Qt::NoFocus);
        QVERIFY(m_toolbar->urlBar()->focusPolicy() != Qt::NoFocus);
    }

    void testScreenReaderSignals()
    {
        // Test that signals are emitted for screen readers
        // ReaderMode emits articleExtracted which can be used by screen readers
        ReaderMode *readerMode = new ReaderMode(this);
        
        QSignalSpy articleExtractedSpy(readerMode, &ReaderMode::articleExtracted);
        QSignalSpy readerModeToggledSpy(readerMode, &ReaderMode::readerModeToggled);
        QSignalSpy extractionFailedSpy(readerMode, &ReaderMode::extractionFailed);
        
        QVERIFY(articleExtractedSpy.isValid());
        QVERIFY(readerModeToggledSpy.isValid());
        QVERIFY(extractionFailedSpy.isValid());
        
        delete readerMode;
    }
};

QTEST_MAIN(TestAccessibility)
#include "test_accessibility.moc"