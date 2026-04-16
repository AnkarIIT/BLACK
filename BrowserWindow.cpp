
#include "BrowserWindow.h"
#include <QFrame>
#include <QStyle>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
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

// ── SVG strings ──────────────────────────────────────────────────────────────
const QString svgSidebar   = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"2\"/><line x1=\"9\" y1=\"3\" x2=\"9\" y2=\"21\"/></svg>";
const QString svgBack      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2.2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"15 18 9 12 15 6\"/></svg>";
const QString svgForward   = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2.2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"9 18 15 12 9 6\"/></svg>";
const QString svgShield    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z\"/></svg>";
const QString svgReload    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.59-8.31l4.25-4.26\"/></svg>";
const QString svgShare     = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8\"/><polyline points=\"16 6 12 2 8 6\"/><line x1=\"12\" y1=\"2\" x2=\"12\" y2=\"15\"/></svg>";
const QString svgAddTab    = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"/><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>";
const QString svgClose_tl  = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#4b0000\" stroke-width=\"3\" stroke-linecap=\"round\"><line x1=\"18\" y1=\"6\" x2=\"6\" y2=\"18\"/><line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/></svg>";
const QString svgMinimize_tl = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#5b4000\" stroke-width=\"3\" stroke-linecap=\"round\"><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>";
const QString svgMaximize_tl = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#004b00\" stroke-width=\"3\" stroke-linecap=\"round\"><polyline points=\"15 3 21 3 21 9\"/><polyline points=\"9 21 3 21 3 15\"/><line x1=\"21\" y1=\"3\" x2=\"14\" y2=\"10\"/><line x1=\"3\" y1=\"21\" x2=\"10\" y2=\"14\"/></svg>";
// Grid icon (4 small squares like Safari's tab overview button)
const QString svgGrid      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" width=\"7\" height=\"7\" rx=\"1\"/><rect x=\"14\" y=\"3\" width=\"7\" height=\"7\" rx=\"1\"/><rect x=\"14\" y=\"14\" width=\"7\" height=\"7\" rx=\"1\"/><rect x=\"3\" y=\"14\" width=\"7\" height=\"7\" rx=\"1\"/></svg>";
// Small X for closing a tab / overview card
const QString svgX         = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2.5\" stroke-linecap=\"round\"><line x1=\"18\" y1=\"6\" x2=\"6\" y2=\"18\"/><line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/></svg>";

// ── Helpers ───────────────────────────────────────────────────────────────────
static QString truncate(const QString &s, int max = 22) {
    return s.length() > max ? s.left(max - 1) + "…" : s;
}

// =============================================================================
BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabStack(new QStackedWidget(this))
    , m_urlBar(new QLineEdit(this))
    , m_sidebarButton(new QToolButton(this))
    , m_backButton(new QToolButton(this))
    , m_forwardButton(new QToolButton(this))
    , m_reloadButton(new QToolButton(this))
    , m_shieldButton(new QToolButton(this))
    , m_shareButton(new QToolButton(this))
    , m_addTabButton(new QToolButton(this))
    , m_groupTabsButton(new QToolButton(this))
    , m_tabCountLabel(nullptr)
    , m_overviewOverlay(nullptr)
    , m_overviewPanel(nullptr)
    , m_overviewVisible(false)
    , m_isDragging(false)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground);

    setupUi();
    setupTabOverlay();

    setStyleSheet("QMainWindow { background-color: transparent; }");
    setFont(QFont("Segoe UI", 10));

    addNewTab(QUrl("https://www.apple.com"));

    connect(m_urlBar, &QLineEdit::returnPressed, this, &BrowserWindow::navigateToUrl);
}

BrowserWindow::~BrowserWindow() {}

// ── Window control ───────────────────────────────────────────────────────────
void BrowserWindow::closeWindow()    { close(); }
void BrowserWindow::minimizeWindow() { showMinimized(); }
void BrowserWindow::maximizeWindow() { if (isMaximized()) showNormal(); else showMaximized(); }

// ── Mouse drag (frameless window) ────────────────────────────────────────────
void BrowserWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 60) {
        m_isDragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}
void BrowserWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}
void BrowserWindow::mouseReleaseEvent(QMouseEvent *) { m_isDragging = false; }

void BrowserWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    // keep overlay same size as window
    if (m_overviewOverlay) m_overviewOverlay->setGeometry(rect());
    rebuildInlineTabBar();
}

