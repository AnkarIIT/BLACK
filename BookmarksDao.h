#ifndef BOOKMARKSDAO_H
#define BOOKMARKSDAO_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include "Bookmark.h"

// Room DAO interface for bookmarks
// Mirrors Safari's BookmarksDao with full CRUD + folder support
class BookmarksDao : public QObject
{
    Q_OBJECT
public:
    explicit BookmarksDao(QObject *parent = nullptr);
    virtual ~BookmarksDao();

    // Initialize database tables
    bool initialize();

    // ===== Bookmarks =====
    // Get all bookmarks (optionally filtered by folder)
    QList<Bookmark> getAllBookmarks(const QString& folderId = QString()) const;
    
    // Get bookmark by ID
    Bookmark getBookmarkById(qint64 id) const;
    
    // Get bookmark by URL
    Bookmark getBookmarkByUrl(const QString& url) const;
    
    // Insert new bookmark (returns new ID, or -1 on failure)
    qint64 insertBookmark(const Bookmark& bookmark);
    
    // Update existing bookmark
    bool updateBookmark(const Bookmark& bookmark);
    
    // Delete bookmark by ID
    bool deleteBookmark(qint64 id);
    
    // Delete bookmark by URL
    bool deleteBookmarkByUrl(const QString& url);
    
    // Move bookmark to folder
    bool moveBookmarkToFolder(qint64 bookmarkId, const QString& folderId);
    
    // Get bookmarks by folder (recursive or flat)
    QList<Bookmark> getBookmarksByFolder(const QString& folderId, bool recursive = false) const;
    
    // Search bookmarks by title/URL
    QList<Bookmark> searchBookmarks(const QString& query) const;
    
    // Get recent bookmarks
    QList<Bookmark> getRecentBookmarks(int limit = 10) const;
    
    // Get bookmark count
    int getBookmarkCount(const QString& folderId = QString()) const;

    // ===== Folders =====
    // Root folders
    static const QString ROOT_FOLDER;          // Bookmark bar
    static const QString OTHER_FOLDER;         // Other bookmarks
    static const QString MOBILE_FOLDER;        // Mobile bookmarks
    static const QString SYNCED_FOLDER;        // Synced bookmarks
    
    // Folder operations
    qint64 createFolder(const QString& name, const QString& parentId = ROOT_FOLDER);
    bool renameFolder(qint64 folderId, const QString& newName);
    bool deleteFolder(qint64 folderId, bool deleteChildren = false);
    Bookmark getFolder(qint64 folderId) const;
    Bookmark getFolderByName(const QString& name, const QString& parentId) const;
    QList<Bookmark> getSubFolders(const QString& parentId) const;
    QList<Bookmark> getAllFolders() const;
    bool moveFolder(qint64 folderId, const QString& newParentId);
    
    // Folder tree
    QJsonObject getFolderTree() const;
    
    // ===== Import/Export =====
    // Import from JSON array (Chrome format)
    int importFromJson(const QJsonArray& items, const QString& targetFolderId = QString());
    
    // Export to JSON (Chrome format)
    QJsonArray exportToJson(const QString& folderId = QString()) const;
    
    // Export folder tree
    QJsonObject exportFolderTree() const;
    
    // ===== Utilities =====
    // Get bookmark count in folder (including subfolders)
    int getFolderItemCount(const QString& folderId, bool recursive = true) const;
    
    // Check if URL is already bookmarked
    bool isBookmarked(const QString& url) const;
    
    // Get favicon URL for bookmark
    QString getFaviconUrl(const QString& url) const;
    void setFaviconUrl(const QString& url, const QString& faviconUrl);
    
    // Deduplication
    int deduplicate();
    
signals:
    void bookmarkAdded(const Bookmark& bookmark);
    void bookmarkUpdated(const Bookmark& bookmark);
    void bookmarkRemoved(qint64 id);
    void folderAdded(const Bookmark& folder);
    void folderUpdated(const Bookmark& folder);
    void folderRemoved(qint64 id);
    void dataChanged();

private:
    class Private;
    Private* d;
};

#endif // BOOKMARKSDAO_H