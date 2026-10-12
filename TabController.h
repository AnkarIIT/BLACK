#ifndef TABCONTROLLER_H
#define TABCONTROLLER_H

#include <QObject>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QList>
#include <QVariantMap>

class QWebEngineView;

// TabInfo (view pointer, title, thumbnail, pin/mute/crash state) lives in
// BrowserWindow.h. It is pulled in rather than forward-declared because
// TabManager.cpp instantiates QList<TabInfo> against this header.
#include "BrowserWindow.h"

// Placeholder implementation. Tab lifecycle management (creating QWebEngineView
// tabs, closing them, reordering, pinning, muting, thumbnail capture, session
// save/restore) is not implemented in this build.
//
// There are never any tabs: addTab() always returns nullptr, tabCount() is
// always 0 and currentTabIndex() is always -1. Every mutating call is a no-op
// and NONE of the signals below is ever emitted. The signal set exists because
// TabManager forwards them; it is declared so the forwarding code links, not
// because anything happens behind them.
//
// BrowserWindow manages its own m_tabs list directly and never constructs a
// TabController, so in the current build nothing owns an instance of this
// class.
class TabController : public QObject
{
    Q_OBJECT

public:
    explicit TabController(QObject *parent = nullptr);
    ~TabController() override;

    // Always 0.
    Q_INVOKABLE int tabCount() const;

    // Always -1: there is no current tab.
    int currentTabIndex() const;

    // Placeholder: always returns nullptr. No QWebEngineView is ever created.
    QWebEngineView *addTab(const QUrl &url, bool activate = true);

    // Placeholder: no-op. There is no tab at `index` to close.
    void closeTab(int index);

    // Placeholder: no-op.
    void setCurrentTab(int index);

    // Placeholder: no-op.
    Q_INVOKABLE void activateTab(int index);

    // Placeholder: no-op.
    void togglePinTab(int index);

    // Placeholder: no-op. No audio is ever muted.
    void toggleMuteTab(int index);

    // Placeholder: no-op.
    void moveTab(int fromIndex, int toIndex);

    // Placeholder: always returns nullptr.
    QWebEngineView *currentView() const;

    // Always an empty list.
    const QList<TabInfo> &tabs() const;

    // Placeholder: always returns nullptr.
    QWebEngineView *viewAt(int index) const;

    // Placeholder: always returns a default-constructed TabInfo.
    TabInfo tabInfo(int index) const;

    // Placeholder: no-op. No tab bar screenshot is ever taken.
    void captureThumbnail(int index);

    // Placeholder: no-op. No thumbnail is ever stored, so none can be pruned.
    void pruneThumbnails();

    // Placeholder: no-op.
    void clearThumbnail(int index);

    // Placeholder: no-op. No overview widget exists.
    void hideOverview();

    // Placeholder: no-op. No session file is written.
    void saveSession() const;

    // Placeholder: no-op. The argument is discarded, not restored.
    void restoreSession(const QList<QVariantMap> &sessionData);

    // Always an empty list.
    QList<QVariantMap> sessionData() const;

    // Always an empty list.
    QList<QUrl> recentlyClosed() const;

    // Placeholder: no-op. Nothing is remembered.
    void addToClosedTabs(const QUrl &url);

    // Placeholder: no-op.
    void clearClosedTabs();

    // Placeholder: records the preference; no behaviour depends on it.
    void setActivateNewTabs(bool activate);

    // Placeholder: records the preference; no behaviour depends on it.
    void setNewTabUrl(const QUrl &url);

    // Declared so TabManager's forwarding connections compile. Never emitted:
    // this build never adds, removes or mutates a tab.
signals:
    void tabAdded(int index);
    void tabRemoved(int index);
    void currentTabChanged(int index);
    void tabTitleChanged(int index, const QString &title);
    void tabIconChanged(int index, const QIcon &icon);
    void tabUrlChanged(int index, const QUrl &url);
    void tabLoadingChanged(int index, bool loading);
    void tabAudioStateChanged(int index, bool audible, bool muted);
    void tabCrashCountChanged(int index, int count);
    void thumbnailReady(int index, const QPixmap &thumbnail);
    void recentlyClosedChanged();
    void tabMoved(int fromIndex, int toIndex);

private:
    // Always empty in this build.
    QList<TabInfo> m_tabs;
    QList<QUrl> m_recentlyClosed;
    int m_currentIndex = -1;
    bool m_activateNewTabs = true;
    QUrl m_newTabUrl;
};

#endif // TABCONTROLLER_H