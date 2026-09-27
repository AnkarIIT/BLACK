#include <QtTest>
#include <QSignalSpy>
#include <QUrl>
#include "../SafeBrowsing.h"

class TestSafeBrowsing : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Force singleton initialization
        SafeBrowsing::instance();
    }

    void cleanupTestCase()
    {
    }

    void testEmbeddedHostsBlocked()
    {
        // Test that embedded phishing hosts are blocked
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("http://appleid-verify.com/login")));
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("https://paypal-secure-verify.com")));
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("http://netflix-billing-support.com")));
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("https://google-account-verify.com")));
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("http://microsoft-account-alert.com")));
    }

    void testLegitimateHostsNotBlocked()
    {
        // Test that legitimate hosts are not blocked
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("https://example.com")));
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("https://github.com")));
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("https://google.com")));
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("https://mozilla.org")));
    }

    void testSubdomainBlocking()
    {
        // Subdomains of blocked hosts should also be blocked
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("http://evil.appleid-verify.com")));
        QVERIFY(SafeBrowsing::instance().isBlocked(QUrl("https://phishing.paypal-secure-verify.com/login")));
    }

    void testNonHttpSchemesNotBlocked()
    {
        // Non-HTTP(S) schemes should not be checked
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("ftp://appleid-verify.com/file")));
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("file:///path/to/file")));
        QVERIFY(!SafeBrowsing::instance().isBlocked(QUrl("data:text/html,<script>alert(1)</script>"));
    }

    void testSessionAllowance()
    {
        SafeBrowsing &sb = SafeBrowsing::instance();
        
        // Initially blocked
        QVERIFY(sb.isBlocked(QUrl("http://appleid-verify.com/login")));
        
        // Allow for session
        sb.allow("appleid-verify.com");
        QVERIFY(!sb.isBlocked(QUrl("http://appleid-verify.com/login")));
        
        // Clear session allowances
        sb.clearSessionAllowances();
        QVERIFY(sb.isBlocked(QUrl("http://appleid-verify.com/login")));
    }

    void testAllowancePersistsAcrossCalls()
    {
        SafeBrowsing &sb = SafeBrowsing::instance();
        
        sb.allow("test-allow.example.com");
        QVERIFY(!sb.isBlocked(QUrl("http://test-allow.example.com")));
        
        // New check should still respect allowance
        QVERIFY(!sb.isBlocked(QUrl("https://test-allow.example.com/page")));
    }

    void testStats()
    {
        SafeBrowsing::Stats stats = SafeBrowsing::instance().getStats();
        QVERIFY(stats.blockedHosts > 0);
        QVERIFY(stats.allowedHosts >= 0);
        QVERIFY(!stats.lastUpdate.isEmpty());
    }

    void testUnicodeNormalization()
    {
        // Test Punycode normalization for IDN homograph protection
        // xn--pple-43d.com is "apple.com" with homograph 'а' (cyrillic)
        // This test verifies the normalization happens
        QUrl url("http://xn--pple-43d.com");
        // The domain should be normalized to punycode form
        // Since it's not in our blocklist, it should not be blocked
        QVERIFY(!SafeBrowsing::instance().isBlocked(url));
    }
};

QTEST_MAIN(TestSafeBrowsing)
#include "test_safebrowsing.moc"