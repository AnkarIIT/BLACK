#include <QtTest/QtTest>
#include "../PasswordStore.h"

class TestPasswordStore : public QObject
{
    Q_OBJECT

private slots:
    void testSaveAndRemovePassword()
    {
        PasswordStore store;
        const QString host = QStringLiteral("testsite.com");
        const QString user = QStringLiteral("testuser");
        const QString pass = QStringLiteral("testpass123");

        store.save(host, user, pass);

        const QString fetchedPass = store.passwordFor(host, user);
        QCOMPARE(fetchedPass, pass);

        store.remove(host, user);
        QCOMPARE(store.passwordFor(host, user), QString());
    }
};

QTEST_MAIN(TestPasswordStore)
#include "test_passwordstore.moc"
