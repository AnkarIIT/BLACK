#include "SafeBrowsing.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QUrlQuery>

// Known phishing / credential-harvesting domains (illustrative embedded list).
const char *const kBlockedHosts[] = {
    "appleid-verify.com",
    "appleid-verification.com",
    "apple-security-alert.net",
    "update-apple-security.net",
    "paypal-secure-verify.com",
    "paypal-account-update.info",
    "netflix-billing-support.com",
    "netflix-account-hold.com",
    "bankofamerica-login-verify.com",
    "wellsfargo-secure-online.com",
    "chase-online-verification.com",
    "google-account-verify.com",
    "gmail-verification.info",
    "microsoft-account-alert.com",
    "outlook-security-check.com",
    "irs-tool-help.com",
    "dhl-express-parcel-tracking.com",
    "fedex-delivery-notice.com",
    "instagram-verification.com",
    "whatsapp-verification.net",
    "facebook-security-check.com",
    "secure-amazon-update.com",
    "amazon-gift-card-claim.com",
    "steam-community-gifts.com",
    "bitcoin-wallet-verify.com",
    "crypto-recovery-team.com",
    "telegram-verification-bot.net",
};

SafeBrowsing &SafeBrowsing::instance()
{
    static SafeBrowsing s;
    return s;
}

SafeBrowsing::SafeBrowsing()
{
    for (const char *const host : kBlockedHosts)
        m_blocked.insert(QString::fromLatin1(host));
    loadExtraList();
}

void SafeBrowsing::loadExtraList()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + QStringLiteral("safebrowsing.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isArray()) {
        for (const QJsonValue &v : doc.array()) {
            const QString host = v.toString().trimmed().toLower();
            if (!host.isEmpty())
                m_blocked.insert(host);
        }
    }
    emit changed();
}

void SafeBrowsing::allow(const QString &host)
{
    m_allowed.insert(host.trimmed().toLower());
    emit changed();
}

bool SafeBrowsing::hostMatches(const QString &host) const
{
    if (host.isEmpty())
        return false;
    for (const QString &d : m_blocked) {
        if (host == d || host.endsWith(QLatin1Char('.') + d))
            return true;
    }
    return false;
}

bool SafeBrowsing::isBlocked(const QUrl &url) const
{
    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
        return false;
    const QString host = url.host().toLower();
    if (m_allowed.contains(host))
        return false;
    return hostMatches(host);
}

QUrl SafeBrowsing::warningUrl(const QUrl &original)
{
    QUrl warn(QStringLiteral("qrc:/safebrowsing_warning.html"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("url"), original.toString());
    warn.setQuery(query);
    return warn;
}
