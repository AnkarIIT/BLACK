#include "TrackerBlocker.h"
#include "SafeBrowsing.h"
#include "OSPaths.h"
#include <QWebEngineUrlRequestInfo>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QMetaObject>
#include <algorithm>

namespace {

QString dataFile(const QString &fileName)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + fileName;
}

// Known advertising / tracking / analytics domains (EasyList/EasyPrivacy style).
const char *const kBlockedHosts[] = {
    "2mdn.net",
    "adform.net",
    "adnxs.com",
    "adroll.com",
    "adsafeprotected.com",
    "adsrvr.org",
    "adsymptotic.com",
    "adtechus.com",
    "advertising.com",
    "agkn.com",
    "amobee.com",
    "analytics.google.com",
    "analytics.tiktok.com",
    "analytics.twitter.com",
    "analytics.yahoo.com",
    "appnexus.com",
    "atwola.com",
    "bat.bing.com",
    "bidr.io",
    "bidswitch.net",
    "bidvertiser.com",
    "bluekai.com",
    "bounceexchange.com",
    "casalemedia.com",
    "chartbeat.com",
    "clarity.ms",
    "clicktale.com",
    "comscore.com",
    "connect.facebook.net",
    "contextweb.com",
    "conversantmedia.com",
    "crazyegg.com",
    "criteo.com",
    "criteo.net",
    "crwdcntrl.net",
    "ct.pinterest.com",
    "demdex.net",
    "doubleclick.net",
    "everestads.com",
    "everesttech.net",
    "exelator.com",
    "eyeota.net",
    "facebook.net",
    "flashtalking.com",
    "fullstory.com",
    "gmads.net",
    "googlesyndication.com",
    "googletagmanager.com",
    "googletagservices.com",
    "googleadservices.com",
    "hotjar.com",
    "imrworldwide.com",
    "indexww.com",
    "infolinks.com",
    "inspctbox.com",
    "kissmetrics.com",
    "lijit.com",
    "mathtag.com",
    "mc.yandex.ru",
    "media.net",
    "mixpanel.com",
    "moat.com",
    "moatads.com",
    "mouseflow.com",
    "nuggad.net",
    "omtrdc.net",
    "onaudience.com",
    "openx.net",
    "optimizely.com",
    "outbrain.com",
    "pubmatic.com",
    "px.ads.linkedin.com",
    "quantcount.com",
    "quantserve.com",
    "realmedia.com",
    "revcontent.com",
    "rhythmone.com",
    "rlcdn.com",
    "rubiconproject.com",
    "scorecardresearch.com",
    "segment.io",
    "sharethrough.com",
    "simpli.fi",
    "skimresources.com",
    "smartadserver.com",
    "sonobi.com",
    "sovrn.com",
    "spotxchange.com",
    "stackadapt.com",
    "statcounter.com",
    "stickyadstv.com",
    "taboola.com",
    "taboolasyndication.com",
    "tapad.com",
    "tapjoy.com",
    "teads.tv",
    "tremormedia.com",
    "triplelift.com",
    "turn.com",
    "tynt.com",
    "umeng.com",
    "underdogmedia.com",
    "valueclick.com",
    "viglink.com",
    "weborama.fr",
    "yieldmo.com",
    "yldbt.com",
    "zanox.com",
    "zemanta.com",
    "zergnet.com",
    "zopim.com",
};

const int kMaxSitesPerDay = 200;

// True for "localhost"/*.localhost/*.local hosts and IPv4/IPv6 literals.
// These are exempted from the HTTPS-First upgrade because local and raw-IP
// endpoints are usually self-hosted and often serve plain HTTP.
bool isIpv4Literal(const QString &host)
{
    const QStringList parts = host.split(QLatin1Char('.'));
    if (parts.size() != 4)
        return false;
    for (const QString &p : parts) {
        bool ok = false;
        const int v = p.toInt(&ok);
        if (!ok || v < 0 || v > 255 || QString::number(v) != p)
            return false;
    }
    return true;
}

bool isIpv6Literal(const QString &host)
{
    if (!host.contains(QLatin1Char(':')))
        return false;
    for (const QChar c : host) {
        const ushort u = c.unicode();
        const bool hexDigit = (u >= '0' && u <= '9')
                              || (u >= 'a' && u <= 'f')
                              || (u >= 'A' && u <= 'F')
                              || c == QLatin1Char(':')
                              || c == QLatin1Char('.')
                              || c == QLatin1Char('%');
        if (!hexDigit)
            return false;
    }
    return true;
}

