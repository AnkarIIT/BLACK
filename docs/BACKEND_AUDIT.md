# BLACK Browser — Backend Audit

Focused backend-only follow-up to the full project audit.  
Scope: `BrowserWindow.cpp`, `VaultCrypto.cpp`, `PasswordStore.cpp`, `OAuthManager.cpp`, `ShelfStore`, `Account`, and related bridge registration.

## High-Impact Findings

### Data-loss risk chain
- `VaultCrypto` corrupt/empty master-key handling can silently regenerate and overwrite encrypted vaults.
- `PasswordStore` auth failure currently collapses to “empty vault” behavior; next save overwrites real encrypted data.
- Non-atomic JSON writes across vault/services/accounts/settings/shelf lack crash-safe temp-file + fsync discipline.

### OAuth robustness
- PKCE `verifier`/`state` must use `QRandomGenerator::system()`, not `global()`.
- `startAuth()` listener path can sever callbacks on re-entry.
- State validation currently accepts callbacks without matching state.

### PBKDF2 drift
- Current derivation prepends the password into the HMAC message. Self-consistent today, but non-standard vs RFC 2898 and risky for future migration/interop.

## Recommended Fix Order
1. Make vault writes atomic temp-file + fsync.
2. Separate “auth failed” from “empty vault” in `PasswordStore`.
3. OAuth: system RNG, listener lifecycle, strict state check.
4. PBKDF2: align with RFC 2898 message format or document the deviation as intentional + versioned.

## Status (all recommended-order items applied)
- **Atomic writes — DONE.** `OSPaths::writeFileAtomic()` (temp file in same dir, `flush` + `fsync`/`FlushFileBuffers`, atomic `rename`/`MoveFileExW(REPLACE_EXISTING)`, best-effort parent-dir fsync on POSIX) now backs every data store: `passwords.json`, `never_save.json`, vault key, `account.json`, `accounts.json`, `.first_run_done`, bookmarks/history `store.json`, `settings.json`, `theme`, OAuth connected accounts, `privacy.json`, `session.json`, and encrypted `permissions.json`.
- **Vault key fail-safe — DONE.** `loadOrCreateMasterKey()` never silently rotates an existing-but-corrupt/unreadable key file; it returns empty so callers abort instead of orphaning the vault. New key files are written atomically with owner-only permissions.
- **Auth-failed ≠ empty vault — DONE.** `PasswordStore` tracks a `decryptFailed` state; while set, `save()`/`remove()`/`clearAll()`/`saveArray()` refuse to write, so a failed decrypt can never be mistaken for an empty vault and overwritten. Exposed as `passwords.decryptFailed` (NOTIFY `changed`); settings shows a warning banner and disables "Remove All".
- **Encryption fail-closed — DONE.** `OAuthManager::saveConnected()` and `BrowserWindow::savePermissionChoices()` no longer fall back to plaintext or empty writes when the master key is unavailable.
- **OAuth randomness (M11) — DONE.** PKCE verifier and OAuth `state` now come from `QRandomGenerator::system()` instead of `global()`.
- **OAuth listener lifecycle (M12) — DONE.** Removed the blanket `m_server->disconnect()` in `setupLocalListener()` that severed the `newConnection` handler, permanently killing callback processing on the next `startAuth()` re-entry. `close()` clears the pending queue; in-flight sockets are gated by the path/state check.
- **OAuth callback verification (M13) — DONE.** Callbacks are only accepted when the request targets the exact `/callback` path AND carries a `state` token equal to the active flow's. Missing, wrong-path, or stale callbacks are rejected with an error page and do not tear the active flow down (a forged or stale probe can no longer kill a re-auth loop, and fixed-port squatting cannot complete without the state token).
- **PBKDF2 RFC 2898 (M8) — DONE.** `pbkdf2HmacSha256()` now uses the standard `S || INT_32_BE(i)` U_1 message; new vaults are tagged `kdf: "pbkdf2-hmac-sha256-rfc2898"`. `decrypt()` still reads vaults written with the legacy password-prepended ordering (`kdf: "pbkdf2-hmac-sha256"` or missing) and refuses unknown kdf names. Verified against RFC 7914 PBKDF2-HMAC-SHA256 vectors, round-trip, legacy compat, and unknown-kdf rejection via a standalone test.

## Notes
- `VaultCrypto` TOCTOU and DPAPI entropy issues were already fixed upstream in this tree.
- Onboarding MainWorld bridge exposure remains; `theme` is retained as benign, `bookmarks` removed from onboarding context.
- Compile-verified on Linux (Qt 6.8.0); full link blocked only by missing system OpenGL dev libs in this environment.
