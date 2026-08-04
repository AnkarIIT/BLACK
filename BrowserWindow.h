#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QList>
#include <QUrl>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPointer>

struct TabInfo {
    QWebEngineView* view = nullptr;
    QString title;
    QString url;
    QIcon icon;
    bool loading = false;
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
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
    void rebuildTabBar();
    void rebuildOverviewGrid();
    void rebuildSidebarTabList();
    void updateLoadingBar(int progress);
    void setWindowTitleFromTab();

    QWidget* buildOverviewCard(int index);

    QStackedWidget *m_tabStack;
    QLineEdit      *m_urlBar;
    QFrame         *m_urlContainer;
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

    QLabel      *m_loadingBar;

    QWidget     *m_overviewOverlay;
    QWidget     *m_overviewPanel;
    QScrollArea *m_overviewScroll;
    QWidget     *m_overviewGrid;
    QGridLayout *m_overviewGridLayout;
    QLabel      *m_overviewTitle;
    QPushButton *m_overviewDoneButton;
    QPushButton *m_overviewNewTabButton;
    bool         m_overviewVisible;

    QFrame       *m_sidebar;
    QVBoxLayout  *m_sidebarLayout;
    QLineEdit    *m_sidebarSearch;
    bool          m_sidebarVisible;
    QList<QFrame*> m_sidebarItems;
    QList<QLabel*> m_sidebarItemIcons;
    QList<QLabel*> m_sidebarItemTexts;
    QList<QLabel*> m_sidebarHeaders;
    bool          m_urlFocused;

    QPoint m_dragPosition;
    bool   m_isDragging;

    QList<TabInfo> m_tabs;
    int            m_currentTabIndex;

    QList<QWidget*> m_tabWidgets;
    QList<QUrl>     m_closedTabs;
};

#endif
