#include "HistoryDao.h"
#include "OSPaths.h"
#include "VaultCrypto.h"
#include "History.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include <QSet>

namespace {
const QString kDbFileName = "history.db";
const QString kTableHistory = "history";
const QString kTableVisits = "visits";
const QString kTableSettings = "history_settings";

const char kDefaultRetentionDays[] = "365"; // 1 year default
}

class HistoryDao::Private {
public:
    Private(HistoryDao* parent) : q(parent), m_db(), m_initialized(false) {}

    bool openDatabase() {
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        
        const QString dbPath = dataDir + "/" + "history.db";
        
        m_db = QSqlDatabase::addDatabase("QSQLITE", "history_connection");
        m_db.setDatabaseName(dbPath);
        
        if (!m_db.open()) {
            qWarning() << "HistoryDao: Failed to open database:" << m_db.lastError().text();
            return false;
        }
        
        QSqlQuery query(m_db);
        query.exec("PRAGMA foreign_keys = ON");
        query.exec("PRAGMA journal_mode = WAL");
        query.exec("PRAGMA synchronous = NORMAL");
        
        return true;
    }
    
    void closeDatabase() {
        if (m_db.isOpen()) {
            m_db.close();
        }
        QSqlDatabase::removeDatabase("history_connection");
    }
    
    bool createTables() {
        QSqlQuery query(m_db);
        
        // Main history table - one row per unique URL
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "url TEXT NOT NULL UNIQUE,"
            "normalized_url TEXT NOT NULL UNIQUE,"
            "title TEXT,"
            "visit_count INTEGER NOT NULL DEFAULT 0,"
            "typed_count INTEGER NOT NULL DEFAULT 0,"
            "last_visit_time INTEGER NOT NULL DEFAULT 0,"
            "first_visit_time INTEGER NOT NULL DEFAULT 0,"
            "last_visit_duration INTEGER NOT NULL DEFAULT 0,"
            "favicon_url TEXT,"
            "domain TEXT,"
            "transition_type TEXT DEFAULT 'link',"
            "is_deleted INTEGER NOT NULL DEFAULT 0,"
            "is_synced INTEGER NOT NULL DEFAULT 0,"
            "sync_id TEXT,"
            "extra_data TEXT"  // JSON for extensibility
            ")"
        )) {
            qWarning() << "Failed to create history table:" << query.lastError().text();
            return false;
        }
        
        // Visits table - individual visits
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS visits ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "history_id INTEGER NOT NULL,"
            "visit_time INTEGER NOT NULL,"
            "duration INTEGER NOT NULL DEFAULT 0,"
            "transition_type TEXT DEFAULT 'link',"
            "is_typed INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY(history_id) REFERENCES history(id) ON DELETE CASCADE"
            ")"
        )) {
            qWarning() << "Failed to create visits table:" << query.lastError().text();
            return false;
        }
        
        // Create indexes
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_url ON history(url)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_normalized ON history(normalized_url)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_visit_count ON history(visit_count)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_last_visit ON history(last_visit_time)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_domain ON history(domain)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_deleted ON history(is_deleted)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_visits_history ON visits(history_id)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_visits_time ON visits(visit_time)");
        
        // Settings table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS history_settings ("
            "key TEXT PRIMARY KEY,"
            "value TEXT"
            ")"
        )) {
            qWarning() << "Failed to create history_settings table:" << query.lastError().text();
            return false;
        }
        
        return true;
    }
    
    HistoryEntry rowToEntry(const QSqlQuery& query) const {
        HistoryEntry entry;
        entry.id = query.value("id").toLongLong();
        entry.title = query.value("title").toString();
        entry.url = query.value("url").toString();
        entry.normalizedUrl = query.value("normalized_url").toString();
        entry.visitCount = query.value("visit_count").toInt();
        entry.typedCount = query.value("typed_count").toInt();
        entry.visitTime = query.value("last_visit_time").toLongLong();
        entry.lastVisitTime = query.value("last_visit_time").toLongLong();
        entry.visitDuration = query.value("last_visit_duration").toInt();
        entry.faviconUrl = query.value("favicon_url").toString();
        entry.transitionType = query.value("transition_type").toString();
        entry.isDeleted = query.value("is_deleted").toBool();
        entry.isSynced = query.value("is_sync").toBool();
        
        // Extract domain from URL
        QUrl url(query.value("url").toString());
        entry.normalizedUrl = query.value("normalized_url").toString();
        
        return entry;
    }
    
    qint64 currentTimestamp() const {
        return QDateTime::currentMSecsSinceEpoch();
    }
    
    QSqlDatabase m_db;
    bool m_initialized = false;
    HistoryDao* q;
};

