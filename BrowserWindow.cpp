#include "BrowserWindow.h"
#include "SafariWebView.h"
#include "SafariTheme.h"
#include <QFrame>
#include <QStyle>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QColor>
#include <QWebEngineHistory>
#include <QClipboard>
#include <QDir>
#include <QGridLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QToolTip>
#include <QFont>
#include <QEvent>
#include <QCoreApplication>
#include <QFile>
#include <QShortcut>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QWebEngineProfile>
#include <QActionGroup>
#include <QDesktopServices>
#include <functional>
#include <utility>

// ── Safari Color Palette (dynamic, follows system theme) ─────────────────────
static QString bgWindow()      { return SafariTheme::instance().bgWindow; }
static QString bgToolbar()     { return SafariTheme::instance().bgToolbar; }
static QString bgTabBar()      { return SafariTheme::instance().bgTabBar; }
static QString bgUrlBar()      { return SafariTheme::instance().bgUrlBar; }
static QString bgSidebar()     { return SafariTheme::instance().bgSidebar; }
static QString tabActive()     { return SafariTheme::instance().tabActive; }
static QString tabInactive()   { return SafariTheme::instance().tabInactive; }
static QString tabHover()      { return SafariTheme::instance().tabHover; }
static QString cardBg()        { return SafariTheme::instance().cardBg; }
static QString textPrimary()   { return SafariTheme::instance().textPrimary; }
static QString textSecondary() { return SafariTheme::instance().textSecondary; }
static QString textTertiary()  { return SafariTheme::instance().textTertiary; }
static QString accent()        { return SafariTheme::instance().accent; }
static QString accentHover()   { return SafariTheme::instance().accentHover; }
static QString border()        { return SafariTheme::instance().border; }
static QString hover()         { return SafariTheme::instance().hover; }
static QString searchBg()      { return SafariTheme::instance().searchBg; }
static QString selectedBg()    { return SafariTheme::instance().selectedBg; }

// ── SVG Icons (Safari-style, stroke-based) ─────────────────────────────────
static const QString svgBack       = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"15 18 9 12 15 6\"/></svg>";
static const QString svgForward    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"9 18 15 12 9 6\"/></svg>";
static const QString svgSidebar    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"2\"/><line x1=\"9\" y1=\"3\" x2=\"9\" y2=\"21\"/></svg>";
static const QString svgReload     = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21.5 2v6h-6\"/><path d=\"M21.34 15.57a10 10 0 1 1-.59-8.31L21.5 8\"/></svg>";
static const QString svgStop       = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"6\" y=\"6\" width=\"12\" height=\"12\" rx=\"1\"/></svg>";
static const QString svgShare      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8\"/><polyline points=\"16 6 12 2 8 6\"/><line x1=\"12\" y1=\"2\" x2=\"12\" y2=\"15\"/></svg>";
static const QString svgDownloads  = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4\"/><polyline points=\"7 10 12 15 17 10\"/><line x1=\"12\" y1=\"15\" x2=\"12\" y2=\"3\"/></svg>";
static const QString svgTabOverview = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"14\" y=\"3\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"3\" y=\"14\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"14\" y=\"14\" width=\"7\" height=\"7\" rx=\"1.5\"/></svg>";
static const QString svgPlus        = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"/><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>";
static const QString svgSearch      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"11\" cy=\"11\" r=\"8\"/><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"/></svg>";
static const QString svgShield      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z\"/></svg>";
static const QString svgClose       = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2.5\" stroke-linecap=\"round\"><line x1=\"18\" y1=\"6\" x2=\"6\" y2=\"18\"/><line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/></svg>";
static const QString svgBookmarks   = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z\"/></svg>";
static const QString svgReadingList = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"6\" cy=\"12\" r=\"2\"/><circle cx=\"12\" cy=\"12\" r=\"2\"/><circle cx=\"18\" cy=\"12\" r=\"2\"/><line x1=\"8\" y1=\"12\" x2=\"16\" y2=\"12\"/></svg>";
static const QString svgHistory     = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"10\"/><polyline points=\"12 6 12 12 16 14\"/></svg>";
static const QString svgTabGroups   = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"2\" y=\"4\" width=\"20\" height=\"16\" rx=\"2\"/><line x1=\"2\" y1=\"10\" x2=\"22\" y2=\"10\"/></svg>";
static const QString svgiCloud      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z\"/></svg>";

// ── Helpers ─────────────────────────────────────────────────────────────────
static QString truncate(const QString &s, int max = 20) {
    return s.length() > max ? s.left(max - 1) + QStringLiteral("\u2026") : s;
}

static QString shortUrl(const QString &url) {
    QString display = url;
    if (display.startsWith(QStringLiteral("file://")))
        return QStringLiteral("Start Page");
    if (display.startsWith(QStringLiteral("https://")))
        display.remove(0, 8);
    else if (display.startsWith(QStringLiteral("http://")))
        display.remove(0, 7);
    if (display.startsWith(QStringLiteral("www.")))
        display.remove(0, 4);
    if (display.endsWith(QStringLiteral("/")))
        display.chop(1);
    return display;
}

// =============================================================================
BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabStack(new QStackedWidget(this))
    , m_urlBar(new QLineEdit(this))
    , m_urlContainer(nullptr)
    , m_shieldInside(nullptr)
    , m_central(nullptr)
    , m_backButton(new QToolButton(this))
    , m_forwardButton(new QToolButton(this))
    , m_sidebarButton(new QToolButton(this))
    , m_reloadButton(new QToolButton(this))
    , m_shareButton(new QToolButton(this))
    , m_downloadsButton(new QToolButton(this))
    , m_tabOverviewButton(new QToolButton(this))
    , m_addTabButton(new QToolButton(this))
    , m_closeButton(nullptr)
    , m_minimizeButton(nullptr)
    , m_maximizeButton(nullptr)
    , m_loadingBar(nullptr)
    , m_overviewOverlay(nullptr)
    , m_overviewTitle(nullptr)
    , m_overviewDoneButton(nullptr)
    , m_overviewNewTabButton(nullptr)
    , m_overviewVisible(false)
    , m_sidebar(nullptr)
    , m_sidebarLayout(nullptr)
    , m_sidebarSearch(nullptr)
    , m_sidebarVisible(false)
    , m_urlFocused(false)
    , m_isDragging(false)
    , m_currentTabIndex(-1)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(800, 500);
    setFont(QFont("SF Pro Display", 13));

    setupUi();
    setupTabBar();
    setupSidebar();
    setupFindBar();
    setupDownloads();
    setupTabOverlay();
    setupKeyboardShortcuts();
    applyTheme();

    restoreSession();
    if (m_tabs.isEmpty()) {
        addNewTab(QUrl(QStringLiteral("qrc:/startpage.html")));
    }

    connect(m_urlBar, &QLineEdit::returnPressed, this, &BrowserWindow::navigateToUrl);

    connect(&SafariTheme::instance(), &SafariTheme::schemeChanged, this, [this]() {
        applyTheme();
    });
}

BrowserWindow::~BrowserWindow() {
    saveSession();
}

