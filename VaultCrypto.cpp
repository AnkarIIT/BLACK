#include "VaultCrypto.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
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
QByteArray pbkdf2HmacSha256(const QByteArray &password, const QByteArray &salt,
                            int iterations, int dkLen)
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
            block.append(password);
            block.append(salt);
            appendUint32Be(block, blockIndex);
            u = QMessageAuthenticationCode::hash(block, password, QCryptographicHash::Sha256);
        }
        QByteArray t = u;
        for (int i = 1; i < iterations; ++i) {
            u = QMessageAuthenticationCode::hash(u, password, QCryptographicHash::Sha256);
            for (int j = 0; j < hLen; ++j)
                t[j] ^= u[j];
        }
        const int copySize = qMin(hLen, dLen - written);
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
QByteArray loadOrCreateMasterKey()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QLatin1String("/secrets");
    QDir().mkpath(dir);
    const QString path = dir + QLatin1String("/black_vault.key");

    QFile in(path);
    if (in.open(QIODevice::ReadOnly)) {
        const QByteArray key = in.readAll();
        if (key.size() == kKeySize)
            return key;
    }
    const QByteArray key = randomBytes(kKeySize);
#if !defined(Q_OS_WIN)
    int fd = open(path.toUtf8().constData(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        ssize_t written = write(fd, key.constData(), key.size());
        (void)written;
        close(fd);
    }
#else
    QFile out(path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out.write(key);
        out.flush();
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    }
#endif
    return key;
}

QByteArray hmacCtrEncryptBlob(const QByteArray &plain)
{
    const QByteArray master = loadOrCreateMasterKey();
    if (master.isEmpty())
        return {};
    const QByteArray salt = randomBytes(kSaltSize);
    const QByteArray iv = randomBytes(kIvSize);
    const QByteArray key = pbkdf2HmacSha256(master, salt, kPbkdf2Iterations, kKeySize);
    const QByteArray cipher = hmacCtrXor(key, iv, plain);
    const QByteArray tag = hmacTag(key, iv, cipher);

    QJsonObject obj;
    obj.insert(QStringLiteral("kdf"), QStringLiteral("pbkdf2-hmac-sha256"));
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
    const QByteArray key = pbkdf2HmacSha256(master, salt, kPbkdf2Iterations, kKeySize);
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
