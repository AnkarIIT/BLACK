#ifndef HISTORY_H
#define HISTORY_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QIcon>

// Room entity for history
// Mirrors Safari's History entity with visit counting and clustering support
struct HistoryEntry
{
    qint64 id = 0;                    // Primary key, auto-generated
    QString title;
    QString url;
    QString normalizedUrlValue;       // Normalized for deduplication
    qint64 visitTime = 0;             // Milliseconds since epoch
    qint64 lastVisitTime = 0;         // Most recent visit
    int visitCount = 0;               // Number of visits
    int typedCount = 0;               // Number of times typed in address bar
    QString faviconUrl;               // Favicon URL
    QString transitionType;           // link, typed, auto_bookmark, auto_subframe, manual_subframe, generated, start_page, form_submit, reload, keyword, keyword_generated
    int visitDuration = 0;            // Time spent on page (seconds)
    bool isDeleted = false;           // Soft delete
    bool isSynced = false;            // Synced to cloud

    // For UI
    QIcon favicon;
    QString domain() const;
    QString timeAgo() const;
    bool isValid() const;
    bool operator==(const HistoryEntry& other) const;
    static QString normalizedUrl(const QString& url);
};

// Typed visit transition types (matching Chrome/Chromium)
namespace HistoryTransition {
    const QString LINK = "link";
    const QString TYPED = "typed";
    const QString AUTO_BOOKMARK = "auto_bookmark";
    const QString AUTO_SUBFRAME = "auto_subframe";
    const QString MANUAL_SUBFRAME = "manual_subframe";
    const QString GENERATED = "generated";
    const QString START_PAGE = "start_page";
    const QString FORM_SUBMIT = "form_submit";
    const QString RELOAD = "reload";
    const QString KEYWORD = "keyword";
    const QString KEYWORD_GENERATED = "keyword_generated";
}

Q_DECLARE_METATYPE(HistoryEntry)

#endif // HISTORY_H