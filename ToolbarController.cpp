#include "ToolbarController.h"

#include "BrowserWindow.h"
#include "SafariTheme.h"
#include "ChromeTheme.h"
#include "ChromeLayer.h"
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QStyle>
#include <QShortcut>

namespace {
static bool chromeMode() {
    return BrowserSettings::instance().uiLayout() == BrowserSettings::ClassicChrome;
}

static ChromePalette chromePalette() {
    return SafariTheme::instance().scheme() == SafariTheme::Scheme::Dark
        ? ChromeTheme::dark()
        : ChromeTheme::light();
}

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

static const QString svgBack       = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"15 18 9 12 15 6\"/></svg>";
static const QString svgForward    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"9 18 15 12 9 6\"/></svg>";
static const QString svgSidebar    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"2\"/><line x1=\"9\" y1=\"3\" x2=\"9\" y2=\"21\"/></svg>";
static const QString svgReload     = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21.5 2v6h-6\"/><path d=\"M21.34 15.57a10 10 0 1 1-.59-8.31L21.5 8\"/></svg>";
static const QString svgStop       = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"6\" y=\"6\" width=\"12\" height=\"12\" rx=\"1\"/></svg>";
static const QString svgShare      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8\"/><polyline points=\"16 6 12 2 8 6\"/><line x1=\"12\" y1=\"2\" x2=\"12\" y2=\"15\"/></svg>";
static const QString svgDownloads  = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4\"/><polyline points=\"7 10 12 15 17 10\"/><line x1=\"12\" y1=\"15\" x2=\"12\" y2=\"3\"/></svg>";
static const QString svgTabOverview = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"14\" y=\"3\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"3\" y=\"14\" width=\"7\" height=\"7\" rx=\"1.5\"/><rect x=\"14\" y=\"14\" width=\"7\" height=\"7\" rx=\"1.5\"/></svg>";
static const QString svgReaderMode = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 19.5A2.5 2.5 0 0 1 6.5 17H20\"/><path d=\"M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z\"/><path d=\"M8 10h12\"/><path d=\"M8 14h12\"/><path d=\"M8 18h8\"/></svg>";
static const QString svgTranslate = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 11.5a8.38 8.38 0 0 1-.9 3.8 8.5 8.5 0 0 1-7.6 4.7 8.38 8.38 0 0 1-3.8-.9L3 21l1.9-5.7a8.38 8.38 0 0 1-.9-3.8 8.5 8.5 0 0 1 4.7-7.6 8.38 8.38 0 0 1 3.8.9h.5a8.48 8.48 0 0 1 8 8v.5z\"/></svg>";
static const QString svgPip = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"2\" y=\"2\" width=\"12\" height=\"12\" rx=\"2\"/><rect x=\"10\" y=\"10\" width=\"12\" height=\"12\" rx=\"2\"/></svg>";
static const QString svgSettings    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"3\"/><path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z\"/></svg>";
static const QString svgExtensions  = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M14 7l-3 3-4-1-2 2 4 1-1 4 2 2 1-4 3 3z\"/></svg>";
static const QString svgPlus        = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"/><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>";
}

ToolbarController::ToolbarController(BrowserWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
}

ToolbarController::~ToolbarController()
{
}

void ToolbarController::setupToolbar(QWidget *central, QVBoxLayout *centralLayout)
{
    m_toolbar = new QWidget(central);
    m_toolbar->setObjectName(QStringLiteral("Toolbar"));

    m_toolbarLayout = new QHBoxLayout(m_toolbar);
    m_toolbarLayout->setContentsMargins(8, 4, 8, 4);
    m_toolbarLayout->setSpacing(4);

    // Traffic lights layout (Safari style)
    m_trafficLayout = new QHBoxLayout();
    m_trafficLayout->setContentsMargins(0, 0, 0, 0);
    m_trafficLayout->setSpacing(6);

    m_toolbarLayout->addLayout(m_trafficLayout);

    setupNavigationButtons();
    setupUrlBar();
    setupActionButtons();

    centralLayout->addWidget(m_toolbar);
}

