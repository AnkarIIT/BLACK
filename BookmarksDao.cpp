#include "BookmarksDao.h"
#include "OSPaths.h"
#include "VaultCrypto.h"
#include "Bookmark.h"

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
#include <QQueue>

namespace {
const QString kDbFileName = "bookmarks.db";
const QString kTableBookmarks = "bookmarks";
const QString kTableFolders = "folders";
const QString kTableSettings = "settings";

const QString kRootFolderId = "root";
const QString kOtherFolderId = "other";
const QString kMobileFolderId = "mobile";
const QString kSyncedFolderId = "synced";

const char kRootFolderName[] = "Bookmark Bar";
const char kOtherFolderName[] = "Other Bookmarks";
const char kMobileFolderName[] = "Mobile Bookmarks";
const char kSyncedFolderName[] = "Synced Bookmarks";
}

const QString BookmarksDao::ROOT_FOLDER = kRootFolderId;
const QString BookmarksDao::OTHER_FOLDER = kOtherFolderId;
const QString BookmarksDao::MOBILE_FOLDER = kMobileFolderId;
const QString BookmarksDao::SYNCED_FOLDER = kSyncedFolderId;

class BookmarksDao::Private {
public:
    Private(BookmarksDao* parent) : q(parent), m_db(), m_initialized(false) {}

    bool openDatabase() {
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        
        const QString dbPath = dataDir + "/" + "bookmarks.db";
        
        m_db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks_connection");
        m_db.setDatabaseName(dbPath);
        
        if (!m_db.open()) {
            qWarning() << "BookmarksDao: Failed to open database:" << m_db.lastError().text();
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
        QSqlDatabase::removeDatabase("bookmarks_connection");
    }
    
    bool createTables() {
        QSqlQuery query(m_db);
        
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS bookmarks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "url TEXT NOT NULL,"
            "folder_id TEXT NOT NULL DEFAULT 'root',"
            "date_added INTEGER NOT NULL DEFAULT 0,"
            "date_modified INTEGER NOT NULL DEFAULT 0,"
            "favicon_url TEXT,"
            "is_folder INTEGER NOT NULL DEFAULT 0,"
            "position INTEGER NOT NULL DEFAULT 0,"
            "UNIQUE(url, folder_id)"
            ")"
        )) {
            qWarning() << "Failed to create bookmarks table:" << query.lastError().text();
            return false;
        }
        
        query.exec("CREATE INDEX IF NOT EXISTS idx_bookmarks_folder ON bookmarks(folder_id)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_bookmarks_url ON bookmarks(url)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_bookmarks_date_added ON bookmarks(date_added)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_bookmarks_is_folder ON bookmarks(is_folder)");
        
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS folders ("
            "id TEXT PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "parent_id TEXT NOT NULL DEFAULT 'root',"
            "position INTEGER NOT NULL DEFAULT 0,"
            "date_added INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY(parent_id) REFERENCES folders(id) ON DELETE CASCADE"
            ")"
        )) {
            qWarning() << "Failed to create folders table:" << query.lastError().text();
            return false;
        }
        
        query.exec("CREATE INDEX IF NOT EXISTS idx_folders_parent ON folders(parent_id)");
        
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS settings ("
            "key TEXT PRIMARY KEY,"
            "value TEXT"
            ")"
        )) {
            qWarning() << "Failed to create settings table:" << query.lastError().text();
            return false;
        }
        
        return true;
    }
    
    bool ensureRootFolders() {
        QSqlQuery query(m_db);
        
        query.prepare("SELECT COUNT(*) FROM folders WHERE id IN ('root', 'other', 'mobile', 'synced')");
        if (!query.exec() || !query.next() || query.value(0).toInt() < 4) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            
            struct Folder {
                QString id;
                QString name;
                QString parent;
                int position;
            };
            
            const struct { QString id; QString name; QString parent; int position; } folders[] = {
                {"root", "Bookmark Bar", "", 0},
                {"other", "Other Bookmarks", "", 1},
                {"mobile", "Mobile Bookmarks", "", 2},
                {"synced", "Synced Bookmarks", "", 3},
            };
            
            for (const auto& f : folders) {
                QSqlQuery insertQuery(m_db);
                insertQuery.prepare("INSERT OR IGNORE INTO folders (id, name, parent_id, position, date_added) VALUES (?, ?, ?, ?, ?)");
                insertQuery.addBindValue(f.id);
                insertQuery.addBindValue(f.name);
                insertQuery.addBindValue(f.parent);
                insertQuery.addBindValue(f.position);
                insertQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
                if (!insertQuery.exec()) {
                    qWarning() << "Failed to insert root folder:" << insertQuery.lastError().text();
                }
            }
        }
        return true;
    }
    
    qint64 currentTimestamp() const {
        return QDateTime::currentMSecsSinceEpoch();
    }
    
    Bookmark rowToBookmark(const QSqlQuery& query) const {
        Bookmark b;
        b.id = query.value("id").toLongLong();
        b.title = query.value("title").toString();
        b.url = query.value("url").toString();
        b.folderId = query.value("folder_id").toString();
        b.dateAdded = query.value("date_added").toLongLong();
        b.dateModified = query.value("date_modified").toLongLong();
        b.faviconUrl = query.value("favicon_url").toString();
        b.isFolder = query.value("is_folder").toBool();
        b.position = query.value("position").toInt();
        return b;
    }
    
    Bookmark folderRowToBookmark(const QSqlQuery& query) const {
        Bookmark b;
        b.id = query.value("id").toString().toLongLong();
        b.title = query.value("name").toString();
        b.url = "";
        b.folderId = query.value("parent_id").toString();
        b.dateAdded = query.value("date_added").toLongLong();
        b.dateModified = 0;
        b.faviconUrl = "";
        b.isFolder = true;
        b.position = query.value("position").toInt();
        return b;
    }
    
    QSqlDatabase m_db;
    bool m_initialized = false;
    BookmarksDao* q;
};

