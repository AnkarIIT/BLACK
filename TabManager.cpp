#include "TabManager.h"
#include "TabController.h"

TabManager::TabManager(TabController *tabController, QObject *parent)
    : QObject(parent)
    , m_tabController(tabController)
{
    connectSignals();
}

TabManager::~TabManager()
{
}

void TabManager::connectSignals()
{
    if (!m_tabController)
        return;

    connect(m_tabController, &TabController::tabAdded, this, &TabManager::tabAdded);
    connect(m_tabController, &TabController::tabRemoved, this, &TabManager::tabRemoved);
    connect(m_tabController, &TabController::currentTabChanged, this, &TabManager::currentTabChanged);
    connect(m_tabController, &TabController::tabTitleChanged, this, &TabManager::tabTitleChanged);
    connect(m_tabController, &TabController::tabIconChanged, this, &TabManager::tabIconChanged);
    connect(m_tabController, &TabController::tabUrlChanged, this, &TabManager::tabUrlChanged);
    connect(m_tabController, &TabController::tabLoadingChanged, this, &TabManager::tabLoadingChanged);
    connect(m_tabController, &TabController::tabAudioStateChanged, this, &TabManager::tabAudioStateChanged);
    connect(m_tabController, &TabController::tabCrashCountChanged, this, &TabManager::tabCrashCountChanged);
    connect(m_tabController, &TabController::thumbnailReady, this, &TabManager::thumbnailReady);
    connect(m_tabController, &TabController::recentlyClosedChanged, this, &TabManager::recentlyClosedChanged);
    connect(m_tabController, &TabController::tabMoved, this, &TabManager::tabMoved);
}

QWebEngineView* TabManager::addTab(const QUrl &url, int activateOverride)
{
    if (!m_tabController)
        return nullptr;
    return m_tabController->addTab(url, activateOverride != -1);
}

void TabManager::closeTab(int index)
{
    if (m_tabController)
        m_tabController->closeTab(index);
}

void TabManager::setCurrentTab(int index)
{
    if (m_tabController)
        m_tabController->setCurrentTab(index);
}

void TabManager::togglePinTab(int index)
{
    if (m_tabController)
        m_tabController->togglePinTab(index);
}

void TabManager::toggleMuteTab(int index)
{
    if (m_tabController)
        m_tabController->toggleMuteTab(index);
}

void TabManager::moveTab(int fromIndex, int toIndex)
{
    if (m_tabController)
        m_tabController->moveTab(fromIndex, toIndex);
}

int TabManager::tabCount() const
{
    return m_tabController ? m_tabController->tabCount() : 0;
}

int TabManager::currentTabIndex() const
{
    return m_tabController ? m_tabController->currentTabIndex() : -1;
}

QWebEngineView* TabManager::currentView() const
{
    return m_tabController ? m_tabController->currentView() : nullptr;
}

const QList<TabInfo>& TabManager::tabs() const
{
    static QList<TabInfo> empty;
    return m_tabController ? m_tabController->tabs() : empty;
}

QWebEngineView* TabManager::viewAt(int index) const
{
    return m_tabController ? m_tabController->viewAt(index) : nullptr;
}

TabInfo TabManager::tabInfo(int index) const
{
    return m_tabController ? m_tabController->tabInfo(index) : TabInfo();
}

void TabManager::captureThumbnail(int index)
{
    if (m_tabController)
        m_tabController->captureThumbnail(index);
}

void TabManager::pruneThumbnails()
{
    if (m_tabController)
        m_tabController->pruneThumbnails();
}

void TabManager::clearThumbnail(int index)
{
    if (m_tabController)
        m_tabController->clearThumbnail(index);
}

void TabManager::hideOverview()
{
    if (m_tabController)
        m_tabController->hideOverview();
}

void TabManager::saveSession() const
{
    if (m_tabController)
        m_tabController->saveSession();
}

void TabManager::restoreSession(const QList<QVariantMap> &sessionData)
{
    if (m_tabController)
        m_tabController->restoreSession(sessionData);
}

QList<QVariantMap> TabManager::sessionData() const
{
    return m_tabController ? m_tabController->sessionData() : QList<QVariantMap>();
}

QList<QUrl> TabManager::recentlyClosed() const
{
    return m_tabController ? m_tabController->recentlyClosed() : QList<QUrl>();
}

void TabManager::addToClosedTabs(const QUrl &url)
{
    if (m_tabController)
        m_tabController->addToClosedTabs(url);
}

void TabManager::clearClosedTabs()
{
    if (m_tabController)
        m_tabController->clearClosedTabs();
}

void TabManager::setActivateNewTabs(bool activate)
{
    if (m_tabController)
        m_tabController->setActivateNewTabs(activate);
}

void TabManager::setNewTabUrl(const QUrl &url)
{
    if (m_tabController)
        m_tabController->setNewTabUrl(url);
}

#include "TabManager.moc"