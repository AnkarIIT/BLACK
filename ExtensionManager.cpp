#include "ExtensionManager.h"
#include "ExtensionApi.h"
#include "OSPaths.h"
#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <functional>
#include <QRegularExpression>
#include <QDateTime>
#include <QCryptographicHash>

namespace {

QString extRoot()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("extensions");
}

bool isSafeRelativePath(const QString &p)
{
    if (p.isEmpty())
        return false;
    if (p.startsWith(QLatin1Char('/')) || p.startsWith(QLatin1Char('\\')))
        return false;
    if (p.contains(QLatin1Char('\\')) || p.contains(QLatin1Char(':')))
        return false;
    if (p.contains(QStringLiteral("..")))
        return false;

    QString decoded = QUrl::fromPercentEncoding(p.toUtf8());
    if (decoded.contains(QStringLiteral("..")))
        return false;

    return true;
}

QString globToRegex(const QString &s)
{
    QString out;
    for (const QChar &ch : s) {
        if (ch == QLatin1Char('*'))
            out += QStringLiteral(".*");
        else if (ch.isLetterOrNumber())
            out += ch;
        else
            out += QRegularExpression::escape(QString(ch));
    }
    return out;
}

QString patternToRegex(const QString &pattern)
{
    QString p = pattern.trimmed();
    const QRegularExpression schemeRe(QStringLiteral("^([a-z*]+)://"));
    QString scheme = QStringLiteral("http");
    QString host = QStringLiteral("*");
    QString path = QStringLiteral("*");
    const QRegularExpressionMatch m = schemeRe.match(p);
    if (m.hasMatch()) {
        scheme = m.captured(1);
        p = p.mid(m.capturedLength());
    }
    const int slash = p.indexOf(QLatin1Char('/'));
    if (slash >= 0) {
        host = p.left(slash);
        path = p.mid(slash + 1);
    } else {
        host = p;
    }

    QString hostRe;
    if (host == QLatin1String("*")) {
        hostRe = QStringLiteral("[^/]+");
    } else if (host.startsWith(QStringLiteral("*."))) {
        hostRe = QStringLiteral("(?:[^/]+\\.)?") + globToRegex(host.mid(2));
    } else {
        hostRe = globToRegex(host);
    }

    const QString pathRe = path.isEmpty() ? QString() : QStringLiteral("/") + globToRegex(path);
    const QString schemeRe2 = scheme == QLatin1String("*")
        ? QStringLiteral("(?:http|https)")
        : QRegularExpression::escape(scheme);

    return QStringLiteral("^") + schemeRe2 + QStringLiteral("://") + hostRe + pathRe + QStringLiteral("$");
}

ExtensionManifestV3 parseManifestV3(const QJsonObject &m)
{
    ExtensionManifestV3 manifest;
    manifest.name = m.value("name").toString();
    manifest.version = m.value("version").toString("0.0");
    manifest.description = m.value("description").toString();
    manifest.manifestVersion = m.value("manifest_version").toString("3");
    manifest.defaultLocale = m.value("default_locale").toString();
    manifest.icons = m.value("icons").toObject();
    manifest.action = m.value("action").toObject();
    manifest.background = m.value("background").toObject();
    manifest.contentScripts = m.value("content_scripts").toArray();
    manifest.permissions = m.value("permissions").toArray();
    manifest.optionalPermissions = m.value("optional_permissions").toArray();
    manifest.hostPermissions = m.value("host_permissions").toArray();
    manifest.optionalHostPermissions = m.value("optional_host_permissions").toArray();
    manifest.declarativeNetRequest = m.value("declarative_net_request").toObject();
    manifest.optionsPage = m.value("options_page").toObject();
    manifest.optionsUi = m.value("options_ui").toObject();
    manifest.omnibox = m.value("omnibox").toObject();
    manifest.commands = m.value("commands").toObject();
    manifest.devtoolsPage = m.value("devtools_page").toObject();
    manifest.homepageUrl = m.value("homepage_url").toObject();
    manifest.updateUrl = m.value("update_url").toObject();
    manifest.minimumChromeVersion = m.value("minimum_chrome_version").toObject();
    manifest.exportObj = m.value("export").toObject();
    manifest.key = m.value("key").toObject();
    manifest.sandbox = m.value("sandbox").toObject();
    manifest.sidePanel = m.value("side_panel").toObject();
    manifest.webAccessibleResources = m.value("web_accessible_resources").toArray();
    return manifest;
}

