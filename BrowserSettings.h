#ifndef BROWSERSETTINGS_H
#define BROWSERSETTINGS_H

#include <QObject>
#include <QString>

class QJsonObject;

// Holds the Safari-style General settings (Settings > General) and persists
// them to the shared settings.json, without clobbering the theme preference
// managed by SafariTheme.
class BrowserSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString searchEngine READ searchEngine NOTIFY settingsChanged)
    Q_PROPERTY(QString opensWith READ opensWith NOTIFY settingsChanged)
    Q_PROPERTY(QString newWindowsWith READ newWindowsWith NOTIFY settingsChanged)
    Q_PROPERTY(QString newTabsWith READ newTabsWith NOTIFY settingsChanged)
    Q_PROPERTY(QString removeHistoryItems READ removeHistoryItems NOTIFY settingsChanged)
    Q_PROPERTY(QString removeDownloadListItems READ removeDownloadListItems NOTIFY settingsChanged)
    Q_PROPERTY(bool openSafeFiles READ openSafeFiles NOTIFY settingsChanged)

public:
    static BrowserSettings &instance();

    QString searchEngine() const { return m_searchEngine; }
    QString opensWith() const { return m_opensWith; }
    QString newWindowsWith() const { return m_newWindowsWith; }
    QString newTabsWith() const { return m_newTabsWith; }
    QString removeHistoryItems() const { return m_removeHistoryItems; }
    QString removeDownloadListItems() const { return m_removeDownloadListItems; }
    bool openSafeFiles() const { return m_openSafeFiles; }

    Q_INVOKABLE void setValue(const QString &key, const QString &value);
    Q_INVOKABLE void setBool(const QString &key, bool value);
    Q_INVOKABLE void openDefaultBrowserSettings();

    static QString settingsFilePath();
    static QJsonObject readSettingsObject();
    static void writeSettingsObject(const QJsonObject &obj);

signals:
    void settingsChanged();

private:
    explicit BrowserSettings(QObject *parent = nullptr);
    void load();
    void save();

    QString m_searchEngine;
    QString m_opensWith;
    QString m_newWindowsWith;
    QString m_newTabsWith;
    QString m_removeHistoryItems;
    QString m_removeDownloadListItems;
    bool m_openSafeFiles;
};

#endif
