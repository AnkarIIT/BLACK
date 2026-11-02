#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QIcon>

// Room entity for bookmarks
// Mirrors Safari's Bookmark entity with folder support
struct Bookmark
{
    qint64 id = 0;                    // Primary key, auto-generated
    QString title;
    QString url;
    QString folderId;                 // Empty = root/bookmark bar
    qint64 dateAdded = 0;             // Milliseconds since epoch
    qint64 dateModified = 0;          // Milliseconds since epoch
    QString faviconUrl;               // Favicon URL for caching
    bool isFolder = false;            // true = folder, false = bookmark
    int position = 0;                 // Position within parent folder

    // For UI
    QIcon favicon;
    QString normalizedUrl() const;
    bool isValid() const;

    bool operator==(const Bookmark& other) const;
};

Q_DECLARE_METATYPE(Bookmark)

#endif // BOOKMARK_H