bool validateManifest(const ExtensionManifestV3 &manifest)
{
    if (manifest.name.isEmpty() || manifest.version.isEmpty()) {
        return false;
    }
    if (manifest.manifestVersion != "3") {
        return false;
    }
    return true;
}

QString generateExtensionId(const QString &path)
{
    QFileInfo info(path);
    QString base = info.fileName();
    QByteArray hash = QCryptographicHash::hash(base.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex().left(32);
}

}

ExtensionManager &ExtensionManager::instance()
{
    static ExtensionManager manager;
    return manager;
}

ExtensionManager::ExtensionManager(QObject *parent)
    : QObject(parent)
{
    m_profile = new QWebEngineProfile("BLACK_Extensions", this);
    scan();
}

QString ExtensionManager::json() const
{
    QJsonArray array;
    for (const ExtensionInfo &e : m_extensions) {
        QJsonObject o;
        o["id"] = e.id;
        o["name"] = e.name;
        o["version"] = e.version;
        o["description"] = e.description;
        o["path"] = e.path;
        o["enabled"] = e.enabled;
        o["loaded"] = e.loaded;
        o["installTime"] = e.installTime.toString(Qt::ISODate);
        o["lastUpdateTime"] = e.lastUpdateTime.toString(Qt::ISODate);
        
        QJsonObject manifest;
        manifest["name"] = e.manifest.name;
        manifest["version"] = e.manifest.version;
        manifest["description"] = e.manifest.description;
        manifest["manifest_version"] = e.manifest.manifestVersion;
        manifest["permissions"] = e.manifest.permissions;
        manifest["host_permissions"] = e.manifest.hostPermissions;
        o["manifest"] = manifest;
        
        array.append(o);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

ExtensionInfo* ExtensionManager::getExtension(const QString &id)
{
    for (auto &e : m_extensions) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

ExtensionContext* ExtensionManager::getContext(const QString &id)
{
    auto it = m_contexts.find(id);
    return it != m_contexts.end() ? it.value().get() : nullptr;
}

void ExtensionManager::reload()
{
    // Unload all loaded extensions
    for (auto &e : m_extensions) {
        if (e.loaded) {
            unloadExtension(e.id);
        }
    }
    scan();
    emit changed();
}

bool ExtensionManager::loadExtension(const QString &id)
{
    ExtensionInfo *info = getExtension(id);
    if (!info || info->loaded) {
        return false;
    }
    
    setupExtensionContext(info);
    info->loaded = true;
    emit extensionLoaded(id);
    emit changed();
    return true;
}

bool ExtensionManager::unloadExtension(const QString &id)
{
    ExtensionInfo *info = getExtension(id);
    if (!info || !info->loaded) {
        return false;
    }
    
    teardownExtensionContext(info);
    info->loaded = false;
    emit extensionUnloaded(id);
    emit changed();
    return true;
}

bool ExtensionManager::enableExtension(const QString &id)
{
    ExtensionInfo *info = getExtension(id);
    if (!info || info->enabled) {
        return false;
    }
    
    info->enabled = true;
    if (info->loaded) {
        setupExtensionContext(info);
    }
    emit changed();
    return true;
}

bool ExtensionManager::disableExtension(const QString &id)
{
    ExtensionInfo *info = getExtension(id);
    if (!info || !info->enabled) {
        return false;
    }
    
    info->enabled = false;
    if (info->loaded) {
        teardownExtensionContext(info);
    }
    emit changed();
    return true;
}

QString ExtensionManager::installExtension(const QString &path)
{
    QDir sourceDir(path);
    if (!sourceDir.exists()) {
        return QString();
    }
    
    QFile manifestFile(sourceDir.filePath("manifest.json"));
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    if (!doc.isObject()) {
        return QString();
    }
    
    ExtensionManifestV3 manifest = parseManifestV3(doc.object());
    if (!validateManifest(manifest)) {
        return QString();
    }
    
    QString id = generateExtensionId(path);
    QString destPath = extRoot() + "/" + id;
    
    QDir destDir;
    if (!destDir.mkpath(destPath)) {
        return QString();
    }
    
    // Copy all files
    std::function<bool(const QDir&, const QDir&)> copyRecursive;
    copyRecursive = [&copyRecursive](const QDir &src, const QDir &dst) -> bool {
        QFileInfoList entries = src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            if (entry.isDir()) {
                QDir newDst(dst.filePath(entry.fileName()));
                if (!newDst.exists()) {
                    if (!dst.mkpath(entry.fileName())) return false;
                }
                if (!copyRecursive(QDir(entry.absoluteFilePath()), newDst)) return false;
            } else {
                if (!QFile::copy(entry.absoluteFilePath(), dst.filePath(entry.fileName()))) return false;
            }
        }
        return true;
    };
    
    if (!copyRecursive(sourceDir, QDir(destPath))) {
        return QString();
    }
    
    ExtensionInfo info = createExtensionInfo(id, destPath);
    info.installTime = QDateTime::currentDateTime();
    info.lastUpdateTime = QDateTime::currentDateTime();
    m_extensions.append(info);
    
    emit extensionInstalled(id);
    emit changed();
    return id;
}

bool ExtensionManager::uninstallExtension(const QString &id)
{
    ExtensionInfo *info = getExtension(id);
    if (!info) {
        return false;
    }
    
    if (info->loaded) {
        unloadExtension(id);
    }
    
    QString path = extRoot() + "/" + id;
    QDir dir(path);
    dir.removeRecursively();
    
    m_extensions.removeAll(*info);
    m_contexts.remove(id);
    
    emit extensionUninstalled(id);
    emit changed();
    return true;
}

QList<QWebEngineScript> ExtensionManager::buildScripts() const
{
    QList<QWebEngineScript> scripts;
    const QString root = extRoot();
    for (const ExtensionInfo &e : m_extensions) {
        if (!e.enabled || !e.loaded)
            continue;
        const QString extDir = QDir::cleanPath(root + QLatin1Char('/') + e.id);
        const QString extCanonical = QFileInfo(extDir).canonicalFilePath();
        int entryIndex = 0;
        for (const QJsonValue &csVal : e.manifest.contentScripts) {
            const QJsonObject cs = csVal.toObject();
            const QJsonArray matches = cs.value("matches").toArray();
            const QJsonArray js = cs.value("js").toArray();
            if (js.isEmpty())
                continue;
            if (matches.isEmpty())
                continue;

            QStringList matchers;
            for (const QJsonValue &mv : matches) {
                const QString rx = patternToRegex(mv.toString());
                if (!rx.isEmpty())
                    matchers.append(QStringLiteral("/(?:%1)/.test(location.href)").arg(rx));
            }
            if (matchers.isEmpty())
                continue;
            const QString guard = QStringLiteral("(%1)").arg(matchers.join(QStringLiteral("||")));

            QString body;
            bool ok = true;
            for (const QJsonValue &jv : js) {
                const QString rel = jv.toString();
                if (!isSafeRelativePath(rel)) {
                    ok = false;
                    break;
                }
                const QString file = QDir::cleanPath(extDir + QLatin1Char('/') + rel);
                if (!file.startsWith(extDir + QLatin1Char('/'))) {
                    ok = false;
                    break;
                }
                const QString canonical = QFileInfo(file).canonicalFilePath();
                if (!canonical.isEmpty()
                    && (extCanonical.isEmpty()
                        || !canonical.startsWith(extCanonical + QLatin1Char('/')))) {
                    ok = false;
                    break;
                }
                QFile f(file);
                if (f.open(QIODevice::ReadOnly))
                    body += QString::fromUtf8(f.readAll()) + QLatin1Char('\n');
            }
            if (!ok)
                continue;

            QWebEngineScript script;
            script.setName(QStringLiteral("black-ext:%1:%2").arg(e.id).arg(entryIndex++));
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            script.setWorldId(QWebEngineScript::ApplicationWorld);
            script.setRunsOnSubFrames(cs.value("all_frames").toBool(false));
            script.setSourceCode(QStringLiteral("(function(){if(!(%1))return;\n%2})();")
                                     .arg(guard, body));
            scripts.append(script);
        }
    }
    return scripts;
}

QList<BlockingRule> ExtensionManager::activeBlockingRules() const
{
    QList<BlockingRule> rules;
    for (const ExtensionInfo &e : m_extensions) {
        if (e.enabled && e.loaded) {
            rules.append(e.blockingRules);
        }
    }
    return rules;
}

QJsonObject ExtensionManager::callApi(const QString &extensionId, const QString &apiName, const QString &method, const QJsonArray &args)
{
    ExtensionContext *ctx = getContext(extensionId);
    if (!ctx) {
        return QJsonObject{{"error", "Extension not loaded"}};
    }
    
    ExtensionApi *api = ctx->api(apiName);
    if (!api) {
        return QJsonObject{{"error", "API not found: " + apiName}};
    }
    
    QJsonObject result;
    QEventLoop loop;
    api->handleMessage(method, args, [&result, &loop](const QJsonObject &r) {
        result = r;
        loop.quit();
    });
    loop.exec();
    
    return result;
}

void ExtensionManager::registerMessageListener(const QString &extensionId, const QString &apiName, const QString &eventName, std::function<void(const QJsonObject&)> listener)
{
    ExtensionContext *ctx = getContext(extensionId);
    if (!ctx) return;
    
    ExtensionApi *api = ctx->api(apiName);
    if (!api) return;
    
    // This would connect to the extension's event signals
    // Implementation depends on the specific API
}

QWebEngineProfile* ExtensionManager::defaultProfile() const
{
    return m_profile;
}

void ExtensionManager::scan()
{
    m_extensions.clear();
    QDir dir(extRoot());
    if (!dir.exists())
        return;
    const QStringList ids = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &id : ids) {
        QFile manifestFile(dir.filePath(id) + QStringLiteral("/manifest.json"));
        if (!manifestFile.open(QIODevice::ReadOnly))
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
        if (!doc.isObject())
            continue;
        const QJsonObject m = doc.object();
        
        ExtensionManifestV3 manifest = parseManifestV3(m);
        if (!validateManifest(manifest)) {
            continue;
        }
        
        ExtensionInfo info = createExtensionInfo(id, dir.filePath(id));
        QFileInfo manifestFileInfo(manifestFile);
    info.installTime = manifestFileInfo.birthTime();
        info.lastUpdateTime = QFileInfo(manifestFile).lastModified();
        m_extensions.append(info);
    }
}

ExtensionInfo* ExtensionManager::createExtensionInfo(const QString &id, const QString &path)
{
    QFile manifestFile(path + "/manifest.json");
    ExtensionManifestV3 manifest;
    if (manifestFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
        if (doc.isObject()) {
            manifest = parseManifestV3(doc.object());
        }
    }
    
    ExtensionInfo* info = new ExtensionInfo();
    info->id = id;
    info.name = manifest.name;
    info.version = manifest.version;
    info.description = manifest.description;
    info.path = path;
    info.manifest = manifest;
    info.enabled = true;
    info.loaded = false;
    
    // Parse blocking rules from manifest
    if (manifest.declarativeNetRequest.contains("rules")) {
        BlockingRule rule;
        QJsonArray rules = manifest.declarativeNetRequest["rules"].toArray();
        for (const QJsonValue &v : rules) {
            QJsonObject r = v.toObject();
            QJsonObject condition = r["condition"].toObject();
            QJsonObject action = r["action"].toObject();

            if (action["type"].toString() == "block") {
                if (condition.contains("domains")) {
                    for (const QJsonValue &d : condition["domains"].toArray()) {
                        rule.blockedHosts.append(d.toString());
                    }
                }
                if (condition.contains("urlFilter")) {
                    rule.blockedPaths.append(condition["urlFilter"].toString());
                }
                if (condition.contains("resourceTypes")) {
                    for (const QJsonValue &t : condition["resourceTypes"].toArray()) {
                        rule.blockedTypes.append(t.toString());
                    }
                }
            }
        }
        info->blockingRules = rule;
    }

    return info;
}

void ExtensionManager::setupExtensionContext(ExtensionInfo *info)
{
    auto ctx = std::make_unique<ExtensionContext>(info->id, this);
    ctx->setExtensionPath(info->path);
    ctx->setManifest(info->manifest);
    ctx->setProfile(m_profile);
    
    // Parse permissions
    QSet<ExtensionApiPermission> permissions;
    QSet<QString> hostPermissions;
    
    static QMap<QString, ExtensionApiPermission> permMap = {
        {"activeTab", ExtensionApiPermission::ActiveTab},
        {"alarms", ExtensionApiPermission::Alarms},
        {"bookmarks", ExtensionApiPermission::Bookmarks},
        {"browsingData", ExtensionApiPermission::BrowsingData},
        {"clipboardRead", ExtensionApiPermission::ClipboardRead},
        {"clipboardWrite", ExtensionApiPermission::ClipboardWrite},
        {"contentSettings", ExtensionApiPermission::ContentSettings},
        {"contextMenus", ExtensionApiPermission::ContextMenus},
        {"cookies", ExtensionApiPermission::Cookies},
        {"debugger", ExtensionApiPermission::Debugger},
        {"declarativeNetRequest", ExtensionApiPermission::DeclarativeNetRequest},
        {"declarativeNetRequestWithHostAccess", ExtensionApiPermission::DeclarativeNetRequestWithHostAccess},
        {"desktopCapture", ExtensionApiPermission::DesktopCapture},
        {"downloads", ExtensionApiPermission::Downloads},
        {"fontSettings", ExtensionApiPermission::FontSettings},
        {"geolocation", ExtensionApiPermission::Geolocation},
        {"history", ExtensionApiPermission::History},
        {"identity", ExtensionApiPermission::Identity},
        {"idle", ExtensionApiPermission::Idle},
        {"management", ExtensionApiPermission::Management},
        {"nativeMessaging", ExtensionApiPermission::NativeMessaging},
        {"notifications", ExtensionApiPermission::Notifications},
        {"pageCapture", ExtensionApiPermission::PageCapture},
        {"platformKeys", ExtensionApiPermission::PlatformKeys},
        {"power", ExtensionApiPermission::Power},
        {"printerProvider", ExtensionApiPermission::PrinterProvider},
        {"privacy", ExtensionApiPermission::Privacy},
        {"processes", ExtensionApiPermission::Processes},
        {"proxy", ExtensionApiPermission::Proxy},
        {"scripting", ExtensionApiPermission::Scripting},
        {"search", ExtensionApiPermission::Search},
        {"sessions", ExtensionApiPermission::Sessions},
        {"storage", ExtensionApiPermission::Storage},
        {"systemDisplay", ExtensionApiPermission::SystemDisplay},
        {"tabCapture", ExtensionApiPermission::TabCapture},
        {"tabGroups", ExtensionApiPermission::TabGroups},
        {"tabs", ExtensionApiPermission::Tabs},
        {"topSites", ExtensionApiPermission::TopSites},
        {"tts", ExtensionApiPermission::Tts},
        {"ttsEngine", ExtensionApiPermission::TtsEngine},
        {"vpnProvider", ExtensionApiPermission::VpnProvider},
        {"wallpaper", ExtensionApiPermission::Wallpaper},
        {"webNavigation", ExtensionApiPermission::WebNavigation},
        {"webRequest", ExtensionApiPermission::WebRequest},
        {"webRequestBlocking", ExtensionApiPermission::WebRequestBlocking}
    };
    
    for (const QJsonValue &v : info->manifest.permissions) {
        QString perm = v.toString();
        if (permMap.contains(perm)) {
            permissions.insert(permMap[perm]);
        }
    }
    
    for (const QJsonValue &v : info->manifest.hostPermissions) {
        hostPermissions.insert(v.toString());
    }
    
    ctx->setPermissions(permissions);
    ctx->setHostPermissions(hostPermissions);
    
    // Register APIs
    ctx->registerApi(std::make_unique<ExtensionRuntimeApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionStorageApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionTabsApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionDeclarativeNetRequestApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionScriptingApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionIdentityApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionActionApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionSidePanelApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionCommandsApi>(ctx.get()));
    ctx->registerApi(std::make_unique<ExtensionOffscreenApi>(ctx.get()));
    
    // Connect extension events
    connect(ctx.get(), &ExtensionContext::unloaded, [this, id = info->id]() {
        teardownExtensionContext(getExtension(id));
    });
    
    m_contexts[info->id] = std::move(ctx);
    info->context = m_contexts[info->id].get();
}

void ExtensionManager::teardownExtensionContext(ExtensionInfo *info)
{
    if (info->context) {
        // Shutdown all APIs
        // The context destruction will handle cleanup
    }
    m_contexts.remove(info->id);
    info->context = nullptr;
}

#include "ExtensionManager.moc"