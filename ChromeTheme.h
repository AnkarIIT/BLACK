#ifndef CHROMETHEME_H
#define CHROMETHEME_H

#include <QString>

// Chrome-classic palette tokens, kept completely separate from the Safari
// palette so switching UI layouts never bleeds Chrome colors into Safari.
struct ChromePalette {
    QString windowBg;        // central window / page backdrop
    QString toolbarBg;       // chrome toolbar row behind the omnibox
    QString tabStripBg;      // horizontal tab strip at the top
    QString activeTabBg;     // foreground tab
    QString inactiveTabBg;   // background tabs
    QString inactiveTabHover;
    QString omniboxBg;
    QString omniboxBorder;
    QString textPrimary;
    QString textSecondary;
    QString textTertiary;
    QString accent;
    QString accentHover;
    QString border;
    QString borderLight;
    QString hover;
    QString selectedBg;
    QString scrim;
    QString pageBackground;
};

namespace ChromeTheme {

ChromePalette light();
ChromePalette dark();

} // namespace ChromeTheme

#endif // CHROMETHEME_H