HistoryDao::HistoryDao(QObject *parent)
    : QObject(parent), d(new Private(this))
{
}

HistoryDao::~HistoryDao() {
    if (d->m_db.isOpen()) {
        d->m_db.close();
    }
    QSqlDatabase::removeDatabase("history_connection");
    delete d;
}

bool HistoryDao::initialize() {
    if (d->m_initialized)
        return true;
    
    if (!d->openDatabase())
        return false;
    
    if (!d->createTables())
        return false;
    
    d->m_initialized = true;
    return true;
}

qint64 HistoryDao::addVisit(const HistoryEntry& entry) {
    if (!d->m_initialized) return -1;
    
    const QString normalized = HistoryEntry::normalizedUrl(entry.url);
    if (normalized.isEmpty()) return -1;
    
    QSqlQuery query(d->m_db);
    d->m_db.transaction();
    
    // Check if URL exists
    qint64 historyId = -1;
    QSqlQuery checkQuery(d->m_db);
    checkQuery.prepare("SELECT id, visit_count, typed_count, first_visit_time FROM history WHERE normalized_url = ?");
    checkQuery.addBindValue(HistoryEntry::normalizedUrl(entry.url));
    
    if (checkQuery.exec() && checkQuery.next()) {
        // Update existing
        historyId = checkQuery.value(0).toLongLong();
        int visitCount = checkQuery.value(1).toInt() + 1;
        int typedCount = checkQuery.value(2).toInt() + (entry.transitionType == HistoryTransition::TYPED ? 1 : 0);
        qint64 firstVisit = checkQuery.value(3).toLongLong();
        
        QSqlQuery updateQuery(d->m_db);
        updateQuery.prepare("UPDATE history SET "
                           "title = ?, visit_count = ?, typed_count = ?, "
                           "last_visit_time = ?, last_visit_duration = ?, "
                           "favicon_url = ?, transition_type = ?, date_modified = ? "
                           "WHERE id = ?");
        updateQuery.addBindValue(entry.title);
        updateQuery.addBindValue(visitCount);
        updateQuery.addBindValue(typedCount);
        updateQuery.addBindValue(d->currentTimestamp());
        updateQuery.addBindValue(entry.visitDuration);
        updateQuery.addBindValue(entry.faviconUrl);
        updateQuery.addBindValue(entry.transitionType);
        updateQuery.addBindValue(d->currentTimestamp());
        updateQuery.addBindValue(historyId);
        
        if (!updateQuery.exec()) {
            d->m_db.rollback();
            return -1;
        }
    } else {
        // Insert new
        QSqlQuery insertQuery(d->m_db);
        insertQuery.prepare("INSERT INTO history ("
                           "url, normalized_url, title, visit_count, typed_count, "
                           "first_visit_time, last_visit_time, last_visit_duration, "
                           "favicon_url, domain, transition_type) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        
        QUrl url(entry.url);
        insertQuery.addBindValue(entry.url);
        insertQuery.addBindValue(HistoryEntry::normalizedUrl(entry.url));
        insertQuery.addBindValue(entry.title);
        insertQuery.addBindValue(1);
        insertQuery.addBindValue(entry.transitionType == HistoryTransition::TYPED ? 1 : 0);
        insertQuery.addBindValue(d->currentTimestamp());
        insertQuery.addBindValue(d->currentTimestamp());
        insertQuery.addBindValue(entry.visitDuration);
        insertQuery.addBindValue(entry.faviconUrl);
        insertQuery.addBindValue(url.host());
        insertQuery.addBindValue(entry.transitionType);
        
        if (!insertQuery.exec()) {
            d->m_db.rollback();
            return -1;
        }
        historyId = insertQuery.lastInsertId().toLongLong();
    }
    
    // Add visit record
    QSqlQuery visitQuery(d->m_db);
    visitQuery.prepare("INSERT INTO visits (history_id, visit_time, duration, transition_type, is_typed) "
                       "VALUES (?, ?, ?, ?, ?)");
    visitQuery.addBindValue(historyId);
    visitQuery.addBindValue(d->currentTimestamp());
    visitQuery.addBindValue(entry.visitDuration);
    visitQuery.addBindValue(entry.transitionType);
    visitQuery.addBindValue(entry.transitionType == HistoryTransition::TYPED ? 1 : 0);
    
    if (!visitQuery.exec()) {
        d->m_db.rollback();
        return -1;
    }
    
    if (!d->m_db.commit()) {
        return -1;
    }
    
    emit historyAdded(entry);
    emit dataChanged();
    return historyId;
}

HistoryEntry HistoryDao::getEntryById(qint64 id) const {
    HistoryEntry entry;
    if (!d->m_initialized) return entry;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE id = ? AND is_deleted = 0");
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        // Convert row to entry (simplified)
        entry.id = query.value("id").toLongLong();
        entry.title = query.value("title").toString();
        entry.url = query.value("url").toString();
        entry.visitCount = query.value("visit_count").toInt();
        entry.visitTime = query.value("last_visit_time").toLongLong();
    }
    return entry;
}

