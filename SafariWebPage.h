#ifndef SAFARIWEBPAGE_H
#define SAFARIWEBPAGE_H

#include <QWebEnginePage>
#include <QUrl>
#include <QPointer>

class QWebEngineProfile;
class QWebChannel;

// QWebEnginePage subclass that turns Ctrl/Cmd+clicks on links into new-tab
// requests instead of navigating the current tab. Middle-click and other
// new-window gestures are handled by Qt WebEngine natively and surface
// through QWebEnginePage::newWindowRequested.
//
// Security: the QWebChannel bridge objects (passwords, history, bookmarks,
// settings, account, ...) are only ever reachable by BLACK-owned code.
//   - Internal qrc: pages get the FULL bridge installed in the main world so
//     their own scripts can use it directly.
//   - External http/https pages get a PASSWORD-ONLY bridge installed in a
//     dedicated private world (kPasswordWorld) in which only the native
//     password autofill content script runs.
// Third-party page scripts (main world) AND extension content scripts
// (ApplicationWorld) therefore never see qt.webChannelTransport and can never
// reach the vault or any other registered object.
class SafariWebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit SafariWebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

    // World that hosts the password-only autofill bridge. Deliberately
    // distinct from QWebEngineScript::ApplicationWorld (1) so installed
    // extension content scripts cannot touch it.
    static const quint32 kPasswordWorld = 2;

    // Stores the bridges; they are applied/removed per-navigation in
    // acceptNavigationRequest() depending on the destination URL.
    void setWebChannelObject(QWebChannel *channel);
    void setPasswordChannelObject(QWebChannel *channel);

signals:
    void newTabRequested(const QUrl &url);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

private:
    QPointer<QWebChannel> m_webChannel;
    QPointer<QWebChannel> m_passwordChannel;
};

#endif
