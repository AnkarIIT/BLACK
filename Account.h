#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QObject>
#include <QString>

// Honest, fully-local "sign in": BLACK has no backend and no registered OAuth
// client_id, so cloud sign-in (Apple/Google) is not wired up. This object
// stores a local profile name and remembers that a (local) account exists.
class Account : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QString authMethod READ authMethod NOTIFY changed)

public:
    explicit Account(QObject *parent = nullptr);

    bool signedIn() const { return m_signedIn; }
    QString name() const { return m_name; }
    QString authMethod() const { return m_authMethod; } // "local" | "none"

    Q_INVOKABLE void signIn(const QString &name);
    Q_INVOKABLE void signOut();

signals:
    void changed();

private:
    void load();
    void save();

    bool m_signedIn;
    QString m_name;
    QString m_authMethod;
};

#endif
