#ifndef SAFARIWEBPAGE_H
#define SAFARIWEBPAGE_H

#include <QWebEnginePage>
#include <QUrl>

class QWebEngineProfile;

// QWebEnginePage subclass that turns Ctrl/Cmd+clicks on links into new-tab
// requests instead of navigating the current tab. Middle-click and other
// new-window gestures are handled by Qt WebEngine natively and surface
// through QWebEnginePage::newWindowRequested.
class SafariWebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit SafariWebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

signals:
    void newTabRequested(const QUrl &url);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
};

#endif
