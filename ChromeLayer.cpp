#include "ChromeLayer.h"
#include "BrowserWindow.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QPixmap>

namespace {

QIcon chromeSvgIcon(const QString &svg, int size, const QString &color)
{
    QString filled = svg.contains(QLatin1String("%1")) ? svg.arg(color) : svg;
    QSvgRenderer renderer(filled.toUtf8());
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    return QIcon(pix);
}

QString chromePlusIcon()
{
    return QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\">"
        "<line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"/>"
        "<line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>");
}

QString svgWinMinimize()
{
    return QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"%1\" stroke-width=\"1.6\" stroke-linecap=\"round\">"
        "<line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>");
}

QString svgWinMaximize()
{
    return QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"%1\" stroke-width=\"1.6\" stroke-linecap=\"round\">"
        "<rect x=\"5.5\" y=\"5.5\" width=\"13\" height=\"13\" rx=\"1.5\"/></svg>");
}

QString svgWinClose()
{
    return QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"%1\" stroke-width=\"1.6\" stroke-linecap=\"round\">"
        "<path d=\"M6.5 6.5l11 11M17.5 6.5l-11 11\"/></svg>");
}

QString macTrafficStyle(const QString &color, const QString &hover)
{
    return QStringLiteral(
        "QToolButton { background-color: %1; border-radius: 6px; border: 0.5px solid rgba(0,0,0,0.12); }"
        "QToolButton:hover { background-color: %2; }").arg(color, hover);
}

} // namespace

ChromeLayer::ChromeLayer(BrowserWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_chrome(false)
    , m_tabStrip(nullptr)
    , m_stripLayout(nullptr)
    , m_trafficLayout(nullptr)
    , m_windowCtlLayout(nullptr)
    , m_tabLayout(nullptr)
    , m_newTabButton(nullptr)
{
}

void ChromeLayer::setupUi(QWidget *central, QVBoxLayout *rootLayout)
{
    // Top horizontal tab strip (Chrome-style). Hidden until chrome mode is on.
    m_tabStrip = new QWidget(central);
    m_tabStrip->setObjectName(QStringLiteral("ChromeTabStrip"));
    m_tabStrip->setFixedHeight(38);

    m_stripLayout = new QHBoxLayout(m_tabStrip);
    m_stripLayout->setContentsMargins(8, 0, 4, 0);
    m_stripLayout->setSpacing(0);
    m_stripLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Traffic lights live here while chrome mode is active. The buttons
    // themselves stay owned by BrowserWindow; we only arrange them.
    m_trafficLayout = new QHBoxLayout;
    m_trafficLayout->setContentsMargins(0, 0, 0, 0);
    m_trafficLayout->setSpacing(6);
    m_stripLayout->addLayout(m_trafficLayout);
    m_stripLayout->addSpacing(12);

    // Tab area: [scroll-left][tabs...][scroll-right][new-tab button].
    // Wrap tab layout in a scroll area for overflow handling.
    m_tabScrollArea = new QScrollArea(m_tabStrip);
    m_tabScrollArea->setWidgetResizable(false);
    m_tabScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabScrollArea->setFrameShape(QFrame::NoFrame);
    m_tabScrollArea->setStyleSheet("background: transparent; border: none;");

    QWidget *tabContainer = new QWidget(m_tabScrollArea);
    tabContainer->setStyleSheet("background: transparent;");
    m_tabLayout = new QHBoxLayout(tabContainer);
    m_tabLayout->setContentsMargins(0, 0, 0, 0);
    m_tabLayout->setSpacing(1);
    m_tabLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tabScrollArea->setWidget(tabContainer);

    // Scroll buttons
    m_scrollLeftBtn = new QToolButton(m_tabStrip);
    m_scrollLeftBtn->setFixedSize(24, 24);
    m_scrollLeftBtn->setToolTip("Scroll tabs left");
    m_scrollLeftBtn->setIcon(chromeSvgIcon(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\"><polyline points=\"15 18 9 12 15 6\"/></svg>", 14, "transparent"));
    m_scrollLeftBtn->setVisible(false);
    connect(m_scrollLeftBtn, &QToolButton::clicked, this, [this]() {
        if (m_tabScrollArea) {
            m_tabScrollArea->horizontalScrollBar()->setValue(
                m_tabScrollArea->horizontalScrollBar()->value() - 100);
        }
    });

    m_scrollRightBtn = new QToolButton(m_tabStrip);
    m_scrollRightBtn->setFixedSize(24, 24);
    m_scrollRightBtn->setToolTip("Scroll tabs right");
    m_scrollRightBtn->setIcon(chromeSvgIcon(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" stroke-width=\"2\" stroke-linecap=\"round\"><polyline points=\"9 18 15 12 9 6\"/></svg>", 14, "transparent"));
    m_scrollRightBtn->setVisible(false);
    connect(m_scrollRightBtn, &QToolButton::clicked, this, [this]() {
        if (m_tabScrollArea) {
            m_tabScrollArea->horizontalScrollBar()->setValue(
                m_tabScrollArea->horizontalScrollBar()->value() + 100);
        }
    });

    // Connect scroll bar range changes to update button visibility
    auto *hScrollBar = m_tabScrollArea->horizontalScrollBar();
    connect(hScrollBar, &QScrollBar::rangeChanged, this, [this](int min, int max) {
        if (m_scrollLeftBtn && m_scrollRightBtn) {
            bool canScrollLeft = hScrollBar->value() > min;
            bool canScrollRight = hScrollBar->value() < max;
            m_scrollLeftBtn->setVisible(canScrollLeft);
            m_scrollRightBtn->setVisible(canScrollRight);
        }
    });
    // Also update on value change (for keyboard/automatic scrolling)
    connect(hScrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        if (m_scrollLeftBtn && m_scrollRightBtn) {
            int min = hScrollBar->minimum();
            int max = hScrollBar->maximum();
            m_scrollLeftBtn->setVisible(value > min);
            m_scrollRightBtn->setVisible(value < max);
        }
    });

    m_stripLayout->addWidget(m_scrollLeftBtn);
    m_stripLayout->addWidget(m_tabScrollArea, 1);
    m_stripLayout->addWidget(m_scrollRightBtn);

    m_newTabButton = new QToolButton(m_tabStrip);
    m_newTabButton->setFixedSize(24, 24);
    m_newTabButton->setToolTip(QStringLiteral("New Tab"));
    connect(m_newTabButton, &QToolButton::clicked, m_window, &BrowserWindow::addTabAction);
    m_stripLayout->addWidget(m_newTabButton);

    // Windows/Linux window controls sit on the far right of the strip
    // (Chrome titlebar geometry). Empty on macOS, where the traffic lights
    // stay on the left via m_trafficLayout.
    m_windowCtlLayout = new QHBoxLayout;
    m_windowCtlLayout->setContentsMargins(0, 0, 0, 0);
    m_windowCtlLayout->setSpacing(0);
    m_stripLayout->addLayout(m_windowCtlLayout);

    // Chrome reuses the shared Safari toolbar; it is restyled, not rebuilt.
    // Insert the chrome tab strip at the very top of the window.
    rootLayout->insertWidget(0, m_tabStrip);
    m_tabStrip->setVisible(false);
}

