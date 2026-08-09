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

} // namespace

ChromeLayer::ChromeLayer(BrowserWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_chrome(false)
    , m_tabStrip(nullptr)
    , m_stripLayout(nullptr)
    , m_trafficLayout(nullptr)
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

    // Tab area: [tabs...][new-tab button]. rebuildTabBar() fills the tabs.
    m_tabLayout = new QHBoxLayout;
    m_tabLayout->setContentsMargins(0, 0, 0, 0);
    m_tabLayout->setSpacing(1);
    m_tabLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_stripLayout->addLayout(m_tabLayout, 1);

    m_newTabButton = new QToolButton(m_tabStrip);
    m_newTabButton->setFixedSize(24, 24);
    m_newTabButton->setToolTip(QStringLiteral("New Tab"));
    connect(m_newTabButton, &QToolButton::clicked, m_window, &BrowserWindow::addTabAction);
    m_tabLayout->addWidget(m_newTabButton);

    // Chrome reuses the shared Safari toolbar; it is restyled, not rebuilt.
    // Insert the chrome tab strip at the very top of the window.
    rootLayout->insertWidget(0, m_tabStrip);
    m_tabStrip->setVisible(false);
}

void ChromeLayer::setChromeMode(bool chrome)
{
    if (m_chrome == chrome)
        return;
    m_chrome = chrome;

    // Move the traffic lights between the Safari toolbar and the chrome strip.
    // They always sit inside one of the two traffic-light layouts.
    QWidget *destParent = chrome ? static_cast<QWidget*>(m_tabStrip) : m_window->m_toolbar;
    QHBoxLayout *srcLayout = chrome ? m_window->m_trafficLayout : m_trafficLayout;
    QHBoxLayout *destLayout = chrome ? m_trafficLayout : m_window->m_trafficLayout;
    for (QToolButton *btn : { m_window->m_closeButton,
                              m_window->m_minimizeButton,
                              m_window->m_maximizeButton }) {
        if (!btn)
            continue;
        if (srcLayout)
            srcLayout->removeWidget(btn);
        btn->setParent(destParent);
        if (destLayout)
            destLayout->addWidget(btn);
    }

    m_tabStrip->setVisible(chrome);
    if (m_window->m_tabBar)
        m_window->m_tabBar->setVisible(!chrome);

    // Chrome surfaces Settings in the toolbar (Safari keeps it hidden).
    if (m_window->m_settingsButton) {
        if (chrome) {
            if (m_window->m_settingsButton->parentWidget() != m_window->m_toolbar) {
                m_window->m_settingsButton->setParent(m_window->m_toolbar);
                m_window->m_toolbarLayout->addWidget(m_window->m_settingsButton);
            }
        } else {
            m_window->m_toolbarLayout->removeWidget(m_window->m_settingsButton);
            m_window->m_settingsButton->setParent(m_window);
        }
    }

    // Rebuild tabs into whichever strip is now active.
    m_window->rebuildTabBar();
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
}