HistoryEntry HistoryDao::getEntryByUrl(const QString& url) const {
    HistoryEntry entry;
    if (!d->m_initialized) return entry;
    
    const QString normalized = HistoryEntry::normalizedUrl(url);
    if (normalized.isEmpty()) return entry;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE normalized_url = ? AND is_deleted = 0");
    query.addBindValue(normalized);
    
    if (query.exec() && query.next()) {
        entry.id = query.value("id").toLongLong();
        entry.title = query.value("title").toString();
        entry.url = query.value("url").toString();
        entry.visitCount = query.value("visit_count").toInt();
        entry.visitTime = query.value("last_visit_time").toLongLong();
    }
    return entry;
}

QList<HistoryEntry> HistoryDao::getAllHistory(
    const QString& searchQuery,
    const QDateTime& since,
    const QDateTime& until,
    int limit,
    int offset
) const {
    QList<HistoryEntry> result;
    if (!d->m_initialized) return result;
    
    QString sql = "SELECT * FROM history WHERE is_deleted = 0";
    QList<QVariant> params;
    
    if (!searchQuery.isEmpty()) {
        sql += " AND (LOWER(title) LIKE ? OR LOWER(url) LIKE ?)";
        QString term = "%" + searchQuery.toLower() + "%";
        params.append(term);
        params.append(term);
    }
    
    if (since.isValid()) {
        sql += " AND last_visit_time >= ?";
        params.append(since.toMSecsSinceEpoch());
    }
    
    if (until.isValid()) {
        sql += " AND last_visit_time <= ?";
        params.append(until.toMSecsSinceEpoch());
    }
    
    sql += " ORDER BY last_visit_time DESC LIMIT ? OFFSET ?";
    params.append(limit);
    params.append(offset);
    
    QSqlQuery query(d->m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    
    if (query.exec()) {
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitCount = query.value("visit_count").toInt();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            result.append(entry);
        }
    }
    return result;
}

