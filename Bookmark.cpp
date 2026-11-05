#include "Bookmark.h"
#include "ShelfStore.h"

QString Bookmark::normalizedUrl(const QString& url)
{
    return ShelfStore::normalizedUrl(url);
}

bool Bookmark::isValid() const
{
    if (url.isEmpty() || title.isEmpty())
        return false;
    QUrl qurl(url);
    return qurl.isValid() && (qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"));
}

bool Bookmark::operator==(const Bookmark& other) const
{
    return id == other.id;
}