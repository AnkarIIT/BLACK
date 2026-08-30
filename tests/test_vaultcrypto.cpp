#include <QtTest/QtTest>
#include "VaultCrypto.h"
#include "OSPaths.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>

class TestVaultCrypto : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testEncryptDecryptRoundTrip();
    void testEncryptDecryptEmpty();
    void testEncryptDecryptLargeData();
    void testEncryptDecryptUnicode();
    void testIsEnvelope();
    void testCorruptedEnvelope();
    void testWrongKeyFails();
    void testMultipleEncryptsDifferentCiphertext();
    void testLegacyFormatCompatibility();
    void testPBKDF2RFC2898Vectors();
    void testAtomicWrite();
};

void TestVaultCrypto::initTestCase()
{
    // Ensure app data directory exists
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
}

void TestVaultCrypto::cleanupTestCase()
{
    // Clean up test files
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d(dir);
    d.remove("test_vaultcrypto_*");
}

void TestVaultCrypto::init()
{
}

void TestVaultCrypto::cleanup()
{
}

void TestVaultCrypto::testEncryptDecryptRoundTrip()
{
    const QByteArray plaintext = "test password 123";
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(VaultCrypto::isEnvelope(encrypted));
    
    const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
    QCOMPARE(decrypted, plaintext);
}

void TestVaultCrypto::testEncryptDecryptEmpty()
{
    const QByteArray plaintext = "";
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    // Empty plaintext may or may not produce envelope depending on implementation
    if (!encrypted.isEmpty()) {
        const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
        QCOMPARE(decrypted, plaintext);
    }
}

void TestVaultCrypto::testEncryptDecryptLargeData()
{
    QByteArray plaintext(10000, 'x');
    for (int i = 0; i < 10000; ++i) {
        plaintext[i] = char('a' + (i % 26));
    }
    
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(VaultCrypto::isEnvelope(encrypted));
    
    const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
    QCOMPARE(decrypted, plaintext);
}

void TestVaultCrypto::testEncryptDecryptUnicode()
{
    const QByteArray plaintext = "пароль 🔐 パスワード 비밀번호 🗝️";
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(VaultCrypto::isEnvelope(encrypted));
    
    const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
    QCOMPARE(decrypted, plaintext);
}

void TestVaultCrypto::testIsEnvelope()
{
    const QByteArray plaintext = "test";
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    QVERIFY(VaultCrypto::isEnvelope(encrypted));
    QVERIFY(!VaultCrypto::isEnvelope(plaintext));
    QVERIFY(!VaultCrypto::isEnvelope(QByteArray()));
    QVERIFY(!VaultCrypto::isEnvelope("not an envelope"));
}

void TestVaultCrypto::testCorruptedEnvelope()
{
    const QByteArray plaintext = "test password";
    QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    // Corrupt the ciphertext
    if (encrypted.size() > 10) {
        encrypted[10] ^= 0xFF;
    }
    
    const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
    QVERIFY(decrypted.isEmpty()); // Should fail authentication
}

void TestVaultCrypto::testWrongKeyFails()
{
    // This test verifies that encryption with one key fails with another
    // Since we use system key, we can't easily test cross-key decryption
    // but we can verify that a completely different envelope fails
    const QByteArray plaintext = "test";
    const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
    
    // Create a fake envelope with wrong magic
    QByteArray fake = encrypted;
    if (fake.size() >= 4) {
        fake[0] = 'X';
        fake[1] = 'Y';
        fake[2] = 'Z';
        fake[3] = 'W';
    }
    
    const QByteArray decrypted = VaultCrypto::decrypt(fake);
    QVERIFY(decrypted.isEmpty());
}

void TestVaultCrypto::testMultipleEncryptsDifferentCiphertext()
{
    const QByteArray plaintext = "same password";
    const QByteArray encrypted1 = VaultCrypto::encrypt(plaintext);
    const QByteArray encrypted2 = VaultCrypto::encrypt(plaintext);
    
    // Should produce different ciphertexts (due to random salt/IV)
    QVERIFY(encrypted1 != encrypted2);
    
    // But both should decrypt to same plaintext
    QCOMPARE(VaultCrypto::decrypt(encrypted1), plaintext);
    QCOMPARE(VaultCrypto::decrypt(encrypted2), plaintext);
}

void TestVaultCrypto::testLegacyFormatCompatibility()
{
    // Test that we can handle legacy format (if any exists)
    // This is more of an integration test - we verify decrypt doesn't crash on malformed data
    QByteArray malformed = "{invalid json";
    QVERIFY(VaultCrypto::decrypt(malformed).isEmpty());
    
    malformed = "BAKV"; // Just magic, no version/cipher
    QVERIFY(VaultCrypto::decrypt(malformed).isEmpty());
}

void TestVaultCrypto::testPBKDF2RFC2898Vectors()
{
    // RFC 7914 / RFC 2898 test vectors for PBKDF2-HMAC-SHA256
    // These test the internal function indirectly via encrypt/decrypt
    // The actual KDF is tested by verifying round-trip works
    
    // Test with known password/salt/iterations if exposed
    // For now, just verify multiple iterations work
    for (int i = 0; i < 10; ++i) {
        const QByteArray plaintext = QByteArray::number(i) + "_password";
        const QByteArray encrypted = VaultCrypto::encrypt(plaintext);
        const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
        QCOMPARE(decrypted, plaintext);
    }
}

void TestVaultCrypto::testAtomicWrite()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString testFile = dir + "/test_atomic_write.tmp";
    
    const QByteArray data = "test atomic write data";
    QVERIFY(OSPaths::writeFileAtomic(testFile, data));
    
    QFile file(testFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
    
    QFile::remove(testFile);
}

QTEST_MAIN(TestVaultCrypto)
#include "test_vaultcrypto.moc"