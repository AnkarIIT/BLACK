#include "VaultCrypto.h"
#include "OSPaths.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QtGlobal>
#include <cstring>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

const char kMagic[4] = { 'B', 'A', 'K', 'V' };
const qint8 kVersion = 1;
const char kCipherDpapi = 'D';
const char kCipherHmacCtr = 'H';

const int kKeySize = 32;         // 256-bit master / derived key
const int kSaltSize = 16;
const int kIvSize = 12;
const int kTagSize = 16;
const int kPbkdf2Iterations = 150000;

#if defined(Q_OS_WIN)
const char kDpapiEntropy[] = "BLACK_BROWSER_VAULT_ENTROPY_BLOCK_32";
#endif

QByteArray randomBytes(int n)
{
    QByteArray out;
    out.resize(n);
    for (int i = 0; i < n; ++i)
        out[i] = char(QRandomGenerator::system()->bounded(256));
    return out;
}

void appendUint32Be(QByteArray &out, quint32 value)
{
    out.append(char((value >> 24) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char(value & 0xFF));
}

// PBKDF2-HMAC-SHA256 (RFC 2898). The Qt 6.8 baseline used here does not ship
// QKeyDerivation, so the standard construction is implemented directly.
//
// rfc2898Message selects the U_1 input ordering:
//   - true  (default, standard): S || INT_32_BE(i)
//   - false (legacy):            P || S || INT_32_BE(i)
// Early builds prepended the password into the HMAC message. That deviation
// was self-consistent but non-standard, so new vaults use the RFC 2898 layout
// (kdf "pbkdf2-hmac-sha256-rfc2898") while decrypt() still understands the
// legacy ordering to keep existing vaults readable.
QByteArray pbkdf2HmacSha256(const QByteArray &password, const QByteArray &salt,
                            int iterations, int dkLen, bool rfc2898Message)
{
    const int hLen = 32; // SHA-256 digest length
    QByteArray out;
    out.resize(dkLen);

    quint32 blockIndex = 1;
    int written = 0;
    while (written < dkLen) {
        QByteArray u;
        {
            QByteArray block;
            if (rfc2898Message) {
                block.append(salt);
            } else {
                block.append(password); // legacy pre-RFC ordering, read-compat only
                block.append(salt);
            }
            appendUint32Be(block, blockIndex);
            u = QMessageAuthenticationCode::hash(block, password, QCryptographicHash::Sha256);
        }
        QByteArray t = u;
        for (int i = 1; i < iterations; ++i) {
            u = QMessageAuthenticationCode::hash(u, password, QCryptographicHash::Sha256);
            for (int j = 0; j < hLen; ++j)
                t[j] ^= u[j];
        }
        const int copySize = qMin(hLen, dkLen - written);
        memcpy(out.data() + written, t.constData(), copySize);
        written += copySize;
        ++blockIndex;
    }
    return out;
}

QByteArray hmacCtrXor(const QByteArray &key, const QByteArray &iv, const QByteArray &data)
{
    QByteArray out;
    out.resize(data.size());
    const int blockSize = 32;
    QByteArray counter;
    counter.resize(blockSize);
    memcpy(counter.data(), iv.constData(), qMin(blockSize, iv.size()));

    for (int offset = 0; offset < data.size(); offset += blockSize) {
        const QByteArray keystream = QMessageAuthenticationCode::hash(counter, key, QCryptographicHash::Sha256);
        const int chunk = qMin(blockSize, data.size() - offset);
        for (int i = 0; i < chunk; ++i)
            out[offset + i] = data.at(offset + i) ^ keystream[i];

        for (int i = blockSize - 1; i >= 0; --i) {
            unsigned char v = static_cast<unsigned char>(counter[i]) + 1;
            counter[i] = static_cast<char>(v);
            if (v != 0)
                break;
        }
    }
    return out;
}

QByteArray hmacTag(const QByteArray &key, const QByteArray &iv, const QByteArray &cipher)
{
    QByteArray data;
    data.reserve(10 + iv.size() + cipher.size());
    data.append("BLACK-AUTH", 10);
    data.append(iv);
    data.append(cipher);
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256).left(kTagSize);
}

// Non-Windows key source: a random 256-bit master key stored with owner-only
// permissions. Not a hardware-backed secret store, but combined with PBKDF2 it
// is still a very large improvement over the previous hardcoded XOR key.
//
// Fail-safe: if the key file exists but is unreadable, empty, or the wrong
// size, we REFUSE to generate a replacement. Silently rotating the master key
// would orphan every vault already encrypted with the old key; returning an
// empty key makes callers abort the operation instead of destroying data.
QByteArray loadOrCreateMasterKey()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QLatin1String("/secrets");
    QDir().mkpath(dir);
    const QString path = dir + QLatin1String("/black_vault.key");

    if (QFileInfo::exists(path)) {
        QFile in(path);
        if (!in.open(QIODevice::ReadOnly))
            return {};          // exists but unreadable: never rotate silently
        const QByteArray key = in.readAll();
        if (key.size() == kKeySize)
            return key;
        return {};              // corrupt/partial key: fail safe, no rotation
    }

    const QByteArray key = randomBytes(kKeySize);
    if (OSPaths::writeFileAtomic(path, key)) {
#if !defined(Q_OS_WIN)
        (void)::chmod(path.toUtf8().constData(), S_IRUSR | S_IWUSR);
#else
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
#endif
    }
    return key;
}

