#ifndef BOOKMARKIMPORTER_H
#define BOOKMARKIMPORTER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QList>

// Result of scanning a known browser installation for an importable
// "Bookmarks" file.
struct BrowserInfo {
    QString id;        // OSPaths browser id ("chrome", "edge", ...)
    QString name;      // Display name ("Chrome", "Edge", ...)
    int count = 0;     // Number of URL bookmarks found
    bool found = false; // Whether a readable Bookmarks file exists
};

// Reads bookmark exports from other installed browsers (Chrome, Edge, Brave,
// Vivaldi) via OSPaths::browserProfilePath(). The source "Bookmarks" file is
// copied into a temporary directory before parsing so a browser running in the
// background can never hand us a torn/mid-write read.
class BookmarkImporter : public QObject
{
    Q_OBJECT
public:
    explicit BookmarkImporter(QObject *parent = nullptr);

    // Scans every known browser profile directory. Not Q_INVOKABLE because
    // QList<BrowserInfo> cannot be marshalled over the web channel; use
    // scanBrowsersJson() from JavaScript.
    QList<BrowserInfo> scanBrowsers() const;

    // Imports all URL bookmarks from the given browser id as a QJsonArray of
    // { "title", "url" } objects. Empty when the browser is missing or its
    // file is unreadable.
    Q_INVOKABLE QJsonArray importBookmarksFrom(const QString &browserId) const;

    // Web-channel-friendly form of scanBrowsers(): array of
    // { "id", "name", "count", "found" } objects.
    Q_INVOKABLE QJsonArray scanBrowsersJson() const;

private:
    // Reads <profile>/Bookmarks via a temporary safe copy. Returns false if
    // the browser is not installed or the file cannot be parsed.
    bool loadBookmarks(const QString &browserId, QJsonArray &out) const;

    // Recursively collects "url" nodes, skipping empty entries and the
    // Guest/System subtrees.
    void walkNode(const QJsonObject &node, QJsonArray &out) const;
};

#endif // BOOKMARKIMPORTER_H