void ToolbarController::setupNavigationButtons()
{
    m_backButton = new QToolButton(m_toolbar);
    m_backButton->setObjectName(QStringLiteral("NavButton"));
    m_backButton->setToolTip(QStringLiteral("Back"));
    m_backButton->setAccessibleName(QStringLiteral("Back"));
    m_backButton->setAccessibleDescription(QStringLiteral("Go back to the previous page in history"));
    connect(m_backButton, &QToolButton::clicked, this, &ToolbarController::backRequested);

    m_forwardButton = new QToolButton(m_toolbar);
    m_forwardButton->setObjectName(QStringLiteral("NavButton"));
    m_forwardButton->setToolTip(QStringLiteral("Forward"));
    m_forwardButton->setAccessibleName(QStringLiteral("Forward"));
    m_forwardButton->setAccessibleDescription(QStringLiteral("Go forward to the next page in history"));
    connect(m_forwardButton, &QToolButton::clicked, this, &ToolbarController::forwardRequested);

    m_reloadButton = new QToolButton(m_toolbar);
    m_reloadButton->setObjectName(QStringLiteral("ReloadButton"));
    m_reloadButton->setToolTip(QStringLiteral("Reload"));
    m_reloadButton->setAccessibleName(QStringLiteral("Reload"));
    m_reloadButton->setAccessibleDescription(QStringLiteral("Reload the current page"));
    connect(m_reloadButton, &QToolButton::clicked, this, &ToolbarController::reloadRequested);

    m_toolbarLayout->addWidget(m_backButton);
    m_toolbarLayout->addWidget(m_forwardButton);
    m_toolbarLayout->addWidget(m_reloadButton);

    updateButtonIcons(chromeMode());
}

void ToolbarController::setupUrlBar()
{
    m_urlContainer = new QFrame(m_toolbar);
    m_urlContainer->setObjectName(QStringLiteral("UrlContainer"));

    QHBoxLayout *urlLayout = new QHBoxLayout(m_urlContainer);
    urlLayout->setContentsMargins(8, 0, 8, 0);
    urlLayout->setSpacing(4);

    // Shield/lock icon
    m_shieldInside = new QToolButton(m_urlContainer);
    m_shieldInside->setObjectName(QStringLiteral("ShieldButton"));
    m_shieldInside->setCursor(Qt::ArrowCursor);
    m_shieldInside->setFixedSize(20, 20);
    m_shieldInside->setAccessibleName(QStringLiteral("Privacy Protection"));
    m_shieldInside->setAccessibleDescription(QStringLiteral("Shows privacy protection status for this site"));
    urlLayout->addWidget(m_shieldInside);

    // Lock button (for HTTPS details)
    m_lockButton = new QToolButton(m_urlContainer);
    m_lockButton->setObjectName(QStringLiteral("LockButton"));
    m_lockButton->setCursor(Qt::PointingHandCursor);
    m_lockButton->setFixedSize(20, 20);
    m_lockButton->setAccessibleName(QStringLiteral("Connection Security"));
    m_lockButton->setAccessibleDescription(QStringLiteral("Shows connection security details for this site"));
    m_lockButton->hide();
    urlLayout->addWidget(m_lockButton);

    // URL bar
    m_urlBar = new QLineEdit(m_urlContainer);
    m_urlBar->setObjectName(QStringLiteral("UrlBar"));
    m_urlBar->setPlaceholderText(chromeMode() ? QStringLiteral("Search or enter address") : QStringLiteral("Search or enter website name"));
    m_urlBar->setAccessibleName(QStringLiteral("Address Bar"));
    m_urlBar->setAccessibleDescription(QStringLiteral("Enter a URL or search term"));
    m_urlBar->setFrame(false);
    m_urlBar->setClearButtonEnabled(true);
    m_urlBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_urlBar->setMinimumHeight(32);
    m_urlBar->installEventFilter(m_window);
    connect(m_urlBar, &QLineEdit::returnPressed, this, &ToolbarController::navigateRequested);
    connect(m_urlBar, &QLineEdit::textChanged, this, &ToolbarController::rebuildUrlSuggestions);
    urlLayout->addWidget(m_urlBar);

    // Loading bar
    m_loadingBar = new QProgressBar(m_urlContainer);
    m_loadingBar->setObjectName(QStringLiteral("LoadingBar"));
    m_loadingBar->setTextVisible(false);
    m_loadingBar->setFixedHeight(2);
    m_loadingBar->setRange(0, 100);
    m_loadingBar->setValue(0);
    m_loadingBar->hide();
    urlLayout->addWidget(m_loadingBar);

    m_toolbarLayout->addWidget(m_urlContainer, 1);

    setupUrlSuggestions();

    updateUrlBarStyle(chromeMode());
}

