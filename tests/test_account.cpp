#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../Account.h"
#include "../VaultCrypto.h"
#include "../OSPaths.h"

class TestAccount : public QObject
{
    Q_OBJECT

private:
    Account *m_account = nullptr;
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
        delete m_account;
        delete m_tempDir;
    }

    void init()
    {
        m_account = new Account(this);
        QVERIFY(m_account);
    }

    void cleanup()
    {
        delete m_account;
        m_account = nullptr;
    }

    void testInitialState()
    {
        QVERIFY(m_account);
        QVERIFY(!m_account->signedIn());
        QCOMPARE(m_account->name(), QString("Guest"));
        QVERIFY(m_account->email().isEmpty());
        QVERIFY(m_account->avatar().isEmpty());
    }

    void testSignInLocal()
    {
        m_account->signIn("Test User");
        QVERIFY(m_account->signedIn());
        QCOMPARE(m_account->name(), QString("Test User"));
        QCOMPARE(m_account->authMethod(), QString("local"));
    }

    void testSignOut()
    {
        m_account->signIn("Test User");
        QVERIFY(m_account->signedIn());
        
        m_account->signOut();
        QVERIFY(!m_account->signedIn());
        QCOMPARE(m_account->name(), QString("Guest"));
    }

    void testSaveAccount()
    {
        QString id = m_account->saveAccount("local", "Saved User", "saved@example.com", "avatar.png");
        QVERIFY(!id.isEmpty());
        
        QJsonArray accounts = m_account->accountsJson();
        QVERIFY(!accounts.isEmpty());
        
        QJsonObject first = accounts.first().toObject();
        QCOMPARE(first["name"].toString(), QString("Saved User"));
        QCOMPARE(first["email"].toString(), QString("saved@example.com"));
    }

    void testSelectAccount()
    {
        m_account->saveAccount("local", "User 1", "user1@example.com");
        m_account->saveAccount("local", "User 2", "user2@example.com");
        
        QJsonArray accounts = m_account->accountsJson();
        QVERIFY(accounts.count() == 2);
        
        QString id = accounts[0].toObject()["id"].toString();
        m_account->selectAccount(id);
        
        QVERIFY(m_account->signedIn());
        QCOMPARE(m_account->name(), QString("User 1"));
    }

    void testSignInWithOAuth()
    {
        m_account->signInWithOAuth("google", "OAuth User", "oauth@example.com", "avatar.png");
        
        QVERIFY(m_account->signedIn());
        QCOMPARE(m_account->name(), QString("OAuth User"));
        QCOMPARE(m_account->email(), QString("oauth@example.com"));
        QCOMPARE(m_account->avatar(), QString("avatar.png"));
        QCOMPARE(m_account->authMethod(), QString("google"));
    }

    void testAccountIdGeneration()
    {
        QString id1 = m_account->saveAccount("google", "User", "user@gmail.com");
        QString id2 = m_account->saveAccount("google", "User", "user@gmail.com");
        
        // Same provider + email should produce same ID
        QCOMPARE(id1, id2);
        
        QString id3 = m_account->saveAccount("local", "User", "");
        QVERIFY(id3 != id1);
    }

    void testAccountsPersistence()
    {
        m_account->saveAccount("local", "Persisted User", "persist@example.com");
        
        // Create new account instance to test loading from disk
        Account *newAccount = new Account(this);
        QJsonArray accounts = newAccount->accountsJson();
        
        bool found = false;
        for (const QJsonValue &v : accounts) {
            QJsonObject obj = v.toObject();
            if (obj["name"].toString() == "Persisted User") {
                found = true;
                break;
            }
        }
        
        QVERIFY(found);
        delete newAccount;
    }

    void testCompleteOnboarding()
    {
        // Should not crash
        m_account->completeOnboarding();
        QVERIFY(true);
    }
};

QTEST_MAIN(TestAccount)
#include "test_account.moc"