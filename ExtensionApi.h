#ifndef EXTENSIONAPI_H
#define EXTENSIONAPI_H

#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

#include "ExtensionManager.h"

class QWebEngineProfile;
class ExtensionContext;

// NOTE ON NAMING (deliberate deviation from the original stub sketch):
// `ExtensionApiPermission` MUST be a value type, not a QObject. ExtensionManager.cpp
// stores it in a QMap keyed by QString and in a QSet, i.e. it needs operator< /
// qHash. It is therefore declared here as a scoped enum. The context-holding
// permission object that the sketch asked for is provided separately as
// ExtensionApiPermissions (plural) below.

// Chrome MV3 permission identifiers, mapped by name in ExtensionManager.
enum class ExtensionApiPermission {
    ActiveTab, Alarms, Bookmarks, BrowsingData, ClipboardRead, ClipboardWrite,
    ContentSettings, ContextMenus, Cookies, Debugger, DeclarativeNetRequest,
    DeclarativeNetRequestWithHostAccess, DesktopCapture, Downloads,
    FontSettings, Geolocation, History, Identity, Idle, Management,
    NativeMessaging, Notifications, PageCapture, PlatformKeys, Power,
    PrinterProvider, Privacy, Processes, Proxy, Scripting, Search, Sessions,
    Storage, SystemDisplay, TabCapture, TabGroups, Tabs, TopSites, Tts,
    TtsEngine, VpnProvider, Wallpaper, WebNavigation, WebRequest,
    WebRequestBlocking
};

// qHash overload so ExtensionApiPermission can be used as a QSet key.
inline size_t qHash(ExtensionApiPermission permission, size_t seed = 0) noexcept
{
    return ::qHash(static_cast<int>(permission), seed);
}

// Placeholder implementation. Extension API dispatch (the runtime/storage/
// tabs/... namespaces a MV3 extension calls into) is not implemented in this
// build; calls are no-ops.
//
// handleMessage() immediately invokes its callback with an EMPTY QJsonObject.
// That is a "no result" marker, NOT a success result: an empty object carries
// no payload and must not be read as "the call succeeded". It is done
// synchronously so that ExtensionManager::callApi()'s QEventLoop::quit()
// fires before exec() and the caller never deadlocks.
class ExtensionApi : public QObject
{
    Q_OBJECT

public:
    explicit ExtensionApi(ExtensionContext *context, QObject *parent = nullptr);
    ~ExtensionApi() override;

    // Canonical namespace name, e.g. "storage". Used by ExtensionContext::api().
    virtual QString name() const;

    // Placeholder: no-op. Invokes `callback` synchronously with an empty
    // QJsonObject. Never blocks, never defers, never fails loudly.
    void handleMessage(const QString &method,
                       const QJsonValue &args,
                       std::function<void(const QJsonObject &)> callback);

protected:
    ExtensionContext *m_context = nullptr; // Borrowed, may be null.
};

// Placeholder implementation. Per-extension permission bookkeeping is not
// implemented in this build.
//
// This object only stores the granted permission set. It never checks a
// permission against a manifest, never prompts the user and never enforces
// anything: ExtensionApi::handleMessage() ignores permissions entirely.
class ExtensionApiPermissions : public QObject
{
    Q_OBJECT

public:
    explicit ExtensionApiPermissions(ExtensionContext *context, QObject *parent = nullptr);
    ~ExtensionApiPermissions() override;

    ExtensionContext *context() const;

    // Placeholder: records the granted set; never consulted for enforcement.
    void setPermissions(const QSet<ExtensionApiPermission> &permissions);
    QSet<ExtensionApiPermission> permissions() const;

    // Placeholder: records the granted set; never matched against a URL.
    void setHostPermissions(const QSet<QString> &hostPermissions);
    QSet<QString> hostPermissions() const;