bool isLocalOrIpAddress(const QString &host)
{
    if (host.isEmpty())
        return false;
    if (host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0
        || host.endsWith(QLatin1String(".localhost"), Qt::CaseInsensitive)
        || host.endsWith(QLatin1String(".local"), Qt::CaseInsensitive))
        return true;
    return isIpv4Literal(host) || isIpv6Literal(host);
}

} // namespace

TrackerBlocker &TrackerBlocker::instance()
{
    static TrackerBlocker blocker(false);
    return blocker;
}

TrackerBlocker &TrackerBlocker::privateInstance()
{
    static TrackerBlocker blocker(true);
    return blocker;
}

TrackerBlocker::TrackerBlocker(bool incognito)
    : QWebEngineUrlRequestInterceptor(nullptr)
    , m_incognito(incognito)
    , m_today(0)
    , m_lastDate(QDate::currentDate())
{
    for (const char *host : kBlockedHosts)
        m_blockedHosts.insert(QString::fromLatin1(host));
    buildDomainTrie();
}

TrackerBlocker::~TrackerBlocker()
{
    deleteTrie(m_domainTrie);
}

void TrackerBlocker::buildDomainTrie()
{
    m_domainTrie = new TrieNode();
    for (const QString &domain : m_blockedHosts) {
        TrieNode* node = m_domainTrie;
        // Split domain into labels and reverse for suffix matching
        const QStringList labels = domain.split(QLatin1Char('.'));
        for (int i = labels.size() - 1; i >= 0; --i) {
            const QString &label = labels[i];
            if (!node->children.contains(label))
                node->children[label] = new TrieNode();
            node = node->children[label];
        }
        node->isTerminal = true;
        node->terminalDomain = domain;
    }
}

void TrackerBlocker::deleteTrie(TrieNode* node)
{
    if (node) {
        qDeleteAll(node->children);
        delete node;
    }
}

bool TrackerBlocker::isBlockedHost(const QString &host) const
{
    if (host.isEmpty())
        return false;

    TrieNode* node = m_domainTrie;
    const QStringList labels = host.split(QLatin1Char('.'));

    // Walk the trie from the TLD towards the subdomain
    for (int i = labels.size() - 1; i >= 0; --i) {
        const QString &label = labels[i];
        if (!node->children.contains(label))
            break;
        node = node->children[label];
        if (node->isTerminal)
            return true; // Found a blocked domain suffix
    }
    return false;
}

bool TrackerBlocker::isIncognito() const
{
    return m_incognito;
}

void TrackerBlocker::rollDayIfNeeded() const
{
    // m_today is a cumulative counter, so roll it over the first time it is
    // touched after midnight. Otherwise a multi-day session would carry
    // yesterday's total into today and inflate the 7/30-day stats.
    const QDate today = QDate::currentDate();
    if (today != m_lastDate) {
        m_today = 0;
        m_lastDate = today;
    }
}

void TrackerBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const bool incognito = isIncognito();

    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
        // HTTPS-First: auto-upgrade insecure http:// main-frame loads to
        // https://, except for local hosts and IP literals (typically
        // self-hosted dev/test servers that do not speak TLS). The redirected
        // https request is re-intercepted, so it still gets recorded and
        // Safe-Browsing checked below. Applies to private windows too.
        const QUrl requestUrl = info.requestUrl();
        if (requestUrl.scheme().compare(QLatin1String("http"), Qt::CaseInsensitive) == 0
            && !isLocalOrIpAddress(requestUrl.host())) {
            QUrl upgraded = requestUrl;
            upgraded.setScheme(QStringLiteral("https"));
            if (upgraded.port() == 80)
                upgraded.setPort(-1); // drop the explicit :80 port
            info.redirect(upgraded);
            return;
        }

        // Record every distinct site visited so the Privacy Report can show the
        // percentage of websites that contacted trackers. Never recorded for
        // private-window traffic.
        if (!incognito) {
            const QString host = requestUrl.host().toLower();
            if (!host.isEmpty()) {
                QMetaObject::invokeMethod(this, [this, host]() {
                    const QString day = QDate::currentDate().toString(Qt::ISODate);
                    QSet<QString> &sites = m_sitesByDay[day];
                    if (sites.size() < kMaxSitesPerDay)
                        sites.insert(host);
                    emit privacyChanged();
                }, Qt::QueuedConnection);
            }
        }

        // Offline Safe Browsing: redirect known phishing hosts to a warning page.
        if (SafeBrowsing::instance().isBlocked(requestUrl)) {
            info.redirect(SafeBrowsing::warningUrl(requestUrl));
        }
        return;
    }

    const QString host = info.requestUrl().host().toLower();
    if (host.isEmpty() || !isBlockedHost(host))
        return;

    const QString firstPartyHost = info.firstPartyUrl().host().toLower();
    info.block(true);

    if (!incognito) {
        QMetaObject::invokeMethod(this, [this, host, firstPartyHost]() {
            rollDayIfNeeded();
            const QString day = QDate::currentDate().toString(Qt::ISODate);
            ++m_today;
            m_daily[day] = m_today;
            ++m_hostCounts[day][host];
            if (!firstPartyHost.isEmpty()) {
                QSet<QString> &sites = m_hostSites[day][host];
                if (sites.size() < kMaxSitesPerDay)
                    sites.insert(firstPartyHost);
            }
            emit privacyChanged();
        }, Qt::QueuedConnection);
    }
}

