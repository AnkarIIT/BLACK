#include "ChromeTheme.h"

namespace ChromeTheme {

ChromePalette light()
{
    ChromePalette p;
    p.windowBg         = QLatin1String("#ffffff");
    p.toolbarBg        = QLatin1String("#eef0f2");
    p.tabStripBg       = QLatin1String("#eef0f2");
    p.activeTabBg      = QLatin1String("#ffffff");
    p.inactiveTabBg    = QLatin1String("#e2e4e7");
    p.inactiveTabHover = QLatin1String("#d5d8dc");
    p.omniboxBg        = QLatin1String("#ffffff");
    p.omniboxBorder    = QLatin1String("#d8dadd");
    p.textPrimary      = QLatin1String("#1a1a1a");
    p.textSecondary    = QLatin1String("#5f6368");
    p.textTertiary     = QLatin1String("#9aa0a6");
    p.accent           = QLatin1String("#1a73e8");
    p.accentHover      = QLatin1String("#1765cc");
    p.border           = QLatin1String("rgba(0,0,0,0.14)");
    p.borderLight      = QLatin1String("rgba(0,0,0,0.08)");
    p.hover            = QLatin1String("rgba(60,64,67,0.08)");
    p.selectedBg       = QLatin1String("rgba(26,115,232,0.12)");
    p.scrim            = QLatin1String("rgba(32,33,36,0.45)");
    p.pageBackground   = QLatin1String("#ffffff");
    return p;
}

ChromePalette dark()
{
    ChromePalette p;
    p.windowBg         = QLatin1String("#202124");
    p.toolbarBg        = QLatin1String("#2d2e30");
    p.tabStripBg       = QLatin1String("#2d2e30");
    p.activeTabBg      = QLatin1String("#3c4043");
    p.inactiveTabBg    = QLatin1String("#292a2c");
    p.inactiveTabHover = QLatin1String("#35363a");
    p.omniboxBg        = QLatin1String("#202124");
    p.omniboxBorder    = QLatin1String("#5f6368");
    p.textPrimary      = QLatin1String("#e8eaed");
    p.textSecondary    = QLatin1String("#9aa0a6");
    p.textTertiary     = QLatin1String("#80868b");
    p.accent           = QLatin1String("#8ab4f8");
    p.accentHover      = QLatin1String("#aecbfa");
    p.border           = QLatin1String("rgba(255,255,255,0.14)");
    p.borderLight      = QLatin1String("rgba(255,255,255,0.08)");
    p.hover            = QLatin1String("rgba(232,234,237,0.08)");
    p.selectedBg       = QLatin1String("rgba(138,180,248,0.16)");
    p.scrim            = QLatin1String("rgba(0,0,0,0.55)");
    p.pageBackground   = QLatin1String("#202124");
    return p;
}

} // namespace ChromeTheme
