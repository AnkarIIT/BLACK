#ifndef EXTENSIONMANAGER_H
#define EXTENSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QList>

class QWebEngineScript;
class QWebEngineProfile;

struct ExtensionInfo {
    QString id;          // directory name
    QString name;
    QString version;
    QString description;
    QJsonArray contentScripts; // [{matches:[..], js:[..], all_frames:bool}]
};

// Minimal extension engine: loads folders under
// <AppDataLocation>/extensions/<id>/manifest.json and turns their
// content_scripts into real QWebEngineScript injections. No chrome.* /
// browser.* API surface yet — pure content scripts.
class ExtensionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString json READ json NOTIFY changed)

public:
    explicit ExtensionManager(QObject *parent = nullptr);

    QString json() const;
    const QList<ExtensionInfo> &extensions() const { return m_extensions; }

    Q_INVOKABLE void reload();

    // Scripts for all installed extensions (to insert into a profile).
    QList<QWebEngineScript> buildScripts() const;

signals:
    void changed();

private:
    void scan();
    QList<ExtensionInfo> m_extensions;
};

#endif
