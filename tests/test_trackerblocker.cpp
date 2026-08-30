#include <QtTest/QtTest>
#include "../TrackerBlocker.h"

class TestTrackerBlocker : public QObject
{
    Q_OBJECT

private slots:
    void testTrackerInstance()
    {
        TrackerBlocker &tb = TrackerBlocker::instance();
        QVERIFY(tb.trackersBlockedLast30Days() >= 0);
    }
};

QTEST_MAIN(TestTrackerBlocker)
#include "test_trackerblocker.moc"
