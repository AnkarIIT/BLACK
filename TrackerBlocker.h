#ifndef TRACKERBLOCKER_H
#define TRACKERBLOCKER_H

#include <QWebEngineUrlRequestInterceptor>
#include <QSet>
#include <QString>
#include <QMap>

class TrackerBlocker : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
    Q_PROPERTY(int trackersBlockedToday READ trackersBlockedToday NOTIFY privacyChanged)
    Q_PROPERTY(int trackersBlockedThisWeek READ trackersBlockedThisWeek NOTIFY privacyChanged)

public:
    static TrackerBlocker &instance();

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    int trackersBlockedToday() const { return m_today; }
    int trackersBlockedThisWeek() const;

    void loadData();
    void saveData();

signals:
    void privacyChanged();

private:
    TrackerBlocker();
    Q_DISABLE_COPY(TrackerBlocker)

    bool isBlockedHost(const QString &host) const;

    QSet<QString> m_blockedHosts;
    int m_today;
    QMap<QString, int> m_daily;
};

#endif