    // Placeholder: only reports what was recorded. It does not verify that the
    // manifest actually requested these permissions.
    bool has(ExtensionApiPermission permission) const;

private:
    ExtensionContext *m_context = nullptr; // Borrowed, may be null.
    QSet<ExtensionApiPermission> m_permissions;
    QSet<QString> m_hostPermissions;
};

// Placeholder implementation. The extension runtime (background page / service
// worker hosting, lifecycle, API namespace registration) is not implemented in
// this build.
//
// A context exists and can be registered into, but no background script is ever
// evaluated and no API method ever does anything beyond the empty-reply
// behaviour described on ExtensionApi.
class ExtensionContext : public QObject
{
    Q_OBJECT

public:
    explicit ExtensionContext(const QString &id, QObject *parent = nullptr);
    ~ExtensionContext() override;

    QString id() const;

    // True when the id is non-empty. It does NOT mean the extension is
    // operational.
    bool isValid() const;

    // Recorded only; no file under the path is ever read.
    void setExtensionPath(const QString &path);
    QString extensionPath() const;

    // Recorded only; the manifest is never validated or acted upon.
    void setManifest(const ExtensionManifestV3 &manifest);
    const ExtensionManifestV3 &manifest() const;

    // Recorded only; nothing is injected into the profile.
    void setProfile(QWebEngineProfile *profile);
    QWebEngineProfile *profile() const;

    // Recorded only; see ExtensionApiPermissions for the enforcement caveat.
    void setPermissions(const QSet<ExtensionApiPermission> &permissions);
    QSet<ExtensionApiPermission> permissions() const;

    // Recorded only; never matched against a request URL.
    void setHostPermissions(const QSet<QString> &hostPermissions);
    QSet<QString> hostPermissions() const;

    // Takes ownership. Registries are looked up by ExtensionApi::name().
    void registerApi(std::unique_ptr<ExtensionApi> api);

    // Returns the registered API whose name() matches, or nullptr.
    ExtensionApi *api(const QString &name) const;

    // Emits loaded(). No extension code is executed.
    void load();

    // Emits unloaded(). Nothing is torn down beyond the signal itself.
    void unload();

signals:
    void unloaded();
    void loaded();

private:
    QString m_id;
    QString m_extensionPath;
    ExtensionManifestV3 m_manifest;
    QWebEngineProfile *m_profile = nullptr; // Borrowed, may be null.
    QSet<ExtensionApiPermission> m_permissions;
    QSet<QString> m_hostPermissions;
    QList<std::unique_ptr<ExtensionApi>> m_apis;
};

// ── Placeholder API namespaces ───────────────────────────────────────────────
// ExtensionManager.cpp registers one of each of these by name. They exist so
// that registration list links; every one of them answers with the empty
// QJsonObject described above.

#define BLACK_DECLARE_STUB_API(ClassName)                                    \
    class ClassName : public ExtensionApi                                   \
    {                                                                       \
    public:                                                                 \
        explicit ClassName(ExtensionContext *context, QObject *parent = nullptr); \
        QString name() const override;                                      \
    }

BLACK_DECLARE_STUB_API(ExtensionRuntimeApi);
BLACK_DECLARE_STUB_API(ExtensionStorageApi);
BLACK_DECLARE_STUB_API(ExtensionTabsApi);
BLACK_DECLARE_STUB_API(ExtensionDeclarativeNetRequestApi);
BLACK_DECLARE_STUB_API(ExtensionScriptingApi);
BLACK_DECLARE_STUB_API(ExtensionIdentityApi);
BLACK_DECLARE_STUB_API(ExtensionActionApi);
BLACK_DECLARE_STUB_API(ExtensionSidePanelApi);
BLACK_DECLARE_STUB_API(ExtensionCommandsApi);
BLACK_DECLARE_STUB_API(ExtensionOffscreenApi);

#undef BLACK_DECLARE_STUB_API

#endif // EXTENSIONAPI_H