#include "BrowserSettings.h"
#include "OSPaths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

namespace {
const QLatin1String kGeneralKey("general");
const QLatin1String kThemeKey("theme");
const QLatin1String kTabsKey("tabs");
}

int BrowserSettings::autoCloseDays(const QString &setting)
{
    if (setting == QLatin1String("After one day"))   return 1;
    if (setting == QLatin1String("After one week"))  return 7;
    if (setting == QLatin1String("After two weeks")) return 14;
    if (setting == QLatin1String("After one month")) return 30;
    return 0; // "Manually" or unknown
}

BrowserSettings &BrowserSettings::instance()
{
    static BrowserSettings settings;
    return settings;
}

BrowserSettings::BrowserSettings(QObject *parent)
    : QObject(parent)
    , m_searchEngine(QStringLiteral("Google"))
    , m_opensWith(QStringLiteral("All windows from last session"))
    , m_newWindowsWith(QStringLiteral("Start Page"))
    , m_newTabsWith(QStringLiteral("Start Page"))
    , m_removeHistoryItems(QStringLiteral("After one year"))
    , m_removeDownloadListItems(QStringLiteral("After one day"))
    , m_openSafeFiles(false)
    , m_homepage(QString())
    , m_downloadLocation(QStringLiteral("Downloads"))
    , m_tabLayout(QStringLiteral("Separate"))
    , m_showTabTitles(true)
    , m_openPagesInTabs(QStringLiteral("Automatically"))
    , m_autoCloseTabs(QStringLiteral("Manually"))
    , m_activateNewTabs(true)
    , m_translationTargetLanguage(QStringLiteral("en"))
{
    load();
}

QString BrowserSettings::settingsFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1String("/settings.json");
}

QJsonObject BrowserSettings::readSettingsObject()
{
    QFile file(settingsFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            return doc.object();
    }
    return QJsonObject();
}

bool BrowserSettings::writeSettingsObject(const QJsonObject &obj)
{
    if (!OSPaths::writeFileAtomic(settingsFilePath(), QJsonDocument(obj).toJson())) {
        qWarning() << "Failed to save settings";
        return false;
    }
    return true;
}

void BrowserSettings::load()
{
    const QJsonObject obj = readSettingsObject();
    const QJsonObject general = obj.value(kGeneralKey).toObject();
    m_searchEngine            = general.value(QStringLiteral("searchEngine")).toString(m_searchEngine);
    m_opensWith               = general.value(QStringLiteral("opensWith")).toString(m_opensWith);
    m_newWindowsWith          = general.value(QStringLiteral("newWindowsWith")).toString(m_newWindowsWith);
    m_newTabsWith             = general.value(QStringLiteral("newTabsWith")).toString(m_newTabsWith);
    m_removeHistoryItems      = general.value(QStringLiteral("removeHistoryItems")).toString(m_removeHistoryItems);
    m_removeDownloadListItems = general.value(QStringLiteral("removeDownloadListItems")).toString(m_removeDownloadListItems);
    m_openSafeFiles           = general.value(QStringLiteral("openSafeFiles")).toBool(m_openSafeFiles);
    m_homepage                = general.value(QStringLiteral("homepage")).toString(m_homepage);
    m_downloadLocation        = general.value(QStringLiteral("downloadLocation")).toString(m_downloadLocation);
    m_preloadFavicons         = general.value(QStringLiteral("preloadFavicons")).toBool(m_preloadFavicons);

    const QJsonObject appearance = obj.value(QStringLiteral("appearance")).toObject();
    m_uiLayout = static_cast<UiLayout>(appearance.value(QStringLiteral("uiLayout")).toInt(m_uiLayout));

    const QJsonObject tabs = obj.value(kTabsKey).toObject();
    m_tabLayout             = tabs.value(QStringLiteral("tabLayout")).toString(m_tabLayout);
    m_showTabTitles         = tabs.value(QStringLiteral("showTabTitles")).toBool(m_showTabTitles);
    m_openPagesInTabs       = tabs.value(QStringLiteral("openPagesInTabs")).toString(m_openPagesInTabs);
    m_autoCloseTabs         = tabs.value(QStringLiteral("autoCloseTabs")).toString(m_autoCloseTabs);
    m_activateNewTabs       = tabs.value(QStringLiteral("activateNewTabs")).toBool(m_activateNewTabs);

    const QJsonObject translation = obj.value(QStringLiteral("translation")).toObject();
    m_translationTargetLanguage = translation.value(QStringLiteral("targetLanguage")).toString(m_translationTargetLanguage);
}

