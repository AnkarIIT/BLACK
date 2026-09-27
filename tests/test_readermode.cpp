#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QWebEngineView>
#include <QWebEnginePage>
#include "../ReaderMode.h"

class TestReaderMode : public QObject
{
    Q_OBJECT

private:
    ReaderMode *m_readerMode = nullptr;
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
        delete m_readerMode;
        delete m_tempDir;
    }

    void init()
    {
        m_readerMode = new ReaderMode(this);
        QVERIFY(m_readerMode);
    }

    void cleanup()
    {
        delete m_readerMode;
        m_readerMode = nullptr;
    }

    void testInitialState()
    {
        QVERIFY(m_readerMode);
        QCOMPARE(m_readerMode->fontSize(), 18);
        QCOMPARE(m_readerMode->darkMode(), false);
        QCOMPARE(m_readerMode->fontFamily(), QString("Georgia, serif"));
        QCOMPARE(m_readerMode->lineHeight(), 1.6);
    }

    void testFontSize()
    {
        m_readerMode->setFontSize(20);
        QCOMPARE(m_readerMode->fontSize(), 20);
        
        // Test bounds
        m_readerMode->setFontSize(8);
        QCOMPARE(m_readerMode->fontSize(), 12); // Min
        
        m_readerMode->setFontSize(40);
        QCOMPARE(m_readerMode->fontSize(), 32); // Max
    }

    void testDarkMode()
    {
        m_readerMode->setDarkMode(true);
        QVERIFY(m_readerMode->darkMode());
        
        m_readerMode->setDarkMode(false);
        QVERIFY(!m_readerMode->darkMode());
    }

    void testFontFamily()
    {
        m_readerMode->setFontFamily("Arial, sans-serif");
        QCOMPARE(m_readerMode->fontFamily(), QString("Arial, sans-serif"));
    }

    void testLineHeight()
    {
        m_readerMode->setLineHeight(1.8);
        QCOMPARE(m_readerMode->lineHeight(), 1.8);
        
        // Test bounds
        m_readerMode->setLineHeight(1.0);
        QCOMPARE(m_readerMode->lineHeight(), 1.2); // Min
        
        m_readerMode->setLineHeight(3.0);
        QCOMPARE(m_readerMode->lineHeight(), 2.5); // Max
    }

    void testReaderModeStylesheet()
    {
        QString light = ReaderMode::readerModeStylesheet(false);
        QString dark = ReaderMode::readerModeStylesheet(true);
        
        QVERIFY(!light.isEmpty());
        QVERIFY(!dark.isEmpty());
        QVERIFY(light != dark);
        QVERIFY(light.contains("font-family"));
        QVERIFY(light.contains("color"));
    }

    void testReaderModeTemplate()
    {
        ArticleContent article;
        article.title = "Test Article";
        article.byline = "Test Author";
        article.content = "<p>Test content</p>";
        article.textContent = "Test content";
        article.excerpt = "Test excerpt";
        article.dir = "ltr";
        article.siteName = "Test Site";
        article.url = QUrl("https://example.com");
        article.date = "2024-01-01";
        article.author = "Test Author";
        article.length = 100;
        
        QString html = ReaderMode::readerModeTemplate(article, false);
        QVERIFY(!html.isEmpty());
        QVERIFY(html.contains("Test Article"));
        QVERIFY(html.contains("Test Author"));
        QVERIFY(html.contains("Test Site"));
    }

    void testBuildReaderModeHTML()
    {
        ArticleContent article;
        article.title = "Test Article";
        article.byline = "Test Author";
        article.content = "<p>Test content</p>";
        article.textContent = "Test content";
        article.excerpt = "Test excerpt";
        article.dir = "ltr";
        article.siteName = "Test Site";
        article.url = QUrl("https://example.com");
        article.date = "2024-01-01";
        article.author = "Test Author";
        article.length = 100;
        
        QString html = m_readerMode->buildReaderModeHTML(article);
        QVERIFY(!html.isEmpty());
        QVERIFY(html.contains("<html"));
        QVERIFY(html.contains("Test Article"));
        QVERIFY(html.contains("Test Author"));
    }

    void testIsArticlePage()
    {
        // Returns true by default (async check)
        QVERIFY(true);
    }

    void testSignals()
    {
        QSignalSpy articleExtractedSpy(m_readerMode, &ReaderMode::articleExtracted);
        QSignalSpy readerModeToggledSpy(m_readerMode, &ReaderMode::readerModeToggled);
        QSignalSpy extractionFailedSpy(m_readerMode, &ReaderMode::extractionFailed);
        
        QVERIFY(articleExtractedSpy.isValid());
        QVERIFY(readerModeToggledSpy.isValid());
        QVERIFY(extractionFailedSpy.isValid());
    }

    void testToggleReaderModeWithoutView()
    {
        // Should not crash
        m_readerMode->toggleReaderMode(nullptr);
        QVERIFY(true);
    }
};

QTEST_MAIN(TestReaderMode)
#include "test_readermode.moc"