// ── Window Control ──────────────────────────────────────────────────────────
void BrowserWindow::closeWindow()    { saveSession(); close(); }
void BrowserWindow::minimizeWindow() { showMinimized(); }
void BrowserWindow::maximizeWindow() {
    if (isMaximized()) showNormal(); else showMaximized();
}

// ── Mouse Events (frameless window) ────────────────────────────────────────
void BrowserWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 52) {
        QWidget *child = childAt(event->pos());
        if (!child || child == m_toolbar || child == m_central) {
            m_isDragging = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
}

void BrowserWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        if (isMaximized()) {
            showNormal();
            m_dragPosition = QPoint(width() / 2, 16);
        }
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void BrowserWindow::mouseReleaseEvent(QMouseEvent *) {
    m_isDragging = false;
}

void BrowserWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 32) {
        maximizeWindow();
        event->accept();
    }
}

void BrowserWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_overviewOverlay) m_overviewOverlay->setGeometry(rect());
}

void BrowserWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ThemeChange) {
        SafariTheme::Scheme s =
            (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
                ? SafariTheme::Scheme::Dark : SafariTheme::Scheme::Light;
        SafariTheme::instance().setScheme(s);
    }
}

// ── Icon Helper ─────────────────────────────────────────────────────────────
QIcon BrowserWindow::createSvgIcon(const QString &svgData, int size, const QString &color) {
    QString filled = svgData.contains(QStringLiteral("%1")) ? svgData.arg(color) : svgData;
    QSvgRenderer renderer(filled.toUtf8());
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    return QIcon(pix);
}

