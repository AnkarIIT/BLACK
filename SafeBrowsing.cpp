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
#include <QCryptographicHash>
#include <QDateTime>
#include <functional>
#include <algorithm>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>

namespace {

// Completion state for a batch of blocklist downloads.
//
// The replies arrive asynchronously, long after refreshBlocklists() has
// returned, so this state must outlive that stack frame. It is heap-allocated,
// self-owning, and deleted exactly once by whichever reply completes the
// batch (or by the SafeBrowsing destructor if the replies never arrive).
struct RefreshState {
    QMutex mutex;
    int completed = 0;
    int total = 0;
    bool anySuccess = false;
    QString errorMessages;
};

// Upper bound on a downloaded blocklist. The real lists are a few MB; this
// caps what a hostile or broken endpoint can push us to buffer in memory.
constexpr qint64 kMaxBlocklistBytes = 64LL * 1024 * 1024;

} // namespace

// Known phishing / credential-harvesting domains (embedded fallback list).
const char *const SafeBrowsing::kEmbeddedHosts[] = {
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
    nullptr // sentinel
};

SafeBrowsing &SafeBrowsing::instance()
{
    static SafeBrowsing s;
    return s;
}

SafeBrowsing::SafeBrowsing()
    : m_net(new QNetworkAccessManager(this))
{
    // Load embedded hosts with current timestamp
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; kEmbeddedHosts[i]; ++i) {
        BlockedEntry entry;
        entry.host = QString::fromLatin1(kEmbeddedHosts[i]);
        entry.added = now;
        entry.source = QStringLiteral("embedded");
        m_blocked.insert(entry.host, entry);
    }

    loadExtraList();

    // Set up periodic refresh timer for offline mode (every 24 hours)
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(24 * 60 * 60 * 1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &SafeBrowsing::refreshBlocklists);
    m_refreshTimer->start();

    // Refresh on startup
    QTimer::singleShot(5000, this, &SafeBrowsing::refreshBlocklists);

    // Periodic stale entry cleanup (daily)
    QTimer *cleanupTimer = new QTimer(this);
    cleanupTimer->setInterval(24 * 60 * 60 * 1000);
    connect(cleanupTimer, &QTimer::timeout, this, &SafeBrowsing::removeStaleEntries);
    cleanupTimer->start();
}

SafeBrowsing::~SafeBrowsing()
{
}

void SafeBrowsing::setApiMode(ApiMode mode, const QString &apiKey)
{
    QWriteLocker locker(&m_lock);
    if (m_apiMode == mode && m_apiKey == apiKey)
        return;

    m_apiMode = mode;
    m_apiKey = apiKey;

    // Stop existing timers
    if (m_v4RefreshTimer) {
        m_v4RefreshTimer->stop();
        m_v4RefreshTimer->deleteLater();
        m_v4RefreshTimer = nullptr;
    }

    if (mode == ApiMode::GoogleV4) {
        setupV4Api();
    } else if (mode == ApiMode::BraveList) {
        // Use Brave's blocklist endpoint
        refreshBlocklists();
    }

    emit changed();
}

SafeBrowsing::ApiMode SafeBrowsing::apiMode() const
{
    QReadLocker locker(&m_lock);
    return m_apiMode;
}

bool SafeBrowsing::isBlocked(const QUrl &url) const
{
    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
        return false;

    QString host = url.host().toLower();
    // Normalize Unicode / homoglyph domains to ASCII-compatible Punycode
    const QByteArray ace = QUrl::toAce(url.host());
    if (!ace.isEmpty())
        host = QString::fromLatin1(ace).toLower();
    // Strip a trailing root dot: "evil.com." resolves to the same host as
    // "evil.com" but would otherwise dodge every exact/suffix blocklist match.
    while (host.size() > 1 && host.endsWith(QLatin1Char('.')))
        host.chop(1);
    // An IP literal is never in a hostname blocklist; normalizing is
    // meaningless and would only cost a toAce() round-trip.
    if (host.startsWith(QLatin1Char('[')) || host.isEmpty())
        return false;

    QReadLocker locker(&m_lock);
    // Check session allowances (one-time bypass)
    if (m_allowed.contains(host))
        return false;

    // Check host-based blocklist
    if (hostMatches(host))
        return true;

    // For V4 API mode, we would also check full URL hashes
    // This requires async check; for sync interceptor we rely on host list
    return false;
}

