#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QString>
#include <QMouseEvent>
#include <QPoint>
#include <QStackedWidget>
#include <QList>
#include <QUrl>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

struct TabInfo {
    QWebEngineView* view;
    QString title;
    QString url;
    QIcon icon;                       // site favicon
    QWidget* overviewCard = nullptr;  // card in tab overview grid
};

class TabButton;

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void navigateToUrl();
    void updateUrlBar(const QUrl &url);
    void updateNavigationState();
    
    // Window control slots
    void closeWindow();
    void minimizeWindow();
    void maximizeWindow();

    void shareAction();
    void onTabIconChanged(const QIcon &icon);
    void addTabAction();
    void closeTab(int index);
    void showTabOverview();
    void hideTabOverview();

private:
    void setupUi();
    void setupTabOverlay();
    QIcon createSvgIcon(const QString &svgData, int size = 24, QString color = "#4D4D4D");
    QToolButton* createTrafficLight(const QString &color, const QString &hoverColor, const QString &iconSvg = "");
    void addNewTab(const QUrl &url);
    void setCurrentTab(int index);
    void rebuildInlineTabBar();
    void updateTabCountBadge();
    QWidget* buildOverviewCard(int index);
    void rebuildOverviewGrid();

    // ── Core layout ──────────────────────────────────────────
    QStackedWidget *m_tabStack;
    QLineEdit      *m_urlBar;

    // ── Top bar ──────────────────────────────────────────────
    QFrame         *m_topBar;
    QHBoxLayout    *m_navLayout;

    // ── Inline tab strip (inside top bar) ──────────────────
    QWidget        *m_tabStrip;          // container visible only when >1 tab
    QHBoxLayout    *m_tabStripLayout;

    // ── Safari Layout Buttons ────────────────────────────────
    QToolButton *m_sidebarButton;
    QToolButton *m_backButton;
    QToolButton *m_forwardButton;
    QToolButton *m_reloadButton;
    QToolButton *m_shieldButton;
    QToolButton *m_shareButton;
    QToolButton *m_addTabButton;
    QToolButton *m_groupTabsButton;      // shows count badge + opens overview

    // ── Traffic Lights ───────────────────────────────────────
    QToolButton *m_closeButton;
    QToolButton *m_minimizeButton;
    QToolButton *m_maximizeButton;

    // ── Tab count badge ──────────────────────────────────────
    QLabel      *m_tabCountLabel;        // badge on top of m_groupTabsButton

    // ── Tab Overview Overlay ─────────────────────────────────
    QWidget     *m_overviewOverlay;      // full-window dimmed overlay
    QWidget     *m_overviewPanel;        // white panel that slides up
    QScrollArea *m_overviewScroll;
    QWidget     *m_overviewGrid;
    QGridLayout *m_overviewGridLayout;
    bool         m_overviewVisible = false;

    // ── Drag ────────────────────────────────────────────────
    QPoint m_dragPosition;
    bool   m_isDragging = false;

    QList<TabInfo> m_tabs;
    int            m_currentTabIndex = -1;
};

#endif // BROWSERWINDOW_H