QList<QPair<QDate, QList<HistoryEntry>>> HistoryDao::getHistoryByDay(
    const QDateTime& since,
    int daysLimit
) const {
    QList<QPair<QDate, QList<HistoryEntry>>> result;
    if (!d->m_initialized) return result;
    
    QDateTime cutoff = since.isValid() ? since : QDateTime::currentDateTime().addDays(-daysLimit);
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT DATE(last_visit_time / 1000, 'unixepoch') as visit_date, "
                  "id, url, title, visit_count, last_visit_time "
                  "FROM history "
                  "WHERE is_deleted = 0 AND last_visit_time >= ? "
                  "ORDER BY last_visit_time DESC");
    query.addBindValue(cutoff.toMSecsSinceEpoch());
    
    if (query.exec()) {
        QMap<QDate, QList<HistoryEntry>> byDate;
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitCount = query.value("visit_count").toInt();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            
            QDate date = QDate::fromString(query.value("visit_date").toString(), Qt::ISODate);
            byDate[date].append(entry);
        }
        
        for (auto it = byDate.constBegin(); it != byDate.constEnd(); ++it) {
            result.append(qMakePair(it.key(), it.value()));
        }
    }
    return result;
}

QList<HistoryEntry> HistoryDao::getTopSites(int limit) const {
    QList<HistoryEntry> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE is_deleted = 0 "
                  "ORDER BY visit_count DESC, last_visit_time DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitCount = query.value("visit_count").toInt();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            result.append(entry);
        }
    }
    return result;
}

QList<HistoryEntry> HistoryDao::getRecentlyClosed(int limit) const {
    QList<HistoryEntry> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE is_deleted = 1 "
                  "ORDER BY last_visit_time DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            result.append(entry);
        }
    }
    return result;
}

QList<HistoryEntry> HistoryDao::searchHistory(const QString& query, int limit) const {
    return getAllHistory(query, QDateTime(), QDateTime(), limit, 0);
}

QList<HistoryEntry> HistoryDao::getHistoryForDomain(const QString& domain, int limit) const {
    QList<HistoryEntry> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE domain = ? AND is_deleted = 0 "
                  "ORDER BY last_visit_time DESC LIMIT ?");
    query.addBindValue(domain);
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitCount = query.value("visit_count").toInt();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            result.append(entry);
        }
    }
    return result;
}

