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
#elif defined(Q_OS_MACOS)
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <secret/secret.h>
#endif

#if defined(HAVE_OPENSSL)
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace {

const char kMagic[4] = { 'B', 'A', 'K', 'V' };
const qint8 kVersion = 1;
const char kCipherDpapi = 'D';
const char kCipherHmacCtr = 'H';
const char kCipherAesGcm = 'A';

const int kKeySize = 32;         // 256-bit master / derived key
const int kSaltSize = 16;
const int kIvSize = 12;
const int kTagSize = 16;
const int kPbkdf2Iterations = 150000;

#if defined(Q_OS_WIN)
const char kDpapiEntropy[] = "BLACK_BROWSER_VAULT_ENTROPY_BLOCK_32";
#endif

// Forward declarations for functions in this namespace
QByteArray loadOrCreateMasterKeyFile();
QByteArray loadOrCreateMasterKey();

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

// Non-Windows key source: try OS keychain first (macOS Keychain, Linux libsecret),
// fall back to a random 256-bit master key stored with owner-only permissions.
// Not a hardware-backed secret store, but combined with PBKDF2 it
// is still a very large improvement over the previous hardcoded XOR key.
//
// Fail-safe: if the key file exists but is unreadable, empty, or the wrong
// size, we REFUSE to generate a replacement. Silently rotating the master key
// would orphan every vault already encrypted with the old key; returning an
// empty key makes callers abort the operation instead of destroying data.
QByteArray loadOrCreateMasterKey()
{
#if defined(Q_OS_MACOS)
    // Try to load from macOS Keychain first
    QByteArray key = loadKeyFromMacOSKeychain();
    if (!key.isEmpty())
        return key;
    // Fall back to file-based storage
    return loadOrCreateMasterKeyFile();
#elif defined(Q_OS_LINUX) && defined(HAVE_LIBSECRET)
    // Try to load from libsecret first
    QByteArray key = loadKeyFromLibsecret();
    if (!key.isEmpty())
        return key;
    // Fall back to file-based storage
    return loadOrCreateMasterKeyFile();
#else
    // Windows uses DPAPI, file-based storage not used for master key
    return loadOrCreateMasterKeyFile();
#endif
}

QByteArray loadOrCreateMasterKeyFile()
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
    // Persist with owner-only permissions (applied to the temp file before the
    // atomic rename, so 0600 survives on disk). If the write fails we must not
    // proceed: encrypting with a key that was never durably stored would leave
    // vault data unrecoverable after a restart. (Distinct from the fail-safe
    // above, which refuses to rotate an *existing* key file.)
    if (!OSPaths::writeFileAtomic(path, key, QFile::ReadOwner | QFile::WriteOwner))
        return {};
    return key;
}