void BrowserSettings::save()
{
    QJsonObject obj = readSettingsObject();
    QJsonObject general;
    general.insert(QStringLiteral("searchEngine"), m_searchEngine);
    general.insert(QStringLiteral("opensWith"), m_opensWith);
    general.insert(QStringLiteral("newWindowsWith"), m_newWindowsWith);
    general.insert(QStringLiteral("newTabsWith"), m_newTabsWith);
    general.insert(QStringLiteral("removeHistoryItems"), m_removeHistoryItems);
    general.insert(QStringLiteral("removeDownloadListItems"), m_removeDownloadListItems);
    general.insert(QStringLiteral("openSafeFiles"), m_openSafeFiles);
    general.insert(QStringLiteral("homepage"), m_homepage);
    general.insert(QStringLiteral("downloadLocation"), m_downloadLocation);
    general.insert(QStringLiteral("preloadFavicons"), m_preloadFavicons);
    obj.insert(kGeneralKey, general);

    QJsonObject appearance;
    appearance.insert(QStringLiteral("uiLayout"), static_cast<int>(m_uiLayout));
    obj.insert(QStringLiteral("appearance"), appearance);

    QJsonObject tabs;
    tabs.insert(QStringLiteral("tabLayout"), m_tabLayout);
    tabs.insert(QStringLiteral("showTabTitles"), m_showTabTitles);
    tabs.insert(QStringLiteral("openPagesInTabs"), m_openPagesInTabs);
    tabs.insert(QStringLiteral("autoCloseTabs"), m_autoCloseTabs);
    tabs.insert(QStringLiteral("activateNewTabs"), m_activateNewTabs);
    obj.insert(kTabsKey, tabs);

    QJsonObject translation;
    translation.insert(QStringLiteral("targetLanguage"), m_translationTargetLanguage);
    obj.insert(QStringLiteral("translation"), translation);

    if (!writeSettingsObject(obj)) {
        return;
    }
}

void BrowserSettings::setValue(const QString &key, const QString &value)
{
    bool changed = false;
    if (key == QLatin1String("searchEngine") && m_searchEngine != value) {
        m_searchEngine = value;
        changed = true;
    } else if (key == QLatin1String("opensWith") && m_opensWith != value) {
        m_opensWith = value;
        changed = true;
    } else if (key == QLatin1String("newWindowsWith") && m_newWindowsWith != value) {
        m_newWindowsWith = value;
        changed = true;
    } else if (key == QLatin1String("newTabsWith") && m_newTabsWith != value) {
        m_newTabsWith = value;
        changed = true;
    } else if (key == QLatin1String("removeHistoryItems") && m_removeHistoryItems != value) {
        m_removeHistoryItems = value;
        changed = true;
    } else if (key == QLatin1String("removeDownloadListItems") && m_removeDownloadListItems != value) {
        m_removeDownloadListItems = value;
        changed = true;
    } else if (key == QLatin1String("homepage") && m_homepage != value) {
        m_homepage = value;
        changed = true;
    } else if (key == QLatin1String("downloadLocation") && m_downloadLocation != value) {
        m_downloadLocation = value;
        changed = true;
    } else if (key == QLatin1String("tabLayout") && m_tabLayout != value) {
        m_tabLayout = value;
        changed = true;
    } else if (key == QLatin1String("openPagesInTabs") && m_openPagesInTabs != value) {
        m_openPagesInTabs = value;
        changed = true;
    } else if (key == QLatin1String("autoCloseTabs") && m_autoCloseTabs != value) {
        m_autoCloseTabs = value;
        changed = true;
    } else if (key == QLatin1String("translationTargetLanguage") && m_translationTargetLanguage != value) {
        m_translationTargetLanguage = value;
        changed = true;
    }
    if (changed) {
        save();
        emit settingsChanged();
    }
}