// ── Traffic Light Helper ────────────────────────────────────────────────────
QToolButton* BrowserWindow::createTrafficLight(const QString &color, const QString &hoverColor) {
    auto *btn = new QToolButton(this);
    btn->setFixedSize(12, 12);
    btn->setStyleSheet(QString(
        "QToolButton { background-color: %1; border-radius: 6px; border: 0.5px solid rgba(0,0,0,0.12); }"
        "QToolButton:hover { background-color: %2; }"
    ).arg(color, hoverColor));
    return btn;
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Setup Main UI (Safari Style) ──────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupUi()
{
    m_central = new QWidget(this);
    m_central->setObjectName(QStringLiteral("CentralWidget"));
    setCentralWidget(m_central);

    QVBoxLayout *rootLayout = new QVBoxLayout(m_central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    m_toolbar = new QWidget(m_central);
    m_toolbar->setObjectName(QStringLiteral("Toolbar"));
    m_toolbar->setFixedHeight(52);

    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(12, 0, 12, 0);
    toolbarLayout->setSpacing(6);

    // Traffic lights
    m_closeButton    = createTrafficLight(QStringLiteral("#ff5f56"), QStringLiteral("#e0443e"));
    m_minimizeButton = createTrafficLight(QStringLiteral("#ffbd2e"), QStringLiteral("#dea124"));
    m_maximizeButton = createTrafficLight(QStringLiteral("#27c93f"), QStringLiteral("#1aab29"));

    connect(m_closeButton,    &QToolButton::clicked, this, &BrowserWindow::closeWindow);
    connect(m_minimizeButton, &QToolButton::clicked, this, &BrowserWindow::minimizeWindow);
    connect(m_maximizeButton, &QToolButton::clicked, this, &BrowserWindow::maximizeWindow);

    QHBoxLayout *trafficLayout = new QHBoxLayout();
    trafficLayout->setContentsMargins(8, 0, 8, 0);
    trafficLayout->setSpacing(6);
    trafficLayout->addWidget(m_closeButton);
    trafficLayout->addWidget(m_minimizeButton);
    trafficLayout->addWidget(m_maximizeButton);
    trafficLayout->addSpacing(8);
    toolbarLayout->addLayout(trafficLayout);

    m_sidebarButton->setToolTip(QStringLiteral("Toggle Sidebar"));
    connect(m_sidebarButton, &QToolButton::clicked, this, &BrowserWindow::toggleSidebar);
    toolbarLayout->addWidget(m_sidebarButton);

    toolbarLayout->addSpacing(2);

    m_backButton->setToolTip(QStringLiteral("Back"));
    toolbarLayout->addWidget(m_backButton);

    m_forwardButton->setToolTip(QStringLiteral("Forward"));
    toolbarLayout->addWidget(m_forwardButton);

    m_reloadButton->setToolTip(QStringLiteral("Reload"));
    toolbarLayout->addWidget(m_reloadButton);

    toolbarLayout->addSpacing(6);

    // URL Bar (container with shield icon + borderless line edit)
    m_urlBar->setPlaceholderText(QStringLiteral("Search or enter website name"));
    m_urlBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_urlBar->setMinimumHeight(30);
    m_urlBar->setMaximumHeight(30);
    m_urlBar->setMinimumWidth(200);
    m_urlBar->installEventFilter(this);

    m_urlContainer = new QFrame(m_toolbar);
    m_urlContainer->setObjectName(QStringLiteral("UrlContainer"));

    QHBoxLayout *urlLayout = new QHBoxLayout(m_urlContainer);
    urlLayout->setContentsMargins(8, 0, 8, 0);
    urlLayout->setSpacing(4);

    m_shieldInside = new QToolButton(m_urlContainer);
    m_shieldInside->setFixedSize(18, 18);
    m_shieldInside->setToolTip(QStringLiteral("Privacy Report"));
    urlLayout->addWidget(m_shieldInside);

    urlLayout->addWidget(m_urlBar, 1);
    toolbarLayout->addWidget(m_urlContainer, 1);

    toolbarLayout->addSpacing(6);

    // Right side buttons
    m_shareButton->setToolTip(QStringLiteral("Share"));
    connect(m_shareButton, &QToolButton::clicked, this, &BrowserWindow::shareAction);
    toolbarLayout->addWidget(m_shareButton);

    m_downloadsButton->setToolTip(QStringLiteral("Downloads"));
    toolbarLayout->addWidget(m_downloadsButton);

    m_tabOverviewButton->setToolTip(QStringLiteral("Tab Overview"));
    connect(m_tabOverviewButton, &QToolButton::clicked, this, &BrowserWindow::toggleTabOverview);
    toolbarLayout->addWidget(m_tabOverviewButton);

    rootLayout->addWidget(m_toolbar);

    // ── Loading Bar ────────────────────────────────────────────────────────
    m_loadingBar = new QLabel(m_central);
    m_loadingBar->setFixedHeight(2);
    m_loadingBar->setVisible(false);
    rootLayout->addWidget(m_loadingBar);

    // ── Content area (sidebar + tab stack) ─────────────────────────────────
    m_sidebar = new QFrame(m_central);
    m_sidebar->setObjectName(QStringLiteral("Sidebar"));
    m_sidebar->setFixedWidth(260);
    m_sidebar->setVisible(false);

    QHBoxLayout *contentRow = new QHBoxLayout();
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);

    contentRow->addWidget(m_sidebar);
    contentRow->addWidget(m_tabStack, 1);

    rootLayout->addLayout(contentRow, 1);

    // ── Connect Navigation ─────────────────────────────────────────────────
    connect(m_backButton, &QToolButton::clicked, this, [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->back();
    });
    connect(m_forwardButton, &QToolButton::clicked, this, [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->forward();
    });
    connect(m_reloadButton, &QToolButton::clicked, this, [this]() {
        if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) {
            QWebEngineView *v = m_tabs[m_currentTabIndex].view;
            if (m_tabs[m_currentTabIndex].loading) v->stop();
            else v->reload();
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Tab Bar Setup ─────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupTabBar()
{
    m_tabBar = new QWidget(centralWidget());
    m_tabBar->setObjectName(QStringLiteral("TabBar"));
    m_tabBar->setFixedHeight(36);

    m_tabBarLayout = new QHBoxLayout(m_tabBar);
    m_tabBarLayout->setContentsMargins(76, 0, 4, 0);
    m_tabBarLayout->setSpacing(2);
    m_tabBarLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Add tab button
    m_addTabButton->setFixedSize(26, 26);
    m_addTabButton->setToolTip(QStringLiteral("New Tab"));
    connect(m_addTabButton, &QToolButton::clicked, this, &BrowserWindow::addTabAction);

    // Insert tab bar into root layout (between toolbar and tab stack)
    QLayout *rootLayout = centralWidget()->layout();
    static_cast<QVBoxLayout*>(rootLayout)->insertWidget(2, m_tabBar);

    m_tabBarLayout->addStretch();
    m_tabBarLayout->addWidget(m_addTabButton);
    m_tabBarLayout->addSpacing(4);
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Sidebar Setup ─────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupSidebar()
{
    m_sidebarLayout = new QVBoxLayout(m_sidebar);
    m_sidebarLayout->setContentsMargins(12, 8, 12, 12);
    m_sidebarLayout->setSpacing(4);

    // Sidebar search
    m_sidebarSearch = new QLineEdit(m_sidebar);
    m_sidebarSearch->setPlaceholderText(QStringLiteral("\U0001F50D  Search tabs, bookmarks\u2026"));
    m_sidebarSearch->setFixedHeight(28);
    m_sidebarLayout->addWidget(m_sidebarSearch);
    m_sidebarLayout->addSpacing(8);

    connect(m_sidebarSearch, &QLineEdit::textChanged, this, [this](const QString &query) {
        QString q = query.trimmed().toLower();
        for (int i = 0; i < m_sidebarItems.count(); ++i) {
            if (i < m_sidebarItemTexts.count()) {
                QString text = m_sidebarItemTexts[i]->text().toLower();
                m_sidebarItems[i]->setVisible(q.isEmpty() || text.contains(q));
            }
        }
    });

    auto addSectionHeader = [this](const QString &title) {
        QLabel *lbl = new QLabel(title, m_sidebar);
        m_sidebarLayout->addWidget(lbl);
        m_sidebarHeaders.append(lbl);
    };

    auto addSidebarItem = [this](const QString &icon, const QString &text, bool isActive = false) -> QFrame* {
        QFrame *item = new QFrame(m_sidebar);
        item->setFixedHeight(28);
        item->setCursor(Qt::PointingHandCursor);
        item->setProperty("sidebarActive", isActive);

        QHBoxLayout *lay = new QHBoxLayout(item);
        lay->setContentsMargins(8, 2, 8, 2);
        lay->setSpacing(6);

        QLabel *iconLbl = new QLabel(icon, item);
        iconLbl->setFixedSize(18, 18);
        iconLbl->setAlignment(Qt::AlignCenter);
        lay->addWidget(iconLbl);

        QLabel *textLbl = new QLabel(text, item);
        lay->addWidget(textLbl, 1);

        m_sidebarLayout->addWidget(item);
        m_sidebarItems.append(item);
        m_sidebarItemIcons.append(iconLbl);
        m_sidebarItemTexts.append(textLbl);
        return item;
    };

    // Tab Groups
    addSectionHeader(QStringLiteral("Tab Groups"));
    addSidebarItem(QStringLiteral("\U0001F4C2"), QStringLiteral("All Tabs"), true);
    addSidebarItem(QStringLiteral("\U0001F4CB"), QStringLiteral("Personal"));
    addSidebarItem(QStringLiteral("\U0001F4BC"), QStringLiteral("Work"));

    m_sidebarLayout->addSpacing(8);

    // Bookmarks
    addSectionHeader(QStringLiteral("Bookmarks"));
    addSidebarItem(QStringLiteral("\u2B50"), QStringLiteral("Favorites"));
    addSidebarItem(svgBookmarks, QStringLiteral("Bookmarks"));
    addSidebarItem(svgReadingList, QStringLiteral("Reading List"));

    m_sidebarLayout->addSpacing(8);

    // Recently Closed
    addSectionHeader(QStringLiteral("Recently Closed"));
    addSidebarItem(svgHistory, QStringLiteral("No recent items"));

    m_sidebarLayout->addSpacing(8);

    // iCloud
    addSidebarItem(svgiCloud, QStringLiteral("iCloud Tabs"));

    m_sidebarLayout->addStretch();
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Tab Overview Overlay ──────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupTabOverlay()
{
    m_overviewOverlay = new QWidget(this);
    m_overviewOverlay->setObjectName(QStringLiteral("OverviewOverlay"));
    m_overviewOverlay->setGeometry(rect());
    m_overviewOverlay->setVisible(false);
    m_overviewOverlay->raise();

    m_overviewPanel = new QWidget(m_overviewOverlay);
    m_overviewPanel->setObjectName(QStringLiteral("OverviewPanel"));

    QVBoxLayout *panelLayout = new QVBoxLayout(m_overviewPanel);
    panelLayout->setContentsMargins(16, 12, 16, 16);
    panelLayout->setSpacing(10);

    // Header
    QHBoxLayout *hdr = new QHBoxLayout();
    m_overviewTitle = new QLabel(QStringLiteral("Tabs"), m_overviewPanel);
    hdr->addWidget(m_overviewTitle);
    hdr->addStretch();

    m_overviewDoneButton = new QPushButton(QStringLiteral("Done"), m_overviewPanel);
    connect(m_overviewDoneButton, &QPushButton::clicked, this, &BrowserWindow::hideTabOverview);
    hdr->addWidget(m_overviewDoneButton);
    panelLayout->addLayout(hdr);

    // Scroll area with grid
    m_overviewScroll = new QScrollArea(m_overviewPanel);
    m_overviewScroll->setWidgetResizable(true);
    m_overviewScroll->setFrameShape(QFrame::NoFrame);
    m_overviewScroll->setStyleSheet(QStringLiteral("background: transparent;"));

    m_overviewGrid = new QWidget;
    m_overviewGrid->setStyleSheet(QStringLiteral("background: transparent;"));
    m_overviewGridLayout = new QGridLayout(m_overviewGrid);
    m_overviewGridLayout->setSpacing(14);
    m_overviewGridLayout->setContentsMargins(0, 0, 0, 0);

    m_overviewScroll->setWidget(m_overviewGrid);
    panelLayout->addWidget(m_overviewScroll);

    // New tab button
    m_overviewNewTabButton = new QPushButton(QStringLiteral("+ New Tab"), m_overviewPanel);
    connect(m_overviewNewTabButton, &QPushButton::clicked, this, [this]() {
        hideTabOverview();
        addTabAction();
    });
    panelLayout->addWidget(m_overviewNewTabButton);

    m_overviewOverlay->installEventFilter(this);
}

void BrowserWindow::showTabOverview()
{
    for (int i = 0; i < m_tabs.count(); ++i) {
        if (m_tabs[i].view) {
            m_tabs[i].thumbnail = m_tabs[i].view->grab().scaled(240, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
    }

    m_overviewVisible = true;
    rebuildOverviewGrid();

    int pw = qMin(width() - 80, 900);
    int ph = qMin(height() - 80, 600);
    int px = (width() - pw) / 2;
    int py = (height() - ph) / 2;
    m_overviewPanel->setGeometry(px, py, pw, ph);

    m_overviewOverlay->setGeometry(rect());
    m_overviewOverlay->setVisible(true);
    m_overviewOverlay->raise();
}

void BrowserWindow::hideTabOverview()
{
    m_overviewVisible = false;
    m_overviewOverlay->setVisible(false);
}

void BrowserWindow::toggleTabOverview()
{
    if (m_overviewVisible) hideTabOverview();
    else showTabOverview();
}

QWidget* BrowserWindow::buildOverviewCard(int index)
{
    const TabInfo &tab = m_tabs[index];
    bool isActive = (index == m_currentTabIndex);

    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("OverviewCard"));
    card->setMinimumWidth(200);
    card->setMaximumWidth(320);
    card->setFixedHeight(100);
    card->setStyleSheet(QString(
        "#OverviewCard { background-color: %1; border-radius: 10px; border: 1.5px solid %2; }"
    ).arg(isActive ? tabActive() : cardBg(),
         isActive ? accent() : border()));
    card->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *lay = new QHBoxLayout(card);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    // Favicon
    QLabel *iconLbl = new QLabel(card);
    QPixmap pix = tab.icon.pixmap(20, 20);
    if (!pix.isNull()) {
        iconLbl->setPixmap(pix);
    } else {
        iconLbl->setText(QStringLiteral("\U0001F310"));
        iconLbl->setStyleSheet(QString("font-size: 16px; background: transparent; color: %1;").arg(textSecondary()));
    }
    iconLbl->setFixedSize(20, 20);
    iconLbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(iconLbl);

    // Info
    QVBoxLayout *info = new QVBoxLayout;
    info->setSpacing(2);

    if (!tab.thumbnail.isNull()) {
        QLabel *thumbLbl = new QLabel(card);
        thumbLbl->setPixmap(tab.thumbnail);
        thumbLbl->setFixedSize(100, 65);
        thumbLbl->setScaledContents(true);
        thumbLbl->setStyleSheet(QStringLiteral("border-radius: 6px; border: 0.5px solid rgba(0,0,0,0.15);"));
        lay->addWidget(thumbLbl);
    }

    QString shownTitle = tab.title.isEmpty() ? QStringLiteral("New Tab") : tab.title;
    QLabel *titleLbl = new QLabel(truncate(shownTitle, 24));
    titleLbl->setStyleSheet(QString("font-size: 13px; font-weight: 600; color: %1; background: transparent;").arg(textPrimary()));
    info->addWidget(titleLbl);

    QString shownUrl = tab.url.isEmpty() ? QString() : shortUrl(tab.url);
    QLabel *urlLbl = new QLabel(truncate(shownUrl, 30));
    urlLbl->setStyleSheet(QString("font-size: 11px; color: %1; background: transparent;").arg(textSecondary()));
    info->addWidget(urlLbl);
    info->addStretch();

    lay->addLayout(info, 1);

    // Close button
    QPushButton *closeBtn = new QPushButton(QStringLiteral("\u2715"), card);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; color: %1; font-size: 10px; border-radius: 10px; }"
        "QPushButton:hover { background: rgba(255,59,48,0.12); color: #ff3b30; }"
    ).arg(textSecondary()));
    connect(closeBtn, &QPushButton::clicked, this, [this, index]() { closeTab(index); });
    lay->addWidget(closeBtn, 0, Qt::AlignTop);

    card->installEventFilter(this);
    card->setProperty("tabIndex", index);

    return card;
}

void BrowserWindow::rebuildOverviewGrid()
{
    QLayoutItem *item;
    while ((item = m_overviewGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int columns = qMax(2, qMin(4, width() / 280));
    int row = 0, col = 0;
    for (int i = 0; i < m_tabs.count(); ++i) {
        QWidget *card = buildOverviewCard(i);
        m_overviewGridLayout->addWidget(card, row, col);
        if (++col >= columns) { col = 0; row++; }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Tab Management ────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::addNewTab(const QUrl &url) {
    auto *view = new SafariWebView(this);
    view->page()->setBackgroundColor(QColor(SafariTheme::instance().pageBackground));
    m_tabStack->addWidget(view);

    TabInfo info;
    info.view  = view;
    info.title = QStringLiteral("New Tab");
    info.url   = url.toString();
    m_tabs.append(info);

    int index = m_tabs.count() - 1;

    // FIX: Lambda captures QPointer to the view, finds tab by view pointer
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &t) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].title = t;
                if (i == m_currentTabIndex) setWindowTitleFromTab();
                break;
            }
        }
        rebuildTabBar();
    });

    connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].icon = icon;
                break;
            }
        }
        rebuildTabBar();
    });

    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &u) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].url = u.toString();
                if (i == m_currentTabIndex) updateUrlBar(u);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::loadStarted, this, [this, view]() {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].loading = true;
                break;
            }
        }
        onLoadStarted();
    });
    connect(view, &QWebEngineView::loadProgress, this, &BrowserWindow::onLoadProgress);
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].loading = false;
                if (ok) saveHistoryItem(m_tabs[i].title, m_tabs[i].url);
                break;
            }
        }
        onLoadFinished(ok);
    });
    connect(view, &QWebEngineView::loadFinished, this, &BrowserWindow::updateNavigationState);

    connect(view, &SafariWebView::newTabRequested, this, &BrowserWindow::addNewTab);

    view->setUrl(url);
    setCurrentTab(index);
}