BookmarksDao::BookmarksDao(QObject *parent)
    : QObject(parent), d(new Private(this))
{
}

BookmarksDao::~BookmarksDao() {
    if (d->m_db.isOpen()) {
        d->m_db.close();
    }
    QSqlDatabase::removeDatabase("bookmarks_connection");
    delete d;
}

bool BookmarksDao::initialize() {
    if (d->m_initialized)
        return true;
    
    if (!d->openDatabase())
        return false;
    
    if (!d->createTables())
        return false;
    
    if (!d->ensureRootFolders())
        return false;
    
    d->m_initialized = true;
    return true;
}

// ===== Bookmarks =====

QList<Bookmark> BookmarksDao::getAllBookmarks(const QString& folderId) const {
    QList<Bookmark> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    if (folderId.isEmpty()) {
        query.prepare("SELECT * FROM bookmarks WHERE is_folder = 0 ORDER BY position, date_added DESC");
    } else {
        query.prepare("SELECT * FROM bookmarks WHERE folder_id = ? AND is_folder = 0 ORDER BY position, date_added DESC");
        query.addBindValue(folderId);
    }
    
    if (query.exec()) {
        while (query.next()) {
            result.append(d->rowToBookmark(query));
        }
    }
    return result;
}

Bookmark BookmarksDao::getBookmarkById(qint64 id) const {
    Bookmark b;
    if (!d->m_initialized) return b;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM bookmarks WHERE id = ?");
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        return d->rowToBookmark(query);
    }
    return Bookmark();
}

Bookmark BookmarksDao::getBookmarkByUrl(const QString& url) const {
    Bookmark b;
    if (!d->m_initialized) return b;
    
    const QString normalized = Bookmark::normalizedUrl(url);
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM bookmarks WHERE url = ? AND is_folder = 0");
    query.addBindValue(normalized);
    
    if (query.exec() && query.next()) {
        return d->rowToBookmark(query);
    }
    return Bookmark();
}

