#include "Account.h"
#include "OSPaths.h"
#include "VaultCrypto.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QStringList>

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

QByteArray readEncryptedFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray raw = file.readAll();
    // Fail CLOSED. Accepting a non-envelope file as-is meant a hand-written
    // account.json could inject arbitrary profile fields (name, email and
    // signedIn:true) that the UI then renders as a genuine signed-in profile.
    // Only an authenticated envelope is trusted.
    if (!VaultCrypto::isEnvelope(raw)) {
        qWarning("account.json is not a vault envelope - refusing to load it as plaintext");
        return {};
    }
    return VaultCrypto::decrypt(raw);
}

bool writeEncryptedFile(const QString &path, const QByteArray &plain)
{
    const QByteArray blob = VaultCrypto::encrypt(plain);
    if (blob.isEmpty())
        return false;
    // account.json carries the user's name and email; restrict it to the owner
    // on POSIX. (On Windows QFile permissions only toggle the read-only
    // attribute, so protection there comes from DPAPI at rest plus the
    // per-user %APPDATA% ACL rather than the mode bits.)
    return OSPaths::writeFileAtomic(path, blob, QFile::ReadOwner | QFile::WriteOwner);
}
} // namespace

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
    // The provider arrives over the QWebChannel bridge. Without an allowlist a
    // caller could store an arbitrary string in authMethod, which the UI then
    // renders as the account's identity source. Only the three documented
    // identity providers are accepted; anything else is treated as local.
    static const QStringList kAllowedProviders = {
        QStringLiteral("google"), QStringLiteral("apple"), QStringLiteral("microsoft")
    };
    const QString normalized = provider.trimmed().toLower();

    m_signedIn = true;
    m_name = name.trimmed().isEmpty() ? QStringLiteral("Guest") : name.trimmed();
    m_authMethod = kAllowedProviders.contains(normalized) ? normalized : QStringLiteral("local");
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
    if (!OSPaths::writeFileAtomic(firstRunMarker(), QByteArrayLiteral("1"))) {
        qWarning() << "Failed to write first-run marker";
    }
}

void Account::load()
{
    const QByteArray data = readEncryptedFile(accountFile());
    if (data.isEmpty())
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
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
    const QByteArray payload = QJsonDocument(o).toJson();
    if (!writeEncryptedFile(accountFile(), payload)) {
        qWarning() << "Failed to save account data";
        return;
    }
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
    const QByteArray data = readEncryptedFile(accountsFile());
    if (data.isEmpty())
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray())
        m_accounts = doc.array();
}

void Account::saveAccounts() const
{
    const QByteArray payload = QJsonDocument(m_accounts).toJson();
    if (!writeEncryptedFile(accountsFile(), payload)) {
        qWarning() << "Failed to save accounts list";
        return;
    }
}