// ── Icon helper ──────────────────────────────────────────────────────────────
QIcon BrowserWindow::createSvgIcon(const QString &svgData, int size, QString color) {
    QString filled = svgData.contains("%1") ? svgData.arg(color) : svgData;
    QSvgRenderer renderer(filled.toUtf8());
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    return QIcon(pix);
}

// ── Traffic light helper ─────────────────────────────────────────────────────
QToolButton* BrowserWindow::createTrafficLight(const QString &color, const QString &hoverColor, const QString &) {
    QToolButton *btn = new QToolButton(this);
    btn->setFixedSize(14, 14);
    btn->setStyleSheet(QString(
        "QToolButton { background-color: %1; border-radius: 7px; border: 1px solid rgba(0,0,0,0.1); }"
        "QToolButton:hover { background-color: %2; }").arg(color, hoverColor));
    return btn;
}

// ── Setup Main UI ─────────────────────────────────────────────────────────────
void BrowserWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("CentralContainer");
    centralWidget->setStyleSheet(
        "#CentralContainer { background-color: #F2F2F7; border-radius: 16px; border: 1px solid rgba(0,0,0,0.08); }"
    );
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Top bar ───────────────────────────────────────────────────────────
    m_topBar = new QFrame(centralWidget);
    m_topBar->setObjectName("TopBar");
    m_topBar->setFixedHeight(56);
    m_topBar->setStyleSheet(
        "#TopBar { background-color: rgba(255,255,255,0.88); "
        "border-bottom: 1px solid rgba(0,0,0,0.08); "
        "border-top-left-radius: 16px; border-top-right-radius: 16px; }"
    );

    m_navLayout = new QHBoxLayout(m_topBar);
    m_navLayout->setContentsMargins(14, 0, 14, 0);
    m_navLayout->setSpacing(8);

    // Traffic lights
    m_closeButton    = createTrafficLight("#FF5F56", "#E0443E");
    m_minimizeButton = createTrafficLight("#FFBD2E", "#DEA124");
    m_maximizeButton = createTrafficLight("#27C93F", "#1AAB29");
    m_navLayout->addWidget(m_closeButton);
    m_navLayout->addSpacing(4);
    m_navLayout->addWidget(m_minimizeButton);
    m_navLayout->addSpacing(4);
    m_navLayout->addWidget(m_maximizeButton);
    m_navLayout->addSpacing(14);

    connect(m_closeButton,    &QToolButton::clicked, this, &BrowserWindow::closeWindow);
    connect(m_minimizeButton, &QToolButton::clicked, this, &BrowserWindow::minimizeWindow);
    connect(m_maximizeButton, &QToolButton::clicked, this, &BrowserWindow::maximizeWindow);

    // Flat icon button style
    const QString flatBtn =
        "QToolButton { border: none; background: transparent; border-radius: 10px; padding: 6px; }"
        "QToolButton:hover { background-color: rgba(60,60,67,0.10); }"
        "QToolButton:disabled { opacity: 0.25; }";

    m_sidebarButton->setIcon(createSvgIcon(svgSidebar, 18));
    m_sidebarButton->setStyleSheet(flatBtn);
    m_navLayout->addWidget(m_sidebarButton);

    m_backButton->setIcon(createSvgIcon(svgBack, 18));
    m_backButton->setStyleSheet(flatBtn);
    m_navLayout->addWidget(m_backButton);

    m_forwardButton->setIcon(createSvgIcon(svgForward, 18));
    m_forwardButton->setStyleSheet(flatBtn);
    m_navLayout->addWidget(m_forwardButton);

    m_navLayout->addSpacing(4);

    // ── Inline tab strip ─────────────────────────────────────────────────
    // Safari compact mode: tabs live between nav buttons and the URL bar.
    // Hidden when only 1 tab is open.
    m_tabStrip = new QWidget(m_topBar);
    m_tabStrip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_tabStripLayout = new QHBoxLayout(m_tabStrip);
    m_tabStripLayout->setContentsMargins(0, 6, 0, 6);
    m_tabStripLayout->setSpacing(6);
    m_tabStrip->setVisible(false);
    m_navLayout->addWidget(m_tabStrip, 10);

    m_navLayout->addSpacing(4);

    // ── URL / Address bar ─────────────────────────────────────────────────
    m_urlBar->setStyleSheet(
        "QLineEdit { "
        "   background-color: rgba(0,0,0,0.04); "
        "   color: #1D1D1F; "
        "   border: 1px solid rgba(0,0,0,0.10); "
        "   border-radius: 18px; "
        "   padding: 0 42px; "
        "   font-size: 13px; "
        "   font-weight: 400; "
        "}"
        "QLineEdit:focus { "
        "   background-color: #FFFFFF; "
        "   border: 1px solid rgba(0,0,0,0.16); "
        "   color: #1D1D1F; "
        "}"
    );
    m_urlBar->setPlaceholderText("Search or enter website name");
    m_urlBar->setAlignment(Qt::AlignLeft);
    m_urlBar->setMinimumHeight(34);
    m_urlBar->setMaximumHeight(34);

    // Shield left, reload right — inside address bar
    m_shieldButton->setIcon(createSvgIcon(svgShield, 16, "#007AFF"));
    m_shieldButton->setStyleSheet("border: none; background: transparent;");
    m_shieldButton->setFixedSize(22, 22);

    m_reloadButton->setIcon(createSvgIcon(svgReload, 16, "#5C5C5E"));
    m_reloadButton->setStyleSheet("border: none; background: transparent;");
    m_reloadButton->setFixedSize(22, 22);

    QHBoxLayout *urlLayout = new QHBoxLayout(m_urlBar);
    urlLayout->setContentsMargins(8, 0, 8, 0);
    urlLayout->addWidget(m_shieldButton);
    urlLayout->addStretch();
    urlLayout->addWidget(m_reloadButton);

    m_urlBar->setMinimumWidth(180); 
    m_navLayout->addWidget(m_urlBar, 2); // Lower stretch factor for the URL bar side piece

    m_navLayout->addSpacing(8);

    // ── Right controls ────────────────────────────────────────────────────
    m_shareButton->setIcon(createSvgIcon(svgShare, 18));
    m_shareButton->setStyleSheet(flatBtn);
    m_navLayout->addWidget(m_shareButton);

    m_addTabButton->setIcon(createSvgIcon(svgAddTab, 18));
    m_addTabButton->setStyleSheet(flatBtn);
    m_navLayout->addWidget(m_addTabButton);

    // Tab overview (grid) button — with badge on top
    QWidget *gridBtnWrapper = new QWidget(m_topBar);
    gridBtnWrapper->setFixedSize(38, 38);
    gridBtnWrapper->setStyleSheet("background: transparent;");

    m_groupTabsButton->setParent(gridBtnWrapper);
    m_groupTabsButton->setGeometry(0, 0, 38, 38);
    m_groupTabsButton->setIcon(createSvgIcon(svgGrid, 18));
    m_groupTabsButton->setStyleSheet(flatBtn);

    // Count badge (e.g. "1")
    m_tabCountLabel = new QLabel("1", gridBtnWrapper);
    m_tabCountLabel->setAlignment(Qt::AlignCenter);
    m_tabCountLabel->setFixedSize(16, 14);
    m_tabCountLabel->move(22, 2);   // top-right of the button
    m_tabCountLabel->setStyleSheet(
        "QLabel { "
        "   background-color: #007AFF; "
        "   color: white; "
        "   font-size: 9px; "
        "   font-weight: 700; "
        "   border-radius: 4px; "
        "}"
    );
    m_tabCountLabel->raise();

    m_navLayout->addWidget(gridBtnWrapper);

    // ── Assemble ──────────────────────────────────────────────────────────
    mainLayout->addWidget(m_topBar);
    mainLayout->addWidget(m_tabStack, 1);

    // ── Separator line ────────────────────────────────────────────────────
    QFrame *separator = new QFrame(centralWidget);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: rgba(0,0,0,0.08);");
    mainLayout->insertWidget(1, separator); // insert between topBar and tabStack

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_addTabButton,    &QToolButton::clicked, this, &BrowserWindow::addTabAction);
    connect(m_shareButton,     &QToolButton::clicked, this, &BrowserWindow::shareAction);
    connect(m_groupTabsButton, &QToolButton::clicked, this, [this](){
        m_overviewVisible ? hideTabOverview() : showTabOverview();
    });
    connect(m_backButton,    &QToolButton::clicked, this, [this](){
        if (auto* v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->back();
    });
    connect(m_forwardButton, &QToolButton::clicked, this, [this](){
        if (auto* v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->forward();
    });
    connect(m_reloadButton,  &QToolButton::clicked, this, [this](){
        if (auto* v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget())) v->reload();
    });
}

