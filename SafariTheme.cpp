#include "SafariTheme.h"

#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>
#include <QtGlobal>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

namespace {

QString settingsFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1String("/settings.json");
}

SafariTheme::Preference preferenceFromString(const QString &value)
{
    if (value == QLatin1String("dark"))
        return SafariTheme::Preference::Dark;
    if (value == QLatin1String("light"))
        return SafariTheme::Preference::Light;
    return SafariTheme::Preference::System;
}

QString preferenceToString(SafariTheme::Preference preference)
{
    switch (preference) {
    case SafariTheme::Preference::Dark:  return QStringLiteral("dark");
    case SafariTheme::Preference::Light: return QStringLiteral("light");
    default:                             return QStringLiteral("system");
    }
}

} // namespace

SafariTheme &SafariTheme::instance()
{
    static SafariTheme theme;
    return theme;
}

SafariTheme::SafariTheme(QObject *parent)
    : QObject(parent)
    , m_scheme(Scheme::Light)
    , m_preference(Preference::System)
{
    QFile file(settingsFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            m_preference = preferenceFromString(doc.object().value(QStringLiteral("theme")).toString());
    }
    refreshScheme();
}

void SafariTheme::refreshScheme()
{
    Scheme effective;
    switch (m_preference) {
    case Preference::Dark:
        effective = Scheme::Dark;
        break;
    case Preference::Light:
        effective = Scheme::Light;
        break;
    default:
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        effective = (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
                        ? Scheme::Dark : Scheme::Light;
#else
        effective = (QGuiApplication::palette().color(QPalette::Window).value() < 128)
                        ? Scheme::Dark : Scheme::Light;
#endif
        break;
    }
    setScheme(effective);
}

void SafariTheme::setPreference(Preference preference)
{
    if (m_preference == preference)
        return;
    m_preference = preference;

    // Read-modify-write so the "general" settings block (BrowserSettings) is preserved.
    QJsonObject obj;
    QFile file(settingsFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            obj = doc.object();
        file.close();
    }
    obj.insert(QStringLiteral("theme"), preferenceToString(preference));
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson());

    refreshScheme();
}

void SafariTheme::setThemePreference(int preference)
{
    if (preference < 0 || preference > 2)
        return;
    setPreference(static_cast<Preference>(preference));
}

void SafariTheme::setScheme(Scheme scheme)
{
    const bool changed = (m_scheme != scheme);
    m_scheme = scheme;
    if (changed || !m_paletteInitialized) {
        loadPalette();
        m_paletteInitialized = true;
    }
    if (changed) {
        emit schemeChanged();
        emit themeSchemeChanged();
    }
}

void SafariTheme::loadPalette()
{
    if (m_scheme == Scheme::Dark) {
        // Safari 17.5/18 Dark Mode colors - matching Chrome dark theme
        bgWindow      = QStringLiteral("#1e1e1e");      // Darker window background
        bgToolbar     = QStringLiteral("#2d2d2d");      // Toolbar with slight highlight
        bgTabBar      = QStringLiteral("#2d2d2d");      // Tab bar matching toolbar
        bgUrlBar      = QStringLiteral("#3c3c3c");      // URL bar (slightly lighter)
        bgSidebar     = QStringLiteral("#252526");      // Sidebar background (darker)
        tabActive     = QStringLiteral("rgba(255,255,255,0.16)");      // Active tab indicator
        tabInactive   = QStringLiteral("transparent");
        tabHover      = QStringLiteral("rgba(255,255,255,0.08)");
        cardBg        = QStringLiteral("#303030");
        textPrimary   = QStringLiteral("#f5f5f7");
        textSecondary = QStringLiteral("#b5b5b5");
        textTertiary  = QStringLiteral("#8a8a8a");
        accent        = QStringLiteral("#0a84ff");
        accentHover   = QStringLiteral("#3395ff");
        border        = QStringLiteral("rgba(255,255,255,0.12)");
        borderLight   = QStringLiteral("rgba(255,255,255,0.06)");
        hover         = QStringLiteral("rgba(255,255,255,0.08)");
        searchBg      = QStringLiteral("rgba(255,255,255,0.10)");
        selectedBg    = QStringLiteral("rgba(0,122,255,0.25)");
        scrim         = QStringLiteral("rgba(0,0,0,0.60)");
        pageBackground = QStringLiteral("#1e1e1e");
    } else {
        // Light mode - Safari/Chrome style
        bgWindow      = QStringLiteral("#f5f5f7");
        bgToolbar     = QStringLiteral("#e8e8ed");
        bgTabBar      = QStringLiteral("#e8e8ed");
        bgUrlBar      = QStringLiteral("#ffffff");
        bgSidebar     = QStringLiteral("#f0f0f2");
        tabActive     = QStringLiteral("rgba(0,0,0,0.10)");
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