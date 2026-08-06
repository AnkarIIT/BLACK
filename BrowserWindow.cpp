#include "BrowserWindow.h"
#include "SafariWebView.h"
#include "SafariTheme.h"
#include "TrackerBlocker.h"
#include "BrowserSettings.h"
#include <QFrame>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
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
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineDownloadRequest>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineFindTextResult>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QProgressBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QStandardPaths>
#include <QActionGroup>
#include <QDesktopServices>
#include <QWebChannel>
#include <QMessageBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <functional>
#include <utility>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

// ── Safari Color Palette (dynamic, follows system theme) ─────────────────────
static QString bgWindow()      { return SafariTheme::instance().bgWindow; }
static QString bgToolbar()     { return SafariTheme::instance().bgToolbar; }
static QString bgTabBar()      { return SafariTheme::instance().bgTabBar; }
static QString bgUrlBar()      { return SafariTheme::instance().bgUrlBar; }
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
static QString borderLight()   { return SafariTheme::instance().borderLight; }
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
static const QString svgSettings    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"3\"/><path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z\"/></svg>";
static const QString svgStar        = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2\"/></svg>";
static const QString svgUser        = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2\"/><circle cx=\"12\" cy=\"7\" r=\"4\"/></svg>";
static const QString svgBriefcase   = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"2\" y=\"7\" width=\"20\" height=\"14\" rx=\"2\"/><path d=\"M16 21V5a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16\"/></svg>";
static const QString svgExtensions  = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M14 7l-3 3-4-1-2 2 4 1-1 4 2 2 1-4 3 3z\"/></svg>";
static const QString svgFlag        = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z\"/><line x1=\"4\" y1=\"22\" x2=\"4\" y2=\"15\"/></svg>";
static const QString svgVolume2     = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"11 5 6 9 2 9 2 15 6 15 11 19 11 5\"/><path d=\"M19.07 4.93a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07\"/></svg>";
static const QString svgVolumeMute = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"11 5 6 9 2 9 2 15 6 15 11 19 11 5\"/><line x1=\"23\" y1=\"9\" x2=\"17\" y2=\"15\"/><line x1=\"17\" y1=\"9\" x2=\"23\" y2=\"15\"/></svg>";

// ── Page Theme (applied to web pages so they match the browser) ─────────────
static const char *kPageThemeCss = "html.black-dark{color-scheme:dark;}html.black-dark{filter:invert(0.88) hue-rotate(180deg) contrast(0.92);}html.black-dark img,html.black-dark video,html.black-dark iframe,html.black-dark canvas,html.black-dark embed,html.black-dark object{filter:invert(1) hue-rotate(180deg) contrast(0.92);}html.black-light{color-scheme:light;}";

static const QString kPageThemeStyleScript = QStringLiteral(
    "(function(){var css=%1;var s=document.createElement('style');s.id='black-theme-style';s.textContent=css;"
    "(document.head||document.documentElement).appendChild(s);})();").arg(
        QLatin1Char('"') + QString::fromLatin1(kPageThemeCss).replace(QLatin1Char('"'), QStringLiteral("\\\"")) + QLatin1Char('"'));

static const QString kPageThemeClassScript = QStringLiteral(
    "(function(dark){var el=document.documentElement;if(!el)return;"
    "el.classList.remove('black-dark','black-light');"
    "el.classList.add(dark?'black-dark':'black-light');})(%1);");

// ── Helpers ─────────────────────────────────────────────────────────────────
static QString truncate(const QString &s, int max = 20) {
    return s.length() > max ? s.left(max - 1) + QStringLiteral("\u2026") : s;
}

static QString shortUrl(const QString &url) {
    QString display = url;
    if (display.startsWith(QStringLiteral("qrc:/startpage.html"))) return QStringLiteral("Start Page");
    if (display.startsWith(QStringLiteral("qrc:/settings.html"))) return QStringLiteral("Settings");
    if (display.startsWith(QStringLiteral("qrc:/privacyreport.html"))) return QStringLiteral("Privacy Report");
    if (display.startsWith(QStringLiteral("qrc:/extensions.html"))) return QStringLiteral("Extensions");
    if (display.startsWith(QStringLiteral("qrc:/features.html"))) return QStringLiteral("Features");
    if (display.startsWith(QStringLiteral("qrc:/"))) return QStringLiteral("Start Page");
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

static QString dataFile(const QString &fileName) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + fileName;
}

// =============================================================================
BrowserWindow::BrowserWindow(bool incognito, QWidget *parent)
    : QMainWindow(parent)
    , m_tabStack(new QStackedWidget(this))
    , m_urlBar(new QLineEdit(this))
    , m_urlContainer(nullptr)
    , m_urlAnim(nullptr)
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
    , m_settingsButton(new QToolButton(this))
    , m_settingsDialog(nullptr)
    , m_settingsView(nullptr)
    , m_loadingBar(nullptr)
    , m_overviewOverlay(nullptr)
    , m_overviewTitle(nullptr)
    , m_overviewSearch(nullptr)
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
    , m_incognito(incognito)
    , m_profile(incognito ? new QWebEngineProfile(this) : webProfile())
    , m_webChannel(nullptr)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(800, 500);
    setFont(QFont("SF Pro Display", 13));

    m_webChannel = new QWebChannel(this);
    m_webChannel->registerObject(QStringLiteral("privacy"), &TrackerBlocker::instance());
    m_webChannel->registerObject(QStringLiteral("theme"), &SafariTheme::instance());
    m_webChannel->registerObject(QStringLiteral("browserSettings"), &BrowserSettings::instance());

    // Inject the class-based theme stylesheet into every page of this profile.
    QWebEngineScript styleScript;
    styleScript.setName(QStringLiteral("black-page-theme"));
    styleScript.setSourceCode(kPageThemeStyleScript);
    styleScript.setInjectionPoint(QWebEngineScript::DocumentReady);
    styleScript.setWorldId(QWebEngineScript::ApplicationWorld);
    styleScript.setRunsOnSubFrames(true);
    m_profile->scripts()->insert(styleScript);

    if (m_incognito) {
        QWebEngineSettings *s = m_profile->settings();
        s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
        s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
        s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
        s->setAttribute(QWebEngineSettings::PluginsEnabled, false);
        m_profile->setHttpUserAgent(QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.6261.167 Safari/537.36 BLACK/1.0"));
    }

    setupUi();
    setupTabBar();
    setupSidebar();
    setupFindBar();
    setupDownloads();
    setupTabOverlay();
    setupKeyboardShortcuts();
    applyTheme();

    if (!m_incognito)
        restoreSession();
    if (m_tabs.isEmpty()) {
        addNewTab(QUrl(QStringLiteral("qrc:/startpage.html")));
    }

    if (QCoreApplication::arguments().contains(QStringLiteral("--sidebar")))
        toggleSidebar();

    if (QCoreApplication::arguments().contains(QStringLiteral("--debug-focus"))) {
        QTimer::singleShot(5000, this, [this]() {
            m_urlBar->setFocus();
            m_urlBar->selectAll();
        });
        QTimer::singleShot(7000, this, [this]() {
            QFile f(QStringLiteral("C:/Users/ankar/AppData/Local/Temp/opencode/urlw.txt"));
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(QByteArray("focus_in=").append(QByteArray::number(m_urlContainer->width())));
                f.close();
            }
            grab().save(QStringLiteral("C:/Users/ankar/AppData/Local/Temp/opencode/window_focus_in.png"));
        });
        QTimer::singleShot(9000, this, [this]() { m_toolbar->setFocus(); });
        QTimer::singleShot(11000, this, [this]() {
            QFile f(QStringLiteral("C:/Users/ankar/AppData/Local/Temp/opencode/urlw.txt"));
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(QByteArray("focus_out=").append(QByteArray::number(m_urlContainer->width())));
                f.close();
            }
            grab().save(QStringLiteral("C:/Users/ankar/AppData/Local/Temp/opencode/window_focus_out.png"));
        });
    }

    connect(m_urlBar, &QLineEdit::returnPressed, this, &BrowserWindow::navigateToUrl);

    connect(&SafariTheme::instance(), &SafariTheme::schemeChanged, this, [this]() {
        applyTheme();
    });
}