// ── Tab Overview Overlay ──────────────────────────────────────────────────────
void BrowserWindow::setupTabOverlay()
{
    // Semi-transparent full-window dimmer
    m_overviewOverlay = new QWidget(this);
    m_overviewOverlay->setObjectName("OverviewOverlay");
    m_overviewOverlay->setGeometry(rect());
    m_overviewOverlay->setStyleSheet(
        "#OverviewOverlay { background-color: rgba(28,28,30,0.36); }"
    );
    m_overviewOverlay->setVisible(false);
    m_overviewOverlay->raise();

    // White panel — slides up from bottom
    m_overviewPanel = new QWidget(m_overviewOverlay);
    m_overviewPanel->setObjectName("OverviewPanel");
    m_overviewPanel->setStyleSheet(
        "#OverviewPanel { background-color: #FFFFFF; border-radius: 18px; }"
    );

    QVBoxLayout *panelLayout = new QVBoxLayout(m_overviewPanel);
    panelLayout->setContentsMargins(20, 16, 20, 20);
    panelLayout->setSpacing(12);

    QFrame *dragHandle = new QFrame(m_overviewPanel);
    dragHandle->setFixedSize(40, 4);
    dragHandle->setStyleSheet("background: rgba(60,60,67,0.18); border-radius: 2px;");
    panelLayout->addWidget(dragHandle, 0, Qt::AlignHCenter);

    // Panel header
    QHBoxLayout *hdr = new QHBoxLayout;
    QLabel *title = new QLabel("Tabs", m_overviewPanel);
    title->setStyleSheet("font-size: 17px; font-weight: 700; color: #1D1D1F;");
    hdr->addWidget(title);
    hdr->addStretch();

    QPushButton *doneBtn = new QPushButton("Done", m_overviewPanel);
    doneBtn->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: #007AFF; font-size: 15px; font-weight: 600; padding: 0; }"
        "QPushButton:hover { color: #0051D5; }"
    );
    connect(doneBtn, &QPushButton::clicked, this, &BrowserWindow::hideTabOverview);
    hdr->addWidget(doneBtn);
    panelLayout->addLayout(hdr);

    // Scroll area for the grid
    m_overviewScroll = new QScrollArea(m_overviewPanel);
    m_overviewScroll->setWidgetResizable(true);
    m_overviewScroll->setFrameShape(QFrame::NoFrame);
    m_overviewScroll->setStyleSheet("background: transparent;");

    m_overviewGrid = new QWidget;
    m_overviewGrid->setStyleSheet("background: transparent;");
    m_overviewGridLayout = new QGridLayout(m_overviewGrid);
    m_overviewGridLayout->setSpacing(18);
    m_overviewGridLayout->setContentsMargins(0, 0, 0, 0);
    m_overviewGridLayout->setColumnStretch(0, 1);
    m_overviewGridLayout->setColumnStretch(1, 1);
    m_overviewGridLayout->setColumnStretch(2, 1);

    m_overviewScroll->setWidget(m_overviewGrid);
    panelLayout->addWidget(m_overviewScroll);

    // New Tab button at bottom
    QPushButton *newTabBtn = new QPushButton("+ New Tab", m_overviewPanel);
    newTabBtn->setStyleSheet(
        "QPushButton { background-color: #007AFF; color: white; border-radius: 12px; "
        "font-size: 14px; font-weight: 600; padding: 10px 0; border: none; }"
        "QPushButton:hover { background-color: #0051D5; }"
    );
    connect(newTabBtn, &QPushButton::clicked, this, [this](){
        hideTabOverview();
        addTabAction();
    });
    panelLayout->addWidget(newTabBtn);

    // Click overlay to dismiss
    m_overviewOverlay->installEventFilter(this);
}



