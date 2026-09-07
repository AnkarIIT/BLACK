#include "TabManager.h"
#include "SafariWebView.h"
#include "SafariWebPage.h"
#include "SafariTheme.h"
#include "TrackerBlocker.h"
#include "BrowserSettings.h"
#include <QWebEngineNewWindowRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineDownloadRequest>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QPointer>
#include <QUrlQuery>
#include <QPixmap>
#include <QDebug>

static const char *kPageThemeCss =
    "html.black-dark,html.black-dark body{background-color:#1a1a1a !important;color:#e3e3e3 !important;}"
    "html.black-dark{color-scheme:dark;}"
    "html.black-dark *:not(a):not(button):not(input):not(select):not(textarea):not(img):not(video):not(canvas):not(iframe):not(svg):not(path){color:inherit !important;background-color:transparent !important;border-color:rgba(255,255,255,0.15) !important;}"
    "html.black-dark header,html.black-dark nav,html.black-dark [role=\"banner\"]{background-color:#242424 !important;border-bottom:1px solid rgba(255,255,255,0.1) !important;}"
    "html.black-dark a{color:#8ab4f8 !important;}"
    "html.black-light,html.black-light body{background-color:#ffffff !important;color:#1d1d1f !important;}"
    "html.black-light{color-scheme:light;}";

static const QString kPageThemeStyleScript = QStringLiteral(
    "(function(){var css=%1;var s=document.createElement('style');s.id='black-theme-style';s.textContent=css;"
    "(document.head||document.documentElement).appendChild(s);})();").arg(
        QLatin1Char('"') + QString::fromLatin1(kPageThemeCss).replace(QLatin1Char('"'), QStringLiteral("\\\"")) + QLatin1Char('"'));

static const QString kPageThemeClassScript = QStringLiteral(
    "(function(dark){var el=document.documentElement;if(!el)return;"
    "el.classList.remove('black-dark','black-light');"
    "el.classList.add(dark?'black-dark':'black-light');})(%1);");

static const char *kPagePlatformScript =
    "(function(){"
    "if(window.__blackPlatformReady)return;window.__blackPlatformReady=true;"
    "function bp(){var w=window.innerWidth||document.documentElement.clientWidth||0;"
    "var p=w<=480?'phone':(w<=1024?'tablet':'desktop');"
    "var el=document.documentElement;if(el)el.setAttribute('data-platform',p);"
    "document.documentElement.style.setProperty('--platform-detect',(w+'px'));}"
    "if(document.readyState==='loading'){"
    "document.addEventListener('DOMContentLoaded',bp);"
    "}else{bp();}"
    "window.addEventListener('resize',bp);"
    "window.addEventListener('orientationchange',bp);"
    "})();";

TabManager::TabManager(QWebEngineProfile *profile, QWebChannel *webChannel, QWebChannel *passwordChannel, QObject *parent)
    : QObject(parent)
    , m_profile(profile)
    , m_webChannel(webChannel)
    , m_passwordChannel(passwordChannel)
{
}

TabManager::~TabManager()
{
    for (TabInfo &tab : m_tabs) {
        if (tab.view) {
            tab.view->deleteLater();
        }
    }
    m_tabs.clear();
}

QWebEngineView* TabManager::addTab(const QUrl &url, QWebEngineNewWindowRequest *request, int activateOverride)
{
    QWebEngineView *view = createTabView(url, request);
    int index = m_tabs.count() - 1;

    bool activate;
    if (activateOverride >= 0) {
        activate = (activateOverride == 1);
    } else {
        activate = m_activateNewTabs;
    }
    if (activate || m_tabs.count() == 1)
        setCurrentTab(index);

    emit tabAdded(index);
    return view;
}

QWebEngineView* TabManager::addTab(const QUrl &url, int activateOverride)
{
    return addTab(url, nullptr, activateOverride);
}

