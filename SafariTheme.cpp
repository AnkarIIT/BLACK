#include "SafariTheme.h"

#include <QGuiApplication>
#include <QStyleHints>

SafariTheme &SafariTheme::instance()
{
    static SafariTheme theme;
    return theme;
}

SafariTheme::SafariTheme(QObject *parent)
    : QObject(parent)
    , m_scheme(Scheme::Light)
{
    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
        m_scheme = Scheme::Dark;
    loadPalette();
}

void SafariTheme::setScheme(Scheme scheme)
{
    if (m_scheme == scheme)
        return;
    m_scheme = scheme;
    loadPalette();
    emit schemeChanged();
}

void SafariTheme::loadPalette()
{
    if (m_scheme == Scheme::Dark) {
        bgWindow      = QStringLiteral("#1e1e20");
        bgToolbar     = QStringLiteral("#2c2c2e");
        bgTabBar      = QStringLiteral("#2c2c2e");
        bgUrlBar      = QStringLiteral("#3a3a3c");
        bgSidebar     = QStringLiteral("#2c2c2e");
        tabActive     = QStringLiteral("#48484a");
        tabInactive   = QStringLiteral("transparent");
        tabHover      = QStringLiteral("rgba(255,255,255,0.10)");
        cardBg        = QStringLiteral("#2c2c2e");
        textPrimary   = QStringLiteral("#f5f5f7");
        textSecondary = QStringLiteral("#98989d");
        textTertiary  = QStringLiteral("#6e6e73");
        accent        = QStringLiteral("#0a84ff");
        accentHover   = QStringLiteral("#3395ff");
        border        = QStringLiteral("rgba(255,255,255,0.12)");
        borderLight   = QStringLiteral("rgba(255,255,255,0.06)");
        hover         = QStringLiteral("rgba(255,255,255,0.08)");
        searchBg      = QStringLiteral("rgba(255,255,255,0.06)");
        selectedBg    = QStringLiteral("rgba(10,132,255,0.30)");
        scrim         = QStringLiteral("rgba(0,0,0,0.6)");
        pageBackground = QStringLiteral("#1e1e20");
    } else {
        bgWindow      = QStringLiteral("#f5f5f7");
        bgToolbar     = QStringLiteral("#e8e8ed");
        bgTabBar      = QStringLiteral("#e8e8ed");
        bgUrlBar      = QStringLiteral("#ffffff");
        bgSidebar     = QStringLiteral("#f0f0f2");
        tabActive     = QStringLiteral("#ffffff");
        tabInactive   = QStringLiteral("transparent");
        tabHover      = QStringLiteral("rgba(0,0,0,0.06)");
        cardBg        = QStringLiteral("#ffffff");
        textPrimary   = QStringLiteral("#1d1d1f");
        textSecondary = QStringLiteral("#86868b");
        textTertiary  = QStringLiteral("#aeaeb2");
        accent        = QStringLiteral("#0066cc");
        accentHover   = QStringLiteral("#005bb5");
        border        = QStringLiteral("rgba(0,0,0,0.10)");
        borderLight   = QStringLiteral("rgba(0,0,0,0.06)");
        hover         = QStringLiteral("rgba(0,0,0,0.06)");
        searchBg      = QStringLiteral("rgba(0,0,0,0.04)");
        selectedBg    = QStringLiteral("rgba(0,102,204,0.15)");
        scrim         = QStringLiteral("rgba(0,0,0,0.4)");
        pageBackground = QStringLiteral("#f5f5f7");
    }
}