BrowserWindow::~BrowserWindow() {
    if (!m_incognito)
        saveSession();
}

QWebEngineProfile *BrowserWindow::webProfile()
{
    static QWebEngineProfile *profile = []() {
        return new QWebEngineProfile(QStringLiteral("BLACK"), nullptr);
    }();
    return profile;
}

// ── Public Page Loading Methods ─────────────────────────────────────────────
void BrowserWindow::loadStartPage() {
    if (!m_tabs.isEmpty() && m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) {
        m_tabs[m_currentTabIndex].view->setUrl(QUrl(QStringLiteral("qrc:/startpage_enhanced.html")));
    } else {
        addNewTab(QUrl(QStringLiteral("qrc:/startpage_enhanced.html")));
    }
}

void BrowserWindow::loadLoginPage() {
    if (!m_tabs.isEmpty() && m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) {
        m_tabs[m_currentTabIndex].view->setUrl(QUrl(QStringLiteral("qrc:/login.html")));
    } else {
        addNewTab(QUrl(QStringLiteral("qrc:/login.html")));
    }
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

bool BrowserWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#if defined(Q_OS_WIN)
    if (eventType == QByteArrayLiteral("windows_generic_MSG")
        || eventType == QByteArrayLiteral("windows_dispatcher_MSG")) {
        MSG *msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
            const int globalX = static_cast<short>(LOWORD(msg->lParam));
            const int globalY = static_cast<short>(HIWORD(msg->lParam));
            const qreal dpr = devicePixelRatioF();
            const QPoint local = mapFromGlobal(
                QPoint(qRound(globalX / dpr), qRound(globalY / dpr)));
            const int margin = 8;
            const bool left   = local.x() < margin;
            const bool right  = local.x() > width() - margin;
            const bool top    = local.y() < margin;
            const bool bottom = local.y() > height() - margin;
            if (top && left)      *result = HTTOPLEFT;
            else if (top && right) *result = HTTOPRIGHT;
            else if (bottom && left) *result = HTBOTTOMLEFT;
            else if (bottom && right) *result = HTBOTTOMRIGHT;
            else if (top)          *result = HTTOP;
            else if (bottom)       *result = HTBOTTOM;
            else if (left)         *result = HTLEFT;
            else if (right)        *result = HTRIGHT;
            else return QMainWindow::nativeEvent(eventType, message, result);
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return QMainWindow::nativeEvent(eventType, message, result);
#endif
}

void BrowserWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 32) {
        QWidget *child = childAt(event->pos());
        if (child == m_closeButton || child == m_minimizeButton || child == m_maximizeButton) {
            event->accept();
            return;
        }
        maximizeWindow();
        event->accept();
    }
}

void BrowserWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_overviewOverlay) m_overviewOverlay->setGeometry(rect());
    if (m_sidebarVisible && m_sidebarLayout) {
        m_sidebarLayout->invalidate();
        m_sidebarLayout->activate();
    }
}

void BrowserWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ThemeChange) {
        SafariTheme::instance().refreshScheme();
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
    m_toolbar->setFixedHeight(44);

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

    // Reload now lives inside the address bar (right edge).

    // Center the address bar with flexible space on both sides
    toolbarLayout->addStretch(1);

    // URL Bar (container with shield icon + borderless line edit)
    m_urlBar->setPlaceholderText(QStringLiteral("Search or enter website name"));
    m_urlBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_urlBar->setMinimumHeight(30);
    m_urlBar->setMaximumHeight(30);
    m_urlBar->setMinimumWidth(200);
    m_urlBar->installEventFilter(this);

    m_urlContainer = new QFrame(m_toolbar);
    m_urlContainer->setObjectName(QStringLiteral("UrlContainer"));
    m_urlContainer->setMinimumWidth(320);
    m_urlContainer->setMaximumWidth(520);
    m_urlContainer->setFixedHeight(30);
    m_urlContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *urlLayout = new QHBoxLayout(m_urlContainer);
    urlLayout->setContentsMargins(8, 0, 4, 0);
    urlLayout->setSpacing(4);

    m_shieldInside = new QToolButton(m_urlContainer);
    m_shieldInside->setFixedSize(18, 18);
    m_shieldInside->setToolTip(QStringLiteral("Privacy Report"));
    urlLayout->addWidget(m_shieldInside);

    urlLayout->addWidget(m_urlBar, 1);

    m_reloadButton->setFixedSize(22, 22);
    m_reloadButton->setToolTip(QStringLiteral("Reload"));
    urlLayout->addWidget(m_reloadButton);

    toolbarLayout->addWidget(m_urlContainer, 2);
    toolbarLayout->setAlignment(m_urlContainer, Qt::AlignVCenter);
    toolbarLayout->addStretch(1);

    // Right side buttons
    m_shareButton->setToolTip(QStringLiteral("Share"));
    connect(m_shareButton, &QToolButton::clicked, this, &BrowserWindow::shareAction);
    toolbarLayout->addWidget(m_shareButton);

    m_downloadsButton->setToolTip(QStringLiteral("Downloads"));
    toolbarLayout->addWidget(m_downloadsButton);

    m_tabOverviewButton->setToolTip(QStringLiteral("Tab Overview"));
    connect(m_tabOverviewButton, &QToolButton::clicked, this, &BrowserWindow::toggleTabOverview);
    toolbarLayout->addWidget(m_tabOverviewButton);

    // Native Safari keeps Settings out of the toolbar (Cmd/Ctrl+, instead).

    rootLayout->addWidget(m_toolbar);

    // ── Loading Bar ────────────────────────────────────────────────────────
    m_loadingBar = new QProgressBar(m_central);
    m_loadingBar->setObjectName(QStringLiteral("LoadingBar"));
    m_loadingBar->setFixedHeight(3);
    m_loadingBar->setRange(0, 100);
    m_loadingBar->setValue(0);
    m_loadingBar->setTextVisible(false);
    m_loadingBar->setVisible(false);
    rootLayout->addWidget(m_loadingBar);

    // ── Content area (sidebar + tab stack) ─────────────────────────────────
    m_sidebar = new QFrame(m_central);
    m_sidebar->setObjectName(QStringLiteral("Sidebar"));
    m_sidebar->setFixedWidth(260);
    m_sidebar->setVisible(true);

    // Floating card: inset host + rounded corners + soft shadow
    auto *sidebarShadow = new QGraphicsDropShadowEffect(m_sidebar);
    sidebarShadow->setBlurRadius(24);
    sidebarShadow->setOffset(0, 4);
    sidebarShadow->setColor(QColor(0, 0, 0, 140));
    m_sidebar->setGraphicsEffect(sidebarShadow);

    m_sidebarHost = new QWidget(m_central);
    m_sidebarHost->setObjectName(QStringLiteral("SidebarHost"));
    m_sidebarHost->setFixedWidth(276);
    m_sidebarHost->setVisible(false);

    QHBoxLayout *hostLayout = new QHBoxLayout(m_sidebarHost);
    hostLayout->setContentsMargins(8, 8, 8, 8);
    hostLayout->setSpacing(0);
    hostLayout->addWidget(m_sidebar);
    hostLayout->addStretch();

    QHBoxLayout *contentRow = new QHBoxLayout();
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);

    contentRow->addWidget(m_sidebarHost);
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
    m_tabBarLayout->setContentsMargins(8, 0, 8, 0);
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
    m_sidebarSearch->setPlaceholderText(QStringLiteral("Search tabs, bookmarks\u2026"));
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

    auto addSectionHeader = [this](QVBoxLayout *lay, const QString &title) {
        QLabel *lbl = new QLabel(title, m_sidebar);
        lay->addWidget(lbl);
        m_sidebarHeaders.append(lbl);
    };

    auto addSidebarItem = [this](QVBoxLayout *lay, const QString &svg, const QString &text, const QString &action, bool isActive = false) -> QFrame* {
        QFrame *item = new QFrame(m_sidebar);
        item->setFixedHeight(28);
        item->setCursor(Qt::PointingHandCursor);
        item->setProperty("sidebarActive", isActive);
        item->setProperty("sidebarAction", action);
        item->installEventFilter(this);

        QHBoxLayout *itemLay = new QHBoxLayout(item);
        itemLay->setContentsMargins(8, 2, 8, 2);
        itemLay->setSpacing(6);

        QLabel *iconLbl = new QLabel(item);
        iconLbl->setFixedSize(18, 18);
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setPixmap(createSvgIcon(svg, 18, isActive ? accent() : textPrimary()).pixmap(18, 18));
        itemLay->addWidget(iconLbl);

        QLabel *textLbl = new QLabel(text, item);
        itemLay->addWidget(textLbl, 1);

        lay->addWidget(item);
        m_sidebarItems.append(item);
        m_sidebarItemIcons.append(iconLbl);
        m_sidebarItemTexts.append(textLbl);
        m_sidebarItemSvg.append(svg);
        return item;
    };

    // Top section stretches; bottom section stays pinned to the bottom.
    auto *sidebarTop = new QWidget(m_sidebar);
    auto *sidebarTopLayout = new QVBoxLayout(sidebarTop);
    sidebarTopLayout->setContentsMargins(0, 0, 0, 0);
    sidebarTopLayout->setSpacing(4);

    sidebarTopLayout->addWidget(m_sidebarSearch);
    sidebarTopLayout->addSpacing(8);

    // Tab Groups
    addSectionHeader(sidebarTopLayout, QStringLiteral("Tab Groups"));
    addSidebarItem(sidebarTopLayout, svgBriefcase, QStringLiteral("Work"), QStringLiteral("group_work"));
    addSidebarItem(sidebarTopLayout, svgBriefcase, QStringLiteral("Shopping"), QStringLiteral("group_shopping"));
    addSidebarItem(sidebarTopLayout, svgBriefcase, QStringLiteral("Dev"), QStringLiteral("group_dev"));

    sidebarTopLayout->addSpacing(8);

    // Favourites
    addSectionHeader(sidebarTopLayout, QStringLiteral("Favourites"));
    addSidebarItem(sidebarTopLayout, svgStar, QStringLiteral("Favourites"), QStringLiteral("start"), true);

    // Reading List
    addSectionHeader(sidebarTopLayout, QStringLiteral("Reading List"));
    addSidebarItem(sidebarTopLayout, svgReadingList, QStringLiteral("Reading List"), QStringLiteral("reading"));

    sidebarTopLayout->addSpacing(8);

    // Recently Closed
    addSectionHeader(sidebarTopLayout, QStringLiteral("Recently Closed"));
    addSidebarItem(sidebarTopLayout, svgHistory, QStringLiteral("No recent items"), QStringLiteral("recent"));

    sidebarTopLayout->addStretch();

    auto *sidebarBottom = new QWidget(m_sidebar);
    auto *sidebarBottomLayout = new QVBoxLayout(sidebarBottom);
    sidebarBottomLayout->setContentsMargins(0, 0, 0, 0);
    sidebarBottomLayout->setSpacing(4);

    // Bottom action area: "+ New Tab Group"
    m_newGroupButton = new QPushButton(QStringLiteral("+ New Tab Group"), m_sidebar);
    m_newGroupButton->setObjectName(QStringLiteral("NewGroupBtn"));
    m_newGroupButton->setCursor(Qt::PointingHandCursor);
    m_newGroupButton->setFixedHeight(26);
    sidebarBottomLayout->addWidget(m_newGroupButton);
    connect(m_newGroupButton, &QPushButton::clicked, this,
            [this, sidebarBottomLayout, addSidebarItem]() {
        static int groupCounter = 1;
        addSidebarItem(sidebarBottomLayout, svgBriefcase,
                       QStringLiteral("New Group %1").arg(groupCounter),
                       QStringLiteral("group_new%1").arg(groupCounter));
        ++groupCounter;
        styleSidebarItems();
    });

    m_sidebarLayout->addWidget(sidebarTop, 1);
    m_sidebarLayout->addWidget(sidebarBottom, 0);

    m_activeSidebarAction = QStringLiteral("start");
}

void BrowserWindow::openSidebarAction(const QString &action)
{
    if (action == QStringLiteral("recent")) {
        if (!m_closedTabs.isEmpty())
            addNewTab(m_closedTabs.takeLast());
        setSidebarActive(action);
        return;
    }
    if (action.startsWith(QStringLiteral("group"))) {
        navigateCurrentTo(QUrl(QStringLiteral("qrc:/startpage.html")));
        setSidebarActive(action);
        return;
    }
    if (action == QStringLiteral("reading")) {
        navigateCurrentTo(QUrl(QStringLiteral("qrc:/startpage.html#reading")));
    } else {
        navigateCurrentTo(QUrl(QStringLiteral("qrc:/startpage.html")));
    }
    setSidebarActive(action);
}

void BrowserWindow::navigateCurrentTo(const QUrl &url)
{
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count())
        m_tabs[m_currentTabIndex].view->setUrl(url);
    else
        addNewTab(url);
}

void BrowserWindow::setSidebarActive(const QString &action)
{
    m_activeSidebarAction = action;
    for (int i = 0; i < m_sidebarItems.count(); ++i) {
        bool isActive = (m_sidebarItems[i]->property("sidebarAction").toString() == action);
        m_sidebarItems[i]->setProperty("sidebarActive", isActive);
    }
    styleSidebarItems();
}