QWebEngineView* TabManager::createTabView(const QUrl &url, QWebEngineNewWindowRequest *request)
{
    auto *view = new SafariWebView(this);
    view->setWebProfile(m_profile);
    view->setWebChannelObject(m_webChannel);
    view->setPasswordChannelObject(m_passwordChannel);
    view->page()->setBackgroundColor(QColor(SafariTheme::instance().pageBackground));

    TabInfo info;
    info.view  = view;
    info.title = QStringLiteral("New Tab");
    info.url   = request ? request->requestedUrl().toString() : url.toString();
    info.lastActive = QDateTime::currentMSecsSinceEpoch();
    m_tabs.append(info);

    int index = m_tabs.count() - 1;
    setupTabConnections(view, index);

    if (request) {
        request->openIn(view->page());
    } else if (url.isValid() && !url.isEmpty()) {
        view->setUrl(url);
    } else {
        view->setUrl(m_newTabUrl);
    }

    return view;
}

void TabManager::setupTabConnections(QWebEngineView *view, int index)
{
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &t) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].title = t;
                if (i == m_currentTabIndex)
                    emit tabTitleChanged(i, t);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].icon = icon;
                emit tabIconChanged(i, icon);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &u) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].url = u.toString();
                if (i == m_currentTabIndex)
                    emit tabUrlChanged(i, u);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::loadStarted, this, [this, view]() {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].loading = true;
                emit tabLoadingChanged(i, true);
                break;
            }
        }
    });

    connect(view->page(), &QWebEnginePage::recentlyAudibleChanged, this, [this, view](bool audible) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].isAudible = audible;
                emit tabAudioStateChanged(i, audible, m_tabs[i].isMuted);
                break;
            }
        }
    });

    connect(view->page(), &QWebEnginePage::audioMutedChanged, this, [this, view](bool muted) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].isMuted = muted;
                emit tabAudioStateChanged(i, m_tabs[i].isAudible, muted);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].loading = false;
                emit tabLoadingChanged(i, false);

                if (ok) {
                    const QUrl loadedUrl = view->url();
                    const bool onCrashPage = loadedUrl.scheme() == QStringLiteral("qrc")
                                             && loadedUrl.path() == QStringLiteral("/crash.html");
                    if (!onCrashPage) {
                        m_tabs[i].crashCount = 0;
                        m_tabs[i].showingCrashPage = false;
                        emit tabCrashCountChanged(i, 0);
                    }
                }
                break;
            }
        }
        if (ok && !view->url().scheme().startsWith(QStringLiteral("qrc"))) {
            const bool dark = (SafariTheme::instance().scheme() == SafariTheme::Scheme::Dark);
            view->page()->runJavaScript(kPageThemeClassScript.arg(dark ? QStringLiteral("true") : QStringLiteral("false")));
        }
        applyUiLayoutToView(view);
    });

    // Renderer crash handling
    connect(view->page(), &QWebEnginePage::renderProcessTerminated,
            this, [this, view]() {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                if (m_tabs[i].crashCount < 2) {
                    m_tabs[i].crashCount++;
                    emit tabCrashCountChanged(i, m_tabs[i].crashCount);
                    view->reload();
                } else {
                    m_tabs[i].showingCrashPage = true;
                    view->setUrl(QUrl(QStringLiteral("qrc:/crash.html?url=") + QString::fromUtf8(QUrl::toPercentEncoding(view->url().toString()))));
                }
                break;
            }
        }
    });

    // New window/tab requests
    connect(view, &SafariWebView::newTabRequested, this, [this](const QUrl &u) {
        addTab(u);
    });
    connect(view->page(), &QWebEnginePage::newWindowRequested, this, [this](QWebEngineNewWindowRequest &req) {
        const int activate = (req.destination() == QWebEngineNewWindowRequest::InNewBackgroundTab) ? 0 : -1;
        addTab(QUrl(), &req, activate);
    });

    // Certificate errors
    connect(view->page(), &QWebEnginePage::certificateError, this, [this](QWebEngineCertificateError error) {
        if (!error.isOverridable()) {
            error.rejectCertificate();
            return;
        }
        error.defer();
        // The UI will handle the dialog via BrowserWindow
    });

    // Permission requests
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(view->page(), &QWebEnginePage::permissionRequested, this, [this](QWebEnginePermission permission) {
        // Forward to UI for handling
    });
