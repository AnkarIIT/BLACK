#ifndef SAFARIWEBVIEW_H
#define SAFARIWEBVIEW_H

#include <QWebEngineView>
#include <QUrl>

class QWebEngineProfile;

class SafariWebView : public QWebEngineView
{
    Q_OBJECT

public:
    explicit SafariWebView(QWidget *parent = nullptr);

    void setWebProfile(QWebEngineProfile *profile);

signals:
    void newTabRequested(const QUrl &url);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif
