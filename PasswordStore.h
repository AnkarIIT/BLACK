#ifndef PASSWORDSTORE_H
#define PASSWORDSTORE_H

#include <QObject>
#include <QString>
#include <QVariantList>

// Local password manager, exposed to the QWebChannel and to injected page
// scripts. Persists credentials to <AppDataLocation>/passwords.json with a
// light XOR+Base64 obfuscation (NOT real encryption — used to avoid storing
// plaintext on disk). No cloud sync; data stays on this device.
class PasswordStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString json READ json NOTIFY changed)
    Q_PROPERTY(QString neverSaveJson READ neverSaveJson NOTIFY changed)

public:
    explicit PasswordStore(QObject *parent = nullptr);

    QString json() const;
    QString neverSaveJson() const;

    Q_INVOKABLE void save(const QString &host, const QString &username, const QString &password);
    Q_INVOKABLE void remove(const QString &host, const QString &username);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE QVariantList entriesFor(const QString &host) const;
    Q_INVOKABLE bool isNeverSave(const QString &host) const;
    Q_INVOKABLE void setNeverSave(const QString &host, bool neverSave);

signals:
    void changed();

private:
    QJsonArray loadArray() const;
    void saveArray(const QJsonArray &array) const;
    QString obfuscate(const QString &s) const;
    QString deobfuscate(const QString &s) const;
};

#endif
