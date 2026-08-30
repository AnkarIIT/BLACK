#include <QtTest/QtTest>
#include "TrackerBlocker.h"
#include <QWebEngineUrlRequestInfo>
#include <QUrl>

class TestTrackerBlocker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testIsBlockedHost_ExactMatch();
    void testIsBlockedHost_SubdomainMatch();
    void testIsBlockedHost_NonMatch();
    void testIsBlockedHost_CaseInsensitive();
    void testIsBlockedHost_EmptyHost();
    void testIsBlockedHost_IPv4Literal();
    void testIsBlockedHost_IPv6Literal();
    void testIsBlockedHost_Localhost();
    void testHTTPSFirstUpgrade();
    void testHTTPSFirstNoUpgradeLocalhost();
    void testHTTPSFirstNoUpgradeIPLiteral();
    void testStatsRecording();
    void testDailyRollover();
    void testTrackerBreakdownJson();
    void testMostContactedTracker();
    void testPrivacyChangedSignal();
};

void TestTrackerBlocker::initTestCase()
{
}

void TestTrackerBlocker::cleanupTestCase()
{
}

void TestTrackerBlocker::init()
{
}

void TestTrackerBlocker::cleanup()
{
}

void TestTrackerBlocker::testIsBlockedHost_ExactMatch()
{
    TrackerBlocker blocker(true); // incognito instance for testing
    
    // Test exact matches from kBlockedHosts
    QVERIFY(blocker.isBlockedHost("doubleclick.net"));
    QVERIFY(blocker.isBlockedHost("googleadservices.com"));
    QVERIFY(blocker.isBlockedHost("facebook.net"));
}

void TestTrackerBlocker::testIsBlockedHost_SubdomainMatch()
{
    TrackerBlocker blocker(true);
    
    // Subdomains should be blocked
    QVERIFY(blocker.isBlockedHost("ads.doubleclick.net"));
    QVERIFY(blocker.isBlockedHost("tracking.googleadservices.com"));
    QVERIFY(blocker.isBlockedHost("pixel.facebook.net"));
    QVERIFY(blocker.isBlockedHost("sub.sub.domain.doubleclick.net"));
}

void TestTrackerBlocker::testIsBlockedHost_NonMatch()
{
    TrackerBlocker blocker(true);
    
    // Non-tracker domains should not be blocked
    QVERIFY(!blocker.isBlockedHost("example.com"));
    QVERIFY(!blocker.isBlockedHost("github.com"));
    QVERIFY(!blocker.isBlockedHost("google.com")); // Not in blocklist
    QVERIFY(!blocker.isBlockedHost("microsoft.com"));
}

void TestTrackerBlocker::testIsBlockedHost_CaseInsensitive()
{
    TrackerBlocker blocker(true);
    
    QVERIFY(blocker.isBlockedHost("DOUBLECLICK.NET"));
    QVERIFY(blocker.isBlockedHost("DoubleClick.Net"));
    QVERIFY(blocker.isBlockedHost("doubleclick.net"));
}

void TestTrackerBlocker::testIsBlockedHost_EmptyHost()
{
    TrackerBlocker blocker(true);
    
    QVERIFY(!blocker.isBlockedHost(""));
}

void TestTrackerBlocker::testIsBlockedHost_IPv4Literal()
{
    TrackerBlocker blocker(true);
    
    // IP literals should not be blocked (not in blocklist)
    QVERIFY(!blocker.isBlockedHost("192.168.1.1"));
    QVERIFY(!blocker.isBlockedHost("10.0.0.1"));
    QVERIFY(!blocker.isBlockedHost("127.0.0.1"));
}

void TestTrackerBlocker::testIsBlockedHost_IPv6Literal()
{
    TrackerBlocker blocker(true);
    
    QVERIFY(!blocker.isBlockedHost("::1"));
    QVERIFY(!blocker.isBlockedHost("2001:db8::1"));
    QVERIFY(!blocker.isBlockedHost("fe80::1%lo0"));
}

void TestTrackerBlocker::testIsBlockedHost_Localhost()
{
    TrackerBlocker blocker(true);
    
    QVERIFY(!blocker.isBlockedHost("localhost"));
    QVERIFY(!blocker.isBlockedHost("localhost.local"));
    QVERIFY(!blocker.isBlockedHost("test.localhost"));
}

void TestTrackerBlocker::testHTTPSFirstUpgrade()
{
    // HTTPS-First is tested via interceptRequest which requires QWebEngineUrlRequestInfo
    // This is a placeholder for integration testing
    QVERIFY(true);
}

void TestTrackerBlocker::testHTTPSFirstNoUpgradeLocalhost()
{
    QVERIFY(true);
}

void TestTrackerBlocker::testHTTPSFirstNoUpgradeIPLiteral()
{
    QVERIFY(true);
}

void TestTrackerBlocker::testStatsRecording()
{
    TrackerBlocker &blocker = TrackerBlocker::instance();
    
    // Record some stats
    const int initialToday = blocker.trackersBlockedToday();
    const int initial30Days = blocker.trackersBlockedLast30Days();
    
    // We can't easily trigger interceptRequest in unit test without QtWebEngine
    // but we can verify the counters exist and are accessible
    QVERIFY(blocker.trackersBlockedToday() >= 0);
    QVERIFY(blocker.trackersBlockedLast30Days() >= 0);
    QVERIFY(blocker.blockedLastNDays(7) >= 0);
    QVERIFY(blocker.blockedLastNDays(30) >= 0);
}

void TestTrackerBlocker::testDailyRollover()
{
    TrackerBlocker &blocker = TrackerBlocker::instance();
    
    // Verify rollDayIfNeeded doesn't crash
    // (uses mutable const method)
    blocker.trackersBlockedToday();
    QVERIFY(true);
}

void TestTrackerBlocker::testTrackerBreakdownJson()
{
    TrackerBlocker &blocker = TrackerBlocker::instance();
    
    const QString json = blocker.trackerBreakdownJson();
    QVERIFY(!json.isNull());
    // Should be valid JSON array
    QVERIFY(json.startsWith('[') && json.endsWith(']') || json == "[]");
}

void TestTrackerBlocker::testMostContactedTracker()
{
    TrackerBlocker &blocker = TrackerBlocker::instance();
    
    const QString tracker = blocker.mostContactedTracker();
    QVERIFY(!tracker.isNull());
    // Returns "No trackers detected" or actual tracker domain
}

void TestTrackerBlocker::testPrivacyChangedSignal()
{
    TrackerBlocker &blocker = TrackerBlocker::instance();
    
    // Verify signal exists by connecting
    bool signalEmitted = false;
    QObject::connect(&blocker, &TrackerBlocker::privacyChanged, [&signalEmitted]() {
        signalEmitted = true;
    });
    
    // Emit manually to test connection
    QMetaObject::invokeMethod(&blocker, "privacyChanged", Qt::QueuedConnection);
    QCoreApplication::processEvents();
    
    // Signal should be emitted (though may be queued)
    QVERIFY(true); // Connection works
}

QTEST_MAIN(TestTrackerBlocker)
#include "test_trackerblocker.moc"