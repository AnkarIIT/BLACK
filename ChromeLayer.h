#ifndef CHROMELAYER_H
#define CHROMELAYER_H

#include <QObject>
#include <QString>

#include "ChromeTheme.h"

class QWidget;
class QHBoxLayout;
class QVBoxLayout;
class QToolButton;
class BrowserWindow;

// Classic Chrome UI layer. Built alongside the Safari layer so a single mode
// gate can swap between them without tearing down the Safari widgets.
//
// Owns the Chrome-specific chrome:
//   - a top horizontal tab strip: [traffic lights][tabs...][new-tab button]
//   - a restyle of the shared Safari toolbar (omnibox + nav buttons)
//
// Shared behavior (tabs, navigation, sidebar, suggestions, ...) stays in
// BrowserWindow; this class only arranges and styles the shared widgets.
class ChromeLayer : public QObject
{
    Q_OBJECT

public:
    explicit ChromeLayer(BrowserWindow *window, QObject *parent = nullptr);

    // Creates the chrome tab strip, hidden by default.
    void setupUi(QWidget *central, QVBoxLayout *rootLayout);

    // Single mode gate: moves the traffic lights between the Safari toolbar
    // and the chrome tab strip, toggles layer visibility, surfaces Settings in
    // the chrome toolbar, and rebuilds the tab bar into whichever strip is
    // active. The caller (BrowserWindow::applyUiLayout) keeps the web-page
    // data-ui-layout attribute and omnibox sizing in sync.
    void setChromeMode(bool chrome);
    bool chromeMode() const { return m_chrome; }

    // Applies the given Chrome palette to every widget the layer manages.
    void applyTheme(const ChromePalette &palette);
    const ChromePalette &palette() const { return m_palette; }

    // Accessors used by BrowserWindow::rebuildTabBar().
    QWidget *tabStrip() const { return m_tabStrip; }
    QHBoxLayout *tabLayout() const { return m_tabLayout; }
    QToolButton *newTabButton() const { return m_newTabButton; }

private:
    BrowserWindow *m_window;
    bool m_chrome;
    ChromePalette m_palette;

    QWidget     *m_tabStrip;
    QHBoxLayout *m_stripLayout;    // tab strip root: [traffic lights][tabs]
    QHBoxLayout *m_trafficLayout;  // traffic-light container inside the strip
    QHBoxLayout *m_tabLayout;      // [tabs...][new-tab button]
    QToolButton *m_newTabButton;
};

#endif // CHROMELAYER_H
