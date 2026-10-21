#include "TabController.h"

#include <QWebEngineView>

TabController::TabController(QObject *parent)
    : QObject(parent)
{
}

TabController::~TabController() = default;

int TabController::tabCount() const
{
    // Placeholder: no tab is ever added, so the list is always empty.
    return m_tabs.size();
}

int TabController::currentTabIndex() const
{
    return m_currentIndex;
}

QWebEngineView *TabController::addTab(const QUrl &url, bool activate)
{
    Q_UNUSED(url)
    Q_UNUSED(activate)
    // Placeholder: a real implementation would create a QWebEngineView, wrap it
    // in a TabInfo, add it to the tab bar and wire up the page signals. None of
    // that happens here -- returning nullptr (rather than a half-built view)
    // keeps callers from dereferencing something that is not a browser tab.
    return nullptr;
}

void TabController::closeTab(int index)
{
    Q_UNUSED(index)
    // Placeholder: there is never a tab to close.
}

void TabController::setCurrentTab(int index)
{
    Q_UNUSED(index)
    // Placeholder: there is never a tab to activate.
}

void TabController::activateTab(int index)
{
    Q_UNUSED(index)
    // Placeholder: there is never a tab to activate.
}

void TabController::togglePinTab(int index)
{
    Q_UNUSED(index)
    // Placeholder: there is never a tab to pin.
}

void TabController::toggleMuteTab(int index)
{
    Q_UNUSED(index)
    // Placeholder: there is never a tab to mute.
}

void TabController::moveTab(int fromIndex, int toIndex)
{
    Q_UNUSED(fromIndex)
    Q_UNUSED(toIndex)
    // Placeholder: the tab list is always empty, so there is nothing to move.
}

QWebEngineView *TabController::currentView() const
{
    return nullptr;
}

const QList<TabInfo> &TabController::tabs() const
{
    return m_tabs;
}

QWebEngineView *TabController::viewAt(int index) const
{
    Q_UNUSED(index)
    return nullptr;
}

TabInfo TabController::tabInfo(int index) const
{
    Q_UNUSED(index)
    return TabInfo();
}

void TabController::captureThumbnail(int index)
{
    Q_UNUSED(index)
    // Placeholder: a real implementation would ask the view for a scaled
    // grab() and emit thumbnailReady(). No image is captured.
}

void TabController::pruneThumbnails()
{
    // Placeholder: no thumbnail is ever stored, so there is nothing to free.
}

void TabController::clearThumbnail(int index)
{
    Q_UNUSED(index)
    // Placeholder: no thumbnail is ever stored.
}

void TabController::hideOverview()
{
    // Placeholder: no overview widget exists in this build.
}

void TabController::saveSession() const
{
    // Placeholder: no session file is written anywhere.
}

void TabController::restoreSession(const QList<QVariantMap> &sessionData)
{
    Q_UNUSED(sessionData)
    // Placeholder: no tab is recreated from the supplied session data.
}

QList<QVariantMap> TabController::sessionData() const
{
    return QList<QVariantMap>();
}

QList<QUrl> TabController::recentlyClosed() const
{
    return m_recentlyClosed;
}

void TabController::addToClosedTabs(const QUrl &url)
{
    Q_UNUSED(url)
    // Placeholder: nothing is remembered, so no tab is ever restorable.
}

void TabController::clearClosedTabs()
{
    m_recentlyClosed.clear();
}

void TabController::setActivateNewTabs(bool activate)
{
    // Recorded only: no tab is ever created, so this changes nothing.
    m_activateNewTabs = activate;
}

void TabController::setNewTabUrl(const QUrl &url)
{
    // Recorded only: no tab is ever created, so this changes nothing.
    m_newTabUrl = url;
}
