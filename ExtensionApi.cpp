#include "ExtensionApi.h"

#include <QWebEngineProfile>

// ── ExtensionApi ─────────────────────────────────────────────────────────────

ExtensionApi::ExtensionApi(ExtensionContext *context, QObject *parent)
    : QObject(parent)
    , m_context(context)
{
}

ExtensionApi::~ExtensionApi() = default;

QString ExtensionApi::name() const
{
    return QString();
}

void ExtensionApi::handleMessage(const QString &method,
                                 const QJsonValue &args,
                                 std::function<void(const QJsonObject &)> callback)
{
    Q_UNUSED(method)
    Q_UNUSED(args)

    // Placeholder: no method of any API namespace is implemented, so there is
    // nothing to compute. Reply synchronously with an EMPTY object.
    //
    // This empty object is a "no result" marker, not a success result: callers
    // must not read it as a successful return value. It is invoked
    // synchronously on purpose so ExtensionManager::callApi()'s
    // QEventLoop::quit() is called before exec(), which then returns
    // immediately instead of blocking forever.
    if (callback)
        callback(QJsonObject());
}

// ── ExtensionApiPermissions ──────────────────────────────────────────────────

ExtensionApiPermissions::ExtensionApiPermissions(ExtensionContext *context, QObject *parent)
    : QObject(parent)
    , m_context(context)
{
}

ExtensionApiPermissions::~ExtensionApiPermissions() = default;

ExtensionContext *ExtensionApiPermissions::context() const
{
    return m_context;
}

void ExtensionApiPermissions::setPermissions(const QSet<ExtensionApiPermission> &permissions)
{
    // Placeholder: stored but never enforced. ExtensionApi::handleMessage()
    // does not consult this set.
    m_permissions = permissions;
}

QSet<ExtensionApiPermission> ExtensionApiPermissions::permissions() const
{
    return m_permissions;
}

void ExtensionApiPermissions::setHostPermissions(const QSet<QString> &hostPermissions)
{
    // Placeholder: stored but never matched against a request URL.
    m_hostPermissions = hostPermissions;
}

QSet<QString> ExtensionApiPermissions::hostPermissions() const
{
    return m_hostPermissions;
}

bool ExtensionApiPermissions::has(ExtensionApiPermission permission) const
{
    // Placeholder: reports only what was recorded; it does not verify the
    // manifest actually asked for it, and nothing blocks on a false result.
    return m_permissions.contains(permission);
}

// ── ExtensionContext ─────────────────────────────────────────────────────────

ExtensionContext::ExtensionContext(const QString &id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
}

ExtensionContext::~ExtensionContext() = default;

QString ExtensionContext::id() const
{
    return m_id;
}

bool ExtensionContext::isValid() const
{
    // A context is "valid" only in the sense that it is addressable.
    return !m_id.isEmpty();
}

void ExtensionContext::setExtensionPath(const QString &path)
{
    // Placeholder: recorded only. No file under `path` is ever read.
    m_extensionPath = path;
}

QString ExtensionContext::extensionPath() const
{
    return m_extensionPath;
}

void ExtensionContext::setManifest(const ExtensionManifestV3 &manifest)
{
    // Placeholder: recorded only. Never validated, never acted upon.
    m_manifest = manifest;
}

const ExtensionManifestV3 &ExtensionContext::manifest() const
{
    return m_manifest;
}

void ExtensionContext::setProfile(QWebEngineProfile *profile)
{
    // Placeholder: recorded only. No script is injected into the profile.
    m_profile = profile;
}

QWebEngineProfile *ExtensionContext::profile() const
{
    return m_profile;
}

void ExtensionContext::setPermissions(const QSet<ExtensionApiPermission> &permissions)
{
    // Placeholder: recorded only. Nothing consults or enforces this set.
    m_permissions = permissions;
}

QSet<ExtensionApiPermission> ExtensionContext::permissions() const
{
    return m_permissions;
}

void ExtensionContext::setHostPermissions(const QSet<QString> &hostPermissions)
{
    // Placeholder: recorded only. Never matched against a request URL.
    m_hostPermissions = hostPermissions;
}

QSet<QString> ExtensionContext::hostPermissions() const
{
    return m_hostPermissions;
}

void ExtensionContext::registerApi(std::shared_ptr<ExtensionApi> api)
{
    if (!api)
        return;
    m_apis.append(api);
}

ExtensionApi *ExtensionContext::api(const QString &name) const
{
    for (const std::shared_ptr<ExtensionApi> &a : m_apis) {
        if (a && a->name() == name)
            return a.get();
    }
    return nullptr;
}

void ExtensionContext::load()
{
    // Placeholder: a real implementation would evaluate the background page /
    // service worker here. No extension code runs.
    emit loaded();
}

void ExtensionContext::unload()
{
    // Placeholder: a real implementation would tear the background context
    // down here. Only the signal is emitted.
    emit unloaded();
}

// ── Placeholder API namespaces ───────────────────────────────────────────────
// Each ctor stores the context (via ExtensionApi) and each name() returns the
// namespace name ExtensionManager registers it under. handleMessage() is
// inherited unchanged and replies with an empty object for every method.

#define BLACK_DEFINE_STUB_API(ClassName, ApiNameString)                     \
    ClassName::ClassName(ExtensionContext *context, QObject *parent)        \
        : ExtensionApi(context, parent)                                     \
    {                                                                       \
    }                                                                       \
    QString ClassName::name() const                                         \
    {                                                                       \
        return QStringLiteral(ApiNameString);                               \
    }

BLACK_DEFINE_STUB_API(ExtensionRuntimeApi, "runtime")
BLACK_DEFINE_STUB_API(ExtensionStorageApi, "storage")
BLACK_DEFINE_STUB_API(ExtensionTabsApi, "tabs")
BLACK_DEFINE_STUB_API(ExtensionDeclarativeNetRequestApi, "declarativeNetRequest")
BLACK_DEFINE_STUB_API(ExtensionScriptingApi, "scripting")
BLACK_DEFINE_STUB_API(ExtensionIdentityApi, "identity")
BLACK_DEFINE_STUB_API(ExtensionActionApi, "action")
BLACK_DEFINE_STUB_API(ExtensionSidePanelApi, "sidePanel")
BLACK_DEFINE_STUB_API(ExtensionCommandsApi, "commands")
BLACK_DEFINE_STUB_API(ExtensionOffscreenApi, "offscreen")

#undef BLACK_DEFINE_STUB_API