void BrowserWindow::showTabOverview()
{
    m_overviewVisible = true;
    rebuildOverviewGrid();

    // Size the panel: 80% of window height, full width minus margins
    int pw = width();
    int ph = (int)(height() * 0.82);
    m_overviewPanel->setGeometry(0, height() - ph, pw, ph);

    m_overviewOverlay->setGeometry(rect());
    m_overviewOverlay->setVisible(true);
    m_overviewOverlay->raise();
}

void BrowserWindow::hideTabOverview()
{
    m_overviewVisible = false;
    m_overviewOverlay->setVisible(false);
}

// ── Overview card builder ─────────────────────────────────────────────────────
QWidget* BrowserWindow::buildOverviewCard(int index)
{
    const TabInfo &tab = m_tabs[index];
    bool isActive = (index == m_currentTabIndex);

    QFrame *card = new QFrame;
    card->setObjectName("OverviewCard");
    card->setMinimumWidth(220);
    card->setMaximumWidth(400);
    card->setFixedHeight(118);
    card->setStyleSheet(QString(
        "#OverviewCard { background-color: %1; border-radius: 16px; "
        "border: %2; box-shadow: 0 20px 45px rgba(0,0,0,0.08); }"
    ).arg(isActive ? "#FFFFFF" : "#F7F7FA",
          isActive ? "1px solid rgba(0,0,0,0.10)" : "1px solid rgba(0,0,0,0.08)"));

    QHBoxLayout *lay = new QHBoxLayout(card);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(12);

    QFrame *preview = new QFrame(card);
    preview->setFixedSize(108, 80);
    preview->setStyleSheet(
        "background: #E9E9EE; border-radius: 14px;"
    );
    lay->addWidget(preview);

    QVBoxLayout *info = new QVBoxLayout;
    info->setSpacing(4);

    QLabel *titleLbl = new QLabel(truncate(tab.title.isEmpty() ? "New Tab" : tab.title));
    titleLbl->setStyleSheet("font-size: 14px; font-weight: 700; color: #1D1D1F;");
    info->addWidget(titleLbl);

    QLabel *urlLbl = new QLabel(truncate(tab.url, 38));
    urlLbl->setStyleSheet("font-size: 12px; color: #6E6E73;");
    info->addWidget(urlLbl);
    info->addStretch();

    lay->addLayout(info);
    lay->addStretch();

    // Close button (×)
    QPushButton *closeBtn = new QPushButton("✕", card);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet(
        "QPushButton { background: rgba(0,0,0,0.07); border-radius: 12px; "
        "color: #8E8E93; font-size: 11px; font-weight: 700; border: none; }"
        "QPushButton:hover { background: rgba(255,59,48,0.18); color: #FF3B30; }"
    );
    int capturedIndex = index;
    connect(closeBtn, &QPushButton::clicked, this, [this, capturedIndex](){
        closeTab(capturedIndex);
    });
    lay->addWidget(closeBtn);

    // Click card → switch to tab
    card->setCursor(Qt::PointingHandCursor);
    card->installEventFilter(this);
    card->setProperty("tabIndex", index);

    return card;
}