void BrowserWindow::setCurrentTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;
    m_currentTabIndex = index;
    m_tabStack->setCurrentIndex(index);

    QWebEngineView *v = m_tabs[index].view;
    updateUrlBar(v->url());
    updateNavigationState();
    rebuildTabBar();
    setWindowTitleFromTab();

    m_loadingBar->setVisible(m_tabs[index].loading);
}

void BrowserWindow::closeTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;

    if (m_tabs.count() == 1) {
        addNewTab(QUrl(QStringLiteral("qrc:/startpage.html")));
    }

    const QUrl closedUrl(m_tabs[index].url);
    if (closedUrl.isValid() && !closedUrl.isEmpty()) {
        m_closedTabs.append(closedUrl);
        while (m_closedTabs.count() > 20)
            m_closedTabs.removeFirst();
    }

    QWebEngineView *v = m_tabs[index].view;
    m_tabStack->removeWidget(v);
    v->deleteLater();
    m_tabs.removeAt(index);

    if (m_tabs.isEmpty()) return;

    int newIdx = m_currentTabIndex;
    if (index <= m_currentTabIndex) {
        newIdx = qMax(0, m_currentTabIndex - 1);
    }
    newIdx = qMin(newIdx, m_tabs.count() - 1);

    m_currentTabIndex = -1;
    setCurrentTab(newIdx);

    if (m_overviewVisible) rebuildOverviewGrid();
}

