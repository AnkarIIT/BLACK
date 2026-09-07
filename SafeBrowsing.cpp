#include "SafeBrowsing.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonObject>

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
    : m_net(new QNetworkAccessManager(this))
{
    for (const char *const host : kBlockedHosts)
        m_blocked.insert(QString::fromLatin1(host));
    loadExtraList();

    // Set up periodic refresh timer (every 24 hours)
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(24 * 60 * 60 * 1000); // 24 hours
    connect(m_refreshTimer, &QTimer::timeout, this, &SafeBrowsing::refreshBlocklists);
    m_refreshTimer->start();

    // Also refresh on startup (with a short delay to not block UI)
    QTimer::singleShot(5000, this, &SafeBrowsing::refreshBlocklists);
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
        QWriteLocker locker(&m_lock);
        for (const QJsonValue &v : doc.array()) {
            const QString host = v.toString().trimmed().toLower();
            if (!host.isEmpty())
                m_blocked.insert(host);
        }
    }
    emit changed();
}

void SafeBrowsing::saveExtraList()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + QStringLiteral("safebrowsing.json"));
    if (!file.open(QIODevice::WriteOnly))
        return;
    QWriteLocker locker(&m_lock);
    QJsonArray array;
    for (const QString &host : m_blocked) {
        // Only save non-embedded hosts (those not in kBlockedHosts)
        bool isEmbedded = false;
        for (const char *const embedded : kBlockedHosts) {
            if (host == QLatin1String(embedded)) {
                isEmbedded = true;
                break;
            }
        }
        if (!isEmbedded)
            array.append(host);
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void SafeBrowsing::allow(const QString &host)
{
    {
        QWriteLocker locker(&m_lock);
        m_allowed.insert(host.trimmed().toLower());
    }
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
    QString host = url.host().toLower();
    // Normalize Unicode / homoglyph domains to ASCII-compatible Punycode so
    // that visually similar phishing domains are still caught.
    const QByteArray ace = QUrl::toAce(url.host());
    if (!ace.isEmpty())
        host = QString::fromLatin1(ace).toLower();
    QReadLocker locker(&m_lock);
    if (m_allowed.contains(host))
        return false;
    return hostMatches(host);
}

void SafeBrowsing::refreshBlocklists()
{
    // List of blocklist sources to fetch
    const QVector<QUrl> sources = {
        QUrl(QStringLiteral("https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts")),
        // Add more sources as needed:
        // QUrl(QStringLiteral("https://phishing.army/download/phishing_army_blocklist.txt")),
    };

    int completed = 0;
    int total = sources.size();
    bool anySuccess = false;
    QString errorMessages;

    for (const QUrl &sourceUrl : sources) {
        QNetworkRequest request(sourceUrl);
        QNetworkReply *reply = m_net->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, sourceUrl, &completed, total, &anySuccess, &errorMessages]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                errorMessages += QStringLiteral("Failed to fetch %1: %2\n").arg(sourceUrl.toString(), reply->errorString());
            } else {
                const QByteArray data = reply->readAll();
                mergeBlocklist(data, sourceUrl.toString());
                anySuccess = true;
            }
            completed++;
            if (completed == total) {
                if (anySuccess) {
                    saveExtraList();
                    emit blocklistsRefreshed(true, QStringLiteral("Blocklists refreshed successfully"));
                } else {
                    emit blocklistsRefreshed(false, QStringLiteral("Blocklist refresh failed: ") + errorMessages);
                }
            }
        });
    }
}

void SafeBrowsing::mergeBlocklist(const QByteArray &data, const QString &source)
{
    // Parse various blocklist formats:
    // - hosts file format: "0.0.0.0 example.com"
    // - plain domain list: "example.com"
    // - JSON array: ["example.com", "test.com"]
    QWriteLocker locker(&m_lock);

    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        // Skip comments and empty lines
        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;

        // Check if it's JSON
        if (trimmed.startsWith('[')) {
            QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &v : doc.array()) {
                    QString host = v.toString().trimmed().toLower();
                    if (!host.isEmpty())
                        m_blocked.insert(host);
                }
            }
            continue;
        }

        // Parse hosts file format or plain domain
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QString host;
        if (parts.size() >= 2) {
            // hosts file format: IP domain
            host = parts[1];
        } else {
            // plain domain
            host = parts[0];
        }

        host = host.trimmed().toLower();
        if (!host.isEmpty() && host != QLatin1String("localhost")) {
            m_blocked.insert(host);
        }
    }

    emit changed();
}

QUrl SafeBrowsing::warningUrl(const QUrl &original)
{
    QUrl warn(QStringLiteral("qrc:/safebrowsing_warning.html"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("url"), original.toString());
    warn.setQuery(query);
    return warn;
}