void BrowserWindow::rebuildOverviewGrid()
{
    // Clear old cards
    QLayoutItem *item;
    while ((item = m_overviewGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int columns = qMax(1, width() / 320);
    int row = 0;
    int col = 0;
    for (int i = 0; i < m_tabs.count(); ++i) {
        QWidget *card = buildOverviewCard(i);
        m_overviewGridLayout->addWidget(card, row, col);
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }

    // Fill remaining columns with empty spacers to keep layout balanced
    while (col < columns && col > 0) {
        m_overviewGridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), row, col);
        col++;
    }
}

// ── Inline tab strip (Safari-style) ───────────────────────────────────────
void BrowserWindow::rebuildInlineTabBar()
{
    // Clear ALL pills
    while (m_tabStripLayout->count() > 0) {
        QLayoutItem *item = m_tabStripLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    bool multiTab = m_tabs.count() > 1;
    m_tabStrip->setVisible(multiTab);
    if (!multiTab) return;

    int tabCount = m_tabs.count();
    
    // Safari-like spacing: tighter at extremes, more breathing room in the middle
    int spacing = (tabCount > 40) ? 0 : (tabCount > 20 ? 2 : 3);
    m_tabStripLayout->setSpacing(spacing);
    m_tabStripLayout->setContentsMargins(0, 2, 0, 2);

    int totalStripWidth = m_tabStrip->width();
    if (totalStripWidth < 260) totalStripWidth = 260;
    int availableWidth = totalStripWidth - (tabCount - 1) * spacing;
    int estWidth = qMax(50, availableWidth / tabCount);

    bool showText = estWidth >= 100;
    bool showCloseBtn = estWidth >= 120;

    for (int i = 0; i < tabCount; ++i) {
        bool active = (i == m_currentTabIndex);
        const TabInfo &tab = m_tabs[i];

        // Safari-style frame
        QFrame *pill = new QFrame(m_tabStrip);
        pill->setObjectName(active ? "SafariActiveTab" : "SafariInactiveTab");
        pill->setFixedHeight(34);
        pill->setMinimumWidth(50);
        pill->setMaximumWidth(280);
        pill->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        pill->setToolTip(tab.title.isEmpty() ? "New Tab" : tab.title);

        if (active) {
            // Active tab: white background with soft shadow effect
            pill->setStyleSheet(
                "#SafariActiveTab { "
                "background: #FFFFFF; "
                "border-radius: 16px; "
                "border: 1px solid rgba(0,0,0,0.06); "
                "box-shadow: 0 4px 12px rgba(0,0,0,0.08); "
                "}"
            );
        } else {
            // Inactive tab: very subtle, semi-transparent
            pill->setStyleSheet(
                "#SafariInactiveTab { "
                "background: rgba(120,120,128,0.04); "
                "border-radius: 16px; "
                "border: 1px solid rgba(120,120,128,0.05); "
                "}"
                "#SafariInactiveTab:hover { "
                "background: rgba(120,120,128,0.10); "
                "border: 1px solid rgba(120,120,128,0.10); "
                "}"
            );
        }

        QHBoxLayout *lay = new QHBoxLayout(pill);
        lay->setContentsMargins(10, 0, (showCloseBtn ? 6 : 10), 0);
        lay->setSpacing(6);

        // Favicon: always visible
        QLabel *iconLabel = new QLabel(pill);
        QPixmap pix = tab.icon.pixmap(16, 16);
        if (pix.isNull()) {
            iconLabel->setText("🌐");
            iconLabel->setStyleSheet("font-size: 13px; color: #999;");
        } else {
            iconLabel->setPixmap(pix);
        }
        iconLabel->setFixedSize(16, 16);
        iconLabel->setAlignment(Qt::AlignCenter);
        lay->addWidget(iconLabel, 0, Qt::AlignCenter);

        // Title: shown only when there's space
        if (showText) {
            QString shownText = tab.title.isEmpty() ? "New Tab" : tab.title;
            QLabel *lbl = new QLabel(truncate(shownText, qMax(1, estWidth / 11)), pill);
            lbl->setStyleSheet(QString(
                "color: %1; "
                "font-size: 12px; "
                "font-weight: %2; "
                "background: transparent; "
                )
                .arg(active ? "#000000" : "#555555")
                .arg(active ? "500" : "400"));
            lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            lay->addWidget(lbl, 1);
        }

        // Close button: visible based on width, hidden for inactive on hover
        if (showCloseBtn) {
            QPushButton *x = new QPushButton("✕", pill);
            x->setFixedSize(16, 16);
            x->setObjectName(active ? "ActiveCloseBtn" : "InactiveCloseBtn");
            
            if (active) {
                x->setStyleSheet(
                    "QPushButton { "
                    "background: transparent; "
                    "border: none; "
                    "color: #999; "
                    "font-size: 9px; "
                    "font-weight: 700; "
                    "padding: 0; "
                    "border-radius: 8px; "
                    "}"
                    "QPushButton:hover { "
                    "background: rgba(255,59,48,0.15); "
                    "color: #FF3B30; "
                    "}"
                );
            } else {
                x->setStyleSheet(
                    "QPushButton { "
                    "background: transparent; "
                    "border: none; "
                    "color: #CCC; "
                    "font-size: 9px; "
                    "font-weight: 700; "
                    "padding: 0; "
                    "border-radius: 8px; "
                    "opacity: 0; "
                    "}"
                    "QPushButton:hover { "
                    "opacity: 1; "
                    "background: rgba(0,0,0,0.08); "
                    "color: #666; "
                    "}"
                );
            }
            
            int ci = i;
            connect(x, &QPushButton::clicked, this, [this, ci](){ closeTab(ci); });
            lay->addWidget(x, 0, Qt::AlignRight | Qt::AlignVCenter);
        }

        pill->setCursor(Qt::PointingHandCursor);
        pill->installEventFilter(this);
        pill->setProperty("tabIndex", i);

        m_tabStripLayout->addWidget(pill, 1);
    }
}

// eventFilter for pill & card clicks
bool BrowserWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *w = qobject_cast<QWidget*>(obj);
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
            // dismiss overlay on click outside panel
            if (obj == m_overviewOverlay) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                if (!m_overviewPanel->geometry().contains(me->pos()))
                    hideTabOverview();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ── Tab management ────────────────────────────────────────────────────────────
void BrowserWindow::addNewTab(const QUrl &url) {
    QWebEngineView* view = new QWebEngineView(this);
    m_tabStack->addWidget(view);

    TabInfo info;
    info.view  = view;
    info.title = "New Tab";
    info.url   = url.toString();
    m_tabs.append(info);

    int index = m_tabs.count() - 1;

    connect(view, &QWebEngineView::titleChanged, this, [this, index](const QString &t){
        if (index < m_tabs.count()) {
            m_tabs[index].title = t;
            rebuildInlineTabBar();
        }
    });

    connect(view, &QWebEngineView::iconChanged,  this, &BrowserWindow::onTabIconChanged);

    connect(view, &QWebEngineView::urlChanged,  this, [this, index](const QUrl &u){
        if (index < m_tabs.count()) {
            m_tabs[index].url = u.toString();
            if (index == m_currentTabIndex) updateUrlBar(u);
        }
    });
    connect(view, &QWebEngineView::loadFinished, this, &BrowserWindow::updateNavigationState);

    view->setUrl(url);
    setCurrentTab(index);
}

void BrowserWindow::setCurrentTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;
    m_currentTabIndex = index;
    m_tabStack->setCurrentIndex(index);

    updateUrlBar(m_tabs[index].view->url());
    updateNavigationState();
    updateTabCountBadge();
    rebuildInlineTabBar();
}

