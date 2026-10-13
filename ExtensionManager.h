#ifndef EXTENSIONMANAGER_H
#define EXTENSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QSet>
#include <memory>
#include <functional>

class QWebEngineScript;
class QWebEngineProfile;
class ExtensionContext;

using MessageListener = std::function<void(const QJsonObject&)>;

struct BlockingRule {
    QStringList blockedHosts;
    QStringList blockedPaths;
    QStringList blockedTypes;
};

struct ExtensionManifestV3 {
    QString name;
    QString version;
    QString description;
    QString manifestVersion = "3";
    QString defaultLocale;
    QJsonObject icons;
    QJsonObject action;
    QJsonObject background;
    QJsonArray contentScripts;
    QJsonArray permissions;
    QJsonArray optionalPermissions;
    QJsonArray hostPermissions;
    QJsonArray optionalHostPermissions;
    QJsonObject declarativeNetRequest;
    QJsonObject optionsPage;
    QJsonObject optionsUi;
    QJsonObject omnibox;
    QJsonObject commands;
    QJsonObject devtoolsPage;
    QJsonObject homepageUrl;
    QJsonObject updateUrl;
    QJsonObject minimumChromeVersion;
    QJsonObject exportObj;
    QJsonObject key;
    QJsonObject sandbox;
    QJsonObject sidePanel;
    QJsonArray webAccessibleResources;
};

struct ExtensionInfo {
    QString id;
    QString name;
    QString version;
    QString description;
    QString path;
    ExtensionManifestV3 manifest;
    // Declarative network-blocking rules contributed by this extension's
    // manifest (activeBlockingRules() aggregates these, and manifest parsing
    // populates them at load time).
    BlockingRule blockingRules;
    bool enabled = true;
    bool loaded = false;
    QDateTime installTime;
    QDateTime lastUpdateTime;
    ExtensionContext *context = nullptr;
};

class ExtensionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString json READ json NOTIFY changed)

public:
    explicit ExtensionManager(QObject *parent = nullptr);

    static ExtensionManager &instance();

    QString json() const;
    const QList<ExtensionInfo> &extensions() const { return m_extensions; }
    
    ExtensionInfo* getExtension(const QString &id);
    ExtensionContext* getContext(const QString &id);

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool loadExtension(const QString &id);
    Q_INVOKABLE bool unloadExtension(const QString &id);
    Q_INVOKABLE bool enableExtension(const QString &id);
    Q_INVOKABLE bool disableExtension(const QString &id);
    Q_INVOKABLE QString installExtension(const QString &path);
    Q_INVOKABLE bool uninstallExtension(const QString &id);

    QList<QWebEngineScript> buildScripts() const;
    QList<BlockingRule> activeBlockingRules() const;
    
    Q_INVOKABLE QJsonObject callApi(const QString &extensionId, const QString &apiName, const QString &method, const QJsonArray &args);
    Q_INVOKABLE void registerMessageListener(const QString &extensionId, const QString &apiName, const QString &eventName, MessageListener listener);
    
    QWebEngineProfile* defaultProfile() const;

signals:
    void changed();
    void extensionLoaded(const QString &id);
    void extensionUnloaded(const QString &id);
    void extensionInstalled(const QString &id);
    void extensionUninstalled(const QString &id);

private:
    void scan();
    bool parseManifest(const QString &path, ExtensionManifestV3 &manifest);
    bool validateManifest(const ExtensionManifestV3 &manifest);
    ExtensionInfo createExtensionInfo(const QString &id, const QString &path);
    void setupExtensionContext(ExtensionInfo *info);
    void teardownExtensionContext(ExtensionInfo *info);
    
    QList<ExtensionInfo> m_extensions;
    QMap<QString, std::shared_ptr<ExtensionContext>> m_contexts;
    QWebEngineProfile *m_profile = nullptr;
};

#endif