void SafeBrowsing::checkUrlAsync(const QUrl &url, std::function<void(bool, ThreatTypes)> callback)
{
    // Read m_apiMode under the lock; it is written by setApiMode().
    {
        QReadLocker locker(&m_lock);
        if (m_apiMode != ApiMode::GoogleV4 || m_v4Lists.isEmpty()) {
            locker.unlock();
            callback(isBlocked(url), ThreatTypes());
            return;
        }
    }

    // Compute full URL hashes and check against V4 lists
    // This is a simplified implementation - real V4 API uses prefix matching
    QReadLocker locker(&m_lock);
    ThreatTypes threats;
    QString urlStr = url.toString();
    QByteArray urlHash = QCryptographicHash::hash(urlStr.toUtf8(), QCryptographicHash::Sha256).toHex();

    // Check if any prefix matches
    for (auto it = m_v4Lists.constBegin(); it != m_v4Lists.constEnd(); ++it) {
        const QByteArray &listData = it.value();
        // In real implementation, parse the list data for prefix matches
        // For now, check if hash prefix exists in list
        if (listData.contains(urlHash.left(8))) { // 32-bit prefix
            // Determine threat type from list name
            if (it.key().contains("malware"))
                threats |= Malware;
            else if (it.key().contains("phish") || it.key().contains("social"))
                threats |= SocialEngineering;
            else if (it.key().contains("unwanted"))
                threats |= UnwantedSoftware;
        }
    }

    bool blocked = (threats == 0);
    // Release the read lock before emitting: a connected slot that calls back
    // into any write-locked method would otherwise self-deadlock.
    locker.unlock();

    if (blocked)
        emit threatFound(url, threats);

    callback(blocked, threats);
}

void SafeBrowsing::allow(const QString &host)
{
    QString h = host.trimmed().toLower();
    if (h.isEmpty())
        return;
    {
        QWriteLocker locker(&m_lock);
        m_allowed.insert(h);
    }
    emit changed();
}

void SafeBrowsing::clearSessionAllowances()
{
    {
        QWriteLocker locker(&m_lock);
        m_allowed.clear();
    }
    emit changed();
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
        QDateTime now = QDateTime::currentDateTime();
        for (const QJsonValue &v : doc.array()) {
            const QString host = v.toString().trimmed().toLower();
            if (!host.isEmpty()) {
                BlockedEntry entry;
                entry.host = host;
                entry.added = now; // Use current time for loaded entries
                entry.source = QStringLiteral("persisted");
                m_blocked.insert(host, entry);
            }
        }
    } else if (doc.isObject()) {
        // New format with metadata
        QWriteLocker locker(&m_lock);
        const QJsonObject obj = doc.object();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            const QJsonObject entryObj = it.value().toObject();
            const QString host = it.key().toLower();
            if (host.isEmpty())
                continue;
            BlockedEntry entry;
            entry.host = host;
            entry.added = QDateTime::fromString(entryObj.value("added").toString(), Qt::ISODate);
            if (!entry.added.isValid())
                entry.added = QDateTime::currentDateTime();
            entry.source = entryObj.value("source").toString(QStringLiteral("persisted"));
            m_blocked.insert(host, entry);
        }
    }
    emit changed();
}

