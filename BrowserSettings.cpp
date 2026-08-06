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
    obj.insert(kGeneralKey, general);
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
    }
}

void BrowserSettings::openDefaultBrowserSettings()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
}
