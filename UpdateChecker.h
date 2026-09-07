#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QTimer>
#include <QFile>

// Lightweight auto-update checker for Windows.
// Checks a JSON endpoint (e.g., GitHub releases API) for newer versions.
// If an update is found, downloads the installer and launches it.
// Uses QNetworkAccessManager for async HTTP requests.

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateStatusChanged)
    Q_PROPERTY(QString updateUrl READ updateUrlString NOTIFY updateStatusChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateStatusChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateStatusChanged)
    Q_PROPERTY(bool checkingForUpdates READ checkingForUpdates NOTIFY updateStatusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY updateStatusChanged)

public:
    explicit UpdateChecker(const QString &currentVersion,
                           const QUrl &updateCheckUrl,
                           QObject *parent = nullptr);

    QString currentVersion() const { return m_currentVersion; }
    QString latestVersion() const { return m_latestVersion; }
    QString updateUrlString() const { return m_updateUrl.toString(); }
    QString releaseNotes() const { return m_releaseNotes; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool checkingForUpdates() const { return m_checking; }
    QString lastError() const { return m_lastError; }

    // Start an update check. Emits updateStatusChanged when done.
    Q_INVOKABLE void checkForUpdates();

    // Download and run the installer (Windows only)
    Q_INVOKABLE void installUpdate();

signals:
    void updateStatusChanged();
    void checkFinished(bool updateAvailable);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString &errorMessage);

private slots:
    void onCheckReplyFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    QString m_currentVersion;
    QUrl m_updateCheckUrl;
    QString m_latestVersion;
    QUrl m_updateUrl;
    QString m_releaseNotes;
    bool m_updateAvailable = false;
    bool m_checking = false;
    QString m_lastError;

    QNetworkAccessManager m_network;
    QNetworkReply *m_checkReply = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    QFile *m_downloadFile = nullptr;
};

#endif // UPDATECHECKER_H