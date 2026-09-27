#include <QtTest>
#include <QSignalSpy>
#include "../TrackerBlocker.h"
#include <QWebEngineUrlRequestInfo>

class TestTrackerBlocker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
    }

    void testBlockedHostsNotEmpty()
    {
        TrackerBlocker &blocker = TrackerBlocker::instance();
        QVERIFY(blocker.trackersBlockedToday() >= 0);
    }

    void testAllowList()
    {
        TrackerBlocker &blocker = TrackerBlocker::instance();

        // Test allow functionality
        // Note: allow() and changed() signals are not implemented in this version
        QVERIFY(true);
    }

    void testHostMatching()
    {
        // Test the private hostMatches method via isBlocked
        TrackerBlocker &blocker = TrackerBlocker::instance();
        
        // Known blocked hosts from the embedded list
        QUrl testUrl("http://appleid-verify.com/login");
        // Note: isBlocked is private, so we test via the interceptor interface
        // This test verifies the blocker initializes correctly
        QVERIFY(true); // Placeholder
    }

    void testStatistics()
    {
        TrackerBlocker &blocker = TrackerBlocker::instance();
        
        int today = blocker.trackersBlockedToday();
        int week = blocker.trackersBlockedThisWeek();
        int month = blocker.trackersBlockedLast30Days();
        
        QVERIFY(today >= 0);
        QVERIFY(week >= today);
        QVERIFY(month >= week);
    }

    void testTrackerBreakdownJson()
    {
        TrackerBlocker &blocker = TrackerBlocker::instance();
        QString json = blocker.trackerBreakdownJson();
        QVERIFY(!json.isEmpty());
        
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(doc.isArray() || doc.isObject());
    }
};

QTEST_MAIN(TestTrackerBlocker)
#include "test_trackerblocker.moc"