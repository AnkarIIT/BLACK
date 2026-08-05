#include "TrackerBlocker.h"
#include <QWebEngineUrlRequestInfo>
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

} // namespace

TrackerBlocker &TrackerBlocker::instance()
{
    static TrackerBlocker blocker;
    return blocker;
}

TrackerBlocker::TrackerBlocker()
    : QWebEngineUrlRequestInterceptor(nullptr)
    , m_today(0)
{
    for (const char *host : kBlockedHosts)
        m_blockedHosts.insert(QString::fromLatin1(host));
}

bool TrackerBlocker::isBlockedHost(const QString &host) const
{
    for (const QString &domain : m_blockedHosts) {
        if (host == domain || host.endsWith(QLatin1Char('.') + domain))
            return true;
    }
    return false;
}

void TrackerBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    // Record every distinct site visited so the Privacy Report can show the
    // percentage of websites that contacted trackers.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
        const QString host = info.requestUrl().host().toLower();
        if (!host.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, host]() {
                const QString day = QDate::currentDate().toString(Qt::ISODate);
                QSet<QString> &sites = m_sitesByDay[day];
                if (sites.size() < kMaxSitesPerDay)
                    sites.insert(host);
                emit privacyChanged();
            }, Qt::QueuedConnection);
        }
        return;
    }

    const QString host = info.requestUrl().host().toLower();
    if (!host.isEmpty() && isBlockedHost(host)) {
        const QString firstPartyHost = info.firstPartyUrl().host().toLower();
        info.block(true);
        QMetaObject::invokeMethod(this, [this, host, firstPartyHost]() {
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
}

void TrackerBlocker::saveData()
{
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    m_daily[today] = m_today;

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

    QFile file(dataFile(QStringLiteral("privacy.json")));
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(root).toJson());
}