void BrowserWindow::addTabAction() {
    addNewTab(QUrl(QStringLiteral("qrc:/startpage.html")));
}

void BrowserWindow::rebuildTabBar()
{
    // Clear the layout completely (widgets are deleted, addTabButton is kept)
    while (QLayoutItem *item = m_tabBarLayout->takeAt(0)) {
        QWidget *w = item->widget();
        if (w && w != m_addTabButton)
            w->deleteLater();
        delete item;
    }
    m_tabWidgets.clear();

    // Rebuild: stretch -> tabs -> addTabButton -> spacing
    m_tabBarLayout->addStretch();

    for (int i = 0; i < m_tabs.count(); ++i) {
        bool isActive = (i == m_currentTabIndex);
        const TabInfo &tab = m_tabs[i];

        auto *tabWidget = new QWidget(m_tabBar);
        tabWidget->setFixedHeight(28);
        tabWidget->setMinimumWidth(80);
        tabWidget->setMaximumWidth(200);
        tabWidget->setCursor(Qt::PointingHandCursor);
        tabWidget->setStyleSheet(QString(
            "QWidget { background-color: %1; border-radius: 6px; }"
            "QWidget:hover { background-color: %2; }"
        ).arg(isActive ? tabActive() : tabInactive(),
             isActive ? tabActive() : tabHover()));

        if (isActive) {
            auto *shadow = new QGraphicsDropShadowEffect;
            shadow->setBlurRadius(8);
            shadow->setOffset(0, 1);
            shadow->setColor(QColor(0, 0, 0, 30));
            tabWidget->setGraphicsEffect(shadow);
        }

        QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);
        tabLayout->setContentsMargins(8, 2, 4, 2);
        tabLayout->setSpacing(4);

        // Favicon
        QLabel *iconLabel = new QLabel(tabWidget);
        QPixmap pix = tab.icon.pixmap(14, 14);
        if (!pix.isNull()) {
            iconLabel->setPixmap(pix);
        } else {
            iconLabel->setText(QStringLiteral("\U0001F310"));
            iconLabel->setStyleSheet(QString("font-size: 11px; background: transparent; color: %1;").arg(textSecondary()));
        }
        iconLabel->setFixedSize(14, 14);
        iconLabel->setAlignment(Qt::AlignCenter);
        tabLayout->addWidget(iconLabel);

        // Title
        QString shownText = tab.title.isEmpty() ? QStringLiteral("New Tab") : tab.title;
        auto *titleLbl = new QLabel(truncate(shownText, 18), tabWidget);
        titleLbl->setStyleSheet(QString(
            "color: %1; font-size: 12px; font-weight: %2; background: transparent;"
        ).arg(isActive ? textPrimary() : textSecondary(),
             isActive ? "600" : "400"));
        titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tabLayout->addWidget(titleLbl, 1);

        // Close button
        auto *closeBtn = new QPushButton(QStringLiteral("\u2715"), tabWidget);
        closeBtn->setFixedSize(18, 18);
        closeBtn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; color: %1; font-size: 9px; border-radius: 9px; }"
            "QPushButton:hover { background: rgba(255,59,48,0.12); color: #ff3b30; }"
        ).arg(textTertiary()));
        connect(closeBtn, &QPushButton::clicked, this, [this, i]() { closeTab(i); });
        tabLayout->addWidget(closeBtn);

        tabWidget->installEventFilter(this);
        tabWidget->setProperty("tabIndex", i);

        m_tabBarLayout->addWidget(tabWidget);
        m_tabWidgets.append(tabWidget);
    }

    m_tabBarLayout->addWidget(m_addTabButton);
    m_tabBarLayout->addSpacing(4);
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Sidebar Toggle ────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::toggleSidebar()
{
    m_sidebarVisible = !m_sidebarVisible;
    m_sidebar->setVisible(m_sidebarVisible);
}

void BrowserWindow::rebuildSidebarTabList()
{
    // Placeholder for dynamic sidebar tab list updates
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Event Filter ──────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
bool BrowserWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto *w = qobject_cast<QWidget*>(obj);
        if (w) {
            QVariant v = w->property("tabIndex");
            if (v.isValid()) {
                int idx = v.toInt();
                if (idx >= 0 && idx < m_tabs.count()) {
                    if (m_overviewVisible) hideTabOverview();
                    setCurrentTab(idx);
                    return true;
                }
            }
            if (obj == m_overviewOverlay) {
                auto *me = static_cast<QMouseEvent*>(event);
                if (!m_overviewPanel->geometry().contains(me->pos()))
                    hideTabOverview();
                return true;
            }
        }
    }

    // URL bar focus ring
    if (obj == m_urlContainer || obj == m_urlBar) {
        if (event->type() == QEvent::FocusIn) {
            m_urlFocused = true;
            updateUrlContainerStyle();
        } else if (event->type() == QEvent::FocusOut) {
            m_urlFocused = false;
            updateUrlContainerStyle();
        }
        return false;
    }

    return QMainWindow::eventFilter(obj, event);
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Slots ─────────────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::navigateToUrl() {
    QString input = m_urlBar->text().trimmed();
    if (input.isEmpty()) return;

    if (!input.startsWith(QStringLiteral("http://")) && !input.startsWith(QStringLiteral("https://"))) {
        if (input.contains(QStringLiteral(".")) && !input.contains(QStringLiteral(" "))) {
            input = QStringLiteral("https://") + input;
        } else {
            input = QStringLiteral("https://www.google.com/search?q=") + QString::fromUtf8(input.toUtf8().toPercentEncoding());
        }
    }

    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
        v->setUrl(QUrl(input));
    m_urlBar->clearFocus();
}

void BrowserWindow::updateUrlBar(const QUrl &url) {
    if (m_currentTabIndex < 0 || m_currentTabIndex >= m_tabs.count()) return;
    m_urlBar->setText(shortUrl(url.toString()));

    if (m_shieldInside) {
        if (url.scheme() == QStringLiteral("https")) {
            m_shieldInside->setIcon(createSvgIcon(svgShield, 16, QStringLiteral("#34c759"))); // Green secure shield
            m_shieldInside->setToolTip(QStringLiteral("Connection is secure (HTTPS) — BLACK Protection Active"));
        } else if (url.scheme() == QStringLiteral("http")) {
            m_shieldInside->setIcon(createSvgIcon(svgShield, 16, QStringLiteral("#ff3b30"))); // Red unsecure shield
            m_shieldInside->setToolTip(QStringLiteral("Connection is Not Secure (HTTP)"));
        } else {
            m_shieldInside->setIcon(createSvgIcon(svgShield, 16, textSecondary()));
            m_shieldInside->setToolTip(QStringLiteral("Privacy Report"));
        }
    }
}

