#include "TrackerBlocker.h"
#include <QWebEngineUrlRequestInfo>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QMetaObject>

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
    // Never break top-level navigation.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)
        return;

    const QString host = info.requestUrl().host().toLower();
    if (!host.isEmpty() && isBlockedHost(host)) {
        info.block(true);
        QMetaObject::invokeMethod(this, [this]() {
            ++m_today;
            m_daily[QDate::currentDate().toString(Qt::ISODate)] = m_today;
            emit privacyChanged();
        }, Qt::QueuedConnection);
    }
}

int TrackerBlocker::trackersBlockedThisWeek() const
{
    const QDate today = QDate::currentDate();
    int total = 0;
    for (auto it = m_daily.constBegin(); it != m_daily.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (date.isValid() && today.daysTo(date) <= 0 && date.daysTo(today) <= 6)
            total += it.value();
    }
    // m_daily is only touched on the main thread; m_today covers the live session.
    return total;
}

void TrackerBlocker::loadData()
{
    QFile file(dataFile(QStringLiteral("privacy.json")));
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
                m_daily.insert(it.key(), it.value().toInt());
        }
    }
    m_today = m_daily.value(QDate::currentDate().toString(Qt::ISODate), 0);
}

void TrackerBlocker::saveData()
{
    m_daily[QDate::currentDate().toString(Qt::ISODate)] = m_today;
    QFile file(dataFile(QStringLiteral("privacy.json")));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        for (auto it = m_daily.constBegin(); it != m_daily.constEnd(); ++it)
            obj.insert(it.key(), it.value());
        file.write(QJsonDocument(obj).toJson());
    }
}
