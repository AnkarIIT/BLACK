#ifndef SAFEBROWSING_H
#define SAFEBROWSING_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QSet>

// Offline Safe Browsing: blocks navigation to hosts on an embedded list plus
// any extra domains in <AppDataLocation>/safebrowsing.json. No cloud Google
// Safe Browsing API. The main-frame interceptor redirects blocked navigations
// to a warning page; allow() lets a user continue past a warning once.
class SafeBrowsing : public QObject
{
    Q_OBJECT

public:
    static SafeBrowsing &instance();

    bool isBlocked(const QUrl &url) const;

    Q_INVOKABLE void allow(const QString &host);

    void loadExtraList();
    static QUrl warningUrl(const QUrl &original);

signals:
    void changed();

private:
    SafeBrowsing();
    Q_DISABLE_COPY(SafeBrowsing)

    bool hostMatches(const QString &host) const;

    QSet<QString> m_blocked;
    QSet<QString> m_allowed;
};

#endif