void BrowserWindow::updateNavigationState() {
    auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget());
    if (!v) return;
    m_backButton->setEnabled(v->history()->canGoBack());
    m_forwardButton->setEnabled(v->history()->canGoForward());

    // Update reload/stop icon
    bool loading = (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count())
                   && m_tabs[m_currentTabIndex].loading;
    if (loading) {
        m_reloadButton->setIcon(createSvgIcon(svgStop, 18, textPrimary()));
        m_reloadButton->setToolTip(QStringLiteral("Stop"));
    } else {
        m_reloadButton->setIcon(createSvgIcon(svgReload, 18, textPrimary()));
        m_reloadButton->setToolTip(QStringLiteral("Reload"));
    }
}

void BrowserWindow::onLoadStarted() {
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()
        && m_tabs[m_currentTabIndex].loading) {
        m_loadingBar->setVisible(true);
        updateLoadingBar(0);
    }
    updateNavigationState();
}

void BrowserWindow::onLoadProgress(int progress) {
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()
        && m_tabs[m_currentTabIndex].loading) {
        updateLoadingBar(progress);
    }
}

void BrowserWindow::onLoadFinished(bool) {
    m_loadingBar->setVisible(false);
    updateNavigationState();
}

void BrowserWindow::updateLoadingBar(int progress) {
    Q_UNUSED(progress);
    // Loading bar is a thin line; we could animate its width, but for simplicity just show/hide
}

void BrowserWindow::shareAction() {
    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) {
        QApplication::clipboard()->setText(v->url().toString());
        m_shareButton->setToolTip(QStringLiteral("URL Copied!"));
        QTimer::singleShot(1500, this, [this]() {
            m_shareButton->setToolTip(QStringLiteral("Share"));
        });
    }
}

