#ifndef SAFEBROWSING_H
#define SAFEBROWSING_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QSet>
#include <QReadWriteLock>
#include <QTimer>

// Offline Safe Browsing: blocks navigation to hosts on an embedded list plus
// any extra domains in <AppDataLocation>/safebrowsing.json. No cloud Google
// Safe Browsing API. The main-frame interceptor redirects blocked navigations
// to a warning page; allow() lets a user continue past a warning once.
// Supports automatic periodic refresh from configured remote sources.
class SafeBrowsing : public QObject
{
    Q_OBJECT

public:
    static SafeBrowsing &instance();

    bool isBlocked(const QUrl &url) const;

    Q_INVOKABLE void allow(const QString &host);

    void loadExtraList();
    void refreshBlocklists(); // Fetch and merge remote blocklists
    static QUrl warningUrl(const QUrl &original);

signals:
    void changed();
    void blocklistsRefreshed(bool success, const QString &message);

private:
    SafeBrowsing();
    Q_DISABLE_COPY(SafeBrowsing)

    bool hostMatches(const QString &host) const;
    void mergeBlocklist(const QByteArray &data, const QString &source);
    void saveExtraList();

    QSet<QString> m_blocked;
    QSet<QString> m_allowed;
    // Guarded: allow() runs on the GUI thread while interceptRequest() reads
    // isBlocked() from Chromium's IO thread.
    mutable QReadWriteLock m_lock;

    QTimer *m_refreshTimer = nullptr;
    QNetworkAccessManager *m_net = nullptr;
};
#endif
