#ifndef VAULTCRYPTO_H
#define VAULTCRYPTO_H

#include <QByteArray>
#include <QString>

// Whole-blob encryption for the local password vault.
//
//  - Windows: the vault is protected with DPAPI (CryptProtectData), the same
//    OS-level encryption that backs Windows Credential Manager. The key is
//    derived by the OS from the current user's credentials, so the data is
//    useless without the logged-in user account and requires no passphrase.
//  - macOS/Linux with OpenSSL: AES-256-GCM provides authenticated encryption
//    with a fresh random IV per vault write and a 16-byte authentication tag.
//  - macOS/Linux fallback (no OpenSSL): a random 256-bit master key stored
//    with owner-only permissions is combined with a fresh random salt via
//    PBKDF2-HMAC-SHA256. The derived key feeds a counter-mode keystream built
//    on HMAC-SHA256; a 16-byte HMAC tag authenticates the ciphertext+IV.
//
// The on-disk envelope is self-describing: "BAKV" magic + version + cipher id
// + base64 payload, so cipher selection is transparent to the store.
namespace VaultCrypto {

// Encrypts the serialized vault. Returns an empty QByteArray on failure.
QByteArray encrypt(const QByteArray &plaintext);

// Decrypts an envelope produced by encrypt(). Returns the plaintext, or an
// empty QByteArray if the data is malformed or fails authentication.
QByteArray decrypt(const QByteArray &envelope);

// True if the buffer looks like an encrypted envelope (vs. legacy plaintext).
bool isEnvelope(const QByteArray &data);

// Master key management (for backup/recovery/rotation)
// All functions return empty QByteArray on failure.

// Export the current master key as base64-encoded raw bytes (32 bytes).
// Use for offline backup. Returns empty on Windows (DPAPI) or if key unavailable.
QByteArray exportMasterKey();

// Import a master key from base64-encoded raw bytes (32 bytes).
// Overwrites the existing key file/keychain entry. Returns true on success.
// Use with extreme caution - importing wrong key makes existing vaults unreadable.
bool importMasterKey(const QByteArray &base64Key);

// Rotate the master key: generate new key, re-encrypt all vaults.
// Not yet implemented - requires coordination with all stores.
bool rotateMasterKey();

// Get information about the current key source.
// Returns: "dpapi" (Windows), "keychain" (macOS), "libsecret" (Linux), "file" (fallback)
QString getKeySource();

// Test if the OS credential service is available and working.
// Returns true if a test key can be stored and retrieved.
bool testKeychainAvailability();

// Check if a master key exists (in file or keychain).
bool hasMasterKey();

// Remove the master key (for testing/reset). Returns true on success.
// WARNING: This will make all encrypted vaults permanently unreadable!
bool removeMasterKey();

} // namespace VaultCrypto

#endif // VAULTCRYPTO_H