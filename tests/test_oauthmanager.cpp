#include <QtTest/QtTest>
#include "../OAuthManager.h"

class TestOAuthManager : public QObject
{
    Q_OBJECT

private slots:
    void testOAuthManagerInit()
    {
        OAuthManager manager;
        QCOMPARE(manager.isConfigured(0), false);
    }
};

QTEST_MAIN(TestOAuthManager)
#include "test_oauthmanager.moc"