private:
    QScrollArea *m_tabScrollArea = nullptr;
    QToolButton *m_scrollLeftBtn = nullptr;
    QToolButton *m_scrollRightBtn = nullptr;

void ChromeLayer::setChromeMode(bool chrome)
{
    if (m_chrome == chrome)
        return;
    m_chrome = chrome;

    // Move the window controls between the Safari toolbar and the chrome strip.
    // They always sit inside exactly one traffic-light / window-control layout.
    // macOS keeps the colored traffic lights on the left of the strip; on
    // Windows/Linux they move to the far right as flat Chrome-style glyphs.
#if defined(Q_OS_MAC)
    QHBoxLayout *srcLayout  = chrome ? m_window->m_trafficLayout : m_trafficLayout;
    QHBoxLayout *destLayout = chrome ? m_trafficLayout : m_window->m_trafficLayout;
#else
    QHBoxLayout *srcLayout  = chrome ? m_window->m_trafficLayout : m_windowCtlLayout;
    QHBoxLayout *destLayout = chrome ? m_windowCtlLayout : m_window->m_trafficLayout;
#endif
    QWidget *destParent = chrome ? static_cast<QWidget*>(m_tabStrip) : m_window->m_toolbar;
    for (QToolButton *btn : { m_window->m_closeButton,
                              m_window->m_minimizeButton,
                              m_window->m_maximizeButton }) {
        if (!btn)
            continue;
        if (srcLayout)
            srcLayout->removeWidget(btn);
        btn->setParent(destParent);
        if (destLayout)
            destLayout->addWidget(btn, 0);
    }

#if !defined(Q_OS_MAC)
    applyWindowControlStyles(chrome);
#endif

    m_tabStrip->setVisible(chrome);
    if (m_window->m_tabBar)
        m_window->m_tabBar->setVisible(!chrome);

    // Chrome docks the sidebar as a flush panel instead of a floating card.
    m_window->applySidebarLayout(chrome);

    // Chrome surfaces Settings in the toolbar (Safari keeps it hidden).
    if (m_window->m_settingsButton) {
        if (chrome) {
            if (m_window->m_settingsButton->parentWidget() != m_window->m_toolbar) {
                m_window->m_settingsButton->setParent(m_window->m_toolbar);
                m_window->m_toolbarLayout->addWidget(m_window->m_settingsButton);
            }
            m_window->m_settingsButton->show();
        } else {
            m_window->m_toolbarLayout->removeWidget(m_window->m_settingsButton);
            m_window->m_settingsButton->setParent(m_window);
            m_window->m_settingsButton->hide();
        }
    }
}

