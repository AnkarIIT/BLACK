#ifndef TRACKERBLOCKER_H
#define TRACKERBLOCKER_H

#include <QWebEngineUrlRequestInterceptor>
#include <QSet>
#include <QString>
#include <QMap>
#include <QList>
#include <QDate>

class TrackerBlocker : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
    Q_PROPERTY(int trackersBlockedToday READ trackersBlockedToday NOTIFY privacyChanged)
    Q_PROPERTY(int trackersBlockedThisWeek READ trackersBlockedThisWeek NOTIFY privacyChanged)
    Q_PROPERTY(int trackersBlockedLast30Days READ trackersBlockedLast30Days NOTIFY privacyChanged)
    Q_PROPERTY(QString mostContactedTracker READ mostContactedTracker NOTIFY privacyChanged)
    Q_PROPERTY(int mostContactedTrackerSites READ mostContactedTrackerSites NOTIFY privacyChanged)
    Q_PROPERTY(int websitesContactedTrackers READ websitesContactedTrackers NOTIFY privacyChanged)
    Q_PROPERTY(int websitesVisited READ websitesVisited NOTIFY privacyChanged)

public:
    static TrackerBlocker &instance();

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    int trackersBlockedToday() const { return m_today; }
    int trackersBlockedThisWeek() const { return blockedLastNDays(7); }
    int trackersBlockedLast30Days() const { return blockedLastNDays(30); }
    QString mostContactedTracker() const;
    int mostContactedTrackerSites() const;
    int websitesContactedTrackers() const;
    int websitesVisited() const;

    Q_INVOKABLE QString trackerBreakdownJson() const;

    void loadData();
    void saveData();

signals:
    void privacyChanged();

private:
    TrackerBlocker();
    Q_DISABLE_COPY(TrackerBlocker)

    bool isBlockedHost(const QString &host) const;
    int blockedLastNDays(int days) const;
    QList<QDate> daysInWindow(int days) const;

    QSet<QString> m_blockedHosts;
    int m_today;
    QMap<QString, int> m_daily;                        // day (ISO) -> total blocked
    QMap<QString, QMap<QString, int>> m_hostCounts;    // day -> host -> blocked count
    QMap<QString, QMap<QString, QSet<QString>>> m_hostSites; // day -> host -> distinct sites
    QMap<QString, QSet<QString>> m_sitesByDay;         // day -> distinct visited sites
};

#endif