qint64 BookmarksDao::insertBookmark(const Bookmark& bookmark) {
    if (!d->m_initialized) return -1;
    
    const QString normalizedUrl = Bookmark::normalizedUrl(bookmark.url);
    if (normalizedUrl.isEmpty()) return -1;
    
    QSqlQuery query(d->m_db);
    query.prepare("INSERT INTO bookmarks (title, url, folder_id, date_added, date_modified, favicon_url, is_folder, position) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(bookmark.title);
    query.addBindValue(Bookmark::normalizedUrl(bookmark.url));
    query.addBindValue(bookmark.folderId.isEmpty() ? "root" : bookmark.folderId);
    query.addBindValue(d->currentTimestamp());
    query.addBindValue(d->currentTimestamp());
    query.addBindValue(bookmark.faviconUrl);
    query.addBindValue(bookmark.isFolder ? 1 : 0);
    query.addBindValue(bookmark.position);
    
    if (query.exec()) {
        qint64 id = query.lastInsertId().toLongLong();
        Bookmark b = bookmark;
        b.id = id;
        emit bookmarkAdded(b);
        emit dataChanged();
        return id;
    }
    
    qWarning() << "Failed to insert bookmark:" << query.lastError().text();
    return -1;
}

bool BookmarksDao::updateBookmark(const Bookmark& bookmark) {
    if (!d->m_initialized) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE bookmarks SET title = ?, url = ?, folder_id = ?, date_modified = ?, "
                  "favicon_url = ?, is_folder = ?, position = ? WHERE id = ?");
    query.addBindValue(bookmark.title);
    query.addBindValue(Bookmark::normalizedUrl(bookmark.url));
    query.addBindValue(bookmark.folderId.isEmpty() ? "root" : bookmark.folderId);
    query.addBindValue(d->currentTimestamp());
    query.addBindValue(bookmark.faviconUrl);
    query.addBindValue(bookmark.isFolder ? 1 : 0);
    query.addBindValue(bookmark.position);
    query.addBindValue(bookmark.id);
    
    if (query.exec()) {
        emit bookmarkUpdated(bookmark);
        emit dataChanged();
        return true;
    }
    
    qWarning() << "Failed to update bookmark:" << query.lastError().text();
    return false;
}

bool BookmarksDao::deleteBookmark(qint64 id) {
    if (!d->m_initialized) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("DELETE FROM bookmarks WHERE id = ?");
    query.addBindValue(id);
    
    if (query.exec()) {
        emit bookmarkRemoved(id);
        emit dataChanged();
        return true;
    }
    return false;
}

bool BookmarksDao::deleteBookmarkByUrl(const QString& url) {
    if (!d->m_initialized) return false;
    
    const QString normalized = Bookmark::normalizedUrl(url);
    QSqlQuery query(d->m_db);
    query.prepare("DELETE FROM bookmarks WHERE url = ?");
    query.addBindValue(Bookmark::normalizedUrl(url));
    
    if (query.exec()) {
        emit dataChanged();
        return query.numRowsAffected() > 0;
    }
    return false;
}

bool BookmarksDao::moveBookmarkToFolder(qint64 bookmarkId, const QString& folderId) {
    if (!d->m_initialized) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE bookmarks SET folder_id = ?, date_modified = ? WHERE id = ?");
    query.addBindValue(folderId.isEmpty() ? "root" : folderId);
    query.addBindValue(d->currentTimestamp());
    query.addBindValue(bookmarkId);
    
    if (query.exec()) {
        emit dataChanged();
        return true;
    }
    return false;
}

QList<Bookmark> BookmarksDao::getBookmarksByFolder(const QString& folderId, bool recursive) const {
    QList<Bookmark> result;
    if (!d->m_initialized) return result;
    
    if (!recursive) {
        return getAllBookmarks(folderId);
    }
    
    // Recursive: get folder and all subfolders
    QSet<QString> folderIds;
    folderIds.insert(folderId);
    
    // Get all descendant folders
    QQueue<QString> queue;
    queue.enqueue(folderId);
    
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        QSqlQuery query(d->m_db);
        query.prepare("SELECT id FROM folders WHERE parent_id = ?");
        query.addBindValue(current);
        if (query.exec()) {
            while (query.next()) {
                QString childId = query.value(0).toString();
                if (!folderIds.contains(childId)) {
                    folderIds.insert(childId);
                    queue.enqueue(childId);
                }
            }
        }
    }
    
    // Get bookmarks from all folders
    for (const QString& fid : folderIds) {
        QSqlQuery query(d->m_db);
        query.prepare("SELECT * FROM bookmarks WHERE folder_id = ? AND is_folder = 0 ORDER BY position, date_added DESC");
        query.addBindValue(fid);
        if (query.exec()) {
            while (query.next()) {
                result.append(d->rowToBookmark(query));
            }
        }
    }
    
    return result;
}

