#ifndef WEBAUTHN_H
#define WEBAUTHN_H

#include <QObject>
#include <QList>

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

private:
    // Recorded for lifetime bookkeeping; never dereferenced. Uses raw pointers
    // because this stub cannot guarantee a QObject connection would be made.
    QList<QWebEnginePage *> m_pages;
};

#endif // WEBAUTHN_H