QList<QDate> TrackerBlocker::daysInWindow(int days) const
{
    const QDate today = QDate::currentDate();
    QList<QDate> dates;
    for (int i = 0; i < days; ++i)
        dates.append(today.addDays(-i));
    return dates;
}

int TrackerBlocker::blockedLastNDays(int days) const
{
    const QList<QDate> window = daysInWindow(days);
    int total = 0;
    for (auto it = m_daily.constBegin(); it != m_daily.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (date.isValid() && window.contains(date))
            total += it.value();
    }
    return total;
}

QString TrackerBlocker::mostContactedTracker() const
{
    const QList<QDate> window = daysInWindow(30);
    QMap<QString, int> counts;
    for (auto it = m_hostCounts.constBegin(); it != m_hostCounts.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid() || !window.contains(date))
            continue;
        for (auto h = it.value().constBegin(); h != it.value().constEnd(); ++h)
            counts[h.key()] += h.value();
    }
    if (counts.isEmpty())
        return QStringLiteral("No trackers detected");
    QString best;
    int bestCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > bestCount) {
            bestCount = it.value();
            best = it.key();
        }
    }
    return best;
}

int TrackerBlocker::mostContactedTrackerSites() const
{
    const QList<QDate> window = daysInWindow(30);
    const QString tracker = mostContactedTracker();
    if (tracker.isEmpty() || tracker == QStringLiteral("No trackers detected"))
        return 0;
    QSet<QString> sites;
    for (auto it = m_hostSites.constBegin(); it != m_hostSites.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid() || !window.contains(date))
            continue;
        auto hit = it.value().constFind(tracker);
        if (hit != it.value().constEnd())
            sites.unite(*hit);
    }
    return sites.size();
}

int TrackerBlocker::websitesContactedTrackers() const
{
    const QList<QDate> window = daysInWindow(30);
    QSet<QString> sites;
    for (auto it = m_hostSites.constBegin(); it != m_hostSites.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid() || !window.contains(date))
            continue;
        for (auto h = it.value().constBegin(); h != it.value().constEnd(); ++h)
            sites.unite(*h);
    }
    return sites.size();
}

int TrackerBlocker::websitesVisited() const
{
    const QList<QDate> window = daysInWindow(30);
    QSet<QString> sites;
    for (auto it = m_sitesByDay.constBegin(); it != m_sitesByDay.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid() || !window.contains(date))
            continue;
        sites.unite(it.value());
    }
    return sites.size();
}

QString TrackerBlocker::trackerBreakdownJson() const
{
    const QList<QDate> window = daysInWindow(30);
    QMap<QString, int> counts;
    QMap<QString, QSet<QString>> sites;
    for (auto it = m_hostCounts.constBegin(); it != m_hostCounts.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid() || !window.contains(date))
            continue;
        for (auto h = it.value().constBegin(); h != it.value().constEnd(); ++h) {
            counts[h.key()] += h.value();
            auto sit = m_hostSites.constFind(it.key());
            if (sit != m_hostSites.constEnd()) {
                auto hs = sit.value().constFind(h.key());
                if (hs != sit.value().constEnd())
                    sites[h.key()].unite(*hs);
            }
        }
    }

    QList<QPair<int, QString>> ranked;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        ranked.append(qMakePair(it.value(), it.key()));
    std::sort(ranked.begin(), ranked.end(),
              [](const QPair<int, QString> &a, const QPair<int, QString> &b) { return a.first > b.first; });

    QJsonArray array;
    for (const auto &pair : ranked) {
        QJsonObject obj;
        obj.insert(QStringLiteral("host"), pair.second);
        obj.insert(QStringLiteral("count"), pair.first);
        obj.insert(QStringLiteral("sites"), sites.value(pair.second).size());
        array.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void TrackerBlocker::loadData()
{
    QFile file(dataFile(QStringLiteral("privacy.json")));
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                const QString day = it.key();
                if (it.value().isDouble()) {
                    // Legacy format: { "date": count }
                    m_daily.insert(day, it.value().toInt());
                    continue;
                }
                if (!it.value().isObject())
                    continue;
                const QJsonObject dayObj = it.value().toObject();
                m_daily.insert(day, dayObj.value(QStringLiteral("trackers")).toInt(0));

                const QJsonObject hosts = dayObj.value(QStringLiteral("hosts")).toObject();
                for (auto h = hosts.constBegin(); h != hosts.constEnd(); ++h) {
                    const QJsonObject stat = h.value().toObject();
                    m_hostCounts[day][h.key()] = stat.value(QStringLiteral("count")).toInt(0);
                    const QJsonArray sites = stat.value(QStringLiteral("sites")).toArray();
                    for (const QJsonValue &v : sites)
                        m_hostSites[day][h.key()].insert(v.toString());
                }
                const QJsonArray sites = dayObj.value(QStringLiteral("sites")).toArray();
                for (const QJsonValue &v : sites)
                    m_sitesByDay[day].insert(v.toString());
            }
        }
    }
    m_today = m_daily.value(QDate::currentDate().toString(Qt::ISODate), 0);
    m_lastDate = QDate::currentDate();
}

