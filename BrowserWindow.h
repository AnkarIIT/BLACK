#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QWebEnginePermission>
#endif
#include <QWebEngineCertificateError>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QList>
#include <QUrl>

// Safari-style User-Agent shared by the main and private-window profiles so
// both present an identical fingerprint to sites.
QString getSafariUserAgent();
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QMap>
#include <QPointer>

class QProgressBar;
class SafariWebView;
class QWebEngineNewWindowRequest;
class QWebEngineDownloadRequest;
class QWebEngineProfile;
class QWebChannel;
class QGraphicsDropShadowEffect;
class ChromeLayer;
class ShelfStore;
class PasswordStore;
class ExtensionManager;
class Account;
class BookmarkImporter;
class PermissionsBridge;
class WebAuthnManager;
class ReaderMode;
class FingerprintProtection;
class CookiePartition;
class TranslationManager;
class TabGroupManager;
class SyncManager;
class SyncEngine;
class SyncCrypto;
class AIManager;
class WalletManager;
class RewardsManager;
class SearchAffiliation;
class PerformanceManager;
class WasmExtensionSupport;
class WebRTCManager;
class DeveloperTools;
class I18nManager;
class ShelfStore;
#include "UpdateChecker.h"

struct TabInfo {
    QWebEngineView* view = nullptr;
    QString title;
    QString url;
    QIcon icon;
    QPixmap thumbnail;
    bool loading = false;
    bool isPinned = false;
    bool isAudible = false;
    bool isMuted = false;
    qint64 lastActive = 0; // ms since epoch, updated on creation/activation
    int crashCount = 0;    // consecutive renderer crashes before the crash page shows
    bool showingCrashPage = false; // persisted so a crashed tab restores to the crash page
};

struct DownloadItemInfo;
class DownloadManager;

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(bool incognito = false, QWidget *parent = nullptr);
    ~BrowserWindow() override;

    static QWebEngineProfile *webProfile();
    static void setWebProfile(QWebEngineProfile *profile);

    // Public methods for tab management
    void loadStartPage();
    void addNewTab(const QUrl &url);

    // Current active tab URL (used by Settings > "Set to Current Page").
    QString currentPageUrl() const;

    // Opens the Settings sheet (used by --open-settings debug flag).
    void openSettingsForTesting();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void animateUrlBar(int targetWidth);
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void navigateToUrl();
    void updateUrlBar(const QUrl &url);
    void updateNavigationState();
    void onLoadStarted();
    void onLoadProgress(int progress);
    void onLoadFinished(bool ok);

    void closeWindow();
    void minimizeWindow();
    void maximizeWindow();

    void shareAction();
    void addTabAction();
    void closeTab(int index);
    void setCurrentTab(int index);
    void togglePinTab(int index);
    void toggleMuteTab(int index);

    void toggleSidebar();
    void toggleTabOverview();
    void showTabOverview();
    void hideTabOverview();
    void toggleReaderMode();
    void translatePage();
    void togglePictureInPicture();
    void toggleAIAssistant();
    void captureOverviewThumbnails();
    void showSettingsMenu();
    void openSettingsDialog();
    void openDevTools();
    void showProfileMenu();
    void showConnectionInfo();
    void updateProfileButton();
    void updateWebViewTheme();
    void installExtensionScripts();
    void openP2PDashboard();

private:
    void setupUi();
    void setupTabBar();
    void setupSidebar();
    void setupTabOverlay();
    void setupKeyboardShortcuts();
    void applyTheme();
    void applyUiLayout();
    void applyUiLayoutToView(QWebEngineView *view);
    void applySidebarLayout(bool chrome);
    void handleRenderProcessCrash(QWebEngineView *view);
    void updateUrlContainerStyle();
    void updateWebViewBackgrounds();

    friend class ChromeLayer;

    QIcon createSvgIcon(const QString &svgData, int size = 18, const QString &color = "#1d1d1f");
    QIcon profileAvatarIcon(int size);
    QToolButton* createTrafficLight(const QString &color, const QString &hoverColor);
    // Touch-aware icon size that scales up on tablet/phone so controls meet
    // the recommended 44px+ touch-target guidelines on coarse-pointer devices.
    int touchIconSize(int desktopSize) const;
    bool isTouchDevice() const;