QLabel* BrowserWindow::sidebarItemTextForAction(const QString &action)
{
    for (int i = 0; i < m_sidebarItems.count(); ++i) {
        if (m_sidebarItems[i]->property("sidebarAction").toString() == action)
            return m_sidebarItemTexts.value(i);
    }
    return nullptr;
}

void BrowserWindow::styleSidebarItems()
{
    for (int i = 0; i < m_sidebarItems.count(); ++i) {
        const bool isActive = m_sidebarItems[i]->property("sidebarActive").toBool();
        m_sidebarItems[i]->setStyleSheet(QString(
            "QFrame { background-color: %1; border-radius: 6px; }"
            "QFrame:hover { background-color: %2; }"
        ).arg(isActive ? selectedBg() : QStringLiteral("transparent"), hover()));
        if (i < m_sidebarItemSvg.count() && i < m_sidebarItemIcons.count()) {
            m_sidebarItemIcons[i]->setPixmap(
                createSvgIcon(m_sidebarItemSvg[i], 18, isActive ? accent() : textPrimary()).pixmap(18, 18));
        }
        m_sidebarItemTexts[i]->setStyleSheet(QString(
            "font-size: 13px; color: %1; font-weight: %2; background: transparent;"
        ).arg(isActive ? accent() : textPrimary(), isActive ? "600" : "400"));
    }
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

    // Search tabs field
    m_overviewSearch = new QLineEdit(m_overviewPanel);
    m_overviewSearch->setPlaceholderText(QStringLiteral("Search Tabs"));
    panelLayout->addWidget(m_overviewSearch);
    connect(m_overviewSearch, &QLineEdit::textChanged, this, &BrowserWindow::filterOverviewGrid);

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
    // Only capture the visible tab synchronously; hidden tabs keep their last
    // thumbnail (or show a favicon), so opening the overview never freezes.
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count() && m_tabs[m_currentTabIndex].view) {
        m_tabs[m_currentTabIndex].thumbnail = m_tabs[m_currentTabIndex].view->grab()
            .scaled(240, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    m_overviewVisible = true;
    if (m_overviewSearch)
        m_overviewSearch->clear();
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
    filterOverviewGrid(m_overviewSearch ? m_overviewSearch->text() : QString());
}

void BrowserWindow::filterOverviewGrid(const QString &query)
{
    const QString q = query.trimmed().toLower();
    for (int i = 0; i < m_overviewGridLayout->count(); ++i) {
        QWidget *card = m_overviewGridLayout->itemAt(i)->widget();
        if (!card) continue;
        bool visible = true;
        if (!q.isEmpty()) {
            const int idx = card->property("tabIndex").toInt();
            if (idx >= 0 && idx < m_tabs.count()) {
                const TabInfo &tab = m_tabs[idx];
                const QString title = tab.title.isEmpty() ? QStringLiteral("New Tab") : tab.title;
                visible = title.toLower().contains(q) || shortUrl(tab.url).toLower().contains(q);
            }
        }
        card->setVisible(visible);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Tab Management ────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::addNewTab(const QUrl &url) {
    addTabView(url, nullptr);
}

SafariWebView* BrowserWindow::addTabView(const QUrl &url, QWebEngineNewWindowRequest *request) {
    auto *view = new SafariWebView(this);
    view->setWebProfile(m_profile);
    view->page()->setWebChannel(m_webChannel);
    view->page()->setBackgroundColor(QColor(SafariTheme::instance().pageBackground));
    m_tabStack->addWidget(view);

    TabInfo info;
    info.view  = view;
    info.title = QStringLiteral("New Tab");
    info.url   = request ? request->requestedUrl().toString() : url.toString();
    m_tabs.append(info);

    int index = m_tabs.count() - 1;

    // FIX: Lambda captures QPointer to the view, finds tab by view pointer
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &t) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].title = t;
                if (i == m_currentTabIndex) setWindowTitleFromTab();
                refreshTabLabel(i);
                break;
            }
        }
    });

    connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].icon = icon;
                refreshTabLabel(i);
                break;
            }
        }
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
    connect(view->page(), &QWebEnginePage::recentlyAudibleChanged, this, [this, view](bool audible) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].isAudible = audible;
                rebuildTabBar();
                break;
            }
        }
    });
    connect(view->page(), &QWebEnginePage::audioMutedChanged, this, [this, view](bool muted) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].isMuted = muted;
                rebuildTabBar();
                break;
            }
        }
    });
    connect(view, &QWebEngineView::loadProgress, this, &BrowserWindow::onLoadProgress);
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == view) {
                m_tabs[i].loading = false;
                if (ok) {
                    saveHistoryItem(m_tabs[i].title, m_tabs[i].url);
                    // Refresh the visible tab's thumbnail so the overview stays fresh.
                    if (i == m_currentTabIndex && !m_overviewVisible) {
                        m_tabs[i].thumbnail = view->grab().scaled(
                            240, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    }
                }
                break;
            }
        }
        if (ok && !view->url().scheme().startsWith(QStringLiteral("qrc"))) {
            const bool dark = (SafariTheme::instance().scheme() == SafariTheme::Scheme::Dark);
            view->page()->runJavaScript(kPageThemeClassScript.arg(dark ? QStringLiteral("true") : QStringLiteral("false")));
        }
        onLoadFinished(ok);
    });
    connect(view, &QWebEngineView::loadFinished, this, &BrowserWindow::updateNavigationState);

    connect(view, &SafariWebView::newTabRequested, this, &BrowserWindow::addNewTab);

    // Open target="_blank", window.open() and other new-window requests as tabs.
    connect(view->page(), &QWebEnginePage::newWindowRequested, this, [this](QWebEngineNewWindowRequest &req) {
        addTabView(QUrl(), &req);
    });

    // SSL certificate errors: let the user decide on overridable errors.
    connect(view->page(), &QWebEnginePage::certificateError, this, [this](QWebEngineCertificateError error) {
        handleCertificateError(std::move(error));
    });

    // Camera / microphone / location / notification permission prompts.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(view->page(), &QWebEnginePage::permissionRequested, this, [this](QWebEnginePermission permission) {
        handlePermissionRequest(std::move(permission));
    });
#else
    connect(view->page(), &QWebEnginePage::featurePermissionRequested, this, [this, page = view->page()](const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
        handlePermissionRequestOld(page, securityOrigin, feature);
    });
#endif

    if (request) {
        request->openIn(view->page());
    } else if (url.isValid() && !url.isEmpty()) {
        view->setUrl(url);
    } else {
        view->setUrl(QUrl(QStringLiteral("qrc:/startpage.html")));
    }
    setCurrentTab(index);
    return view;
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
        QLabel *recentLbl = sidebarItemTextForAction(QStringLiteral("recent"));
        if (recentLbl)
            recentLbl->setText(truncate(shortUrl(closedUrl.toString()), 22));
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

