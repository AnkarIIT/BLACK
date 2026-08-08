#include "BookmarkImporter.h"
#include "OSPaths.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>

namespace {
struct KnownBrowser {
    const char *id;
    const char *name;
};
const KnownBrowser kKnownBrowsers[] = {
    { "chrome",  "Chrome" },
    { "edge",    "Edge" },
    { "brave",   "Brave" },
    { "vivaldi", "Vivaldi" },
};
}

BookmarkImporter::BookmarkImporter(QObject *parent)
    : QObject(parent)
{
}

QList<BrowserInfo> BookmarkImporter::scanBrowsers() const
{
    QList<BrowserInfo> result;
    for (const KnownBrowser &known : kKnownBrowsers) {
        BrowserInfo info;
        info.id = QLatin1String(known.id);
        info.name = QLatin1String(known.name);
        const QString profile = OSPaths::browserProfilePath(info.id);
        const QString file = profile.isEmpty()
            ? QString()
            : profile + QStringLiteral("/Bookmarks");
        info.found = !file.isEmpty() && QFileInfo::exists(file);
        if (info.found) {
            QJsonArray items;
            if (loadBookmarks(info.id, items))
                info.count = items.size();
        }
        result.append(info);
    }
    return result;
}

QJsonArray BookmarkImporter::scanBrowsersJson() const
{
    QJsonArray out;
    const QList<BrowserInfo> browsers = scanBrowsers();
    for (const BrowserInfo &browser : browsers) {
        QJsonObject entry;
        entry[QStringLiteral("id")] = browser.id;
        entry[QStringLiteral("name")] = browser.name;
        entry[QStringLiteral("count")] = browser.count;
        entry[QStringLiteral("found")] = browser.found;
        out.append(entry);
    }
    return out;
}

QJsonArray BookmarkImporter::importBookmarksFrom(const QString &browserId) const
{
    QJsonArray out;
    loadBookmarks(browserId, out);
    return out;
}

bool BookmarkImporter::loadBookmarks(const QString &browserId, QJsonArray &out) const
{
    const QString profile = OSPaths::browserProfilePath(browserId);
    if (profile.isEmpty())
        return false;
    const QString source = profile + QStringLiteral("/Bookmarks");
    if (!QFileInfo::exists(source))
        return false;

    // Copy into a temp dir first: if the browser is currently running and
    // writing its file, we read a consistent snapshot instead of a torn file.
    QTemporaryDir tmp;
    if (!tmp.isValid())
        return false;
    const QString copyPath = tmp.filePath(QStringLiteral("Bookmarks"));
    if (!QFile::copy(source, copyPath))
        return false;

    QFile file(copyPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    const QJsonObject roots = doc.object().value(QStringLiteral("roots")).toObject();
    const QString sections[] = {
        QStringLiteral("bookmark_bar"),
        QStringLiteral("other"),
        QStringLiteral("synced"),
    };
    for (const QString &section : sections)
        walkNode(roots.value(section).toObject(), out);
    return true;
}

void BookmarkImporter::walkNode(const QJsonObject &node, QJsonArray &out) const
{
    if (node.isEmpty())
        return;

    const QString name = node.value(QStringLiteral("name")).toString();
    const QString lowerName = name.toLower();
    // Skip Guest/System profiles and everything nested under them.
    if (lowerName == QLatin1String("guest") || lowerName == QLatin1String("system"))
        return;

    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("url")) {
        const QString url = node.value(QStringLiteral("url")).toString().trimmed();
        if (url.isEmpty())
            return;
        const QString title = name.trimmed();
        QJsonObject item;
        item[QStringLiteral("title")] = title.isEmpty() ? url : title;
        item[QStringLiteral("url")] = url;
        out.append(item);
        return;
    }

    const QJsonArray children = node.value(QStringLiteral("children")).toArray();
    for (const QJsonValue &child : children)
        walkNode(child.toObject(), out);
}