#if defined(Q_OS_WIN)
    void setupWindowsDwm();
#endif

    SafariWebView* addTabView(const QUrl &url, QWebEngineNewWindowRequest *request);
    // activateOverride: -1 = follow activateNewTabs setting, 0 = background tab, 1 = force activate
    SafariWebView* addTabView(const QUrl &url, QWebEngineNewWindowRequest *request, int activateOverride);
    QUrl newTabUrl() const;
    QUrl homepageUrl() const;
    QUrl samePageUrl() const;
    void addBookmarkForCurrentTab();
    void rebuildTabBar();
    void refreshTabLabel(int index);
    void rebuildOverviewGrid();
    void rebuildSidebarTabList();
    void updateLoadingBar(int progress);
    void setWindowTitleFromTab();
    void openPrivateWindow();
    void openNewWindow(const QUrl &url = QUrl());

    void handleCertificateError(QWebEngineCertificateError certificateError);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    void handlePermissionRequest(QWebEnginePermission permission);
    static QString permissionDisplayName(QWebEnginePermission::PermissionType type);
#else
    void handlePermissionRequestOld(QWebEnginePage *page, const QUrl &securityOrigin, QWebEnginePage::Feature feature);
    static QString permissionDisplayNameOld(QWebEnginePage::Feature feature);