void BrowserWindow::setWindowTitleFromTab() {
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) {
        QString title = m_tabs[m_currentTabIndex].title;
        if (title.isEmpty()) title = QStringLiteral("New Tab");
        setWindowTitle(title + QStringLiteral(" — BLACK"));
    } else {
        setWindowTitle(QStringLiteral("BLACK"));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Keyboard Shortcuts ────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupKeyboardShortcuts()
{
    auto addShortcut = [this](const QString &key, std::function<void()> func) {
        auto *s = new QShortcut(QKeySequence(key), this);
        connect(s, &QShortcut::activated, this, std::move(func));
    };

    addShortcut(QStringLiteral("Ctrl+T"), [this]() { addTabAction(); });
    addShortcut(QStringLiteral("Ctrl+W"), [this]() { closeTab(m_currentTabIndex); });
    addShortcut(QStringLiteral("Ctrl+Shift+T"), [this]() {
        if (m_closedTabs.isEmpty()) return;
        addNewTab(m_closedTabs.takeLast());
    });
    addShortcut(QStringLiteral("Ctrl+L"), [this]() {
        m_urlBar->setFocus();
        m_urlBar->selectAll();
    });
    addShortcut(QStringLiteral("F6"), [this]() {
        m_urlBar->setFocus();
        m_urlBar->selectAll();
    });
    addShortcut(QStringLiteral("Ctrl+R"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->reload();
    });
    addShortcut(QStringLiteral("F5"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->reload();
    });
    addShortcut(QStringLiteral("Ctrl+Shift+R"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->reload();
    });
    addShortcut(QStringLiteral("Ctrl+F"), [this]() { showFindBar(); });
    addShortcut(QStringLiteral("Escape"), [this]() {
        if (m_findBar && m_findBar->isVisible()) hideFindBar();
        else if (m_overviewVisible) hideTabOverview();
        else if (m_sidebarVisible) toggleSidebar();
    });
    addShortcut(QStringLiteral("Ctrl+Shift+L"), [this]() { toggleSidebar(); });
    addShortcut(QStringLiteral("Alt+Left"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->back();
    });
    addShortcut(QStringLiteral("Alt+Right"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->forward();
    });
    addShortcut(QStringLiteral("Ctrl+Tab"), [this]() {
        if (m_tabs.count() > 1) {
            int next = (m_currentTabIndex + 1) % m_tabs.count();
            setCurrentTab(next);
        }
    });
    addShortcut(QStringLiteral("Ctrl+Shift+Tab"), [this]() {
        if (m_tabs.count() > 1) {
            int prev = (m_currentTabIndex - 1 + m_tabs.count()) % m_tabs.count();
            setCurrentTab(prev);
        }
    });
    addShortcut(QStringLiteral("Ctrl+1"), [this]() { if (m_tabs.count() > 0) setCurrentTab(0); });
    addShortcut(QStringLiteral("Ctrl+2"), [this]() { if (m_tabs.count() > 1) setCurrentTab(1); });
    addShortcut(QStringLiteral("Ctrl+3"), [this]() { if (m_tabs.count() > 2) setCurrentTab(2); });
    addShortcut(QStringLiteral("Ctrl+4"), [this]() { if (m_tabs.count() > 3) setCurrentTab(3); });
    addShortcut(QStringLiteral("Ctrl+5"), [this]() { if (m_tabs.count() > 4) setCurrentTab(4); });
    addShortcut(QStringLiteral("Ctrl+6"), [this]() { if (m_tabs.count() > 5) setCurrentTab(5); });
    addShortcut(QStringLiteral("Ctrl+7"), [this]() { if (m_tabs.count() > 6) setCurrentTab(6); });
    addShortcut(QStringLiteral("Ctrl+8"), [this]() { if (m_tabs.count() > 7) setCurrentTab(7); });
    addShortcut(QStringLiteral("Ctrl+9"), [this]() { if (!m_tabs.isEmpty()) setCurrentTab(m_tabs.count() - 1); });
    addShortcut(QStringLiteral("F11"), [this]() {
        if (isFullScreen()) showNormal();
        else showFullScreen();
    });
    addShortcut(QStringLiteral("Ctrl+Plus"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->setZoomFactor(v->zoomFactor() + 0.1);
    });
    addShortcut(QStringLiteral("Ctrl+="), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->setZoomFactor(v->zoomFactor() + 0.1);
    });
    addShortcut(QStringLiteral("Ctrl+Minus"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->setZoomFactor(qMax(0.25, v->zoomFactor() - 0.1));
    });
    addShortcut(QStringLiteral("Ctrl+0"), [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->setZoomFactor(1.0);
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Theme ─────────────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::updateUrlContainerStyle()
{
    m_urlContainer->setStyleSheet(QString(
        "#UrlContainer { background-color: %1; border: %2 solid %3; border-radius: 8px; }"
    ).arg(bgUrlBar(), m_urlFocused ? "1.5px" : "1px",
          m_urlFocused ? accent() : border()));
}

void BrowserWindow::applyTheme()
{
    // Central widget + toolbar
    m_central->setStyleSheet(QString(
        "#CentralWidget { background-color: %1; border-radius: 10px; border: 0.5px solid %2; }"
    ).arg(bgWindow(), border()));
    m_toolbar->setStyleSheet(QString(
        "#Toolbar { background-color: %1; border-bottom: 0.5px solid %2; "
        "border-top-left-radius: 10px; border-top-right-radius: 10px; }"
    ).arg(bgToolbar(), border()));

    // Navigation buttons
    const QString navBtnStyle = QString(
        "QToolButton { border: none; background: transparent; border-radius: 6px; padding: 6px; }"
        "QToolButton:hover { background-color: %1; }"
        "QToolButton:disabled { opacity: 0.3; }"
    ).arg(hover());

    m_sidebarButton->setIcon(createSvgIcon(svgSidebar, 18, textPrimary()));
    m_backButton->setIcon(createSvgIcon(svgBack, 18, textPrimary()));
    m_forwardButton->setIcon(createSvgIcon(svgForward, 18, textPrimary()));
    m_reloadButton->setIcon(createSvgIcon(svgReload, 18, textPrimary()));
    m_shareButton->setIcon(createSvgIcon(svgShare, 18, textPrimary()));
    m_downloadsButton->setIcon(createSvgIcon(svgDownloads, 18, textPrimary()));
    m_tabOverviewButton->setIcon(createSvgIcon(svgTabOverview, 18, textPrimary()));

    for (QToolButton *b : { m_sidebarButton, m_backButton, m_forwardButton, m_reloadButton,
                            m_shareButton, m_downloadsButton, m_tabOverviewButton }) {
        b->setStyleSheet(navBtnStyle);
    }
    updateNavigationState();

    // URL bar
    m_urlBar->setStyleSheet(QString(
        "QLineEdit { "
        "  background: transparent; color: %1; border: none; "
        "  font-size: 14px; font-weight: 400; padding: 0 6px; "
        "}"
    ).arg(textPrimary()));
    m_shieldInside->setIcon(createSvgIcon(svgShield, 14, accent()));
    m_shieldInside->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    updateUrlContainerStyle();

    // Loading bar
    m_loadingBar->setStyleSheet(QStringLiteral("background-color: %1; border: none;").arg(accent()));

    // Sidebar
    m_sidebar->setStyleSheet(QString(
        "#Sidebar { background-color: %1; border-right: 0.5px solid %2; }"
    ).arg(bgSidebar(), border()));
    m_sidebarSearch->setStyleSheet(QString(
        "QLineEdit { background-color: %1; border: none; border-radius: 6px; "
        "padding: 6px 10px; font-size: 12px; color: %2; }"
        "QLineEdit:focus { background-color: %3; }"
    ).arg(searchBg(), textPrimary(), hover()));

    for (QLabel *lbl : m_sidebarHeaders) {
        lbl->setStyleSheet(QString(
            "font-size: 11px; font-weight: 600; color: %1; padding: 4px 4px 2px 4px; "
            "text-transform: uppercase; letter-spacing: 0.3px;"
        ).arg(textSecondary()));
    }
    for (int i = 0; i < m_sidebarItems.count(); ++i) {
        const bool isActive = m_sidebarItems[i]->property("sidebarActive").toBool();
        m_sidebarItems[i]->setStyleSheet(QString(
            "QFrame { background-color: %1; border-radius: 6px; }"
            "QFrame:hover { background-color: %2; }"
        ).arg(isActive ? selectedBg() : QStringLiteral("transparent"), hover()));
        m_sidebarItemIcons[i]->setStyleSheet(QString(
            "font-size: 13px; color: %1; background: transparent;").arg(isActive ? accent() : textPrimary()));
        m_sidebarItemTexts[i]->setStyleSheet(QString(
            "font-size: 13px; color: %1; font-weight: %2; background: transparent;"
        ).arg(isActive ? accent() : textPrimary(), isActive ? "600" : "400"));
    }

    // Tab bar
    m_tabBar->setStyleSheet(QString(
        "#TabBar { background-color: %1; border-bottom: 0.5px solid %2; }"
    ).arg(bgTabBar(), border()));
    m_addTabButton->setIcon(createSvgIcon(svgPlus, 14, textSecondary()));
    m_addTabButton->setStyleSheet(QString(
        "QToolButton { border: none; background: transparent; border-radius: 6px; padding: 0; }"
        "QToolButton:hover { background-color: %1; }"
    ).arg(tabHover()));

    // Tab overview
    m_overviewOverlay->setStyleSheet(QString(
        "#OverviewOverlay { background-color: %1; }"
    ).arg(SafariTheme::instance().scrim));
    m_overviewPanel->setStyleSheet(QString(
        "#OverviewPanel { background-color: %1; border-radius: 14px; }"
    ).arg(bgWindow()));
    m_overviewTitle->setStyleSheet(QString("font-size: 16px; font-weight: 700; color: %1;").arg(textPrimary()));
    m_overviewDoneButton->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; color: %1; font-size: 14px; font-weight: 600; }"
        "QPushButton:hover { color: %2; }"
    ).arg(accent(), textPrimary()));
    m_overviewNewTabButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: #ffffff; border-radius: 8px; "
        "font-size: 13px; font-weight: 600; padding: 8px 0; border: none; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(accent(), accentHover()));

    // Refresh dynamically-created widgets
    rebuildTabBar();
    if (m_overviewVisible) rebuildOverviewGrid();

    updateWebViewBackgrounds();
}

