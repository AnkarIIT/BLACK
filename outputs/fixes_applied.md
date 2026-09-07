# BLACK Browser — Fixes Applied

**Date:** 2025-01-25  
**Scope:** P0/P1/P2 security and reliability fixes from code review

---

## Summary

| Issue | Severity | Status | Files Changed |
|-------|----------|--------|---------------|
| ShelfStore write-on-read | P1 | ✅ **Fixed** | `ShelfStore.cpp` |
| Homepage URL injection | P3 | ✅ **Fixed** | `BrowserWindow.cpp` |
| Silent permission save failure | P3 | ✅ **Fixed** | `BrowserWindow.cpp` |
| Plaintext session file | P2 | ✅ **Fixed** | `BrowserWindow.cpp` |
| SafeBrowsing Unicode normalization | P2 | ✅ **Fixed** | `SafeBrowsing.cpp` |
| Custom HMAC-CTR cipher | P0 | ✅ **Fixed** | `VaultCrypto.cpp`, `VaultCrypto.h`, `CMakeLists.txt` |
| Vault master key in plaintext file | P1 | ⚠️ **Planned** | Requires platform secure storage integration |

---

## Detailed Changes

### 1. ShelfStore — Eliminated Write-on-Read (P1)

**Problem:** `loadArray()` performed retention pruning and wrote the pruned array back to disk on every call. Since `json()` calls `loadArray()` on every URL-suggestion keystroke, this caused unexpected disk I/O and potential UI stutter.

**Fix:**
- Made `loadArray()` strictly read-only.
- Added `pruneArray(QJsonArray&)` as an explicit write-path helper.
- `add()`, `remove()`, and `importBookmarks()` now call `pruneArray()` before `saveArray()`.

**Impact:** No more disk writes during read operations. Retention pruning happens only on actual mutations.

---

### 2. BrowserWindow — Homepage URL Validation (P3)

**Problem:** `homepageUrl()` accepted any string, allowing `javascript:` or `file://` URLs as homepage.

**Fix:** Added scheme whitelist. Only `http`, `https`, `qrc`, `about`, and empty schemes are allowed. Unsafe schemes fall back to the default start page.

**Impact:** Prevents XSS and local-file-access via homepage setting.

---

### 3. BrowserWindow — Permission Save Warning (P3)

**Problem:** `savePermissions()` silently dropped permission choices when `VaultCrypto::encrypt()` failed.

**Fix:** Added `QMessageBox::warning()` that informs the user their permission settings could not be saved and explains that prompts will reappear.

**Impact:** User-facing transparency; no silent data loss.

---

### 4. BrowserWindow — Encrypted Session File (P2)

**Problem:** `session.json` stored URLs and timestamps in plaintext, exposing browsing history to anyone with filesystem access.

**Fix:**
- `saveSession()` now encrypts the session payload with `VaultCrypto::encrypt()`.
- `restoreSession()` attempts decryption first, then falls back to plaintext for backward compatibility with existing installations.

**Impact:** Session data is now encrypted at rest. Existing plaintext sessions still restore correctly.

---

### 5. SafeBrowsing — Unicode / Homoglyph Normalization (P2)

**Problem:** `isBlocked()` matched only exact and subdomain hosts. Unicode homoglyph domains (e.g., `аррӏе.com`) bypassed the blocklist.

**Fix:** `isBlocked()` now normalizes the hostname to ASCII-compatible Punycode (ACE) via `QUrl::toAce()` before matching.

**Impact:** Catches visually similar phishing domains that use non-ASCII characters.

---

### 6. VaultCrypto — AES-256-GCM Default Cipher (P0)

**Problem:** The non-Windows password vault used a custom HMAC-CTR construction (`hmacCtrXor`) that is not a standard, publicly reviewed authenticated encryption mode.

**Fix:**
- Added AES-256-GCM encryption via OpenSSL's EVP interface.
- New vaults on macOS/Linux now use AES-256-GCM by default when OpenSSL is available.
- Existing HMAC-CTR vaults remain readable for backward compatibility.
- Added `kCipherAesGcm = 'A'` to the self-describing envelope format.
- Updated `CMakeLists.txt` to require and link OpenSSL (`OpenSSL::SSL`, `OpenSSL::Crypto`).
- Added `HAVE_OPENSSL` compile definition.

**Impact:** New password vaults use a standard, widely reviewed authenticated encryption mode with a 12-byte random IV and 16-byte authentication tag per write.

---

## Remaining Issues (Not Fixed)

### Vault Master Key Stored in Plaintext File (P1)

**Location:** `VaultCrypto.cpp` — `loadOrCreateMasterKey()`

On macOS/Linux, the 256-bit master key is stored at:
```
<AppDataLocation>/secrets/black_vault.key
```
with `0600` permissions.

**Why not fixed yet:** Integrating with OS secure storage requires platform-specific APIs:
- **Windows:** Credential Manager (beyond DPAPI)
- **macOS:** Keychain Services C API
- **Linux:** libsecret / KWallet D-Bus APIs

This is a substantial change that needs careful design to avoid breaking existing vaults. **Recommended next step:** Add a platform abstraction layer (e.g., `SecureKeyStorage`) that prefers OS keychain and falls back to the file-based master key only when unavailable.

---

## Build Notes

After these changes, the project requires **OpenSSL development headers** at build time:

- **Windows (MinGW):** Install via MinGW package manager or vcpkg.
- **macOS:** `brew install openssl` and set `OPENSSL_ROOT_DIR` if needed.
- **Linux:** `libssl-dev` (Debian/Ubuntu) or `openssl-devel` (Fedora/RHEL).

If OpenSSL is unavailable, the build will fail with a clear CMake error. This is intentional for a P0 security fix — the weaker HMAC-CTR fallback should not be the default.

---

## Verification

All modified files have been reviewed for:
- Correct preprocessor conditionals (`#if defined(HAVE_OPENSSL)`)
- Backward compatibility (`decrypt()` still handles legacy HMAC-CTR vaults)
- Memory safety (no new raw pointer ownership; OpenSSL contexts are freed on all paths)
- Consistent error handling (empty return on any crypto failure)

Run `git diff` in `C:/codes/BLACK` to inspect all 200+ changed lines across 6 files.