void ChromeLayer::applyWindowControlStyles(bool chrome)
{
#if !defined(Q_OS_MAC)
    if (chrome) {
        // Flat Chrome titlebar controls: 46px-wide hit targets, glyph icons,
        // hover highlight, red hover on close.
        const QString &text = m_palette.textSecondary;
        m_window->m_minimizeButton->setFixedSize(46, 38);
        m_window->m_maximizeButton->setFixedSize(46, 38);
        m_window->m_closeButton->setFixedSize(46, 38);
        m_window->m_minimizeButton->setIcon(chromeSvgIcon(svgWinMinimize(), 12, text));
        m_window->m_maximizeButton->setIcon(chromeSvgIcon(svgWinMaximize(), 12, text));
        m_window->m_closeButton->setIcon(chromeSvgIcon(svgWinClose(), 12, text));
        const QString hover = m_palette.inactiveTabHover;
        m_window->m_minimizeButton->setStyleSheet(QString(
            "QToolButton { border: none; background: transparent; border-radius: 0; }"
            "QToolButton:hover { background-color: %1; }").arg(hover));
        m_window->m_maximizeButton->setStyleSheet(QString(
            "QToolButton { border: none; background: transparent; border-radius: 0; }"
            "QToolButton:hover { background-color: %1; }").arg(hover));
        m_window->m_closeButton->setStyleSheet(QString(
            "QToolButton { border: none; background: transparent; border-radius: 0; }"
            "QToolButton:hover { background-color: #e81123; }"));
    } else {
        // Restore the macOS-style circles for Safari mode.
        m_window->m_closeButton->setFixedSize(12, 12);
        m_window->m_minimizeButton->setFixedSize(12, 12);
        m_window->m_maximizeButton->setFixedSize(12, 12);
        m_window->m_closeButton->setIcon(QIcon());
        m_window->m_minimizeButton->setIcon(QIcon());
        m_window->m_maximizeButton->setIcon(QIcon());
        m_window->m_closeButton->setStyleSheet(macTrafficStyle(QStringLiteral("#ff5f56"), QStringLiteral("#e0443e")));
        m_window->m_minimizeButton->setStyleSheet(macTrafficStyle(QStringLiteral("#ffbd2e"), QStringLiteral("#dea124")));
        m_window->m_maximizeButton->setStyleSheet(macTrafficStyle(QStringLiteral("#27c93f"), QStringLiteral("#1aab29")));
    }
#endif
}

void ChromeLayer::applyTheme(const ChromePalette &palette)
{
    m_palette = palette;

    m_tabStrip->setStyleSheet(QString(
        "#ChromeTabStrip { background-color: %1; border-bottom: 0.5px solid %2; }"
    ).arg(palette.tabStripBg, palette.border));

    m_newTabButton->setIcon(chromeSvgIcon(chromePlusIcon(), 14, palette.textSecondary));
    m_newTabButton->setStyleSheet(QString(
        "QToolButton { border: none; background: transparent; border-radius: 6px; padding: 0; }"
        "QToolButton:hover { background-color: %1; }"
    ).arg(palette.inactiveTabHover));

#if !defined(Q_OS_MAC)
    // While Chrome mode is active, refresh the window-control glyphs so they
    // pick up the new palette (setChromeMode early-returns when the mode is
    // unchanged, so it never re-applies them on a theme switch).
    if (m_chrome)
        applyWindowControlStyles(true);
#endif
}
