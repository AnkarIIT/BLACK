#ifndef SAFEBROWSING_H
#define SAFEBROWSING_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QSet>
#include <QReadWriteLock>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <QMap>
#include <QDateTime>

// Safe Browsing v4 API integration with fallback to host-based blocking
// Supports Google Safe Browsing Update API v4 and Brave-compatible blocklists
// Threat types: MALWARE, SOCIAL_ENGINEERING, UNWANTED_SOFTWARE, POTENTIALLY_HARMFUL_APPLICATION
class SafeBrowsing : public QObject
{
    Q_OBJECT

public:
    enum ThreatType {
        Malware = 0x1,
        SocialEngineering = 0x2,
        UnwantedSoftware = 0x4,
        PotentiallyHarmfulApplication = 0x8
    };
    Q_DECLARE_FLAGS(ThreatTypes, ThreatType)

    enum class ApiMode {
        Offline,           // Local blocklist only
        GoogleV4,          // Google Safe Browsing Update API v4
        BraveList,         // Brave's aggregated blocklist
        Custom             // Custom API endpoint
    };

    static SafeBrowsing &instance();

    // Configure the API mode and API key (for Google V4)
    void setApiMode(ApiMode mode, const QString &apiKey = QString());
    ApiMode apiMode() const;

    // Check if a URL is blocked (sync for interceptor)
    bool isBlocked(const QUrl &url) const;

    // Async check with full v4 API (for detailed threat info)
    void checkUrlAsync(const QUrl &url, std::function<void(bool, ThreatTypes)> callback);

    // Allow a host for the current session only (not persisted)
    // This is a one-time bypass that is cleared on restart
    Q_INVOKABLE void allow(const QString &host);

    // Clear all session allowances (e.g., on browser restart)
    Q_INVOKABLE void clearSessionAllowances();

    // Load/refresh blocklists
    void loadExtraList();
    void refreshBlocklists();
    static QUrl warningUrl(const QUrl &original);

    // Get statistics
    struct Stats {
        qint64 blockedHosts = 0;
        qint64 allowedHosts = 0;
        QString lastUpdate;
        ApiMode currentMode = ApiMode::Offline;
    };
    Stats getStats() const;

signals:
    void changed();
    void blocklistsRefreshed(bool success, const QString &message);
    void threatFound(const QUrl &url, ThreatTypes threats);

private:
    SafeBrowsing();
    ~SafeBrowsing() override;
    Q_DISABLE_COPY(SafeBrowsing)

    bool hostMatches(const QString &host) const;
    void mergeBlocklist(const QByteArray &data, const QString &source);
    void saveExtraList();
    void setupV4Api();
    void fetchV4Updates();
    void applyV4Update(const QByteArray &response);
    QString computeV4Hash(const QString &url, const QString &listName) const;
    void removeStaleEntries();

    // Blocked host entry with metadata (timestamp for stale removal)
    struct BlockedEntry {
        QString host;
        QDateTime added;
        QString source;
    };
    
    // Using QMap to store host -> entry metadata for stale removal
    QMap<QString, BlockedEntry> m_blocked;
    QSet<QString> m_allowed; // Session-only, not persisted
    mutable QReadWriteLock m_lock;

    // V4 API state
    ApiMode m_apiMode = ApiMode::Offline;
    QString m_apiKey;
    QMap<QString, QByteArray> m_v4Lists; // list name -> state
    QTimer *m_refreshTimer = nullptr;
    QTimer *m_v4RefreshTimer = nullptr;
    QNetworkAccessManager *m_net = nullptr;
    bool m_v4Initialized = false;

    // Embedded fallback hosts
    static const char *const kEmbeddedHosts[];
    
    // Max age for blocked entries (90 days)
    static const int kMaxEntryAgeDays = 90;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SafeBrowsing::ThreatTypes)
#endif