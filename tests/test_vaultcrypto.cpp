#include <QtTest/QtTest>
#include "../VaultCrypto.h"

class TestVaultCrypto : public QObject
{
    Q_OBJECT

private slots:
    void testEncryptionDecryption()
    {
        const QByteArray secret = QByteArrayLiteral("SecretPassword123!");

        const QByteArray encrypted = VaultCrypto::encrypt(secret);
        QVERIFY(!encrypted.isEmpty());
        QVERIFY(encrypted != secret);
        QVERIFY(VaultCrypto::isEnvelope(encrypted));

        const QByteArray decrypted = VaultCrypto::decrypt(encrypted);
        QCOMPARE(decrypted, secret);
    }
};

QTEST_MAIN(TestVaultCrypto)
#include "test_vaultcrypto.moc"