void BrowserWindow::updateWebViewBackgrounds()
{
    const QColor bg(SafariTheme::instance().pageBackground);
    for (const TabInfo &tab : m_tabs) {
        if (tab.view)
            tab.view->page()->setBackgroundColor(bg);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Context Menu ──────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::contextMenuEvent(QContextMenuEvent *event) {
    Q_UNUSED(event);
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Find in Page implementation ───────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::setupFindBar() {
    m_findBar = new QWidget(m_central);
    m_findBar->setObjectName(QStringLiteral("FindBar"));
    m_findBar->setFixedHeight(36);
    m_findBar->setVisible(false);

    QHBoxLayout *lay = new QHBoxLayout(m_findBar);
    lay->setContentsMargins(12, 4, 12, 4);
    lay->setSpacing(6);

    m_findInput = new QLineEdit(m_findBar);
    m_findInput->setPlaceholderText(QStringLiteral("Find in page\u2026"));
    m_findInput->setFixedWidth(200);

    m_findMatchCount = new QLabel(m_findBar);
    m_findMatchCount->setStyleSheet(QStringLiteral("color: #8e8e93; font-size: 11px;"));

    m_findPrevBtn = new QToolButton(m_findBar);
    m_findPrevBtn->setText(QStringLiteral("\u25B2"));
    m_findPrevBtn->setToolTip(QStringLiteral("Previous match"));

    m_findNextBtn = new QToolButton(m_findBar);
    m_findNextBtn->setText(QStringLiteral("\u25BC"));
    m_findNextBtn->setToolTip(QStringLiteral("Next match"));

    m_findCloseBtn = new QToolButton(m_findBar);
    m_findCloseBtn->setText(QStringLiteral("\u2715"));
    m_findCloseBtn->setToolTip(QStringLiteral("Close find bar (Esc)"));

    lay->addWidget(m_findInput);
    lay->addWidget(m_findMatchCount);
    lay->addWidget(m_findPrevBtn);
    lay->addWidget(m_findNextBtn);
    lay->addStretch();
    lay->addWidget(m_findCloseBtn);

    QLayout *rootLayout = centralWidget()->layout();
    static_cast<QVBoxLayout*>(rootLayout)->insertWidget(2, m_findBar);

    connect(m_findInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) {
            v->findText(text, QWebEnginePage::FindFlags(), [this](const QWebEngineFindTextResult &result) {
                if (result.numberOfMatches() > 0)
                    m_findMatchCount->setText(QStringLiteral("%1 of %2").arg(result.activeMatch()).arg(result.numberOfMatches()));
                else
                    m_findMatchCount->setText(m_findInput->text().isEmpty() ? QString() : QStringLiteral("No matches"));
            });
        }
    });
    connect(m_findInput, &QLineEdit::returnPressed, this, &BrowserWindow::findNext);
    connect(m_findNextBtn, &QToolButton::clicked, this, &BrowserWindow::findNext);
    connect(m_findPrevBtn, &QToolButton::clicked, this, &BrowserWindow::findPrevious);
    connect(m_findCloseBtn, &QToolButton::clicked, this, &BrowserWindow::hideFindBar);
}

void BrowserWindow::showFindBar() {
    if (!m_findBar) setupFindBar();
    m_findBar->setVisible(true);
    m_findInput->setFocus();
    m_findInput->selectAll();
}

void BrowserWindow::hideFindBar() {
    if (m_findBar) {
        m_findBar->setVisible(false);
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->findText(QString());
    }
}

void BrowserWindow::findNext() {
    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) {
        v->findText(m_findInput->text());
    }
}

void BrowserWindow::findPrevious() {
    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) {
        v->findText(m_findInput->text(), QWebEnginePage::FindBackward);
    }
}

void BrowserWindow::setupDownloads() {
    connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, [this](QWebEngineDownloadRequest *download) {
        DownloadItemInfo info;
        info.fileName = download->downloadFileName();
        info.receivedBytes = download->receivedBytes();
        info.totalBytes = download->totalBytes();
        info.state = 0;
        m_downloadsList.append(info);

        int idx = m_downloadsList.count() - 1;
        download->accept();

        connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, [this, idx, download]() {
            if (idx < m_downloadsList.count()) {
                m_downloadsList[idx].receivedBytes = download->receivedBytes();
                m_downloadsList[idx].totalBytes = download->totalBytes();
            }
        });

        connect(download, &QWebEngineDownloadRequest::isFinishedChanged, this, [this, idx, download]() {
            if (idx < m_downloadsList.count()) {
                m_downloadsList[idx].state = download->isFinished() ? 1 : 2;
                m_downloadsButton->setToolTip(QStringLiteral("Downloads (%1 completed)").arg(m_downloadsList.count()));
            }
        });

        m_downloadsButton->setToolTip(QStringLiteral("Downloading %1\u2026").arg(download->downloadFileName()));
    });

    connect(m_downloadsButton, &QToolButton::clicked, this, &BrowserWindow::showDownloadsMenu);
}

void BrowserWindow::showDownloadsMenu() {
    QMenu menu(this);
    if (m_downloadsList.isEmpty()) {
        menu.addAction(QStringLiteral("No Downloads"))->setEnabled(false);
    } else {
        for (const auto &item : m_downloadsList) {
            QString status = item.state == 1 ? QStringLiteral("Completed") : (item.state == 2 ? QStringLiteral("Failed") : QStringLiteral("%1 MB").arg(item.receivedBytes / (1024 * 1024)));
            menu.addAction(QStringLiteral("%1 — %2").arg(item.fileName, status));
        }
    }
    menu.exec(m_downloadsButton->mapToGlobal(QPoint(0, m_downloadsButton->height())));
}

void BrowserWindow::saveSession() {
    QString configPath = QDir::currentPath() + QStringLiteral("/session.json");
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray array;
        for (const TabInfo &tab : m_tabs) {
            if (!tab.url.isEmpty() && !tab.url.startsWith(QStringLiteral("qrc:")))
                array.append(tab.url);
        }
        QJsonObject obj;
        obj[QStringLiteral("tabs")] = array;
        obj[QStringLiteral("currentIndex")] = m_currentTabIndex;
        file.write(QJsonDocument(obj).toJson());
    }
}

void BrowserWindow::restoreSession() {
    QString configPath = QDir::currentPath() + QStringLiteral("/session.json");
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            QJsonArray array = doc.object()[QStringLiteral("tabs")].toArray();
            int restoreIdx = doc.object()[QStringLiteral("currentIndex")].toInt(0);
            if (!array.isEmpty()) {
                // Clear initial default start tab
                while (!m_tabs.isEmpty()) {
                    closeTab(0);
                }
                for (const QJsonValue &v : array) {
                    addNewTab(QUrl(v.toString()));
                }
                if (restoreIdx >= 0 && restoreIdx < m_tabs.count())
                    setCurrentTab(restoreIdx);
            }
        }
    }
}

void BrowserWindow::saveHistoryItem(const QString &title, const QString &url) {
    if (url.isEmpty() || url.startsWith(QStringLiteral("qrc:"))) return;
    QString historyPath = QDir::currentPath() + QStringLiteral("/history.json");
    QJsonArray historyArray;
    QFile readFile(historyPath);
    if (readFile.open(QIODevice::ReadOnly)) {
        historyArray = QJsonDocument::fromJson(readFile.readAll()).array();
        readFile.close();
    }
    QJsonObject item;
    item[QStringLiteral("title")] = title.isEmpty() ? shortUrl(url) : title;
    item[QStringLiteral("url")] = url;
    item[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    historyArray.prepend(item);

    // Keep max 500 history entries
    while (historyArray.count() > 500) historyArray.removeLast();

    QFile writeFile(historyPath);
    if (writeFile.open(QIODevice::WriteOnly)) {
        writeFile.write(QJsonDocument(historyArray).toJson());
    }
}

void BrowserWindow::saveBookmark(const QString &title, const QString &url) {
    if (url.isEmpty() || url.startsWith(QStringLiteral("qrc:"))) return;
    QString bookmarkPath = QDir::currentPath() + QStringLiteral("/bookmarks.json");
    QJsonArray bookmarkArray;
    QFile readFile(bookmarkPath);
    if (readFile.open(QIODevice::ReadOnly)) {
        bookmarkArray = QJsonDocument::fromJson(readFile.readAll()).array();
        readFile.close();
    }
    QJsonObject item;
    item[QStringLiteral("title")] = title.isEmpty() ? shortUrl(url) : title;
    item[QStringLiteral("url")] = url;
    bookmarkArray.append(item);

    QFile writeFile(bookmarkPath);
    if (writeFile.open(QIODevice::WriteOnly)) {
        writeFile.write(QJsonDocument(bookmarkArray).toJson());
    }
}