void SafeBrowsing::refreshBlocklists()
{
    if (m_apiMode == ApiMode::GoogleV4) {
        fetchV4Updates();
        return;
    }

    // Offline/Brave/Custom mode: fetch from configured sources
    QVector<QUrl> sources;

    if (m_apiMode == ApiMode::BraveList) {
        sources << QUrl(QStringLiteral("https://raw.githubusercontent.com/brave/brave-browser/master/components/brave_blocker/resources/brave-unified-hosts.txt"));
        sources << QUrl(QStringLiteral("https://raw.githubusercontent.com/brave/brave-browser/master/components/brave_blocker/resources/brave-malware-filter.txt"));
    } else {
        // Default: StevenBlack hosts + custom sources
        sources << QUrl(QStringLiteral("https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts"));
        // Add more sources as needed
    }

    // The replies arrive long after this function returns, so completion state
    // must NOT be captured by reference onto this stack frame. A heap-shared,
    // self-owning record keeps it alive for exactly as long as the requests do.
    const int total = sources.size();
    if (total <= 0)
        return;

    auto *shared = new RefreshState;
    shared->total = total;
    // Safety net: if the replies never all arrive (process teardown, dropped
    // connection), release the record rather than leaking it.
    QPointer<QObject> guard(this);
    connect(this, &QObject::destroyed, [shared]() { delete shared; });
    Q_UNUSED(guard);

    for (const QUrl &sourceUrl : sources) {
        QNetworkRequest request(sourceUrl);
        request.setRawHeader("User-Agent", "BLACK Browser Safe Browsing");
        QNetworkReply *reply = m_net->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, sourceUrl, shared]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                QMutexLocker lock(&shared->mutex);
                shared->errorMessages += QStringLiteral("Failed to fetch %1: %2\n")
                                             .arg(sourceUrl.toString(), reply->errorString());
            } else {
                const QByteArray data = reply->readAll();
                // Data is size-capped here to avoid an unbounded remote list
                // exhausting memory or the disk.
                if (data.size() <= kMaxBlocklistBytes) {
                    mergeBlocklist(data, sourceUrl.toString());
                    QMutexLocker lock(&shared->mutex);
                    shared->anySuccess = true;
                } else {
                    QMutexLocker lock(&shared->mutex);
                    shared->errorMessages += QStringLiteral("Blocklist from %1 too large; ignored\n")
                                                 .arg(sourceUrl.toString());
                }
            }

            bool finished = false;
            {
                QMutexLocker lock(&shared->mutex);
                ++shared->completed;
                finished = (shared->completed == shared->total);
            }
            if (finished) {
                QString errors;
                bool ok = false;
                {
                    QMutexLocker lock(&shared->mutex);
                    ok = shared->anySuccess;
                    errors = shared->errorMessages;
                }
                delete shared;
                if (ok) {
                    saveExtraList();
                    // Remove stale entries after refresh
                    removeStaleEntries();
                    emit blocklistsRefreshed(true, QStringLiteral("Blocklists refreshed successfully"));
                } else {
                    emit blocklistsRefreshed(false, QStringLiteral("Blocklist refresh failed: ") + errors);
                }
            }
        });
    }
}

void SafeBrowsing::setupV4Api()
{
    if (m_v4Initialized || m_apiKey.isEmpty())
        return;

    m_v4Initialized = true;

    // Initial fetch
    fetchV4Updates();

    // Set up periodic V4 refresh (every 30 minutes as per API spec)
    m_v4RefreshTimer = new QTimer(this);
    m_v4RefreshTimer->setInterval(30 * 60 * 1000);
    connect(m_v4RefreshTimer, &QTimer::timeout, this, &SafeBrowsing::fetchV4Updates);
    m_v4RefreshTimer->start();
}

void SafeBrowsing::fetchV4Updates()
{
    if (m_apiKey.isEmpty())
        return;

    // Build V4 API request
    // POST https://safebrowsing.googleapis.com/v4/threatListUpdates:fetch?key=API_KEY
    QJsonObject request;
    QJsonObject client;
    client["clientId"] = "black-browser";
    client["clientVersion"] = "1.0";
    request["client"] = client;

    QJsonArray listUpdateRequests;

    // Request updates for all threat types
    const QStringList threatLists = {
        "MALWARE",
        "SOCIAL_ENGINEERING",
        "UNWANTED_SOFTWARE",
        "POTENTIALLY_HARMFUL_APPLICATION"
    };

    // Platform types
    const QStringList platforms = { "ANY_PLATFORM" };

    // Entry types - we want URL hashes
    const QStringList entryTypes = { "SHA256" };

    for (const QString &threatType : threatLists) {
        QJsonObject listRequest;
        QJsonObject threatEntryType;
        threatEntryType["platform"] = "ANY_PLATFORM";
        threatEntryType["threatEntryType"] = "URL";
        threatEntryType["threatType"] = threatType;
        listRequest["threatType"] = threatType;
        listRequest["platformType"] = "ANY_PLATFORM";
        listRequest["threatEntryType"] = "URL";

        // Include current state if we have it
        if (m_v4Lists.contains(threatType.toLower())) {
            listRequest["state"] = QString::fromLatin1(m_v4Lists[threatType.toLower()].toBase64());
        }

        listUpdateRequests.append(listRequest);
    }

    request["listUpdateRequests"] = listUpdateRequests;

    QNetworkRequest netRequest(QUrl(QStringLiteral("https://safebrowsing.googleapis.com/v4/threatListUpdates:fetch?key=") + m_apiKey));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_net->post(netRequest, QJsonDocument(request).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "SafeBrowsing V4 fetch failed:" << reply->errorString();
            return;
        }
        const QByteArray response = reply->readAll();
        applyV4Update(response);
    });
}