#if defined(Q_OS_MACOS)
QByteArray loadKeyFromMacOSKeychain()
{
    const char *service = "BLACK Browser";
    const char *account = "master_key";

    CFStringRef serviceRef = CFStringCreateWithCString(kCFAllocatorDefault, service, kCFStringEncodingUTF8);
    CFStringRef accountRef = CFStringCreateWithCString(kCFAllocatorDefault, account, kCFStringEncodingUTF8);

    CFDictionaryRef query = CFDictionaryCreate(kCFAllocatorDefault,
        (const void **)&kSecClass, (const void **)&kSecClassGenericPassword,
        (const void **)&kSecAttrService, (const void **)&serviceRef,
        (const void **)&kSecAttrAccount, (const void **)&accountRef,
        (const void **)&kSecReturnData, (const void **)kCFBooleanTrue,
        (const void **)&kSecMatchLimit, (const void **)kSecMatchLimitOne,
        nullptr);

    CFTypeRef result = nullptr;
    OSStatus status = SecItemCopyMatching(query, &result);
    CFRelease(query);
    CFRelease(serviceRef);
    CFRelease(accountRef);

    if (status == errSecSuccess && result) {
        CFDataRef dataRef = (CFDataRef)result;
        QByteArray key(reinterpret_cast<const char *>(CFDataGetBytePtr(dataRef)), CFDataGetLength(dataRef));
        CFRelease(dataRef);
        if (key.size() == kKeySize)
            return key;
    }

    // Key not found or wrong size, generate and store new one
    QByteArray newKey = randomBytes(kKeySize);
    CFDataRef dataRef = CFDataCreate(kCFAllocatorDefault,
        reinterpret_cast<const UInt8 *>(newKey.constData()), newKey.size());

    CFDictionaryRef addQuery = CFDictionaryCreate(kCFAllocatorDefault,
        (const void **)&kSecClass, (const void **)&kSecClassGenericPassword,
        (const void **)&kSecAttrService, (const void **)&serviceRef,
        (const void **)&kSecAttrAccount, (const void **)&accountRef,
        (const void **)&kSecValueData, (const void **)&dataRef,
        (const void **)&kSecAttrAccessible, (const void **)kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
        nullptr);

    status = SecItemAdd(addQuery, nullptr);
    CFRelease(addQuery);
    CFRelease(dataRef);
    CFRelease(serviceRef);
    CFRelease(accountRef);

    if (status == errSecSuccess)
        return newKey;

    return {};
}
#if defined(Q_OS_LINUX) && defined(HAVE_LIBSECRET)
QByteArray loadKeyFromLibsecret()
{
    GError *error = nullptr;
    SecretSchema schema = {
        "com.black.browser.master_key",
        SECRET_SCHEMA_NONE,
        {
            { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
            { "account", SECRET_SCHEMA_ATTRIBUTE_STRING },
            { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
        }
    };

    gchar *secret = secret_password_lookup_sync(&schema, nullptr, &error,
        "service", "BLACK Browser",
        "account", "master_key",
        nullptr);

    if (error) {
        g_error_free(error);
        return {};
    }

    if (secret) {
        QByteArray key(secret);
        g_free(secret);
        if (key.size() == kKeySize)
            return key;
    }

    // Key not found, generate and store new one
    QByteArray newKey = randomBytes(kKeySize);
    error = nullptr;
    gboolean stored = secret_password_store_sync(&schema, nullptr, &error,
        "service", "BLACK Browser",
        "account", "master_key",
        "secret", newKey.constData(),
        nullptr);

    if (error) {
        g_error_free(error);
        return {};
    }

    if (stored)
        return newKey;

    return {};
}
#endif

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

    DATA_BLOB out = { 0, nullptr };
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

    DATA_BLOB out = { 0, nullptr };
    if (CryptUnprotectData(&in, nullptr, &entropy, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
        LocalFree(out.pbData);
        return result;
    }

    out = { 0, nullptr };
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

#if defined(HAVE_OPENSSL)
QByteArray aesGcmEncryptBlob(const QByteArray &plain)
{
    const QByteArray master = loadOrCreateMasterKey();
    if (master.isEmpty())
        return {};

    QByteArray iv;
    iv.resize(12);
    if (RAND_bytes(iv.data(), 12) != 1)
        return {};

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return {};

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, master.constData(), iv.constData()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int len = 0;
    QByteArray ciphertext(plain.size(), 0);
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plain.constData(), plain.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int ciphertextLen = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    ciphertextLen += len;
    ciphertext.resize(ciphertextLen);

    QByteArray tag(16, 0);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    EVP_CIPHER_CTX_free(ctx);

    QJsonObject obj;
    obj.insert(QStringLiteral("kdf"), QStringLiteral("aes-256-gcm"));
    obj.insert(QStringLiteral("iv"), QString::fromLatin1(iv.toBase64()));
    obj.insert(QStringLiteral("data"), QString::fromLatin1(ciphertext.toBase64()));
    obj.insert(QStringLiteral("tag"), QString::fromLatin1(tag.toBase64()));
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray aesGcmDecryptBlob(const QByteArray &envelope)
{
    const QJsonDocument doc = QJsonDocument::fromJson(envelope);
    if (!doc.isObject())
        return {};
    const QJsonObject obj = doc.object();
    const QByteArray iv = QByteArray::fromBase64(obj.value(QStringLiteral("iv")).toString().toLatin1());
    const QByteArray ciphertext = QByteArray::fromBase64(obj.value(QStringLiteral("data")).toString().toLatin1());
    const QByteArray tag = QByteArray::fromBase64(obj.value(QStringLiteral("tag")).toString().toLatin1());
    if (iv.isEmpty() || ciphertext.isEmpty() || tag.isEmpty())
        return {};

    const QByteArray master = loadOrCreateMasterKey();
    if (master.isEmpty())
        return {};

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return {};

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, master.constData(), iv.constData()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int len = 0;
    QByteArray plaintext(ciphertext.size(), 0);
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.constData(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int plaintextLen = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {}; // authentication failed
    }
    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintextLen);
    return plaintext;
}
#endif

QByteArray encrypt(const QByteArray &plaintext)
{
    QByteArray blob;
#if defined(Q_OS_WIN)
    blob = dpapiEncrypt(plaintext);
#elif defined(HAVE_OPENSSL)
    blob = aesGcmEncryptBlob(plaintext);
    if (blob.isEmpty())
        return {};
#else
    blob = hmacCtrEncryptBlob(plaintext);
#endif
    if (blob.isEmpty())
        return {};

    QByteArray envelope;
    envelope.append(kMagic, sizeof(kMagic));
    envelope.append(char(kVersion));
#if defined(Q_OS_WIN)
    envelope.append(kCipherDpapi);
#elif defined(HAVE_OPENSSL)
    envelope.append(kCipherAesGcm);
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
#endif
#if defined(HAVE_OPENSSL)
    if (cipher == kCipherAesGcm)
        return aesGcmDecryptBlob(payload);
#endif
    if (cipher == kCipherHmacCtr)
        return hmacCtrDecryptBlob(payload);
    return {};
}

} // namespace VaultCrypto
} // namespace