void ToolbarController::setupActionButtons()
{
    m_sidebarButton = new QToolButton(m_toolbar);
    m_sidebarButton->setObjectName(QStringLiteral("ActionButton"));
    m_sidebarButton->setToolTip(QStringLiteral("Sidebar"));
    m_sidebarButton->setAccessibleName(QStringLiteral("Sidebar"));
    m_sidebarButton->setAccessibleDescription(QStringLiteral("Toggle the sidebar panel"));
    connect(m_sidebarButton, &QToolButton::clicked, this, &ToolbarController::sidebarToggled);
    m_toolbarLayout->addWidget(m_sidebarButton);

    m_shareButton = new QToolButton(m_toolbar);
    m_shareButton->setObjectName(QStringLiteral("ActionButton"));
    m_shareButton->setToolTip(QStringLiteral("Share"));
    m_shareButton->setAccessibleName(QStringLiteral("Share"));
    m_shareButton->setAccessibleDescription(QStringLiteral("Share this page with other apps"));
    connect(m_shareButton, &QToolButton::clicked, this, &ToolbarController::shareRequested);
    m_toolbarLayout->addWidget(m_shareButton);

    m_downloadsButton = new QToolButton(m_toolbar);
    m_downloadsButton->setObjectName(QStringLiteral("ActionButton"));
    m_downloadsButton->setToolTip(QStringLiteral("Downloads"));
    m_downloadsButton->setAccessibleName(QStringLiteral("Downloads"));
    m_downloadsButton->setAccessibleDescription(QStringLiteral("Show downloaded files"));
    connect(m_downloadsButton, &QToolButton::clicked, this, &ToolbarController::downloadsRequested);
    m_toolbarLayout->addWidget(m_downloadsButton);

    m_tabOverviewButton = new QToolButton(m_toolbar);
    m_tabOverviewButton->setObjectName(QStringLiteral("ActionButton"));
    m_tabOverviewButton->setToolTip(QStringLiteral("Tab Overview"));
    m_tabOverviewButton->setAccessibleName(QStringLiteral("Tab Overview"));
    m_tabOverviewButton->setAccessibleDescription(QStringLiteral("Show all open tabs in a grid"));
    connect(m_tabOverviewButton, &QToolButton::clicked, this, &ToolbarController::tabOverviewRequested);
    m_toolbarLayout->addWidget(m_tabOverviewButton);

    m_readerModeButton = new QToolButton(m_toolbar);
    m_readerModeButton->setObjectName(QStringLiteral("ActionButton"));
    m_readerModeButton->setToolTip(QStringLiteral("Reader Mode"));
    m_readerModeButton->setAccessibleName(QStringLiteral("Reader Mode"));
    m_readerModeButton->setAccessibleDescription(QStringLiteral("Toggle simplified reading view for this page"));
    m_readerModeButton->setCheckable(true);
    connect(m_readerModeButton, &QToolButton::clicked, this, [this]() {
        emit readerModeRequested();
    });
    m_toolbarLayout->addWidget(m_readerModeButton);

    m_translateButton = new QToolButton(m_toolbar);
    m_translateButton->setObjectName(QStringLiteral("ActionButton"));
    m_translateButton->setToolTip(QStringLiteral("Translate Page"));
    m_translateButton->setAccessibleName(QStringLiteral("Translate"));
    m_translateButton->setAccessibleDescription(QStringLiteral("Translate this page to your preferred language"));
    connect(m_translateButton, &QToolButton::clicked, this, &ToolbarController::translateRequested);
    m_toolbarLayout->addWidget(m_translateButton);

    m_pipButton = new QToolButton(m_toolbar);
    m_pipButton->setObjectName(QStringLiteral("ActionButton"));
    m_pipButton->setToolTip(QStringLiteral("Picture-in-Picture"));
    m_pipButton->setAccessibleName(QStringLiteral("Picture in Picture"));
    m_pipButton->setAccessibleDescription(QStringLiteral("Enter picture-in-picture mode for videos on this page"));
    connect(m_pipButton, &QToolButton::clicked, this, [this]() {
        emit pipRequested();
    });
    m_toolbarLayout->addWidget(m_pipButton);

    m_settingsButton = new QToolButton(m_toolbar);
    m_settingsButton->setObjectName(QStringLiteral("ActionButton"));
    m_settingsButton->setToolTip(QStringLiteral("Settings"));
    m_settingsButton->setAccessibleName(QStringLiteral("Settings"));
    m_settingsButton->setAccessibleDescription(QStringLiteral("Open browser settings"));
    connect(m_settingsButton, &QToolButton::clicked, this, &ToolbarController::settingsRequested);
    m_toolbarLayout->addWidget(m_settingsButton);

    m_extensionsButton = new QToolButton(m_toolbar);
    m_extensionsButton->setObjectName(QStringLiteral("ActionButton"));
    m_extensionsButton->setToolTip(QStringLiteral("Extensions"));
    m_extensionsButton->setAccessibleName(QStringLiteral("Extensions"));
    m_extensionsButton->setAccessibleDescription(QStringLiteral("Manage browser extensions"));
    m_extensionsButton->setVisible(false);
    m_toolbarLayout->addWidget(m_extensionsButton);

    m_profileButton = new QToolButton(m_toolbar);
    m_profileButton->setObjectName(QStringLiteral("ActionButton"));
    m_profileButton->setToolTip(QStringLiteral("Profile"));
    m_profileButton->setAccessibleName(QStringLiteral("Profile"));
    m_profileButton->setAccessibleDescription(QStringLiteral("Manage your profile and account settings"));
    m_profileButton->setVisible(false);
    m_toolbarLayout->addWidget(m_profileButton);

    // New tab button
    m_addTabButton = new QToolButton(m_toolbar);
    m_addTabButton->setObjectName(QStringLiteral("NewTabButton"));
    m_addTabButton->setToolTip(QStringLiteral("New Tab"));
    m_addTabButton->setAccessibleName(QStringLiteral("New Tab"));
    m_addTabButton->setAccessibleDescription(QStringLiteral("Open a new tab"));
    connect(m_addTabButton, &QToolButton::clicked, this, &ToolbarController::newTabRequested);
    m_toolbarLayout->addWidget(m_addTabButton);

    updateButtonIcons(chromeMode());
}

