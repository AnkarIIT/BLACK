#ifndef SAFARIWEBVIEW_H
#define SAFARIWEBVIEW_H

#include <QWebEngineView>
#include <QUrl>

class QWebEngineProfile;
class QWebChannel;

class SafariWebView : public QWebEngineView
{
    Q_OBJECT

public:
    explicit SafariWebView(QWidget *parent = nullptr);

    void setWebProfile(QWebEngineProfile *profile);

    // Forwards the QWebChannel bridge to the page, which only exposes it to
    // internal qrc: pages (see SafariWebPage::acceptNavigationRequest).
    void setWebChannelObject(QWebChannel *channel);

    // Forwards the password-only bridge to the page, which exposes it only in
    // the private autofill world on external sites.
    void setPasswordChannelObject(QWebChannel *channel);

signals:
    void newTabRequested(const QUrl &url);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif
