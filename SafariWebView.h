#ifndef SAFARIWEBVIEW_H
#define SAFARIWEBVIEW_H

#include <QWebEngineView>
#include <QUrl>

class SafariWebView : public QWebEngineView
{
    Q_OBJECT

public:
    explicit SafariWebView(QWidget *parent = nullptr);

signals:
    void newTabRequested(const QUrl &url);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif
