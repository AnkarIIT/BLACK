#ifndef HISTORYDAO_H
#define HISTORYDAO_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDateTime>

#include "History.h"

class HistoryDao : public QObject
{
    Q_OBJECT
public:
    explicit HistoryDao(QObject *parent = nullptr);
    virtual ~HistoryDao();

    bool initialize();

    qint64 addVisit(const HistoryEntry& entry);
    HistoryEntry getEntryById(qint64 id) const;
    HistoryEntry getEntryByUrl(const QString& url) const;
    QList<HistoryEntry> getAllHistory(
        const QString& searchQuery = QString(),
        const QDateTime& since = QDateTime(),
        const QDateTime& until = QDateTime(),
        int limit = 1000,
        int offset = 0
    ) const;
    QList<QPair<QDate, QList<HistoryEntry>>> getHistoryByDay(
        const QDateTime& since = QDateTime(),
        int daysLimit = 30
    ) const;
    QList<HistoryEntry> getTopSites(int limit = 10) const;
    QList<HistoryEntry> getRecentlyClosed(int limit = 10) const;
    QList<HistoryEntry> searchHistory(const QString& query, int limit = 100) const;
    QList<HistoryEntry> getHistoryForDomain(const QString& domain, int limit = 50) const;
    int getVisitCount(const QString& url) const;
    int getTotalCount() const;
    int getCountSince(const QDateTime& since) const;

    bool deleteEntry(qint64 id);
    bool deleteByUrl(const QString& url);
    int deleteByDomain(const QString& domain);
    int deleteOlderThan(const QDateTime& before);
    int deleteInRange(const QDateTime& since, const QDateTime& until);
    bool clearAllHistory();

    int markSynced(const QList<qint64>& ids);
    QList<HistoryEntry> getUnsyncedEntries(int limit = 100) const;
    int importFromJson(const QJsonArray& items);
    QJsonArray exportToJson(
        const QDateTime& since = QDateTime(),
        const QDateTime& until = QDateTime()
    ) const;

    struct Statistics {
        int totalVisits = 0;
        int uniqueUrls = 0;
        int uniqueDomains = 0;
        qint64 totalVisitDuration = 0;
        QString mostVisitedDomain;
        int mostVisitedDomainCount = 0;
        QDateTime firstVisit;
        QDateTime lastVisit;
        QMap<QString, int> visitsByDay;
        QMap<QString, int> visitsByDomain;
    };

    Statistics getStatistics() const;
    QMap<QDate, int> getDailyVisitCounts(int days = 30) const;
    QMap<int, int> getHourlyVisitDistribution() const;

signals:
    void historyAdded(const HistoryEntry& entry);
    void historyUpdated(const HistoryEntry& entry);
    void historyRemoved(qint64 id);
    void historyCleared();
    void dataChanged();

private:
    class Private;
    Private* d;
};

#endif // HISTORYDAO_H