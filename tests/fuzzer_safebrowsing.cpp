#include <QtTest>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include "../SafeBrowsing.h"

class SafeBrowsingFuzzer : public QObject {
    Q_OBJECT
private slots:
    void testIsBlocked_data() {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<bool>("expectedBlocked");
        
        QTest::newRow("appleid-verify") << QByteArray("http://appleid-verify.com/login") << true;
        QTest::newRow("paypal-verify") << QByteArray("https://paypal-secure-verify.com") << true;
        QTest::newRow("netflix-billing") << QByteArray("http://netflix-billing-support.com") << true;
        QTest::newRow("google-account") << QByteArray("https://google-account-verify.com") << true;
        QTest::newRow("microsoft-alert") << QByteArray("http://microsoft-account-alert.com") << true;
        QTest::newRow("example-com") << QByteArray("https://example.com") << false;
        QTest::newRow("github-com") << QByteArray("https://github.com") << false;
        QTest::newRow("subdomain-phishing") << QByteArray("http://evil.appleid-verify.com") << true;
        QTest::newRow("unicode-domain") << QByteArray("http://xn--pple-43d.com") << false;
    }
    
    void testIsBlocked() {
        QFETCH(QByteArray, input);
        QFETCH(bool, expectedBlocked);
        
        // Use singleton instance
        SafeBrowsing &sb = SafeBrowsing::instance();
        QUrl url(QString::fromUtf8(input));
        bool blocked = sb.isBlocked(url);
        QCOMPARE(blocked, expectedBlocked);
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 2048) return 0;
    
    QByteArray input(reinterpret_cast<const char*>(data), size);
    QString urlStr = QString::fromUtf8(input).trimmed();
    
    if (urlStr.isEmpty() || urlStr.contains('\0') || urlStr.contains('\n') || urlStr.contains('\r')) {
        return 0;
    }
    
    static SafeBrowsing &sb = SafeBrowsing::instance();
    QUrl url(urlStr);
    if (url.isValid()) {
        sb.isBlocked(url);
    }
    
    return 0;
}

#include "fuzzer_safebrowsing.moc"