QByteArray hmacCtrEncryptBlob(const QByteArray &plain)
{
    const QByteArray master = loadOrCreateMasterKey();
    if (master.isEmpty())
        return {};
    const QByteArray salt = randomBytes(kSaltSize);
    const QByteArray iv = randomBytes(kIvSize);
    const QByteArray key = pbkdf2HmacSha256(master, salt, kPbkdf2Iterations, kKeySize, true);
    const QByteArray cipher = hmacCtrXor(key, iv, plain);
    const QByteArray tag = hmacTag(key, iv, cipher);

    QJsonObject obj;
    obj.insert(QStringLiteral("kdf"), QStringLiteral("pbkdf2-hmac-sha256-rfc2898"));
    obj.insert(QStringLiteral("iter"), kPbkdf2Iterations);
    obj.insert(QStringLiteral("salt"), QString::fromLatin1(salt.toBase64()));
    obj.insert(QStringLiteral("iv"), QString::fromLatin1(iv.toBase64()));
    obj.insert(QStringLiteral("data"), QString::fromLatin1(cipher.toBase64()));
    obj.insert(QStringLiteral("tag"), QString::fromLatin1(tag.toBase64()));
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray hmacCtrDecryptBlob(const QByteArray &envelope)
{
    const QJsonDocument doc = QJsonDocument::fromJson(envelope);
    if (!doc.isObject())
        return {};
    const QJsonObject obj = doc.object();
    const QByteArray salt = QByteArray::fromBase64(obj.value(QStringLiteral("salt")).toString().toLatin1());
    const QByteArray iv = QByteArray::fromBase64(obj.value(QStringLiteral("iv")).toString().toLatin1());
    const QByteArray cipher = QByteArray::fromBase64(obj.value(QStringLiteral("data")).toString().toLatin1());
    const QByteArray tag = QByteArray::fromBase64(obj.value(QStringLiteral("tag")).toString().toLatin1());
    if (salt.isEmpty() || iv.isEmpty() || cipher.isEmpty() || tag.isEmpty())
        return {};

    const QByteArray master = loadOrCreateMasterKey();
    if (master.isEmpty())
        return {};

    // Versioned key derivation: RFC 2898 ordering for new vaults, the legacy
    // (password-prepended) ordering for vaults written by earlier builds.
    // An unknown kdf name is never guessed at; it fails authentication.
    const QString kdf = obj.value(QStringLiteral("kdf")).toString();
    bool rfc2898Message;
    if (kdf == QStringLiteral("pbkdf2-hmac-sha256-rfc2898")) {
        rfc2898Message = true;
    } else if (kdf.isEmpty() || kdf == QStringLiteral("pbkdf2-hmac-sha256")) {
        rfc2898Message = false;
    } else {
        return {};
    }

    const QByteArray key = pbkdf2HmacSha256(master, salt, kPbkdf2Iterations, kKeySize, rfc2898Message);
    if (hmacTag(key, iv, cipher) != tag)
        return {}; // authentication failed: tampered or wrong key
    return hmacCtrXor(key, iv, cipher);
}

#if defined(Q_OS_WIN)
QByteArray dpapiEncrypt(const QByteArray &plain)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()));
    in.cbData = DWORD(plain.size());

    DATA_BLOB entropy;
    entropy.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(kDpapiEntropy));
    entropy.cbData = DWORD(sizeof(kDpapiEntropy) - 1);

    DATA_BLOB out = { nullptr, 0 };
    if (!CryptProtectData(&in, L"BLACK password vault", &entropy, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &out))
        return {};
    QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return result;
}

QByteArray dpapiDecrypt(const QByteArray &blob)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(blob.constData()));
    in.cbData = DWORD(blob.size());

    DATA_BLOB entropy;
    entropy.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(kDpapiEntropy));
    entropy.cbData = DWORD(sizeof(kDpapiEntropy) - 1);

    DATA_BLOB out = { nullptr, 0 };
    if (CryptUnprotectData(&in, nullptr, &entropy, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
        LocalFree(out.pbData);
        return result;
    }

    out = { nullptr, 0 };
    if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
        LocalFree(out.pbData);
        return result;
    }

    return {};
}
#endif

} // namespace

namespace VaultCrypto {

bool isEnvelope(const QByteArray &data)
{
    return data.size() >= 6
           && memcmp(data.constData(), kMagic, sizeof(kMagic)) == 0
           && data.at(4) == kVersion;
}

QByteArray encrypt(const QByteArray &plaintext)
{
#if defined(Q_OS_WIN)
    const QByteArray blob = dpapiEncrypt(plaintext);
#else
    const QByteArray blob = hmacCtrEncryptBlob(plaintext);
#endif
    if (blob.isEmpty())
        return {};

    QByteArray envelope;
    envelope.append(kMagic, sizeof(kMagic));
    envelope.append(char(kVersion));
#if defined(Q_OS_WIN)
    envelope.append(kCipherDpapi);
#else
    envelope.append(kCipherHmacCtr);
#endif
    envelope.append(blob.toBase64());
    return envelope;
}

QByteArray decrypt(const QByteArray &envelope)
{
    if (!isEnvelope(envelope))
        return {};
    const char cipher = envelope.at(5);
    const QByteArray payload = QByteArray::fromBase64(envelope.mid(6));
    if (payload.isEmpty())
        return {};
#if defined(Q_OS_WIN)
    if (cipher == kCipherDpapi)
        return dpapiDecrypt(payload);
#else
    if (cipher == kCipherHmacCtr)
        return hmacCtrDecryptBlob(payload);
#endif
    return {};
}

} // namespace VaultCrypto
