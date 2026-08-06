#include "ExtensionManager.h"
#include <QWebEngineScript>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>

namespace {

QString extRoot()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("extensions");
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
        m_extensions.append(info);
    }
}

QList<QWebEngineScript> ExtensionManager::buildScripts() const
{
    QList<QWebEngineScript> scripts;
    const QString root = extRoot();
    for (const ExtensionInfo &e : m_extensions) {
        for (const QJsonValue &csVal : e.contentScripts) {
            const QJsonObject cs = csVal.toObject();
            const QJsonArray matches = cs.value(QStringLiteral("matches")).toArray();
            const QJsonArray js = cs.value(QStringLiteral("js")).toArray();
            if (js.isEmpty())
                continue;

            QStringList matchers;
            for (const QJsonValue &mv : matches) {
                const QString rx = patternToRegex(mv.toString());
                if (!rx.isEmpty())
                    matchers.append(QStringLiteral("/(?:%1)/.test(location.href)").arg(rx));
            }
            const QString guard = matchers.isEmpty()
                ? QStringLiteral("true")
                : QStringLiteral("(%1)").arg(matchers.join(QStringLiteral("||")));

            QString body;
            for (const QJsonValue &jv : js) {
                QFile f(root + QLatin1Char('/') + e.id + QLatin1Char('/') + jv.toString());
                if (f.open(QIODevice::ReadOnly))
                    body += QString::fromUtf8(f.readAll()) + QLatin1Char('\n');
            }

            QWebEngineScript script;
            script.setName(QStringLiteral("black-ext:%1").arg(e.id));
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setRunsOnSubFrames(cs.value(QStringLiteral("all_frames")).toBool(false));
            script.setSourceCode(QStringLiteral("(function(){if(!(%1))return;\n%2})();")
                                     .arg(guard, body));
            scripts.append(script);
        }
    }
    return scripts;
}