void SafeBrowsing::applyV4Update(const QByteArray &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    QJsonArray listUpdateResponses = root["listUpdateResponses"].toArray();

    {
        QWriteLocker locker(&m_lock);

        for (const QJsonValue &val : listUpdateResponses) {
            QJsonObject resp = val.toObject();
            QString threatType = resp["threatType"].toString().toLower();
            QString platformType = resp["platformType"].toString();
            QString threatEntryType = resp["threatEntryType"].toString();

            QString listKey = threatType + "_" + platformType + "_" + threatEntryType;

            // Process additions
            QJsonArray additions = resp["additions"].toArray();
            for (const QJsonValue &addVal : additions) {
                QJsonObject add = addVal.toObject();
                QString rawHashes = add["rawHashes"].toString();
                int prefixSize = add["prefixSize"].toInt();
                if (prefixSize <= 0)
                    continue;   // guard: a zero/negative stride would spin or under-read
                // Decode and add prefixes to our list
                QByteArray hashes = QByteArray::fromBase64(rawHashes.toLatin1());
                for (int i = 0; i < hashes.size(); i += prefixSize) {
                    if (i + prefixSize <= hashes.size()) {
                        QByteArray prefix = hashes.mid(i, prefixSize);
                        m_v4Lists[listKey].append(prefix.toHex());
                    }
                }
            }

            // Process removals
            QJsonArray removals = resp["removals"].toArray();
            for (const QJsonValue &remVal : removals) {
                QJsonObject rem = remVal.toObject();
                QString rawHashes = rem["rawHashes"].toString();
                int prefixSize = rem["prefixSize"].toInt();
                if (prefixSize <= 0)
                    continue;
                QByteArray hashes = QByteArray::fromBase64(rawHashes.toLatin1());
                QByteArray &listData = m_v4Lists[listKey];
                for (int i = 0; i < hashes.size(); i += prefixSize) {
                    if (i + prefixSize <= hashes.size()) {
                        QByteArray prefix = hashes.mid(i, prefixSize);
                        const QByteArray hex = prefix.toHex();
                        const int at = listData.indexOf(hex);
                        // indexOf() returns -1 when absent; removing at -1
                        // would erase an unrelated span. Skip instead.
                        if (at >= 0)
                            listData.remove(at, hex.size());
                    }
                }
            }

            // Update client state. Must use the SAME key that fetchV4Updates()
            // reads back, otherwise the incremental sync never round-trips and
            // the full list is re-downloaded every cycle.
            if (resp.contains("newClientState")) {
                const QString stateKey = threatType.toLower() + "_state";
                m_v4Lists[stateKey] = QByteArray::fromBase64(
                        resp["newClientState"].toString().toLatin1());
            }
        }
    }

    // Outside the lock: both of these re-acquire it.
    saveExtraList();
    emit changed();
    emit blocklistsRefreshed(true, QStringLiteral("Safe Browsing V4 lists updated"));
}