void ToolbarController::setupUrlSuggestions()
{
    m_urlSuggest = new QWidget(m_window);
    m_urlSuggest->setObjectName(QStringLiteral("UrlSuggest"));
    m_urlSuggest->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_urlSuggest->hide();

    m_urlSuggestLayout = new QVBoxLayout(m_urlSuggest);
    m_urlSuggestLayout->setContentsMargins(0, 4, 0, 4);
    m_urlSuggestLayout->setSpacing(0);

    // Shadow
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(m_urlSuggest);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 60));
    m_urlSuggest->setGraphicsEffect(shadow);
}

void ToolbarController::updateButtonIcons(bool chrome)
{
    const QString color = chrome ? chromePalette().textPrimary : textPrimary();

    m_backButton->setIcon(createSvgIcon(svgBack.arg(color), 18, color));
    m_forwardButton->setIcon(createSvgIcon(svgForward.arg(color), 18, color));
    m_reloadButton->setIcon(createSvgIcon(svgReload.arg(color), 18, color));
    m_sidebarButton->setIcon(createSvgIcon(svgSidebar.arg(color), 18, color));
    m_shareButton->setIcon(createSvgIcon(svgShare.arg(color), 18, color));
    m_downloadsButton->setIcon(createSvgIcon(svgDownloads.arg(color), 18, color));
    m_tabOverviewButton->setIcon(createSvgIcon(svgTabOverview.arg(color), 18, color));
    m_readerModeButton->setIcon(createSvgIcon(svgReaderMode.arg(color), 18, color));
    m_translateButton->setIcon(createSvgIcon(svgTranslate.arg(color), 18, color));
    m_pipButton->setIcon(createSvgIcon(svgPip.arg(color), 18, color));
    m_settingsButton->setIcon(createSvgIcon(svgSettings.arg(color), 18, color));
    m_extensionsButton->setIcon(createSvgIcon(svgExtensions.arg(color), 18, color));
    m_addTabButton->setIcon(createSvgIcon(svgPlus.arg(color), 18, color));
}

QIcon ToolbarController::createSvgIcon(const QString &svg, int size, const QString &color)
{
    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return QIcon(pixmap);
}

