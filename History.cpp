#include "History.h"
#include "ShelfStore.h"
#include <QUrl>
#include <QDateTime>

static QString HistoryEntry::normalizedUrl(const QString& url)
{
    return ShelfStore::normalizedUrl(url);
}

QString HistoryEntry::domain() const
{
    QUrl qurl(url);
    return qurl.host();
}

QString HistoryEntry::timeAgo() const
{
    if (visitTime == 0)
        return QString();
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 diff = now - visitTime;
    
    if (diff < 60000) // Less than 1 minute
        return QStringLiteral("Just now");
    else if (diff < 3600000) { // Less than 1 hour
        int minutes = diff / 60000;
        return QString("%1 minutes ago").arg(minutes);
    } else if (diff < 86400000) { // Less than 1 day
        int hours = diff / 3600000;
        return QString("%1 hours ago").arg(hours);
    } else if (diff < 604800000) { // Less than 1 week
        int days = diff / 86400000;
        return QString("%1 days ago").arg(days);
    } else {
        QDateTime dt;
        dt.setMSecsSinceEpoch(visitTime);
        return dt.toString(Qt::SystemLocaleShortDate);
    }
}

bool HistoryEntry::isValid() const
{
    if (url.isEmpty() || title.isEmpty())
        return false;
    QUrl qurl(url);
    return qurl.isValid() && (qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"));
}

bool HistoryEntry::operator==(const HistoryEntry& other) const
{
    return id == other.id;
}