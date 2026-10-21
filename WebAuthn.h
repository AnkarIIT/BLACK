#ifndef WEBAUTHN_H
#define WEBAUTHN_H

#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QVariantList>

class QWebEnginePage;
class QWebEngineProfile;

// Placeholder implementation. WebAuthn / FIDO2 support (attestation, signing,
// resident credentials, user verification) is not implemented in this build.
//
// registerPage()/unregisterPage() only maintain a set of page pointers so the
// lifetime contract is visible; no QWebEngineInterceptor is attached, no
// authenticator responds and no assertion is ever created.
class WebAuthnManager : public QObject
{
    Q_OBJECT

public:
    explicit WebAuthnManager(QObject *parent = nullptr);
    ~WebAuthnManager() override;

    // Placeholder: records the page only. Nothing is attached to it.
    void registerPage(QWebEnginePage *page);

    // Placeholder: forgets the page only.
    void unregisterPage(QWebEnginePage *page);

    // Whether WebAuthn/FIDO2 is usable. Always false: no authenticator is
    // implemented in this build, so Settings > Security reports "not
    // supported" instead of pretending passkeys work.
    Q_INVOKABLE bool isSupported() const;

    // Whether a platform authenticator (Windows Hello / Touch ID) is present.
    // Always false: none is wired up in this build.
    Q_INVOKABLE bool isPlatformAuthenticatorAvailable() const;

    // Authenticator metadata for the settings pane. Returns an empty map (no
    // AAGUID) because no authenticator exists here.
    Q_INVOKABLE QVariantMap getAuthenticatorInfo() const;

    // Resident credentials known to this build. Always empty: nothing is ever
    // registered or asserted.
    Q_INVOKABLE QVariantList listCredentials() const;

private:
    // Recorded for lifetime bookkeeping; never dereferenced. Uses raw pointers
    // because this stub cannot guarantee a QObject connection would be made.
    QList<QWebEnginePage *> m_pages;
};

#endif // WEBAUTHN_H
