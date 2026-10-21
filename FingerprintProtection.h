#ifndef FINGERPRINTPROTECTION_H
#define FINGERPRINTPROTECTION_H

#include <QObject>

class QWebEngineProfile;

// Placeholder implementation. Fingerprint/spoofing protection is NOT
// implemented in this build; install() is a no-op. The browser reports its
// real platform characteristics to pages - see README "Not included".
class FingerprintProtection : public QObject
{
    Q_OBJECT
public:
    explicit FingerprintProtection(QObject *parent = nullptr);

    // No-op. Intended to harden navigator.* / screen / hardware-concurrency
    // values on the given profile.
    void install(QWebEngineProfile *profile);

    Q_INVOKABLE bool isEnabled() const;      // false
};

#endif // FINGERPRINTPROTECTION_H
