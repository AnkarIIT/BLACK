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

class QWebEngineNewWindowRequest;
class QWebChannel;
class TabController;

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(TabController *tabController, QObject *parent = nullptr);
    ~TabManager();

    // Delegate to TabController
    QWebEngineView* addTab(const QUrl &url, int activateOverride = -1);
    void closeTab(int index);
    void setCurrentTab(int index);
    void togglePinTab(int index);
    void toggleMuteTab(int index);
    void moveTab(int fromIndex, int toIndex);

    // Getters
    int tabCount() const;
    int currentTabIndex() const;
    QWebEngineView* currentView() const;
    const QList<TabInfo>& tabs() const;
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
    QList<QUrl> recentlyClosed() const;
    void addToClosedTabs(const QUrl &url);
    void clearClosedTabs();

    // Settings
    void setActivateNewTabs(bool activate);
    void setNewTabUrl(const QUrl &url);

    // TabController signals (forwarded)
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
    TabController *m_tabController = nullptr;

    void connectSignals();
};

#endif // TABMANAGER_H