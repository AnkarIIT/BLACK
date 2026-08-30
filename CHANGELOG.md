# BLACK Browser — Changelog

## [Unreleased]

### Fixed
- **ShelfStore write-on-read bug**: `loadArray()` no longer writes to disk during read operations. Retention pruning is now performed explicitly in `add()`, `remove()`, and `importBookmarks()` before saving. This eliminates unexpected disk I/O on every URL-suggestion keystroke.
- **Homepage URL injection**: `BrowserWindow::homepageUrl()` now validates the URL scheme. Only `http`, `https`, `qrc`, `about`, and empty schemes are allowed. `javascript:` and `file://` homepages are rejected.
- **Silent permission save failure**: `BrowserWindow::savePermissions()` now shows a `QMessageBox` warning when `VaultCrypto::encrypt()` fails, so users understand why permission prompts reappear.
- **Plaintext session file**: `session.json` is now encrypted via `VaultCrypto::encrypt()`. `restoreSession()` attempts decryption first and falls back to plaintext for backward compatibility with existing installations.

### Security
- **SafeBrowsing Unicode normalization**: `SafeBrowsing::isBlocked()` now normalizes hostnames to ASCII-compatible Punycode (ACE) before matching, catching homoglyph and Unicode-based phishing domains.

### Planned (P0/P1 — requires further implementation)
- Replace custom HMAC-CTR vault cipher with AES-256-GCM or ChaCha20-Poly1305.
- Integrate vault master key with OS secure storage (Windows Credential Manager, macOS Keychain, Linux libsecret).
