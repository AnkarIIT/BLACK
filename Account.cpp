#include "Account.h"
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
}

Account::Account(QObject *parent)
    : QObject(parent)
    , m_signedIn(false)
    , m_name(QStringLiteral("Guest"))
    , m_authMethod(QStringLiteral("none"))
{
    load();
}

void Account::signIn(const QString &name)
{
    m_signedIn = true;
    m_name = name.trimmed().isEmpty() ? QStringLiteral("Guest") : name.trimmed();
    m_authMethod = QStringLiteral("local");
    save();
    emit changed();
}

void Account::signOut()
{
    m_signedIn = false;
    m_name = QStringLiteral("Guest");
    m_authMethod = QStringLiteral("none");
    save();
    emit changed();
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
    m_authMethod = o.value(QStringLiteral("authMethod")).toString(QStringLiteral("none"));
}

void Account::save()
{
    QJsonObject o;
    o[QStringLiteral("signedIn")] = m_signedIn;
    o[QStringLiteral("name")] = m_name;
    o[QStringLiteral("authMethod")] = m_authMethod;
    QFile file(accountFile());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(o).toJson());
}