void BrowserWindow::closeTab(int index) {
    if (index < 0 || index >= m_tabs.count()) return;
    if (m_tabs.count() == 1) { close(); return; }

    QWebEngineView *v = m_tabs[index].view;
    m_tabStack->removeWidget(v);
    v->deleteLater();
    m_tabs.removeAt(index);

    // Fix index
    int newIdx = qMin(m_currentTabIndex, m_tabs.count() - 1);
    m_currentTabIndex = -1; // force update
    setCurrentTab(newIdx);

    if (m_overviewVisible) rebuildOverviewGrid();
}

void BrowserWindow::updateTabCountBadge() {
    if (!m_tabCountLabel) return;
    int n = m_tabs.count();
    m_tabCountLabel->setText(n > 99 ? "99+" : QString::number(n));
    // Scale badge width for >9
    m_tabCountLabel->setFixedWidth(n > 9 ? 20 : 16);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void BrowserWindow::addTabAction() {
    // Verified absolute path
    QString finalPath = "C:/Codes/browser/startpage.html";
    if (QFile::exists(finalPath)) {
        addNewTab(QUrl::fromLocalFile(finalPath));
    } else {
        addNewTab(QUrl("https://www.google.com")); // Fallback
    }
}

void BrowserWindow::shareAction() {
    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
        QApplication::clipboard()->setText(v->url().toString());
}

void BrowserWindow::navigateToUrl() {
    QString input = m_urlBar->text().trimmed();
    if (input.isEmpty()) return;
    if (!input.startsWith("http://") && !input.startsWith("https://")) {
        if (input.contains(".") && !input.contains(" ")) input = "https://" + input;
        else input = "https://www.google.com/search?q=" + input.replace(" ", "+");
    }
    if (auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget()))
        v->setUrl(QUrl(input));
    m_urlBar->clearFocus();
}

void BrowserWindow::updateUrlBar(const QUrl &url) {
    if (m_currentTabIndex < 0 || m_currentTabIndex >= m_tabs.count()) return;
    QString display = url.toString();
    if (display.startsWith("file://")) display = "Start Page";
    else {
        if (display.startsWith("https://")) display.remove(0, 8);
        if (display.startsWith("www."))     display.remove(0, 4);
        if (display.endsWith("/"))          display.chop(1);
    }
    m_urlBar->setText(display);
}

void BrowserWindow::updateNavigationState() {
    auto *v = qobject_cast<QWebEngineView*>(m_tabStack->currentWidget());
    if (!v) return;
    m_backButton->setEnabled(v->history()->canGoBack());
    m_forwardButton->setEnabled(v->history()->canGoForward());
}

void BrowserWindow::onTabIconChanged(const QIcon &icon) {
    QWebEngineView* senderView = qobject_cast<QWebEngineView*>(sender());
    if (!senderView) return;

    for (int i = 0; i < m_tabs.count(); ++i) {
        if (m_tabs[i].view == senderView) {
            m_tabs[i].icon = icon;
            break;
        }
    }
    rebuildInlineTabBar();
}
