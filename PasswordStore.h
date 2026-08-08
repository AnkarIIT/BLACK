#ifndef PASSWORDSTORE_H
#define PASSWORDSTORE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QJsonArray>

// Local password manager, exposed to the QWebChannel and to the injected
// autofill content script. Persists credentials to <AppDataLocation>/passwords.json.
//
// The whole vault blob is encrypted before it touches the disk (see
// VaultCrypto: DPAPI on Windows, PBKDF2+HMAC-CTR fallback elsewhere), so the
// previous hardcoded-XOR "obfuscation" is gone.
//
// The full decrypted vault is never handed out over the bridge. Callers only
// get per-entry lookups:
//   - hosts()        -> host/username/timestamp metadata (no passwords)
//   - passwordFor()  -> the single password for one exact entry
//   - entriesFor()   -> one host's entries (used only by the isolated-world
//                       autofill script for the site being visited)
class PasswordStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString neverSaveJson READ neverSaveJson NOTIFY changed)

public:
    explicit PasswordStore(QObject *parent = nullptr);

    QString neverSaveJson() const;

    Q_INVOKABLE QVariantList hosts() const;
    Q_INVOKABLE QString passwordFor(const QString &host, const QString &username) const;
    Q_INVOKABLE QVariantList entriesFor(const QString &host) const;
    Q_INVOKABLE void save(const QString &host, const QString &username, const QString &password);
    Q_INVOKABLE void remove(const QString &host, const QString &username);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE bool isNeverSave(const QString &host) const;
    Q_INVOKABLE void setNeverSave(const QString &host, bool neverSave);

signals:
    void changed();

private:
    QJsonArray loadArray() const;
    void saveArray(const QJsonArray &array) const;
};

#endif