#else
    connect(view->page(), &QWebEnginePage::featurePermissionRequested, this, [this](const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
        // Forward to UI for handling
    });
#endif
}

void TabManager::applyUiLayoutToView(QWebEngineView *view)
{
    // This will be called from BrowserWindow to sync layout mode
    // view->page()->runJavaScript(...);
}

int TabManager::nextTabIndex(int index) const
{
    if (m_tabs.isEmpty()) return -1;
    return (index + 1) % m_tabs.count();
}

int TabManager::previousTabIndex(int index) const
{
    if (m_tabs.isEmpty()) return -1;
    return (index - 1 + m_tabs.count()) % m_tabs.count();
}

void TabManager::setCurrentTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) return;

    int oldIndex = m_currentTabIndex;
    m_currentTabIndex = index;
    m_tabs[index].lastActive = QDateTime::currentMSecsSinceEpoch();

    emit currentTabChanged(index);
    if (oldIndex >= 0)
        emit tabTitleChanged(oldIndex, m_tabs[oldIndex].title);
}

void TabManager::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) return;

    if (m_tabs.count() == 1) {
        addTab(m_newTabUrl);
    }

    const QUrl closedUrl(m_tabs[index].url);
    if (closedUrl.isValid() && !closedUrl.isEmpty()) {
        m_closedTabs.append(closedUrl);
        while (m_closedTabs.count() > 20)
            m_closedTabs.removeFirst();
        emit recentlyClosedChanged();
    }

    QWebEngineView *v = m_tabs[index].view;
    m_tabs.removeAt(index);

    if (m_tabs.isEmpty()) return;

    int newIdx = m_currentTabIndex;
    if (index <= m_currentTabIndex) {
        newIdx = qMax(0, m_currentTabIndex - 1);
    }
    newIdx = qMin(newIdx, m_tabs.count() - 1);

    m_currentTabIndex = -1;
    setCurrentTab(newIdx);

    emit tabRemoved(index);
}

void TabManager::togglePinTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) return;

    m_tabs[index].isPinned = !m_tabs[index].isPinned;
    rebuildTabOrder();
}

void TabManager::toggleMuteTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) return;

    QWebEngineView *v = m_tabs[index].view;
    if (v) {
        bool mute = !v->page()->isAudioMuted();
        v->page()->setAudioMuted(mute);
        m_tabs[index].isMuted = mute;
        emit tabAudioStateChanged(index, m_tabs[index].isAudible, mute);
    }
}

void TabManager::moveTab(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_tabs.count() ||
        toIndex < 0 || toIndex >= m_tabs.count() ||
        fromIndex == toIndex) return;

    TabInfo tab = m_tabs.takeAt(fromIndex);
    m_tabs.insert(toIndex, tab);

    // Re-sync the view stack
    // This is handled by BrowserWindow's tab stack

    // Update current tab index
    if (m_currentTabIndex == fromIndex)
        m_currentTabIndex = toIndex;
    else if (fromIndex < toIndex && m_currentTabIndex > fromIndex && m_currentTabIndex <= toIndex)
        m_currentTabIndex--;
    else if (fromIndex > toIndex && m_currentTabIndex >= toIndex && m_currentTabIndex < fromIndex)
        m_currentTabIndex++;

    emit tabMoved(fromIndex, toIndex);
}

void TabManager::rebuildTabOrder()
{
    QList<TabInfo> pinned;
    QList<TabInfo> unpinned;
    for (const TabInfo &tab : m_tabs) {
        if (tab.isPinned)
            pinned.append(tab);
        else
            unpinned.append(tab);
    }
    m_tabs = pinned + unpinned;
}