int HistoryDao::getVisitCount(const QString& url) const {
    if (!d->m_initialized) return 0;
    
    const QString normalized = HistoryEntry::normalizedUrl(url);
    if (normalized.isEmpty()) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT visit_count FROM history WHERE normalized_url = ?");
    query.addBindValue(normalized);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int HistoryDao::getTotalCount() const {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.exec("SELECT COUNT(*) FROM history WHERE is_deleted = 0");
    if (query.next()) return query.value(0).toInt();
    return 0;
}

int HistoryDao::getCountSince(const QDateTime& since) const {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT COUNT(*) FROM history WHERE is_deleted = 0 AND last_visit_time >= ?");
    query.addBindValue(since.toMSecsSinceEpoch());
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool HistoryDao::deleteEntry(qint64 id) {
    if (!d->m_initialized) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE history SET is_deleted = 1 WHERE id = ?");
    query.addBindValue(id);
    
    if (query.exec() && query.numRowsAffected() > 0) {
        emit historyRemoved(id);
        emit dataChanged();
        return true;
    }
    return false;
}

bool HistoryDao::deleteByUrl(const QString& url) {
    if (!d->m_initialized) return false;
    
    const QString normalized = HistoryEntry::normalizedUrl(url);
    if (normalized.isEmpty()) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE history SET is_deleted = 1 WHERE normalized_url = ?");
    query.addBindValue(normalized);
    
    if (query.exec()) {
        emit dataChanged();
        return query.numRowsAffected() > 0;
    }
    return false;
}

int HistoryDao::deleteByDomain(const QString& domain) {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE history SET is_deleted = 1 WHERE domain = ?");
    query.addBindValue(domain);
    
    if (query.exec()) {
        emit dataChanged();
        return query.numRowsAffected();
    }
    return 0;
}

int HistoryDao::deleteOlderThan(const QDateTime& before) {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE history SET is_deleted = 1 WHERE last_visit_time < ?");
    query.addBindValue(before.toMSecsSinceEpoch());
    
    if (query.exec()) {
        emit dataChanged();
        return query.numRowsAffected();
    }
    return 0;
}

int HistoryDao::deleteInRange(const QDateTime& since, const QDateTime& until) {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE history SET is_deleted = 1 WHERE last_visit_time >= ? AND last_visit_time <= ?");
    query.addBindValue(since.toMSecsSinceEpoch());
    query.addBindValue(until.toMSecsSinceEpoch());
    
    if (query.exec()) {
        emit dataChanged();
        return query.numRowsAffected();
    }
    return 0;
}

bool HistoryDao::clearAllHistory() {
    if (!d->m_initialized) return false;
    
    QSqlQuery query(d->m_db);
    if (query.exec("UPDATE history SET is_deleted = 1")) {
        emit historyCleared();
        emit dataChanged();
        return true;
    }
    return false;
}

int HistoryDao::markSynced(const QList<qint64>& ids) {
    if (!d->m_initialized || ids.isEmpty()) return 0;
    
    int count = 0;
    for (qint64 id : ids) {
        QSqlQuery query(d->m_db);
        query.prepare("UPDATE history SET is_synced = 1 WHERE id = ?");
        query.addBindValue(id);
        if (query.exec() && query.numRowsAffected() > 0)
            count++;
    }
    return count;
}

QList<HistoryEntry> HistoryDao::getUnsyncedEntries(int limit) const {
    QList<HistoryEntry> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM history WHERE is_synced = 0 AND is_deleted = 0 ORDER BY last_visit_time DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            HistoryEntry entry;
            entry.id = query.value("id").toLongLong();
            entry.title = query.value("title").toString();
            entry.url = query.value("url").toString();
            entry.visitCount = query.value("visit_count").toInt();
            entry.visitTime = query.value("last_visit_time").toLongLong();
            result.append(entry);
        }
    }
    return result;
}

int HistoryDao::importFromJson(const QJsonArray& items) {
    if (!d->m_initialized) return 0;
    
    int imported = 0;
    for (const QJsonValue& val : items) {
        QJsonObject obj = val.toObject();
        QString url = obj.value("url").toString();
        if (url.isEmpty()) continue;
        
        HistoryEntry entry;
        entry.url = url;
        entry.title = obj.value("title").toString();
        entry.visitCount = obj.value("visit_count").toInt(1);
        entry.visitTime = obj.value("last_visit_time").toVariant().toLongLong();
        entry.transitionType = obj.value("transition").toString("link");
        
        if (addVisit(entry) > 0)
            imported++;
    }
    return imported;
}

QJsonArray HistoryDao::exportToJson(const QDateTime& since, const QDateTime& until) const {
    QJsonArray result;
    if (!d->m_initialized) return result;
    
    QString sql = "SELECT * FROM history WHERE is_deleted = 0";
    QList<QVariant> params;
    
    if (since.isValid()) {
        sql += " AND last_visit_time >= ?";
        params.append(since.toMSecsSinceEpoch());
    }
    if (until.isValid()) {
        sql += " AND last_visit_time <= ?";
        params.append(until.toMSecsSinceEpoch());
    }
    sql += " ORDER BY last_visit_time DESC";
    
    QSqlQuery query(d->m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject obj;
            obj["url"] = query.value("url").toString();
            obj["title"] = query.value("title").toString();
            obj["visit_count"] = query.value("visit_count").toInt();
            obj["last_visit_time"] = query.value("last_visit_time").toLongLong();
            obj["transition"] = query.value("transition_type").toString();
            result.append(obj);
        }
    }
    return result;
}