void ToolbarController::updateUrlBarStyle(bool chrome)
{
    if (chrome) {
        const ChromePalette cp = chromePalette();
        m_urlContainer->setStyleSheet(QString(
            "QFrame#UrlContainer { background-color: %1; border: %2 solid %3; border-radius: %4px; }"
            "QLineEdit#UrlBar { background: transparent; color: %5; font-size: 13px; padding: 0 4px; }"
            "QProgressBar#LoadingBar { background: transparent; border: none; border-radius: 1px; }"
            "QProgressBar#LoadingBar::chunk { background: %6; border-radius: 1px; }"
            "QToolButton#ShieldButton, QToolButton#LockButton { background: transparent; border: none; color: %7; }"
            "QToolButton#ShieldButton:hover, QToolButton#LockButton:hover { background: %8; border-radius: 4px; }")
            .arg(cp.omniboxBg,
                 m_urlFocused ? QStringLiteral("1.5px") : QStringLiteral("1px"),
                 m_urlFocused ? cp.accent : cp.omniboxBorder,
                 QString::number(15),
                 cp.textPrimary,
                 cp.accent,
                 cp.textTertiary,
                 cp.hover));
        m_urlBar->setStyleSheet(QString(
            "QLineEdit#UrlBar { background: transparent; color: %1; font-size: 13px; padding: 0 4px; }"
            "QLineEdit#UrlBar:focus { background: transparent; }")
            .arg(cp.textPrimary));
    } else {
        m_urlContainer->setStyleSheet(QString(
            "QFrame#UrlContainer { background-color: %1; border: %2 solid %3; border-radius: %4px; }"
            "QLineEdit#UrlBar { background: transparent; color: %5; font-size: 13px; padding: 0 4px; }"
            "QProgressBar#LoadingBar { background: transparent; border: none; border-radius: 1px; }"
            "QProgressBar#LoadingBar::chunk { background: %6; border-radius: 1px; }"
            "QToolButton#ShieldButton, QToolButton#LockButton { background: transparent; border: none; color: %7; }"
            "QToolButton#ShieldButton:hover, QToolButton#LockButton:hover { background: %8; border-radius: 4px; }")
            .arg(searchBg(),
                 m_urlFocused ? QStringLiteral("1.5px") : QStringLiteral("1px"),
                 m_urlFocused ? accent() : borderLight(),
                 QString::number(8),
                 textPrimary(),
                 accent(),
                 textTertiary(),
                 hover()));
        m_urlBar->setStyleSheet(QString(
            "QLineEdit#UrlBar { background: transparent; color: %1; font-size: 13px; padding: 0 4px; }"
            "QLineEdit#UrlBar:focus { background: transparent; }")
            .arg(textPrimary()));
    }
}

void ToolbarController::updateLoadingBarStyle(bool chrome)
{
    if (m_loadingBar) {
        if (chrome) {
            m_loadingBar->setStyleSheet(QString(
                "QProgressBar#LoadingBar { background: transparent; border: none; border-radius: 1px; }"
                "QProgressBar#LoadingBar::chunk { background: %1; border-radius: 1px; }")
                .arg(chromePalette().accent));
        } else {
            m_loadingBar->setStyleSheet(QString(
                "QProgressBar#LoadingBar { background: transparent; border: none; border-radius: 1px; }"
                "QProgressBar#LoadingBar::chunk { background: %1; border-radius: 1px; }")
                .arg(accent()));
        }
    }
}

void ToolbarController::applyTheme()
{
    bool chrome = chromeMode();
    updateButtonIcons(chrome);
    updateUrlBarStyle(chrome);
    updateLoadingBarStyle(chrome);
}

void ToolbarController::applyUiLayout(bool chrome)
{
    applyTheme();
}

void ToolbarController::updateNavigationState()
{
    // Update back/forward button states
}

void ToolbarController::updateUrlContainerStyle()
{
    applyTheme();
}

void ToolbarController::animateUrlBar(int targetWidth)
{
    // URL bar animation
}

void ToolbarController::rebuildUrlSuggestions()
{
    // URL suggestions
}

void ToolbarController::showUrlSuggestions()
{
    // Show suggestions
}

void ToolbarController::hideUrlSuggestions()
{
    // Hide suggestions
}

void ToolbarController::selectUrlSuggestion(int index)
{
    // Select suggestion
}

void ToolbarController::activateUrlSuggestion(int index)
{
    // Activate suggestion
}

#include "ToolbarController.moc"