void TrackerBlocker::saveData()
{
    rollDayIfNeeded();
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    m_daily[today] = m_today;

    // TTL cleanup: purge privacy data older than 90 days to bound disk usage
    const QDate cutoff = QDate::currentDate().addDays(-90);
    const QString cutoffStr = cutoff.toString(Qt::ISODate);

    // Purge m_daily
    for (auto it = m_daily.begin(); it != m_daily.end(); ) {
        const QDate day = QDate::fromString(it.key(), Qt::ISODate);
        if (day.isValid() && day < cutoff)
            it = m_daily.erase(it);
        else
            ++it;
    }
    // Purge m_hostCounts
    for (auto it = m_hostCounts.begin(); it != m_hostCounts.end(); ) {
        const QDate day = QDate::fromString(it.key(), Qt::ISODate);
        if (day.isValid() && day < cutoff)
            it = m_hostCounts.erase(it);
        else
            ++it;
    }
    // Purge m_hostSites
    for (auto it = m_hostSites.begin(); it != m_hostSites.end(); ) {
        const QDate day = QDate::fromString(it.key(), Qt::ISODate);
        if (day.isValid() && day < cutoff)
            it = m_hostSites.erase(it);
        else
            ++it;
    }
    // Purge m_sitesByDay
    for (auto it = m_sitesByDay.begin(); it != m_sitesByDay.end(); ) {
        const QDate day = QDate::fromString(it.key(), Qt::ISODate);
        if (day.isValid() && day < cutoff)
            it = m_sitesByDay.erase(it);
        else
            ++it;
    }

    QJsonObject root;
    QSet<QString> dayKeys(m_daily.keys().begin(), m_daily.keys().end());
    {
        const QList<QString> hostKeys = m_hostCounts.keys();
        dayKeys.unite(QSet<QString>(hostKeys.begin(), hostKeys.end()));
    }
    {
        const QList<QString> siteKeys = m_sitesByDay.keys();
        dayKeys.unite(QSet<QString>(siteKeys.begin(), siteKeys.end()));
    }
    QStringList days = dayKeys.values();
    std::sort(days.begin(), days.end());
    for (const QString &day : days) {
        QJsonObject dayObj;
        dayObj.insert(QStringLiteral("trackers"), m_daily.value(day));

        QJsonObject hostsObj;
        const QMap<QString, int> counts = m_hostCounts.value(day);
        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
            QJsonObject stat;
            stat.insert(QStringLiteral("count"), it.value());
            QJsonArray sitesArr;
            const QSet<QString> sites = m_hostSites.value(day).value(it.key());
            for (const QString &s : sites)
                sitesArr.append(s);
            stat.insert(QStringLiteral("sites"), sitesArr);
            hostsObj.insert(it.key(), stat);
        }
        dayObj.insert(QStringLiteral("hosts"), hostsObj);

        QJsonArray sitesArr;
        const QSet<QString> visited = m_sitesByDay.value(day);
        for (const QString &s : visited)
            sitesArr.append(s);
        dayObj.insert(QStringLiteral("sites"), sitesArr);

        root.insert(day, dayObj);
    }

    OSPaths::writeFileAtomic(dataFile(QStringLiteral("privacy.json")),
                             QJsonDocument(root).toJson());
}