HistoryDao::Statistics HistoryDao::getStatistics() const {
    Statistics stats;
    if (!d->m_initialized) return stats;
    
    QSqlQuery query(d->m_db);
    
    // Total visits
    query.exec("SELECT SUM(visit_count) FROM history WHERE is_deleted = 0");
    if (query.next()) stats.totalVisits = query.value(0).toInt();
    
    // Unique URLs
    query.exec("SELECT COUNT(*) FROM history WHERE is_deleted = 0");
    if (query.next()) stats.uniqueUrls = query.value(0).toInt();
    
    // Unique domains
    query.exec("SELECT COUNT(DISTINCT domain) FROM history WHERE is_deleted = 0 AND domain IS NOT NULL");
    if (query.next()) stats.uniqueDomains = query.value(0).toInt();
    
    // Total duration
    query.exec("SELECT SUM(last_visit_duration) FROM history WHERE is_deleted = 0");
    if (query.next()) stats.totalVisitDuration = query.value(0).toLongLong();
    
    // Most visited domain
    QSqlQuery domainQuery(d->m_db);
    domainQuery.exec("SELECT domain, COUNT(*) as cnt FROM history WHERE is_deleted = 0 AND domain IS NOT NULL "
                     "GROUP BY domain ORDER BY cnt DESC LIMIT 1");
    if (domainQuery.next()) {
        stats.mostVisitedDomain = domainQuery.value(0).toString();
        stats.mostVisitedDomainCount = domainQuery.value(1).toInt();
    }
    
    // First/last visit
    QSqlQuery timeQuery(d->m_db);
    timeQuery.exec("SELECT MIN(first_visit_time), MAX(last_visit_time) FROM history WHERE is_deleted = 0");
    if (timeQuery.next()) {
        stats.firstVisit = QDateTime::fromMSecsSinceEpoch(timeQuery.value(0).toLongLong());
        stats.lastVisit = QDateTime::fromMSecsSinceEpoch(timeQuery.value(1).toLongLong());
    }
    
    // Visits by day (last 30 days)
    QSqlQuery dayQuery(d->m_db);
    qint64 cutoff = QDateTime::currentDateTime().addDays(-30).toMSecsSinceEpoch();
    dayQuery.prepare("SELECT DATE(last_visit_time / 1000, 'unixepoch') as day, COUNT(*) "
                     "FROM history WHERE is_deleted = 0 AND last_visit_time >= ? "
                     "GROUP BY day ORDER BY day");
    dayQuery.addBindValue(cutoff);
    if (dayQuery.exec()) {
        while (dayQuery.next()) {
            stats.visitsByDay[dayQuery.value(0).toString()] = dayQuery.value(1).toInt();
        }
    }
    
    // Visits by domain
    QSqlQuery domainCountQuery(d->m_db);
    domainCountQuery.exec("SELECT domain, COUNT(*) FROM history WHERE is_deleted = 0 AND domain IS NOT NULL "
                          "GROUP BY domain ORDER BY COUNT(*) DESC LIMIT 20");
    while (domainCountQuery.next()) {
        stats.visitsByDomain[domainCountQuery.value(0).toString()] = domainCountQuery.value(1).toInt();
    }
    
    return stats;
}

QMap<QDate, int> HistoryDao::getDailyVisitCounts(int days) const {
    QMap<QDate, int> result;
    if (!d->m_initialized) return result;
    
    qint64 cutoff = QDateTime::currentDateTime().addDays(-days).toMSecsSinceEpoch();
    QSqlQuery query(d->m_db);
    query.prepare("SELECT DATE(visit_time / 1000, 'unixepoch') as day, COUNT(*) "
                  "FROM visits WHERE visit_time >= ? GROUP BY day ORDER BY day");
    query.addBindValue(cutoff);
    
    if (query.exec()) {
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
            result[date] = query.value(1).toInt();
        }
    }
    return result;
}

QMap<int, int> HistoryDao::getHourlyVisitDistribution() const {
    QMap<int, int> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.exec("SELECT CAST((visit_time / 1000) % 86400 / 3600 AS INTEGER) as hour, COUNT(*) "
               "FROM visits GROUP BY hour ORDER BY hour");
    
    while (query.next()) {
        result[query.value(0).toInt()] = query.value(1).toInt();
    }
    return result;
}

#include "HistoryDao.moc"