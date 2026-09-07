#ifndef TRACKERBLOCKER_H
#define TRACKERBLOCKER_H

#include <QWebEngineUrlRequestInterceptor>
#include <QSet>
#include <QString>
#include <QMap>
#include <QList>
#include <QDate>
#include <QVector>
#include <QHash>

class ExtensionManager;

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
    // Persistent interceptor installed on the main profile: blocks trackers and
    // records privacy-report stats.
    static TrackerBlocker &instance();
    // Separate interceptor for private-window profiles: identical blocking,
    // HTTPS-First and Safe-Browsing protection, but it never records stats, so
    // incognito activity never reaches the persistent privacy.json.
    static TrackerBlocker &privateInstance();

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    int trackersBlockedToday() const { rollDayIfNeeded(); return m_today; }
    int trackersBlockedThisWeek() const { return blockedLastNDays(7); }
    int trackersBlockedLast30Days() const { return blockedLastNDays(30); }
    QString mostContactedTracker() const;
    int mostContactedTrackerSites() const;
    int websitesContactedTrackers() const;
    int websitesVisited() const;

    Q_INVOKABLE QString trackerBreakdownJson() const;

    void loadData();
    void saveData();

    // Set extension manager for declarative blocking rules
    void setExtensionManager(ExtensionManager *manager);

signals:
    void privacyChanged();

private:
    friend class TestTrackerBlocker;
    explicit TrackerBlocker(bool incognito);
    ~TrackerBlocker();
    Q_DISABLE_COPY(TrackerBlocker)

    // Domain suffix trie for O(k) blocked host lookup (k = domain parts)
    struct TrieNode {
        QHash<QString, TrieNode*> children;
        bool isTerminal = false;
        QString terminalDomain;
        TrieNode() = default;
        ~TrieNode() { qDeleteAll(children); }
    };
    void buildDomainTrie();
    void deleteTrie(TrieNode* node);
    bool isBlockedHost(const QString &host) const;
    bool isBlockedByExtensionRules(const QString &url) const;
    bool isIncognito() const;
    int blockedLastNDays(int days) const;
    QList<QDate> daysInWindow(int days) const;
    void rollDayIfNeeded() const;

    bool m_incognito = false;
    QSet<QString> m_blockedHosts;
    TrieNode* m_domainTrie = nullptr;
    mutable int m_today;
    mutable QDate m_lastDate;                          // last day m_today was rolled
    QMap<QString, int> m_daily;                        // day (ISO) -> blocked that day
    QMap<QString, QMap<QString, int>> m_hostCounts;    // day -> host -> blocked count
    QMap<QString, QMap<QString, QSet<QString>>> m_hostSites; // day -> host -> distinct sites
    QMap<QString, QSet<QString>> m_sitesByDay;         // day -> distinct visited sites

    ExtensionManager *m_extensionManager = nullptr;
};

#endif
