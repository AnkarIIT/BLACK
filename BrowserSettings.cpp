#include "BrowserSettings.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

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
    , m_openSafeFiles(true)
    , m_homepage(QString())
    , m_downloadLocation(QStringLiteral("Downloads"))
    , m_tabLayout(QStringLiteral("Separate"))
    , m_showTabTitles(true)
    , m_openPagesInTabs(QStringLiteral("Automatically"))
    , m_autoCloseTabs(QStringLiteral("Manually"))
    , m_activateNewTabs(true)
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

void BrowserSettings::writeSettingsObject(const QJsonObject &obj)
{
    QFile file(settingsFilePath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson());
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

    const QJsonObject tabs = obj.value(kTabsKey).toObject();
    m_tabLayout             = tabs.value(QStringLiteral("tabLayout")).toString(m_tabLayout);
    m_showTabTitles         = tabs.value(QStringLiteral("showTabTitles")).toBool(m_showTabTitles);
    m_openPagesInTabs       = tabs.value(QStringLiteral("openPagesInTabs")).toString(m_openPagesInTabs);
    m_autoCloseTabs         = tabs.value(QStringLiteral("autoCloseTabs")).toString(m_autoCloseTabs);
    m_activateNewTabs       = tabs.value(QStringLiteral("activateNewTabs")).toBool(m_activateNewTabs);
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
    obj.insert(kGeneralKey, general);

    QJsonObject tabs;
    tabs.insert(QStringLiteral("tabLayout"), m_tabLayout);
    tabs.insert(QStringLiteral("showTabTitles"), m_showTabTitles);
    tabs.insert(QStringLiteral("openPagesInTabs"), m_openPagesInTabs);
    tabs.insert(QStringLiteral("autoCloseTabs"), m_autoCloseTabs);
    tabs.insert(QStringLiteral("activateNewTabs"), m_activateNewTabs);
    obj.insert(kTabsKey, tabs);

    writeSettingsObject(obj);
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
    }
    if (changed) {
        save();
        emit settingsChanged();
    }
}

void BrowserSettings::setBool(const QString &key, bool value)
{
    if (key == QLatin1String("openSafeFiles") && m_openSafeFiles != value) {
        m_openSafeFiles = value;
        save();
        emit settingsChanged();
    } else if (key == QLatin1String("showTabTitles") && m_showTabTitles != value) {
        m_showTabTitles = value;
        save();
        emit settingsChanged();
    } else if (key == QLatin1String("activateNewTabs") && m_activateNewTabs != value) {
        m_activateNewTabs = value;
        save();
        emit settingsChanged();
    }
}

void BrowserSettings::openDefaultBrowserSettings()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
}
