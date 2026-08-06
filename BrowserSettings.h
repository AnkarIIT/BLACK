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
    Q_PROPERTY(QString homepage READ homepage NOTIFY settingsChanged)
    Q_PROPERTY(QString downloadLocation READ downloadLocation NOTIFY settingsChanged)
    Q_PROPERTY(QString tabLayout READ tabLayout NOTIFY settingsChanged)
    Q_PROPERTY(bool showTabTitles READ showTabTitles NOTIFY settingsChanged)
    Q_PROPERTY(QString openPagesInTabs READ openPagesInTabs NOTIFY settingsChanged)
    Q_PROPERTY(QString autoCloseTabs READ autoCloseTabs NOTIFY settingsChanged)
    Q_PROPERTY(bool activateNewTabs READ activateNewTabs NOTIFY settingsChanged)

public:
    static BrowserSettings &instance();

    QString searchEngine() const { return m_searchEngine; }
    QString opensWith() const { return m_opensWith; }
    QString newWindowsWith() const { return m_newWindowsWith; }
    QString newTabsWith() const { return m_newTabsWith; }
    QString removeHistoryItems() const { return m_removeHistoryItems; }
    QString removeDownloadListItems() const { return m_removeDownloadListItems; }
    bool openSafeFiles() const { return m_openSafeFiles; }
    QString homepage() const { return m_homepage; }
    QString downloadLocation() const { return m_downloadLocation; }
    QString tabLayout() const { return m_tabLayout; }
    bool showTabTitles() const { return m_showTabTitles; }
    QString openPagesInTabs() const { return m_openPagesInTabs; }
    QString autoCloseTabs() const { return m_autoCloseTabs; }
    bool activateNewTabs() const { return m_activateNewTabs; }

    Q_INVOKABLE void setValue(const QString &key, const QString &value);
    Q_INVOKABLE void setBool(const QString &key, bool value);
    Q_INVOKABLE void openDefaultBrowserSettings();

    static QString settingsFilePath();
    static QJsonObject readSettingsObject();
    static void writeSettingsObject(const QJsonObject &obj);

    // Auto-close policy: number of days after which an inactive tab is dropped
    // at session restore (0 = never).
    static int autoCloseDays(const QString &setting);

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
    QString m_homepage;
    QString m_downloadLocation;
    QString m_tabLayout;
    bool m_showTabTitles;
    QString m_openPagesInTabs;
    QString m_autoCloseTabs;
    bool m_activateNewTabs;
};

#endif
