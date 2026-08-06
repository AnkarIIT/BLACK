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
class QWebEngineProfile;
class QWebChannel;

struct TabInfo {
    QWebEngineView* view = nullptr;
    QString title;
    QString url;
    QIcon icon;
    QPixmap thumbnail;
    bool loading = false;
};

struct DownloadItemInfo {
    QString fileName;
    QString filePath;
    qint64 receivedBytes = 0;
    qint64 totalBytes = -1;
    int state = 0; // 0 = in progress, 1 = completed, 2 = failed/cancelled
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(bool incognito = false, QWidget *parent = nullptr);
    ~BrowserWindow() override;

    static QWebEngineProfile *webProfile();

    // Public methods for login flow and tab management
    void loadStartPage();
    void loadLoginPage();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
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

    void toggleSidebar();
    void toggleTabOverview();
    void showTabOverview();
    void hideTabOverview();
    void showSettingsMenu();
    void openSettingsDialog();
    void updateWebViewTheme();

private:
    void setupUi();
    void setupTabBar();
    void setupSidebar();
    void setupTabOverlay();
    void setupKeyboardShortcuts();
    void applyTheme();
    void updateUrlContainerStyle();
    void updateWebViewBackgrounds();

    QIcon createSvgIcon(const QString &svgData, int size = 18, const QString &color = "#1d1d1f");
    QToolButton* createTrafficLight(const QString &color, const QString &hoverColor);

    void addNewTab(const QUrl &url);
    SafariWebView* addTabView(const QUrl &url, QWebEngineNewWindowRequest *request);
    void rebuildTabBar();
    void refreshTabLabel(int index);
    void rebuildOverviewGrid();
    void rebuildSidebarTabList();
    void updateLoadingBar(int progress);
    void setWindowTitleFromTab();
    void openPrivateWindow();

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
    QWidget        *m_central;

    QWidget        *m_toolbar;
    QWidget        *m_tabBar;
    QHBoxLayout    *m_tabBarLayout;

    QToolButton *m_backButton;
    QToolButton *m_forwardButton;
    QToolButton *m_sidebarButton;
    QToolButton *m_reloadButton;
    QToolButton *m_shareButton;
    QToolButton *m_downloadsButton;
    QToolButton *m_tabOverviewButton;
    QToolButton *m_addTabButton;

    QToolButton *m_closeButton;
    QToolButton *m_minimizeButton;
    QToolButton *m_maximizeButton;
    QToolButton *m_settingsButton;
    QDialog      *m_settingsDialog;
    QWebEngineView *m_settingsView;

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
    bool          m_sidebarVisible;
    QString       m_activeSidebarAction;
    QList<QFrame*> m_sidebarItems;
    QList<QLabel*> m_sidebarItemIcons;
    QList<QLabel*> m_sidebarItemTexts;
    QList<QLabel*> m_sidebarHeaders;
    QPushButton  *m_newGroupButton = nullptr;
    bool          m_urlFocused;

    QPoint m_dragPosition;
    bool   m_isDragging;

    QList<TabInfo> m_tabs;
    int            m_currentTabIndex;

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
    void saveSession();
    void restoreSession();
    void saveHistoryItem(const QString &title, const QString &url);
    void saveBookmark(const QString &title, const QString &url);

    QList<QWidget*> m_tabWidgets;
    QList<QLabel*> m_tabItemIcons;
    QList<QLabel*> m_tabItemTexts;
    QList<QUrl>     m_closedTabs;
    QList<DownloadItemInfo> m_downloadsList;
    QList<QString>  m_sidebarItemSvg;

    bool            m_incognito;
    QWebEngineProfile *m_profile;
    QWebChannel     *m_webChannel;
    QMap<QString, bool> m_permissionChoices;
};

#endif