QList<Bookmark> BookmarksDao::searchBookmarks(const QString& query) const {
    QList<Bookmark> result;
    if (!d->m_initialized || query.trimmed().isEmpty()) return result;
    
    QString searchTerm = "%" + query.toLower() + "%";
    QSqlQuery q(d->m_db);
    q.prepare("SELECT * FROM bookmarks WHERE (LOWER(title) LIKE ? OR LOWER(url) LIKE ?) AND is_folder = 0 "
              "ORDER BY date_added DESC");
    q.addBindValue(searchTerm);
    q.addBindValue(searchTerm);
    
    if (q.exec()) {
        while (q.next()) {
            result.append(d->rowToBookmark(q));
        }
    }
    return result;
}

QList<Bookmark> BookmarksDao::getRecentBookmarks(int limit) const {
    QList<Bookmark> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT * FROM bookmarks WHERE is_folder = 0 ORDER BY date_added DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            result.append(d->rowToBookmark(query));
        }
    }
    return result;
}

int BookmarksDao::getBookmarkCount(const QString& folderId) const {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    if (folderId.isEmpty()) {
        query.prepare("SELECT COUNT(*) FROM bookmarks WHERE is_folder = 0");
    } else {
        query.prepare("SELECT COUNT(*) FROM bookmarks WHERE folder_id = ? AND is_folder = 0");
        query.addBindValue(folderId);
    }
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ===== Folders =====

qint64 BookmarksDao::createFolder(const QString& name, const QString& parentId) {
    if (!d->m_initialized || name.trimmed().isEmpty()) return -1;
    
    QSqlQuery query(d->m_db);
    query.prepare("INSERT INTO folders (id, name, parent_id, position, date_added) VALUES (?, ?, ?, ?, ?)");
    
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    query.addBindValue(id);
    query.addBindValue(name.trimmed());
    query.addBindValue(parentId.isEmpty() ? "root" : parentId);
    query.addBindValue(0);
    query.addBindValue(d->currentTimestamp());
    
    if (query.exec()) {
        Bookmark folder;
        folder.id = id.toLongLong();
        folder.title = name;
        folder.folderId = parentId.isEmpty() ? "root" : parentId;
        folder.isFolder = true;
        folder.dateAdded = d->currentTimestamp();
        emit folderAdded(folder);
        emit dataChanged();
        return id.toLongLong();
    }
    return -1;
}

bool BookmarksDao::renameFolder(qint64 folderId, const QString& newName) {
    if (!d->m_initialized || newName.trimmed().isEmpty()) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE folders SET name = ? WHERE id = ?");
    query.addBindValue(newName.trimmed());
    query.addBindValue(folderId);
    
    if (query.exec() && query.numRowsAffected() > 0) {
        emit dataChanged();
        return true;
    }
    return false;
}

bool BookmarksDao::deleteFolder(qint64 folderId, bool deleteChildren) {
    if (!d->m_initialized) return false;
    
    QString idStr = QString::number(folderId);
    
    // Prevent deleting root folders
    if (idStr == "root" || idStr == "other" || idStr == "mobile" || idStr == "synced")
        return false;
    
    if (deleteChildren) {
        // Delete all bookmarks in this folder and subfolders
        QSet<QString> folderIds;
        QQueue<QString> queue;
        folderIds.insert(idStr);
        queue.enqueue(idStr);
        
        while (!queue.isEmpty()) {
            QString current = queue.dequeue();
            QSqlQuery q(d->m_db);
            q.prepare("SELECT id FROM folders WHERE parent_id = ?");
            q.addBindValue(current);
            if (q.exec()) {
                while (q.next()) {
                    QString childId = q.value(0).toString();
                    if (!folderIds.contains(childId)) {
                        folderIds.insert(childId);
                        queue.enqueue(childId);
                    }
                }
            }
        }
        
        // Delete bookmarks
        for (const QString& fid : folderIds) {
            QSqlQuery delQuery(d->m_db);
            delQuery.prepare("DELETE FROM bookmarks WHERE folder_id = ?");
            delQuery.addBindValue(fid);
            delQuery.exec();
        }
    } else {
        // Move bookmarks to "other" folder
        QSqlQuery moveQuery(d->m_db);
        moveQuery.prepare("UPDATE bookmarks SET folder_id = 'other', date_modified = ? WHERE folder_id = ?");
        moveQuery.addBindValue(d->currentTimestamp());
        moveQuery.addBindValue(idStr);
        moveQuery.exec();
    }
    
    // Delete folder and subfolders (CASCADE handles subfolders)
    QSqlQuery delFolder(d->m_db);
    delFolder.prepare("DELETE FROM folders WHERE id = ?");
    delFolder.addBindValue(idStr);
    
    if (delFolder.exec()) {
        emit folderRemoved(folderId);
        emit dataChanged();
        return true;
    }
    return false;
}

Bookmark BookmarksDao::getFolder(qint64 folderId) const {
    Bookmark b;
    if (!d->m_initialized) return b;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT id, name, parent_id, position, date_added FROM folders WHERE id = ?");
    query.addBindValue(QString::number(folderId));
    
    if (query.exec() && query.next()) {
        return d->folderRowToBookmark(query);
    }
    return Bookmark();
}

Bookmark BookmarksDao::getFolderByName(const QString& name, const QString& parentId) const {
    Bookmark b;
    if (!d->m_initialized) return b;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT id, name, parent_id, position, date_added FROM folders WHERE name = ? AND parent_id = ?");
    query.addBindValue(name);
    query.addBindValue(parentId.isEmpty() ? "root" : parentId);
    
    if (query.exec() && query.next()) {
        return d->folderRowToBookmark(query);
    }
    return Bookmark();
}

QList<Bookmark> BookmarksDao::getSubFolders(const QString& parentId) const {
    QList<Bookmark> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT id, name, parent_id, position, date_added FROM folders WHERE parent_id = ? ORDER BY position");
    query.addBindValue(parentId.isEmpty() ? "root" : parentId);
    
    if (query.exec()) {
        while (query.next()) {
            result.append(d->folderRowToBookmark(query));
        }
    }
    return result;
}

QList<Bookmark> BookmarksDao::getAllFolders() const {
    QList<Bookmark> result;
    if (!d->m_initialized) return result;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT id, name, parent_id, position, date_added FROM folders ORDER BY position");
    
    if (query.exec()) {
        while (query.next()) {
            result.append(d->folderRowToBookmark(query));
        }
    }
    return result;
}

bool BookmarksDao::moveFolder(qint64 folderId, const QString& newParentId) {
    if (!d->m_initialized) return false;
    
    QString idStr = QString::number(folderId);
    
    // Prevent moving root folders
    if (idStr == "root" || idStr == "other" || idStr == "mobile" || idStr == "synced")
        return false;
    
    // Prevent moving into self or descendant
    // (simplified check - would need recursive check for full safety)
    if (newParentId == QString::number(folderId))
        return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE folders SET parent_id = ? WHERE id = ?");
    query.addBindValue(newParentId.isEmpty() ? "root" : newParentId);
    query.addBindValue(folderId);
    
    if (query.exec()) {
        emit dataChanged();
        return true;
    }
    return false;
}

QJsonObject BookmarksDao::getFolderTree() const {
    QJsonObject tree;
    if (!d->m_initialized) return tree;
    
    // Build tree recursively
    std::function<QJsonObject(const QString&)> buildNode = [&](const QString& folderId) -> QJsonObject {
        QJsonObject node;
        
        Bookmark folder = getFolder(folderId.toLongLong());
        if (folder.id == 0 && folderId != "root" && folderId != "other" && folderId != "mobile" && folderId != "synced")
            return QJsonObject();
        
        node["id"] = folderId;
        node["title"] = folder.title;
        node["isFolder"] = true;
        
        QJsonArray children;
        QList<Bookmark> subFolders = getSubFolders(folderId);
        for (const Bookmark& sub : subFolders) {
            children.append(buildNode(QString::number(sub.id)));
        }
        
        // Add bookmarks in this folder
        QList<Bookmark> bookmarks = getAllBookmarks(folderId);
        for (const Bookmark& bm : bookmarks) {
            QJsonObject bmObj;
            bmObj["id"] = bm.id;
            bmObj["title"] = bm.title;
            bmObj["url"] = bm.url;
            bmObj["isFolder"] = false;
            children.append(bmObj);
        }
        
        node["children"] = children;
        return node;
    };
    
    return buildNode("root");
}

// ===== Import/Export =====

int BookmarksDao::importFromJson(const QJsonArray& items, const QString& targetFolderId) {
    if (!d->m_initialized) return 0;
    
    int imported = 0;
    QString folder = targetFolderId.isEmpty() ? "root" : targetFolderId;
    
    for (const QJsonValue& val : items) {
        QJsonObject obj = val.toObject();
        QString url = obj.value("url").toString();
        QString title = obj.value("title").toString();
        
        if (url.isEmpty() || title.isEmpty())
            continue;
        
        // Check if already exists
        if (isBookmarked(url))
            continue;
        
        Bookmark bm;
        bm.title = title;
        bm.url = url;
        bm.folderId = folder;
        bm.dateAdded = d->currentTimestamp();
        bm.dateModified = d->currentTimestamp();
        
        if (insertBookmark(bm) > 0)
            imported++;
    }
    return imported;
}

QJsonArray BookmarksDao::exportToJson(const QString& folderId) const {
    QJsonArray result;
    if (!d->m_initialized) return result;
    
    QList<Bookmark> bookmarks = folderId.isEmpty() ? getAllBookmarks() : getAllBookmarks(folderId);
    
    for (const Bookmark& bm : bookmarks) {
        QJsonObject obj;
        obj["title"] = bm.title;
        obj["url"] = bm.url;
        obj["date_added"] = bm.dateAdded;
        if (!bm.faviconUrl.isEmpty())
            obj["favicon"] = bm.faviconUrl;
        result.append(obj);
    }
    return result;
}

QJsonObject BookmarksDao::exportFolderTree() const {
    return getFolderTree();
}

// ===== Utilities =====

int BookmarksDao::getFolderItemCount(const QString& folderId, bool recursive) const {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    if (!recursive) {
        query.prepare("SELECT COUNT(*) FROM bookmarks WHERE folder_id = ? AND is_folder = 0");
        query.addBindValue(folderId);
    } else {
        // Count recursively
        QSet<QString> folderIds;
        folderIds.insert(folderId);
        QQueue<QString> queue;
        queue.enqueue(folderId);
        
        while (!queue.isEmpty()) {
            QString current = queue.dequeue();
            QSqlQuery q(d->m_db);
            q.prepare("SELECT id FROM folders WHERE parent_id = ?");
            q.addBindValue(current);
            if (q.exec()) {
                while (q.next()) {
                    QString childId = q.value(0).toString();
                    if (!folderIds.contains(childId)) {
                        folderIds.insert(childId);
                        queue.enqueue(childId);
                    }
                }
            }
        }
        
        int total = 0;
        for (const QString& fid : folderIds) {
            QSqlQuery q(d->m_db);
            q.prepare("SELECT COUNT(*) FROM bookmarks WHERE folder_id = ? AND is_folder = 0");
            q.addBindValue(fid);
            if (q.exec() && q.next()) {
                total += q.value(0).toInt();
            }
        }
        return total;
    }
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool BookmarksDao::isBookmarked(const QString& url) const {
    if (!d->m_initialized) return false;
    const QString normalized = Bookmark::normalizedUrl(url);
    if (normalized.isEmpty()) return false;
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT 1 FROM bookmarks WHERE url = ? AND is_folder = 0 LIMIT 1");
    query.addBindValue(normalized);
    
    return query.exec() && query.next();
}

QString BookmarksDao::getFaviconUrl(const QString& url) const {
    if (!d->m_initialized) return QString();
    
    QSqlQuery query(d->m_db);
    query.prepare("SELECT favicon_url FROM bookmarks WHERE url = ? AND is_folder = 0 LIMIT 1");
    query.addBindValue(Bookmark::normalizedUrl(url));
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

void BookmarksDao::setFaviconUrl(const QString& url, const QString& faviconUrl) {
    if (!d->m_initialized) return;
    
    QSqlQuery query(d->m_db);
    query.prepare("UPDATE bookmarks SET favicon_url = ?, date_modified = ? WHERE url = ?");
    query.addBindValue(faviconUrl);
    query.addBindValue(d->currentTimestamp());
    query.addBindValue(Bookmark::normalizedUrl(url));
    query.exec();
}

int BookmarksDao::deduplicate() {
    if (!d->m_initialized) return 0;
    
    QSqlQuery query(d->m_db);
    query.prepare("DELETE FROM bookmarks WHERE id NOT IN ("
                  "SELECT MIN(id) FROM bookmarks WHERE is_folder = 0 "
                  "GROUP BY url, folder_id)");
    
    if (query.exec()) {
        return query.numRowsAffected();
    }
    return 0;
}

#include "BookmarksDao.moc"