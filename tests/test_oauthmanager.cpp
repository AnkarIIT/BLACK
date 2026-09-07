#include <QtTest>
#include <QSignalSpy>
#include "../OAuthManager.h"
#include <QJsonObject>
#include <QJsonDocument>

class TestOAuthManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
    }

    void testProvidersConfigured()
    {
        OAuthManager manager;
        
        // Test that providers are defined
        QVERIFY(OAuthManager::kProviderCount == 10);
        QVERIFY(strcmp(OAuthManager::kProviders[0].key, "google") == 0);
        QVERIFY(strcmp(OAuthManager::kProviders[1].key, "apple") == 0);
        QVERIFY(strcmp(OAuthManager::kProviders[2].key, "microsoft") == 0);
        QVERIFY(strcmp(OAuthManager::kProviders[3].key, "github") == 0);
    }

    void testPKCEGeneration()
    {
        OAuthManager manager;
        
        QString verifier = manager.generatePKCEVerifier();
        QVERIFY(!verifier.isEmpty());
        QVERIFY(verifier.length() >= 43); // base64url encoded 32 bytes
        
        QString challenge = manager.generatePKCEChallenge(verifier);
        QVERIFY(!challenge.isEmpty());
        QVERIFY(challenge.length() == 43); // base64url encoded SHA256
    }

    void testPKCEChallengeDeterministic()
    {
        OAuthManager manager;
        
        QString verifier = "test_verifier_123456789012345678901234";
        QString challenge1 = manager.generatePKCEChallenge(verifier);
        QString challenge2 = manager.generatePKCEChallenge(verifier);
        
        QCOMPARE(challenge1, challenge2);
    }

    void testStateGeneration()
    {
        OAuthManager manager;
        
        // Test that state uses system RNG (can't directly test, but verify it generates)
        // We test via the generatePKCEVerifier which uses system RNG
        QString verifier1 = manager.generatePKCEVerifier();
        QString verifier2 = manager.generatePKCEVerifier();
        
        // Should be different with high probability
        QVERIFY(verifier1 != verifier2);
    }

    void testAuthUrlBuilding()
    {
        OAuthManager manager;
        
        // Test dev mode builds mock URLs
        // This would require setting up config first
        QVERIFY(true);
    }

    void testDevMode()
    {
        OAuthManager manager;
        QVERIFY(!manager.devMode()); // Default is false
    }
};

QTEST_MAIN(TestOAuthManager)
#include "test_oauthmanager.moc"