#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QObject>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QIcon>
#include <QPixmap>
#include <QUrl>
#include <QList>
#include <QMap>

#include "BrowserWindow.h"

class QWebEngineNewWindowRequest;
class QWebChannel;

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(QWebEngineProfile *profile, QWebChannel *webChannel, QWebChannel *passwordChannel, QObject *parent = nullptr);
    ~TabManager();

    // Tab creation
    QWebEngineView* addTab(const QUrl &url, QWebEngineNewWindowRequest *request = nullptr, int activateOverride = -1);
    QWebEngineView* addTab(const QUrl &url, int activateOverride);

    // Tab operations
    void closeTab(int index);
    void setCurrentTab(int index);
    void togglePinTab(int index);
    void toggleMuteTab(int index);
    void moveTab(int fromIndex, int toIndex);

    // Getters
    int tabCount() const { return m_tabs.count(); }
    int currentTabIndex() const { return m_currentTabIndex; }
    QWebEngineView* currentView() const;
    const QList<TabInfo>& tabs() const { return m_tabs; }
    QWebEngineView* viewAt(int index) const;
    TabInfo tabInfo(int index) const;

    // Thumbnails
    void captureThumbnail(int index);
    void pruneThumbnails();
    void clearThumbnail(int index);
    void hideOverview();

    // Session
    void saveSession() const;
    void restoreSession(const QList<QVariantMap> &sessionData);
    QList<QVariantMap> sessionData() const;

    // History for recently closed
    QList<QUrl> recentlyClosed() const { return m_closedTabs; }
    void addToClosedTabs(const QUrl &url);
    void clearClosedTabs();

    // Settings
    void setActivateNewTabs(bool activate) { m_activateNewTabs = activate; }
    void setNewTabUrl(const QUrl &url) { m_newTabUrl = url; }

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
    void setupTabConnections(QWebEngineView *view, int index);
    QWebEngineView* createTabView(const QUrl &url, QWebEngineNewWindowRequest *request);
    int nextTabIndex(int index) const;
    int previousTabIndex(int index) const;
    void rebuildTabOrder();

    QWebEngineProfile *m_profile = nullptr;
    QWebChannel *m_webChannel = nullptr;
    QWebChannel *m_passwordChannel = nullptr;
    QList<TabInfo> m_tabs;
    int m_currentTabIndex = -1;
    QList<QUrl> m_closedTabs;
    QUrl m_newTabUrl;
    bool m_activateNewTabs = true;
    bool m_overviewVisible = false;
    static const int kMaxTabThumbnails = 50;
};

#endif // TABMANAGER_H