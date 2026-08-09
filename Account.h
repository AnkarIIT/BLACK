#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>

// Honest, fully-local profile: BLACK has no cloud backend. The profile can be
// created locally (signIn) or via a real OAuth provider (signInWithOAuth), in
// which case the provider's verified name/email/avatar are stored locally.
// All storage stays on this device.
//
// Multiple profiles can be saved (accounts.json); one of them is the active
// profile (account.json, the single active profile the rest of the app reads).
class Account : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QString email READ email NOTIFY changed)
    Q_PROPERTY(QString avatar READ avatar NOTIFY changed)
    Q_PROPERTY(QString authMethod READ authMethod NOTIFY changed)

public:
    explicit Account(QObject *parent = nullptr);

    bool signedIn() const { return m_signedIn; }
    QString name() const { return m_name; }
    QString email() const { return m_email; }
    QString avatar() const { return m_avatar; }
    QString authMethod() const { return m_authMethod; } // "local" | "none" | "google" | ...

    // Saved accounts (accounts.json) as { "id", "provider", "name", "email",
    // "avatar" } objects, newest first.
    Q_INVOKABLE QJsonArray accountsJson() const;

    // Adds or updates a saved account and returns its id. When provider is a
    // sign-in provider ("google" | "apple" | "microsoft"), the id is stable
    // (provider + email) so per-account service connections keep working.
    Q_INVOKABLE QString saveAccount(const QString &provider, const QString &name,
                                    const QString &email = QString(),
                                    const QString &avatar = QString());

    // Makes a saved account the active profile and persists it to
    // account.json so the real browser picks it up.
    Q_INVOKABLE void selectAccount(const QString &id);

    Q_INVOKABLE void signIn(const QString &name);
    // Records a profile verified by a real OAuth provider; the verified
    // name/email/avatar are stored locally. Provider is one of
    // "google" | "apple" | "microsoft".
    Q_INVOKABLE void signInWithOAuth(const QString &provider, const QString &name,
                                     const QString &email = QString(),
                                     const QString &avatar = QString());
    Q_INVOKABLE void signOut();

    // Marks first-run onboarding as finished once the user actually completes
    // the login page (create profile / continue as guest), instead of when the
    // login page is merely shown. Writes <AppDataLocation>/.first_run_done.
    Q_INVOKABLE void completeOnboarding();

signals:
    void changed();

private:
    void load();
    void save();
    void loadAccounts();
    void saveAccounts() const;
    QJsonObject accountObject(const QString &id, const QString &provider,
                              const QString &name, const QString &email,
                              const QString &avatar) const;

    bool m_signedIn;
    QString m_name;
    QString m_email;
    QString m_avatar;
    QString m_authMethod;
    QJsonArray m_accounts;
};

#endif