void BrowserSettings::setBool(const QString &key, bool value)
{
    bool changed = false;
    if (key == QLatin1String("openSafeFiles") && m_openSafeFiles != value) {
        m_openSafeFiles = value;
        changed = true;
    } else if (key == QLatin1String("showTabTitles") && m_showTabTitles != value) {
        m_showTabTitles = value;
        changed = true;
    } else if (key == QLatin1String("activateNewTabs") && m_activateNewTabs != value) {
        m_activateNewTabs = value;
        changed = true;
    } else if (key == QLatin1String("preloadFavicons") && m_preloadFavicons != value) {
        m_preloadFavicons = value;
        changed = true;
    } else if (key == QLatin1String("crashReportingConsent")) {
        // Store in a separate section
        QJsonObject obj = readSettingsObject();
        QJsonObject privacy = obj.value("privacy").toObject();
        privacy["crashReportingConsent"] = value;
        obj["privacy"] = privacy;
        if (!writeSettingsObject(obj)) {
            return;
        }
        changed = true;
    }
    if (changed) {
        save();
        emit settingsChanged();
    }
}

QVariant BrowserSettings::value(const QString &key, const QVariant &defaultValue) const
{
    QJsonObject obj = readSettingsObject();
    
    if (key == QLatin1String("crashReportingConsent")) {
        QJsonObject privacy = obj.value("privacy").toObject();
        return privacy.value("crashReportingConsent").toBool(false);
    }
    
    const QJsonObject general = obj.value("general").toObject();
    if (general.contains(key)) {
        return general[key].toVariant();
    }
    
    const QJsonObject appearance = obj.value("appearance").toObject();
    if (appearance.contains(key)) {
        return appearance[key].toVariant();
    }
    
    const QJsonObject tabs = obj.value("tabs").toObject();
    if (tabs.contains(key)) {
        return tabs[key].toVariant();
    }
    
    const QJsonObject privacy = obj.value("privacy").toObject();
    if (privacy.contains(key)) {
        return privacy[key].toVariant();
    }
    
    return defaultValue;
}

void BrowserSettings::setUiLayout(UiLayout mode)
{
    if (m_uiLayout != mode) {
        m_uiLayout = mode;
        save();
        // save() will only emit settingsChanged on success
    }
}

void BrowserSettings::openDefaultBrowserSettings()
{
    OSPaths::openDefaultBrowserSettings();
}

QString BrowserSettings::getShortcut(const QString &action) const
{
    // Default shortcuts
    static const QMap<QString, QString> defaults = {
        { "newTab", "Ctrl+T" },
        { "closeTab", "Ctrl+W" },
        { "reopenClosedTab", "Ctrl+Shift+T" },
        { "focusUrlBar", "Ctrl+L" },
        { "findInPage", "Ctrl+F" },
        { "reload", "Ctrl+R" },
        { "hardReload", "Ctrl+Shift+R" },
        { "newPrivateWindow", "Ctrl+Shift+N" },
        { "toggleSidebar", "Ctrl+Shift+L" },
        { "showSettings", "Ctrl+," },
        { "nextTab", "Ctrl+Tab" },
        { "prevTab", "Ctrl+Shift+Tab" },
        { "zoomIn", "Ctrl+Plus" },
        { "zoomOut", "Ctrl+Minus" },
        { "zoomReset", "Ctrl+0" },
        { "back", "Alt+Left" },
        { "forward", "Alt+Right" },
        { "home", "Alt+Home" },
        { "fullscreen", "F11" },
        { "print", "Ctrl+P" },
        { "saveAsPdf", "Ctrl+Shift+P" },
        { "devTools", "Ctrl+Shift+I" },
    };

    const QJsonObject obj = readSettingsObject();
    const QJsonObject shortcuts = obj.value(QStringLiteral("shortcuts")).toObject();
    if (shortcuts.contains(action))
        return shortcuts.value(action).toString();

    return defaults.value(action, QString());
}

void BrowserSettings::setShortcut(const QString &action, const QString &shortcut)
{
    QJsonObject obj = readSettingsObject();
    QJsonObject shortcuts = obj.value(QStringLiteral("shortcuts")).toObject();
    shortcuts.insert(action, shortcut);
    obj.insert(QStringLiteral("shortcuts"), shortcuts);
    if (!writeSettingsObject(obj)) {
        return;
    }
    emit settingsChanged();
}

void BrowserSettings::resetShortcut(const QString &action)
{
    QJsonObject obj = readSettingsObject();
    QJsonObject shortcuts = obj.value(QStringLiteral("shortcuts")).toObject();
    if (shortcuts.remove(action)) {
        obj.insert(QStringLiteral("shortcuts"), shortcuts);
        if (!writeSettingsObject(obj)) {
            return;
        }
        emit settingsChanged();
    }
}