#endif

    QWidget* buildOverviewCard(int index);

    void navigateCurrentTo(const QUrl &url);
    void openSidebarAction(const QString &action);
    void setSidebarActive(const QString &action);
    void styleSidebarItems();
    QLabel* sidebarItemTextForAction(const QString &action);
    void filterOverviewGrid(const QString &query);

    QStackedWidget *m_tabStack;
    QLineEdit      *m_urlBar;
    QFrame         *m_urlContainer;
    QParallelAnimationGroup *m_urlAnim;
    QToolButton    *m_shieldInside;
    QToolButton    *m_lockButton;
    QWidget        *m_central;

    QFrame         *m_urlSuggest;
    QVBoxLayout    *m_urlSuggestLayout;
    QList<QPushButton*> m_urlSuggestRows;
    QList<QUrl>    m_urlSuggestionUrls;
    int             m_urlSuggestionIndex;

    void setupUrlSuggestions();
    void rebuildUrlSuggestions();
    void showUrlSuggestions();
    void hideUrlSuggestions();
    void selectUrlSuggestion(int index);
    void activateUrlSuggestion(int index);

    QWidget        *m_toolbar;
    QWidget        *m_tabBar;
    QHBoxLayout    *m_tabBarLayout;

    QHBoxLayout    *m_toolbarLayout;
    QHBoxLayout    *m_trafficLayout;
    ChromeLayer    *m_chromeLayer;

    QToolButton *m_backButton;
    QToolButton *m_forwardButton;
    QToolButton *m_sidebarButton;
    QToolButton *m_reloadButton;
    QToolButton *m_shareButton;
    QToolButton *m_downloadsButton;
    QToolButton *m_tabOverviewButton;
    QToolButton *m_addTabButton;
    QToolButton *m_readerModeButton;
    QToolButton *m_translateButton;
    QToolButton *m_pipButton;

    QToolButton *m_closeButton;
    QToolButton *m_minimizeButton;
    QToolButton *m_maximizeButton;
    QToolButton *m_settingsButton;
    QToolButton *m_extensionsButton;
    QToolButton *m_profileButton;
    QLabel      *m_privateBadge;
    QDialog      *m_settingsDialog;
    SafariWebView *m_settingsView;

    QProgressBar *m_loadingBar;

    QWidget     *m_overviewOverlay;
    QWidget     *m_overviewPanel;
    QScrollArea *m_overviewScroll;
    QWidget     *m_overviewGrid;
    QGridLayout *m_overviewGridLayout;
    QLabel      *m_overviewTitle;
    QLineEdit   *m_overviewSearch;
    QPushButton *m_overviewDoneButton;
    QPushButton *m_overviewNewTabButton;
    bool         m_overviewVisible;

    QWidget      *m_sidebarHost;
    QFrame       *m_sidebar;
    QVBoxLayout  *m_sidebarLayout;
    QLineEdit    *m_sidebarSearch;
    QGraphicsDropShadowEffect *m_sidebarShadow;
    bool          m_sidebarVisible;
    QString       m_activeSidebarAction;
    QList<QFrame*> m_sidebarItems;
    QList<QLabel*> m_sidebarItemIcons;
    QList<QLabel*> m_sidebarItemTexts;
    QList<QLabel*> m_sidebarHeaders;
    QPushButton  *m_newGroupButton = nullptr;
    QFrame       *m_downloadsSidebarItem = nullptr;
    bool          m_urlFocused;
    bool          m_urlMouseFocusPending;

    QPoint m_dragPosition;
    bool   m_isDragging;
    bool   m_touchDragging = false;

    // Tab drag-reorder state
    bool   m_tabDragActive = false;
    int    m_tabDragSourceIndex = -1;
    QPoint m_tabDragStartPos;

    QList<TabInfo> m_tabs;
    int            m_currentTabIndex;
    // Only the first non-incognito window owns the persisted session: it is the
    // only one allowed to restoreSession() (avoids duplicate-tab cloning) and
    // the only one that saves it on close (avoids a secondary window clobbering
    // the stored session).
    bool m_ownsSession;

    QWidget     *m_findBar;
    QLineEdit   *m_findInput;
    QLabel      *m_findMatchCount;
    QToolButton *m_findNextBtn;
    QToolButton *m_findPrevBtn;
    QToolButton *m_findCloseBtn;

    void setupFindBar();
    void showFindBar();
    void hideFindBar();
    void findNext();
    void findPrevious();
    void setupDownloads();
    void showDownloadsMenu();
    QWidget* buildDownloadRow(int index, const DownloadItemInfo &item);
    void saveSession();
    void restoreSession();
    void saveHistoryItem(const QString &title, const QString &url);
    void saveBookmark(const QString &title, const QString &url);
    void loadPermissions();
    void savePermissions();

    QList<QWidget*> m_tabWidgets;
    QList<QLabel*> m_tabItemIcons;
    QList<QLabel*> m_tabItemTexts;
    QList<QUrl>     m_closedTabs;
    DownloadManager *m_downloads;
    QList<QString>  m_sidebarItemSvg;

    bool            m_incognito;
    QWebEngineProfile *m_profile;
    QWebChannel     *m_webChannel;
    QWebChannel     *m_passwordChannel;
    int              m_settingsCrashCount = 0; // consecutive settings renderer crashes
    ShelfStore      *m_bookmarks;
    ShelfStore      *m_history;
    PasswordStore   *m_passwords;
    ExtensionManager *m_extensions;
    Account         *m_account;
    BookmarkImporter *m_bookmarkImporter;
    PermissionsBridge *m_permissionsBridge;
    UpdateChecker *m_updateChecker;
    WebAuthnManager *m_webAuthnManager;
    ReaderMode *m_readerMode;
    FingerprintProtection *m_fingerprintProtection;
    CookiePartition *m_cookiePartition;
    TranslationManager *m_translationManager;
    TabGroupManager *m_tabGroupManager;
    SyncManager *m_syncManager;
    AIManager *m_aiManager;
    WalletManager *m_walletManager;
    RewardsManager *m_rewardsManager;
    SearchAffiliation *m_searchAffiliation;
    PerformanceManager *m_performanceManager;
    WebRTCManager *m_webRTCManager;
    I18nManager *m_i18nManager;
    CollaborationManager *m_collaborationManager;
    AIAgentManager *m_aiAgentManager;
    VoiceInterface *m_voiceInterface;
    ProductivityManager *m_productivityManager;
    AdvancedSecurity *m_advancedSecurity;
    ShelfStore *m_readingList;
    QMap<QString, bool> m_permissionChoices;

    struct SiteSettings {
        qreal zoomFactor = 1.0;
        bool blockImages = false;
        bool blockScripts = false;
        QString userAgentOverride;
    };
    QMap<QString, SiteSettings> m_siteSettings;

    void loadSiteSettings();
    void saveSiteSettings();
    void applySiteSettings(const QString &host);
    SiteSettings getSiteSettings(const QString &host) const;
    void setSiteSetting(const QString &host, const SiteSettings &settings);

    // Favicon preloading
    void preloadFavicon(const QUrl &url);
    QNetworkAccessManager m_netManager;
    QMap<QString, QPixmap> m_faviconCache;
};

#endif
