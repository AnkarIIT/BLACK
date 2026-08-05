#include "SafariWebView.h"
#include "SafariTheme.h"

#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QApplication>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>

SafariWebView::SafariWebView(QWidget *parent)
    : QWebEngineView(parent)
{
}

void SafariWebView::setWebProfile(QWebEngineProfile *profile)
{
    if (!profile)
        return;
    setPage(new QWebEnginePage(profile, this));
}

void SafariWebView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebEngineContextMenuRequest *req = lastContextMenuRequest();
    if (!req) {
        QWebEngineView::contextMenuEvent(event);
        return;
    }

    const SafariTheme &theme = SafariTheme::instance();
    QMenu menu(this);
    menu.setStyleSheet(QString(
        "QMenu { background-color: %1; color: %2; border: 0.5px solid %3; "
        "border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 28px 6px 12px; border-radius: 5px; font-size: 13px; }"
        "QMenu::item:selected { background-color: %4; }"
        "QMenu::item:disabled { color: %5; }"
        "QMenu::separator { height: 1px; background: %6; margin: 4px 8px; }"
    ).arg(theme.bgUrlBar, theme.textPrimary, theme.border,
          theme.selectedBg, theme.textTertiary, theme.borderLight));

    const QUrl linkUrl = req->linkUrl();
    const QString selected = req->selectedText();

    if (linkUrl.isValid()) {
        QAction *openNewTab = menu.addAction(QStringLiteral("Open Link in New Tab"));
        connect(openNewTab, &QAction::triggered, this, [this, linkUrl]() {
            emit newTabRequested(linkUrl);
        });

        QAction *copyLink = menu.addAction(QStringLiteral("Copy Link"));
        connect(copyLink, &QAction::triggered, this, [linkUrl]() {
            QApplication::clipboard()->setText(linkUrl.toString());
        });
        menu.addSeparator();
    }

    if (!selected.isEmpty()) {
        QAction *copy = menu.addAction(QStringLiteral("Copy"));
        connect(copy, &QAction::triggered, this, [selected]() {
            QApplication::clipboard()->setText(selected);
        });
        menu.addSeparator();
    }

    QAction *back = menu.addAction(QStringLiteral("Back"));
    back->setEnabled(history()->canGoBack());
    connect(back, &QAction::triggered, this, &SafariWebView::back);

    QAction *forward = menu.addAction(QStringLiteral("Forward"));
    forward->setEnabled(history()->canGoForward());
    connect(forward, &QAction::triggered, this, &SafariWebView::forward);

    QAction *reload = menu.addAction(QStringLiteral("Reload"));
    connect(reload, &QAction::triggered, this, &SafariWebView::reload);

    menu.exec(event->globalPos());
    event->accept();
}