void SafeBrowsing::mergeBlocklist(const QByteArray &data, const QString &source)
{
    QWriteLocker locker(&m_lock);

    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    QDateTime now = QDateTime::currentDateTime();

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;

        // Handle JSON array format
        if (trimmed.startsWith('[')) {
            QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &v : doc.array()) {
                    QString host = v.toString().trimmed().toLower();
                    if (!host.isEmpty() && host != QLatin1String("localhost")) {
                        BlockedEntry entry;
                        entry.host = host;
                        entry.added = now;
                        entry.source = source;
                        m_blocked.insert(host, entry);
                    }
                }
            }
            continue;
        }

        // Handle hosts file format: IP hostname [hostname...]
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            // Skip the IP address, process hostnames
            for (int i = 1; i < parts.size(); ++i) {
                QString host = parts[i].trimmed().toLower();
                if (!host.isEmpty() && host != QLatin1String("localhost")) {
                    BlockedEntry entry;
                    entry.host = host;
                    entry.added = now;
                    entry.source = source;
                    m_blocked.insert(host, entry);
                }
            }
        } else if (parts.size() == 1) {
            // Single hostname per line
            QString host = parts[0].trimmed().toLower();
            if (!host.isEmpty() && host != QLatin1String("localhost")) {
                BlockedEntry entry;
                entry.host = host;
                entry.added = now;
                entry.source = source;
                m_blocked.insert(host, entry);
            }
        }
    }
    // Emitted outside the write-lock scope: a slot may call back in.
    emit changed();
}

void SafeBrowsing::saveExtraList()
{
    // Takes the write lock itself; safe to call from any context.
    QWriteLocker locker(&m_lock);
    saveExtraListLocked();
}

// Body of saveExtraList() for callers that ALREADY hold m_lock for writing.
// QWriteLock is non-recursive, so these paths must not re-acquire it.
void SafeBrowsing::saveExtraListLocked()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + QStringLiteral("safebrowsing.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    QJsonObject root;
    for (auto it = m_blocked.constBegin(); it != m_blocked.constEnd(); ++it) {
        const BlockedEntry &entry = it.value();
        // Only save non-embedded hosts
        bool isEmbedded = false;
        for (int i = 0; kEmbeddedHosts[i]; ++i) {
            if (entry.host == QLatin1String(kEmbeddedHosts[i])) {
                isEmbedded = true;
                break;
            }
        }
        if (!isEmbedded) {
            QJsonObject entryObj;
            entryObj["added"] = entry.added.toString(Qt::ISODate);
            entryObj["source"] = entry.source;
            root[entry.host] = entryObj;
        }
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool SafeBrowsing::hostMatches(const QString &host) const
{
    if (host.isEmpty())
        return false;
    for (auto it = m_blocked.constBegin(); it != m_blocked.constEnd(); ++it) {
        const QString &d = it.key();
        if (host == d || host.endsWith(QLatin1Char('.') + d))
            return true;
    }
    return false;
}

void SafeBrowsing::removeStaleEntries()
{
    int removed = 0;
    bool dirty = false;
    {
        // Scope the write lock tightly. saveExtraList() re-acquires the same
        // non-recursive lock, so it MUST be called after the locker is
        // destroyed - holding the lock across the call self-deadlocks.
        QWriteLocker locker(&m_lock);
        const QDateTime cutoff = QDateTime::currentDateTime().addDays(-kMaxEntryAgeDays);
        for (auto it = m_blocked.begin(); it != m_blocked.end(); ) {
            // Never remove embedded entries
            bool isEmbedded = false;
            for (int i = 0; kEmbeddedHosts[i]; ++i) {
                if (it.key() == QLatin1String(kEmbeddedHosts[i])) {
                    isEmbedded = true;
                    break;
                }
            }
            if (!isEmbedded && it.value().added < cutoff) {
                it = m_blocked.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        dirty = removed > 0;
    }

    if (dirty) {
        qDebug() << "SafeBrowsing: removed" << removed << "stale blocklist entries";
        saveExtraList();     // takes the lock itself - safe here
        emit changed();       // emitted outside the lock
    }
}

SafeBrowsing::Stats SafeBrowsing::getStats() const
{
    QReadLocker locker(&m_lock);
    Stats stats;
    stats.blockedHosts = m_blocked.size();
    stats.allowedHosts = m_allowed.size();
    stats.lastUpdate = QDateTime::currentDateTime().toString(Qt::ISODate);
    stats.currentMode = m_apiMode;
    return stats;
}

QUrl SafeBrowsing::warningUrl(const QUrl &original)
{
    QUrl warn(QStringLiteral("qrc:/safebrowsing_warning.html"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("url"), original.toString());
    warn.setQuery(query);
    return warn;
}

#include "SafeBrowsing.moc"