QWebEngineView* TabManager::currentView() const
{
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count())
        return m_tabs[m_currentTabIndex].view;
    return nullptr;
}

QWebEngineView* TabManager::viewAt(int index) const
{
    if (index >= 0 && index < m_tabs.count())
        return m_tabs[index].view;
    return nullptr;
}

TabInfo TabManager::tabInfo(int index) const
{
    if (index >= 0 && index < m_tabs.count())
        return m_tabs[index];
    return TabInfo();
}

void TabManager::captureThumbnail(int index)
{
    if (!m_overviewVisible || index < 0 || index >= m_tabs.count())
        return;

    QWebEngineView *view = m_tabs[index].view;
    if (!view || !m_tabs[index].thumbnail.isNull())
        return;

    QPointer<QWebEngineView> safeView(view);
    QTimer::singleShot(50, this, [this, safeView, index]() {
        if (!m_overviewVisible || !safeView)
            return;
        if (index < m_tabs.count() && m_tabs[index].view == safeView) {
            QPixmap thumb = safeView->grab().scaled(
                240, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_tabs[index].thumbnail = thumb;
            emit thumbnailReady(index, thumb);
            pruneThumbnails();
        }
    });
}

void TabManager::pruneThumbnails()
{
    int thumbCount = 0;
    for (const TabInfo &tab : m_tabs) {
        if (!tab.thumbnail.isNull())
            ++thumbCount;
    }
    if (thumbCount <= kMaxTabThumbnails)
        return;

    QList<std::pair<qint64, int>> sortedTabs;
    for (int i = 0; i < m_tabs.count(); ++i) {
        if (!m_tabs[i].thumbnail.isNull())
            sortedTabs.append({m_tabs[i].lastActive, i});
    }
    std::sort(sortedTabs.begin(), sortedTabs.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    int toClear = thumbCount - kMaxTabThumbnails;
    for (int i = 0; i < toClear && i < sortedTabs.count(); ++i) {
        m_tabs[sortedTabs[i].second].thumbnail = QPixmap();
    }
}

void TabManager::clearThumbnail(int index)
{
    if (index >= 0 && index < m_tabs.count())
        m_tabs[index].thumbnail = QPixmap();
}

void TabManager::hideOverview()
{
    m_overviewVisible = false;
    for (TabInfo &tab : m_tabs)
        tab.thumbnail = QPixmap();
}

void TabManager::addToClosedTabs(const QUrl &url)
{
    if (url.isValid() && !url.isEmpty()) {
        m_closedTabs.append(url);
        while (m_closedTabs.count() > 20)
            m_closedTabs.removeFirst();
        emit recentlyClosedChanged();
    }
}

void TabManager::clearClosedTabs()
{
    m_closedTabs.clear();
    emit recentlyClosedChanged();
}

QList<QVariantMap> TabManager::sessionData() const
{
    QList<QVariantMap> data;
    for (const TabInfo &tab : m_tabs) {
        if (tab.url.startsWith(QStringLiteral("qrc:")).toBool())
            continue;
        QVariantMap map;
        map["url"] = tab.url;
        map["title"] = tab.title;
        map["pinned"] = tab.isPinned;
        map["active"] = (&tab == &m_tabs[m_currentTabIndex]);
        data.append(map);
    }
    return data;
}

void TabManager::restoreSession(const QList<QVariantMap> &sessionData)
{
    for (const QVariantMap &map : sessionData) {
        QUrl url = map["url"].toUrl();
        if (!url.isValid() || url.isEmpty())
            continue;
        bool pinned = map["pinned"].toBool(false);
        QWebEngineView *view = addTab(url);
        if (pinned && view) {
            int index = m_tabs.count() - 1;
            m_tabs[index].isPinned = true;
        }
    }
    rebuildTabOrder();
}

void TabManager::saveSession() const
{
    // Session saving is handled by BrowserWindow
}

#include "TabManager.moc"