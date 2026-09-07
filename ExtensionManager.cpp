#include "ExtensionManager.h"
#include <QWebEngineScript>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>

namespace {

QString extRoot()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("extensions");
}

// Accept only safe relative paths from a manifest "js" entry: no absolute
// paths, no drive/colon references, no backslashes, no ".." traversal.
// The result is further verified by a canonical containment check in
// buildScripts() so even symlinked or cleaned paths cannot escape the
// extension directory.
bool isSafeRelativePath(const QString &p)
{
    if (p.isEmpty())
        return false;
    if (p.startsWith(QLatin1Char('/')) || p.startsWith(QLatin1Char('\\')))
        return false;                 // absolute path
    if (p.contains(QLatin1Char('\\')) || p.contains(QLatin1Char(':')))
        return false;                 // Windows separators / drive letters
    if (p.contains(QStringLiteral("..")))
        return false;                 // parent traversal (also covers "a/../b")

    // Check for URL-encoded traversal attempts (e.g., %2e%2e%2f = ../)
    QString decoded = QUrl::fromPercentEncoding(p.toUtf8());
    if (decoded.contains(QStringLiteral("..")))
        return false;

    return true;
}

// Turn a glob-like token into regex source: '*' -> '.*', everything else escaped.
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

// Convert a Chrome-style match pattern ("https://*.example.com/*") to a JS
// RegExp source tested against location.href.
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

// Parse blocking rules from manifest JSON
BlockingRule parseBlockingRules(const QJsonObject &m)
{
    BlockingRule rule;
    if (m.contains(QStringLiteral("blocking"))) {
        const QJsonObject blocking = m.value(QStringLiteral("blocking")).toObject();
        const QJsonArray hostsArray = blocking.value(QStringLiteral("blockedHosts")).toArray();
        for (const QJsonValue &v : hostsArray) {
            rule.blockedHosts.append(v.toString());
        }
        const QJsonArray pathsArray = blocking.value(QStringLiteral("blockedPaths")).toArray();
        for (const QJsonValue &v : pathsArray) {
            rule.blockedPaths.append(v.toString());
        }
        const QJsonArray typesArray = blocking.value(QStringLiteral("blockedTypes")).toArray();
        for (const QJsonValue &v : typesArray) {
            rule.blockedTypes.append(v.toString());
        }
    }
    return rule;
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
    scan();
}

QString ExtensionManager::json() const
{
    QJsonArray array;
    for (const ExtensionInfo &e : m_extensions) {
        QJsonObject o;
        o[QStringLiteral("id")] = e.id;
        o[QStringLiteral("name")] = e.name;
        o[QStringLiteral("version")] = e.version;
        o[QStringLiteral("description")] = e.description;
        o[QStringLiteral("enabled")] = e.enabled;
        QJsonObject blocking;
        QStringList hosts;
        for (const QString &h : e.blockingRules.blockedHosts) hosts.append(h);
        QStringList paths;
        for (const QString &p : e.blockingRules.blockedPaths) paths.append(p);
        QStringList types;
        for (const QString &t : e.blockingRules.blockedTypes) types.append(t);
        blocking[QStringLiteral("blockedHosts")] = QJsonArray::fromStringList(hosts);
        blocking[QStringLiteral("blockedPaths")] = QJsonArray::fromStringList(paths);
        blocking[QStringLiteral("blockedTypes")] = QJsonArray::fromStringList(types);
        o[QStringLiteral("blocking")] = blocking;
        array.append(o);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void ExtensionManager::reload()
{
    scan();
    emit changed();
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
        ExtensionInfo info;
        info.id = id;
        info.name = m.value(QStringLiteral("name")).toString(id);
        info.version = m.value(QStringLiteral("version")).toString(QStringLiteral("0.0"));
        info.description = m.value(QStringLiteral("description")).toString();
        info.contentScripts = m.value(QStringLiteral("content_scripts")).toArray();
        if (info.contentScripts.isEmpty() && m.contains(QStringLiteral("content_scripts")))
            continue;
        info.blockingRules = parseBlockingRules(m);
        info.enabled = m.value(QStringLiteral("enabled")).toBool(true);
        m_extensions.append(info);
    }
}

QList<QWebEngineScript> ExtensionManager::buildScripts() const
{
    QList<QWebEngineScript> scripts;
    const QString root = extRoot();
    for (const ExtensionInfo &e : m_extensions) {
        if (!e.enabled)
            continue;
        const QString extDir = QDir::cleanPath(root + QLatin1Char('/') + e.id);
        const QString extCanonical = QFileInfo(extDir).canonicalFilePath();
        int entryIndex = 0;
        for (const QJsonValue &csVal : e.contentScripts) {
            const QJsonObject cs = csVal.toObject();
            const QJsonArray matches = cs.value(QStringLiteral("matches")).toArray();
            const QJsonArray js = cs.value(QStringLiteral("js")).toArray();
            if (js.isEmpty())
                continue;
            // A content script without at least one explicit match pattern
            // would run on every page, so it is skipped (Chrome requires
            // "matches" for the same reason).
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

            // Load only manifest entries that resolve inside the extension's
            // own directory (blocks "../..", absolute paths and symlink
            // escapes from being read).
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
            // Unique per-entry name so an extension with several content
            // scripts does not have later entries silently overwrite earlier
            // ones in the profile's script collection.
            script.setName(QStringLiteral("black-ext:%1:%2").arg(e.id).arg(entryIndex++));
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            // Isolated ApplicationWorld: the page's own JavaScript and the
            // extension script cannot read each other's variables, so a site
            // cannot inspect or tamper with what an extension does.
            script.setWorldId(QWebEngineScript::ApplicationWorld);
            script.setRunsOnSubFrames(cs.value(QStringLiteral("all_frames")).toBool(false));
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
        if (e.enabled) {
            rules.append(e.blockingRules);
        }
    }
    return rules;
}