void BrowserWindow::togglePinTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;

    QWebEngineView *activeView = (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) ? m_tabs[m_currentTabIndex].view : nullptr;

    m_tabs[index].isPinned = !m_tabs[index].isPinned;

    QList<TabInfo> pinned;
    QList<TabInfo> unpinned;
    for (const TabInfo &tab : m_tabs) {
        if (tab.isPinned) {
            pinned.append(tab);
        } else {
            unpinned.append(tab);
        }
    }

    m_tabs = pinned + unpinned;

    if (activeView) {
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs[i].view == activeView) {
                m_currentTabIndex = i;
                break;
            }
        }
    }

    rebuildTabBar();
    if (m_overviewVisible) rebuildOverviewGrid();
}

void BrowserWindow::toggleMuteTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;
    QWebEngineView *v = m_tabs[index].view;
    if (v) {
        bool mute = !v->page()->isAudioMuted();
        v->page()->setAudioMuted(mute);
        m_tabs[index].isMuted = mute;
        rebuildTabBar();
    }
}

void BrowserWindow::addTabAction() {
    addNewTab(QUrl(QStringLiteral("qrc:/startpage.html")));
}

void BrowserWindow::openPrivateWindow() {
    auto *window = new BrowserWindow(true);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->resize(size());
    window->show();
}

void BrowserWindow::updateWebViewTheme() {
    const bool dark = (SafariTheme::instance().scheme() == SafariTheme::Scheme::Dark);
    const QString js = kPageThemeClassScript.arg(dark ? QStringLiteral("true") : QStringLiteral("false"));
    for (const TabInfo &tab : m_tabs) {
        if (!tab.view)
            continue;
        if (tab.url.startsWith(QStringLiteral("qrc:")))
            continue;
        tab.view->page()->runJavaScript(js);
    }
}

void BrowserWindow::showSettingsMenu() {
    QMenu menu(this);
    menu.setStyleSheet(QString(
        "QMenu { background-color: %1; color: %2; border: 0.5px solid %3; "
        "border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 28px 6px 12px; border-radius: 5px; font-size: 13px; }"
        "QMenu::item:selected { background-color: %4; }"
        "QMenu::item:checked { color: %5; }"
        "QMenu::separator { height: 1px; background: %6; margin: 4px 8px; }"
    ).arg(bgUrlBar(), textPrimary(), border(), selectedBg(), accent(), border()));

    auto addThemeAction = [&menu, this](const QString &label, SafariTheme::Preference preference) {
        QAction *action = menu.addAction(label);
        action->setCheckable(true);
        action->setChecked(SafariTheme::instance().preference() == preference);
        connect(action, &QAction::triggered, this, [preference]() {
            SafariTheme::instance().setPreference(preference);
        });
        return action;
    };

    QActionGroup *group = new QActionGroup(&menu);
    group->setExclusive(true);
    group->addAction(addThemeAction(QStringLiteral("Theme: System"), SafariTheme::Preference::System));
    group->addAction(addThemeAction(QStringLiteral("Theme: Light"), SafariTheme::Preference::Light));
    group->addAction(addThemeAction(QStringLiteral("Theme: Dark"), SafariTheme::Preference::Dark));

    menu.addSeparator();

    QAction *openSettings = menu.addAction(QStringLiteral("Settings\u2026"));
    connect(openSettings, &QAction::triggered, this, &BrowserWindow::openSettingsDialog);

    QAction *privateWindow = menu.addAction(QStringLiteral("New Private Window"));
    connect(privateWindow, &QAction::triggered, this, &BrowserWindow::openPrivateWindow);

    QAction *downloads = menu.addAction(QStringLiteral("Downloads"));
    connect(downloads, &QAction::triggered, this, &BrowserWindow::showDownloadsMenu);

    menu.exec(mapToGlobal(QPoint(width() - 250, m_toolbar->height())));
}

