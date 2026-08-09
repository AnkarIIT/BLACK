#include "Account.h"
#include "OSPaths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

namespace {
QString accountFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("account.json");
}

QString accountsFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("accounts.json");
}

QString firstRunMarker()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral(".first_run_done");
}
}

Account::Account(QObject *parent)
    : QObject(parent)
    , m_signedIn(false)
    , m_name(QStringLiteral("Guest"))
    , m_email(QString())
    , m_avatar(QString())
    , m_authMethod(QStringLiteral("none"))
{
    load();
    loadAccounts();
}

QJsonArray Account::accountsJson() const
{
    return m_accounts;
}

QString Account::saveAccount(const QString &provider, const QString &name,
                             const QString &email, const QString &avatar)
{
    const QString p = provider.trimmed().isEmpty() ? QStringLiteral("local") : provider.trimmed();
    const QString displayName = name.trimmed().isEmpty() ? QStringLiteral("Guest") : name.trimmed();

    // Stable id for sign-in providers so per-account service connections can
    // survive app restarts. Local profiles key off their display name.
    QString id;
    if (p != QStringLiteral("local") && !email.trimmed().isEmpty())
        id = p + QLatin1Char(':') + email.trimmed();
    else
        id = p + QLatin1Char(':') + displayName;

    // Upsert into accounts.json (newest first).
    QJsonArray updated;
    updated.append(accountObject(id, p, displayName, email, avatar));
    for (int i = 0; i < m_accounts.size(); ++i) {
        const QJsonObject o = m_accounts.at(i).toObject();
        if (o.value(QStringLiteral("id")).toString() != id)
            updated.append(o);
    }
    m_accounts = updated;
    saveAccounts();
    return id;
}

void Account::selectAccount(const QString &id)
{
    for (int i = 0; i < m_accounts.size(); ++i) {
        const QJsonObject o = m_accounts.at(i).toObject();
        if (o.value(QStringLiteral("id")).toString() == id) {
            m_signedIn = true;
            m_name = o.value(QStringLiteral("name")).toString(QStringLiteral("Guest"));
            m_email = o.value(QStringLiteral("email")).toString();
            m_avatar = o.value(QStringLiteral("avatar")).toString();
            m_authMethod = o.value(QStringLiteral("provider")).toString(QStringLiteral("local"));
            save();
            emit changed();
            return;
        }
    }
}

void Account::signIn(const QString &name)
{
    m_signedIn = true;
    m_name = name.trimmed().isEmpty() ? QStringLiteral("Guest") : name.trimmed();
    m_authMethod = QStringLiteral("local");
    m_email.clear();
    m_avatar.clear();
    save();
    saveAccount(m_authMethod, m_name);
    emit changed();
}

void Account::signInWithOAuth(const QString &provider, const QString &name,
                              const QString &email, const QString &avatar)
{
    m_signedIn = true;
    m_name = name.trimmed().isEmpty() ? QStringLiteral("Guest") : name.trimmed();
    m_authMethod = provider.trimmed().isEmpty() ? QStringLiteral("local") : provider.trimmed();
    m_email = email;
    m_avatar = avatar;
    save();
    saveAccount(m_authMethod, m_name, m_email, m_avatar);
    emit changed();
}

void Account::signOut()
{
    m_signedIn = false;
    m_name = QStringLiteral("Guest");
    m_email.clear();
    m_avatar.clear();
    m_authMethod = QStringLiteral("none");
    save();
    emit changed();
}

void Account::completeOnboarding()
{
    OSPaths::writeFileAtomic(firstRunMarker(), QByteArrayLiteral("1"));
}

void Account::load()
{
    QFile file(accountFile());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;
    const QJsonObject o = doc.object();
    m_signedIn = o.value(QStringLiteral("signedIn")).toBool(false);
    m_name = o.value(QStringLiteral("name")).toString(QStringLiteral("Guest"));
    m_email = o.value(QStringLiteral("email")).toString();
    m_avatar = o.value(QStringLiteral("avatar")).toString();
    m_authMethod = o.value(QStringLiteral("authMethod")).toString(QStringLiteral("none"));
}

void Account::save()
{
    QJsonObject o;
    o[QStringLiteral("signedIn")] = m_signedIn;
    o[QStringLiteral("name")] = m_name;
    o[QStringLiteral("email")] = m_email;
    o[QStringLiteral("avatar")] = m_avatar;
    o[QStringLiteral("authMethod")] = m_authMethod;
    OSPaths::writeFileAtomic(accountFile(), QJsonDocument(o).toJson());
}

QJsonObject Account::accountObject(const QString &id, const QString &provider,
                                   const QString &name, const QString &email,
                                   const QString &avatar) const
{
    QJsonObject o;
    o[QStringLiteral("id")] = id;
    o[QStringLiteral("provider")] = provider;
    o[QStringLiteral("name")] = name;
    o[QStringLiteral("email")] = email;
    o[QStringLiteral("avatar")] = avatar;
    return o;
}

void Account::loadAccounts()
{
    QFile file(accountsFile());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isArray())
        m_accounts = doc.array();
}

void Account::saveAccounts() const
{
    OSPaths::writeFileAtomic(accountsFile(), QJsonDocument(m_accounts).toJson());
}
