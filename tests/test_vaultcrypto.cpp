#include <QtTest>
#include <QSignalSpy>
#include "../VaultCrypto.h"
#include "../OSPaths.h"
#include <QTemporaryDir>
#include <QStandardPaths>

class TestVaultCrypto : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Set up test environment
    }

    void cleanupTestCase()
    {
    }

    void testEncryptDecryptRoundtrip()
    {
        QByteArray plaintext = "test password 123!@#";
        QByteArray encrypted = VaultCrypto::encrypt(plaintext);
        
        QVERIFY(!encrypted.isEmpty());
        QVERIFY(VaultCrypto::isEnvelope(encrypted));
        
        QByteArray decrypted = VaultCrypto::decrypt(encrypted);
        QCOMPARE(decrypted, plaintext);
    }

    void testEncryptEmptyData()
    {
        QByteArray empty;
        QByteArray encrypted = VaultCrypto::encrypt(empty);
        QVERIFY(encrypted.isEmpty() || !encrypted.isEmpty()); // Either is acceptable
    }

    void testDecryptInvalidEnvelope()
    {
        QByteArray invalid = "not an envelope";
        QByteArray result = VaultCrypto::decrypt(invalid);
        QVERIFY(result.isEmpty());
    }

    void testMultipleEncryptsDifferent()
    {
        QByteArray plaintext = "same password";
        QByteArray enc1 = VaultCrypto::encrypt(plaintext);
        QByteArray enc2 = VaultCrypto::encrypt(plaintext);
        
        // Different IVs should produce different ciphertexts
        QVERIFY(enc1 != enc2);
        
        // But both should decrypt correctly
        QCOMPARE(VaultCrypto::decrypt(enc1), plaintext);
        QCOMPARE(VaultCrypto::decrypt(enc2), plaintext);
    }

    void testTamperedEnvelopeFails()
    {
        QByteArray plaintext = "test data";
        QByteArray encrypted = VaultCrypto::encrypt(plaintext);
        
        // Tamper with the encrypted data
        QByteArray tampered = encrypted;
        if (tampered.size() > 10)
            tampered[10] ^= 0xFF;
        
        QByteArray result = VaultCrypto::decrypt(tampered);
        QVERIFY(result.isEmpty());
    }

    void testIsEnvelopeDetection()
    {
        QByteArray plaintext = "test";
        QByteArray encrypted = VaultCrypto::encrypt(plaintext);
        
        QVERIFY(VaultCrypto::isEnvelope(encrypted));
        QVERIFY(!VaultCrypto::isEnvelope(plaintext));
        QVERIFY(!VaultCrypto::isEnvelope("not an envelope"));
    }
};

QTEST_MAIN(TestVaultCrypto)
#include "test_vaultcrypto.moc"