void BrowserWindow::openSettingsDialog()
{
    if (m_settingsDialog && m_settingsDialog->isVisible()) {
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
        return;
    }

    if (!m_settingsDialog) {
        m_settingsDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
        m_settingsDialog->setWindowTitle(QStringLiteral("Settings"));
        m_settingsDialog->setFixedSize(760, 560);
        m_settingsDialog->setStyleSheet(QString(
            "QDialog { background-color: %1; color: %2; }"
        ).arg(bgWindow(), textPrimary()));

        QVBoxLayout *layout = new QVBoxLayout(m_settingsDialog);
        layout->setContentsMargins(0, 0, 0, 0);

        m_settingsView = new SafariWebView(m_settingsDialog);
        m_settingsView->page()->setWebChannel(m_webChannel);
        m_settingsView->setUrl(QUrl(QStringLiteral("qrc:/settings.html")));
        layout->addWidget(m_settingsView);
    }

    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void BrowserWindow::handleCertificateError(QWebEngineCertificateError certificateError) {
    if (!certificateError.isOverridable()) {
        certificateError.rejectCertificate();
        return;
    }

    QMessageBox box(QMessageBox::Warning,
                    QStringLiteral("Security Warning"),
                    QStringLiteral("BLACK cannot verify the identity of this website."),
                    QMessageBox::NoButton,
                    this);
    box.setInformativeText(certificateError.url().toString() + QStringLiteral("\n\n")
                           + certificateError.description()
                           + QStringLiteral("\n\nProceeding anyway could allow someone to intercept the data you send."));
    QPushButton *proceedButton = box.addButton(QStringLiteral("Proceed Anyway"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == proceedButton)
        certificateError.acceptCertificate();
    else
        certificateError.rejectCertificate();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
QString BrowserWindow::permissionDisplayName(QWebEnginePermission::PermissionType type) {
    switch (type) {
    case QWebEnginePermission::PermissionType::Notifications:      return QStringLiteral("notifications");
    case QWebEnginePermission::PermissionType::Geolocation:        return QStringLiteral("your location");
    case QWebEnginePermission::PermissionType::MediaAudioCapture:  return QStringLiteral("your microphone");
    case QWebEnginePermission::PermissionType::MediaVideoCapture:  return QStringLiteral("your camera");
    case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
        return QStringLiteral("your camera and microphone");
    case QWebEnginePermission::PermissionType::MouseLock:          return QStringLiteral("pointer lock");
    case QWebEnginePermission::PermissionType::DesktopVideoCapture: return QStringLiteral("your screen");
    case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
        return QStringLiteral("your screen and audio");
    case QWebEnginePermission::PermissionType::ClipboardReadWrite: return QStringLiteral("clipboard access");
    case QWebEnginePermission::PermissionType::LocalFontsAccess:   return QStringLiteral("fonts installed on this device");
    default: return QStringLiteral("a restricted feature");
    }
}

void BrowserWindow::handlePermissionRequest(QWebEnginePermission permission) {
    if (!permission.isValid())
        return;

    const QString key = permission.origin().toString() + QLatin1Char('|')
                        + QString::number(static_cast<int>(permission.permissionType()));
    const auto remembered = m_permissionChoices.constFind(key);
    if (remembered != m_permissionChoices.constEnd()) {
        if (remembered.value())
            permission.grant();
        else
            permission.deny();
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Permission Request"));
    QVBoxLayout *lay = new QVBoxLayout(&dialog);
    lay->setSpacing(12);

    QLabel *prompt = new QLabel(QStringLiteral("Allow <b>%1</b> to use %2?")
                                    .arg(permission.origin().host(),
                                         permissionDisplayName(permission.permissionType())),
                                &dialog);
    prompt->setWordWrap(true);
    lay->addWidget(prompt);

    QCheckBox *remember = new QCheckBox(QStringLiteral("Remember my decision for this website"), &dialog);
    lay->addWidget(remember);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    QPushButton *allowButton = buttons->addButton(QStringLiteral("Allow"), QDialogButtonBox::AcceptRole);
    lay->addWidget(buttons);

    connect(allowButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const bool allowed = (dialog.exec() == QDialog::Accepted);
    if (remember->isChecked())
        m_permissionChoices.insert(key, allowed);

    if (allowed)
        permission.grant();
    else
        permission.deny();
}
#else
QString BrowserWindow::permissionDisplayNameOld(QWebEnginePage::Feature feature) {
    switch (feature) {
    case QWebEnginePage::Notifications:          return QStringLiteral("notifications");
    case QWebEnginePage::Geolocation:            return QStringLiteral("your location");
    case QWebEnginePage::MediaAudioCapture:      return QStringLiteral("your microphone");
    case QWebEnginePage::MediaVideoCapture:      return QStringLiteral("your camera");
    case QWebEnginePage::MediaAudioVideoCapture: return QStringLiteral("your camera and microphone");
    case QWebEnginePage::MouseLock:              return QStringLiteral("pointer lock");
    case QWebEnginePage::DesktopVideoCapture:     return QStringLiteral("your screen");
    case QWebEnginePage::DesktopAudioVideoCapture: return QStringLiteral("your screen and audio");
    default: return QStringLiteral("a restricted feature");
    }
}

void BrowserWindow::handlePermissionRequestOld(QWebEnginePage *page, const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
    if (!page)
        return;

    const QString key = securityOrigin.toString() + QLatin1Char('|')
                        + QString::number(static_cast<int>(feature));
    const auto remembered = m_permissionChoices.constFind(key);
    if (remembered != m_permissionChoices.constEnd()) {
        page->setFeaturePermission(securityOrigin, feature,
            remembered.value() ? QWebEnginePage::PermissionGrantedByUser : QWebEnginePage::PermissionDeniedByUser);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Permission Request"));
    QVBoxLayout *lay = new QVBoxLayout(&dialog);
    lay->setSpacing(12);

    QLabel *prompt = new QLabel(QStringLiteral("Allow <b>%1</b> to use %2?")
                                    .arg(securityOrigin.host(),
                                         permissionDisplayNameOld(feature)),
                                &dialog);
    prompt->setWordWrap(true);
    lay->addWidget(prompt);

    QCheckBox *remember = new QCheckBox(QStringLiteral("Remember my decision for this website"), &dialog);
    lay->addWidget(remember);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    QPushButton *allowButton = buttons->addButton(QStringLiteral("Allow"), QDialogButtonBox::AcceptRole);
    lay->addWidget(buttons);

    connect(allowButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const bool allowed = (dialog.exec() == QDialog::Accepted);
    if (remember->isChecked())
        m_permissionChoices.insert(key, allowed);

    page->setFeaturePermission(securityOrigin, feature,
        allowed ? QWebEnginePage::PermissionGrantedByUser : QWebEnginePage::PermissionDeniedByUser);
}
#endif

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
    m_tabItemIcons.clear();
    m_tabItemTexts.clear();

    // Rebuild: tabs left-aligned, add-tab button right after the last tab
    for (int i = 0; i < m_tabs.count(); ++i) {
        bool isActive = (i == m_currentTabIndex);
        const TabInfo &tab = m_tabs[i];

        auto *tabWidget = new QWidget(m_tabBar);
        tabWidget->setFixedHeight(28);
        tabWidget->setCursor(Qt::PointingHandCursor);
        tabWidget->setProperty("tabIndex", i);

        if (tab.isPinned) {
            tabWidget->setFixedSize(32, 28);
            tabWidget->setStyleSheet(QString(
                "QWidget { background-color: %1; border-radius: 7px; border: none; }"
                "QWidget:hover { background-color: %2; }"
            ).arg(isActive ? tabActive() : tabInactive(),
                 isActive ? tabActive() : tabHover()));

            QGridLayout *gridLayout = new QGridLayout(tabWidget);
            gridLayout->setContentsMargins(4, 4, 4, 4);
            gridLayout->setSpacing(0);

            QLabel *iconLabel = new QLabel(tabWidget);
            QPixmap pix = tab.icon.pixmap(16, 16);
            if (!pix.isNull()) {
                iconLabel->setPixmap(pix);
            } else {
                iconLabel->setText(QStringLiteral("\U0001F310"));
                iconLabel->setStyleSheet(QString("font-size: 11px; background: transparent; color: %1;").arg(textSecondary()));
            }
            iconLabel->setFixedSize(16, 16);
            iconLabel->setAlignment(Qt::AlignCenter);
            gridLayout->addWidget(iconLabel, 0, 0, Qt::AlignCenter);

            if (tab.isAudible) {
                QPushButton *audioBtn = new QPushButton(tabWidget);
                audioBtn->setFixedSize(12, 12);
                QIcon volIcon = createSvgIcon(tab.isMuted ? svgVolumeMute : svgVolume2, 10, isActive ? accent() : textPrimary());
                audioBtn->setIcon(volIcon);
                audioBtn->setIconSize(QSize(10, 10));
                audioBtn->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; padding: 0; }"));
                connect(audioBtn, &QPushButton::clicked, this, [this, i](bool) {
                    toggleMuteTab(i);
                });
                gridLayout->addWidget(audioBtn, 0, 0, Qt::AlignTop | Qt::AlignRight);
            }

            tabWidget->installEventFilter(this);
            m_tabBarLayout->addWidget(tabWidget);
            m_tabWidgets.append(tabWidget);
            m_tabItemIcons.append(iconLabel);
            m_tabItemTexts.append(nullptr); // Null text label for pinned tabs
        } else {
            tabWidget->setMinimumWidth(80);
            tabWidget->setMaximumWidth(200);
            tabWidget->setStyleSheet(QString(
                "QWidget { background-color: %1; border-radius: 7px; border: none; }"
                "QWidget:hover { background-color: %2; }"
            ).arg(isActive ? tabActive() : tabInactive(),
                 isActive ? tabActive() : tabHover()));

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

            if (tab.isAudible) {
                QPushButton *audioBtn = new QPushButton(tabWidget);
                audioBtn->setFixedSize(16, 16);
                QIcon volIcon = createSvgIcon(tab.isMuted ? svgVolumeMute : svgVolume2, 12, textSecondary());
                audioBtn->setIcon(volIcon);
                audioBtn->setIconSize(QSize(12, 12));
                audioBtn->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; padding: 0; }"));
                connect(audioBtn, &QPushButton::clicked, this, [this, i](bool) {
                    toggleMuteTab(i);
                });
                tabLayout->addWidget(audioBtn);
            }

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
            m_tabBarLayout->addWidget(tabWidget);
            m_tabWidgets.append(tabWidget);
            m_tabItemIcons.append(iconLabel);
            m_tabItemTexts.append(titleLbl);
        }
    }

    m_tabBarLayout->addWidget(m_addTabButton);
    m_tabBarLayout->addSpacing(4);
}

void BrowserWindow::refreshTabLabel(int index)
{
    if (index < 0 || index >= m_tabItemIcons.count()) return;

    if (index < m_tabs.count()) {
        const TabInfo &tab = m_tabs[index];
        QPixmap pix = tab.icon.pixmap(14, 14);
        if (m_tabItemIcons[index]) {
            if (!pix.isNull()) {
                m_tabItemIcons[index]->setPixmap(pix);
                m_tabItemIcons[index]->setText(QString());
            } else {
                m_tabItemIcons[index]->setPixmap(QPixmap());
                m_tabItemIcons[index]->setText(QStringLiteral("\U0001F310"));
            }
        }
        if (m_tabItemTexts[index]) {
            const QString shownText = tab.title.isEmpty() ? QStringLiteral("New Tab") : tab.title;
            m_tabItemTexts[index]->setText(truncate(shownText, 18));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ── Sidebar Toggle ────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════
void BrowserWindow::toggleSidebar()
{
    m_sidebarVisible = !m_sidebarVisible;
    m_sidebarHost->setVisible(m_sidebarVisible);
    if (m_sidebarVisible && m_sidebarLayout) {
        m_sidebarLayout->invalidate();
        m_sidebarLayout->activate();
    }
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
        auto *me = static_cast<QMouseEvent*>(event);
        auto *w = qobject_cast<QWidget*>(obj);
        if (w) {
            QString action = w->property("sidebarAction").toString();
            if (!action.isEmpty()) {
                openSidebarAction(action);
                return true;
            }
            QVariant v = w->property("tabIndex");
            if (v.isValid()) {
                int idx = v.toInt();
                if (me->button() == Qt::RightButton) {
                    if (idx >= 0 && idx < m_tabs.count()) {
                        QMenu menu(this);
                        menu.setStyleSheet(QString(
                            "QMenu { background-color: %1; color: %2; border: 0.5px solid %3; "
                            "border-radius: 8px; padding: 4px; }"
                            "QMenu::item { padding: 6px 28px 6px 12px; border-radius: 5px; font-size: 13px; }"
                            "QMenu::item:selected { background-color: %4; }"
                        ).arg(bgUrlBar(), textPrimary(), border(), selectedBg()));

                        const TabInfo &tab = m_tabs[idx];
                        QAction *pinAction = menu.addAction(tab.isPinned ? QStringLiteral("Unpin Tab") : QStringLiteral("Pin Tab"));
                        connect(pinAction, &QAction::triggered, this, [this, idx]() {
                            togglePinTab(idx);
                        });

                        QAction *muteAction = menu.addAction(tab.isMuted ? QStringLiteral("Unmute Tab") : QStringLiteral("Mute Tab"));
                        connect(muteAction, &QAction::triggered, this, [this, idx]() {
                            toggleMuteTab(idx);
                        });

                        QAction *closeAction = menu.addAction(QStringLiteral("Close Tab"));
                        connect(closeAction, &QAction::triggered, this, [this, idx]() {
                            closeTab(idx);
                        });

                        menu.exec(me->globalPosition().toPoint());
                        return true;
                    }
                } else if (me->button() == Qt::LeftButton) {
                    if (idx >= 0 && idx < m_tabs.count()) {
                        if (m_overviewVisible) hideTabOverview();
                        setCurrentTab(idx);
                        return true;
                    }
                }
            }
            if (obj == m_overviewOverlay) {
                if (!m_overviewPanel->geometry().contains(me->pos()))
                    hideTabOverview();
                return true;
            }
        }
    }

    // URL bar focus ring + elastic expansion
    if (obj == m_urlContainer || obj == m_urlBar) {
        if (event->type() == QEvent::FocusIn) {
            m_urlFocused = true;
            animateUrlBar(820);
            updateUrlContainerStyle();
        } else if (event->type() == QEvent::FocusOut) {
            m_urlFocused = false;
            animateUrlBar(520);
            updateUrlContainerStyle();
        }
        return false;
    }

    return QMainWindow::eventFilter(obj, event);
}

void BrowserWindow::animateUrlBar(int targetWidth)
{
    if (m_urlAnim) {
        m_urlAnim->stop();
        m_urlAnim->deleteLater();
        m_urlAnim = nullptr;
    }

    auto *group = new QParallelAnimationGroup(this);
    group->setDirection(QAbstractAnimation::Forward);

    auto addAnim = [group, targetWidth](QWidget *w, const char *prop) {
        auto *anim = new QPropertyAnimation(w, prop, group);
        anim->setDuration(220);
        anim->setStartValue(w->property(prop));
        anim->setEndValue(targetWidth);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        group->addAnimation(anim);
    };
    addAnim(m_urlContainer, "minimumWidth");
    addAnim(m_urlContainer, "maximumWidth");

    connect(group, &QParallelAnimationGroup::finished, this, [this, group]() {
        if (m_urlAnim == group) m_urlAnim = nullptr;
        group->deleteLater();
    });

    m_urlAnim = group;
    group->start();
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
    if (m_loadingBar) {
        m_loadingBar->setValue(progress);
        m_loadingBar->setVisible(m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()
                                && m_tabs[m_currentTabIndex].loading);
    }
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
    addShortcut(QStringLiteral("Ctrl+Shift+N"), [this]() { openPrivateWindow(); });
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
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) {
            v->page()->profile()->clearHttpCache();
            v->reload();
        }
    });
    addShortcut(QStringLiteral("Ctrl+F"), [this]() { showFindBar(); });
    addShortcut(QStringLiteral("Escape"), [this]() {
        if (m_findBar && m_findBar->isVisible()) hideFindBar();
        else if (m_overviewVisible) hideTabOverview();
        else if (m_sidebarVisible) toggleSidebar();
    });
    addShortcut(QStringLiteral("Ctrl+Shift+L"), [this]() { toggleSidebar(); });
    addShortcut(QStringLiteral("Ctrl+,"), [this]() { showSettingsMenu(); });
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
    ).arg(searchBg(), m_urlFocused ? "1.5px" : "1px",
          m_urlFocused ? accent() : borderLight()));
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
    m_settingsButton->setIcon(createSvgIcon(svgSettings, 18, textPrimary()));

    for (QToolButton *b : { m_sidebarButton, m_backButton, m_forwardButton, m_reloadButton,
                            m_shareButton, m_downloadsButton, m_tabOverviewButton, m_settingsButton }) {
        b->setStyleSheet(navBtnStyle);
    }
    // Reload sits inside the address bar, so it gets a tighter hit area.
    m_reloadButton->setFixedSize(22, 22);
    m_reloadButton->setStyleSheet(QString(
        "QToolButton { border: none; background: transparent; border-radius: 5px; padding: 0; }"
        "QToolButton:hover { background-color: %1; }"
    ).arg(hover()));
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
    m_loadingBar->setStyleSheet(QString(
        "QProgressBar { background-color: transparent; border: none; border-radius: 0px; }"
        "QProgressBar::chunk { background-color: %1; }"
    ).arg(accent()));

    // Sidebar (floating card)
    m_sidebar->setStyleSheet(QString(
        "#Sidebar { background-color: %1; border: 1px solid %2; border-radius: 12px; }"
    ).arg(cardBg(), border()));
    m_sidebarHost->setStyleSheet(QStringLiteral("#SidebarHost { background: transparent; }"));
    m_sidebarSearch->setStyleSheet(QString(
        "QLineEdit { background-color: %1; border: none; border-radius: 6px; "
        "padding: 6px 10px; font-size: 12px; color: %2; }"
        "QLineEdit:focus { background-color: %3; }"
    ).arg(searchBg(), textPrimary(), hover()));

    for (QLabel *lbl : m_sidebarHeaders) {
        lbl->setStyleSheet(QString(
            "font-size: 10px; font-weight: 500; color: %1; padding: 6px 4px 2px 4px; "
            "letter-spacing: 0.2px;"
        ).arg(textTertiary()));
    }
    if (m_newGroupButton) {
        m_newGroupButton->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; color: %1; "
            "font-size: 12px; font-weight: 500; text-align: left; padding: 2px 8px; border-radius: 6px; }"
            "QPushButton:hover { background-color: %2; }"
        ).arg(textSecondary(), hover()));
    }
    styleSidebarItems();

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
    if (m_overviewScroll) {
        m_overviewScroll->setStyleSheet(QString(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }"
            "QScrollBar::handle:vertical { background: rgba(128,128,128,0.35); border-radius: 4px; min-height: 24px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(128,128,128,0.6); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        ));
    }
    m_overviewTitle->setStyleSheet(QString("font-size: 16px; font-weight: 700; color: %1;").arg(textPrimary()));
    if (m_overviewSearch) {
        m_overviewSearch->setStyleSheet(QString(
            "QLineEdit { background-color: %1; border: none; border-radius: 7px; "
            "padding: 7px 12px; font-size: 13px; color: %2; }"
            "QLineEdit:focus { background-color: %3; }"
        ).arg(searchBg(), textPrimary(), hover()));
    }
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
    updateWebViewTheme();
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
    QMenu menu(this);
    menu.setStyleSheet(QString(
        "QMenu { background-color: %1; color: %2; border: 0.5px solid %3; "
        "border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 28px 6px 12px; border-radius: 5px; font-size: 13px; }"
        "QMenu::item:selected { background-color: %4; }"
        "QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }"
    ).arg(bgUrlBar(), textPrimary(), border(), selectedBg(), border()));

    QAction *newTab = menu.addAction(QStringLiteral("New Tab"));
    connect(newTab, &QAction::triggered, this, &BrowserWindow::addTabAction);

    QAction *privateWindow = menu.addAction(QStringLiteral("New Private Window"));
    connect(privateWindow, &QAction::triggered, this, &BrowserWindow::openPrivateWindow);

    menu.addSeparator();

    QAction *reload = menu.addAction(QStringLiteral("Reload"));
    connect(reload, &QAction::triggered, this, [this]() {
        if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
            v->reload();
    });

    menu.addSeparator();

    QAction *openDownloads = menu.addAction(QStringLiteral("Downloads"));
    connect(openDownloads, &QAction::triggered, this, &BrowserWindow::showDownloadsMenu);

    menu.exec(event->globalPos());
    event->accept();
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
    connect(m_profile, &QWebEngineProfile::downloadRequested, this, [this](QWebEngineDownloadRequest *download) {
        DownloadItemInfo info;
        info.fileName = download->downloadFileName();
        info.filePath = QDir(download->downloadDirectory()).filePath(download->downloadFileName());
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

        connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, idx, download](QWebEngineDownloadRequest::DownloadState state) {
            if (idx < m_downloadsList.count()) {
                switch (state) {
                case QWebEngineDownloadRequest::DownloadCompleted:
                    m_downloadsList[idx].state = 1;
                    break;
                case QWebEngineDownloadRequest::DownloadCancelled:
                case QWebEngineDownloadRequest::DownloadInterrupted:
                    m_downloadsList[idx].state = 2;
                    break;
                default:
                    break;
                }
                int completed = 0;
                for (const DownloadItemInfo &d : m_downloadsList)
                    if (d.state == 1) ++completed;
                m_downloadsButton->setToolTip(QStringLiteral("Downloads (%1 completed)").arg(completed));
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
        for (int i = 0; i < m_downloadsList.count(); ++i) {
            const DownloadItemInfo &item = m_downloadsList[i];
            QString status;
            switch (item.state) {
            case 1: status = QStringLiteral("Completed"); break;
            case 2: status = QStringLiteral("Failed"); break;
            default:
                if (item.totalBytes > 0)
                    status = QStringLiteral("%1%").arg(int(100 * double(item.receivedBytes) / double(item.totalBytes)));
                else
                    status = QStringLiteral("%1 MB").arg(double(item.receivedBytes) / (1024.0 * 1024.0), 0, 'f', 1);
                break;
            }
            QAction *act = menu.addAction(QStringLiteral("%1 \u2014 %2").arg(item.fileName, status));
            if (item.state == 1 && !item.filePath.isEmpty()) {
                connect(act, &QAction::triggered, this, [item]() {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(item.filePath));
                });
            } else {
                act->setEnabled(false);
            }
        }
    }
    menu.exec(m_downloadsButton->mapToGlobal(QPoint(0, m_downloadsButton->height())));
}

void BrowserWindow::saveSession() {
    if (m_incognito) return;
    QFile file(dataFile(QStringLiteral("session.json")));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray array;
        int currentSavedIndex = -1;
        for (int i = 0; i < m_tabs.count(); ++i) {
            const TabInfo &tab = m_tabs[i];
            if (tab.url.isEmpty() || tab.url.startsWith(QStringLiteral("qrc:")))
                continue;
            if (i == m_currentTabIndex)
                currentSavedIndex = array.size();
            array.append(tab.url);
        }
        QJsonObject obj;
        obj[QStringLiteral("tabs")] = array;
        obj[QStringLiteral("currentIndex")] = currentSavedIndex >= 0 ? currentSavedIndex : 0;
        file.write(QJsonDocument(obj).toJson());
    }
}

void BrowserWindow::restoreSession() {
    if (m_incognito) return;
    QFile file(dataFile(QStringLiteral("session.json")));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            QJsonArray array = doc.object()[QStringLiteral("tabs")].toArray();
            int restoreIdx = doc.object()[QStringLiteral("currentIndex")].toInt(0);
            if (!array.isEmpty()) {
                // Remove any pre-existing tabs without closeTab()'s side effects.
                while (!m_tabs.isEmpty()) {
                    QWebEngineView *v = m_tabs.takeLast().view;
                    if (v) { m_tabStack->removeWidget(v); v->deleteLater(); }
                }
                m_currentTabIndex = -1;
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
    if (m_incognito || url.isEmpty() || url.startsWith(QStringLiteral("qrc:"))) return;
    QString historyPath = dataFile(QStringLiteral("history.json"));
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
    QString bookmarkPath = dataFile(QStringLiteral("bookmarks.json"));
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
