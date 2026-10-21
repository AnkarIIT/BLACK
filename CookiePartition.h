#ifndef COOKIEPARTITION_H
#define COOKIEPARTITION_H

#include <QObject>

class QWebEngineProfile;

// Placeholder implementation. Cookie partitioning / CHIPS-style top-level
// site separation is NOT implemented in this build; install() is a no-op and
// the browser relies on Qt WebEngine's built-in cookie handling.
class CookiePartition : public QObject
{
    Q_OBJECT
public:
    explicit CookiePartition(QObject *parent = nullptr);

    // No-op. Intended to install per-top-level-site cookie partitioning on the
    // given profile.
    void install(QWebEngineProfile *profile);

    Q_INVOKABLE bool isEnabled() const;      // false
};

#endif // COOKIEPARTITION_H
