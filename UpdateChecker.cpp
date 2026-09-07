#include "UpdateChecker.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QJsonArray>

UpdateChecker::UpdateChecker(const QString &currentVersion,
                             const QUrl &updateCheckUrl,
                             QObject *parent)
    : QObject(parent)
    , m_currentVersion(currentVersion)
    , m_updateCheckUrl(updateCheckUrl)
{
}

void UpdateChecker::checkForUpdates()
{
    if (m_checking)
        return;

    m_checking = true;
    m_lastError.clear();
    emit updateStatusChanged();

    QNetworkRequest request(m_updateCheckUrl);
    request.setRawHeader("User-Agent", "BLACK-Browser-UpdateChecker/1.0");
    request.setRawHeader("Accept", "application/json");

    m_checkReply = m_network.get(request);
    connect(m_checkReply, &QNetworkReply::finished, this, &UpdateChecker::onCheckReplyFinished);
}

void UpdateChecker::onCheckReplyFinished()
{
    m_checking = false;

    if (!m_checkReply) {
        m_lastError = QStringLiteral("No reply object");
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    if (m_checkReply->error() != QNetworkReply::NoError) {
        m_lastError = m_checkReply->errorString();
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    const QByteArray data = m_checkReply->readAll();
    m_checkReply->deleteLater();
    m_checkReply = nullptr;

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_lastError = QStringLiteral("Invalid JSON response");
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    const QJsonObject obj = doc.object();

    // Support both GitHub releases format and custom format
    // GitHub: { "tag_name": "v1.2.3", "assets": [{ "browser_download_url": "...", "name": "..." }], "body": "..." }
    // Custom: { "version": "1.2.3", "url": "...", "notes": "..." }

    QString version;
    QUrl downloadUrl;
    QString notes;

    if (obj.contains(QStringLiteral("tag_name"))) {
        // GitHub releases format
        version = obj.value(QStringLiteral("tag_name")).toString();
        if (version.startsWith(QLatin1Char('v')))
            version.remove(0, 1);

        const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue &asset : assets) {
            const QJsonObject a = asset.toObject();
            const QString name = a.value(QStringLiteral("name")).toString();
            if (name.endsWith(QLatin1String(".exe")) || name.endsWith(QLatin1String(".msi"))) {
                downloadUrl = QUrl(a.value(QStringLiteral("browser_download_url")).toString());
                break;
            }
        }
        notes = obj.value(QStringLiteral("body")).toString();
    } else {
        // Custom format
        version = obj.value(QStringLiteral("version")).toString();
        downloadUrl = QUrl(obj.value(QStringLiteral("url")).toString());
        notes = obj.value(QStringLiteral("notes")).toString();
    }

    if (version.isEmpty()) {
        m_lastError = QStringLiteral("No version found in response");
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    m_latestVersion = version;
    m_updateUrl = downloadUrl;
    m_releaseNotes = notes;

    const QVersionNumber current = QVersionNumber::fromString(m_currentVersion);
    const QVersionNumber latest = QVersionNumber::fromString(version);

    m_updateAvailable = (latest > current);

    emit updateStatusChanged();
    emit checkFinished(m_updateAvailable);
}

void UpdateChecker::installUpdate()
{
    if (!m_updateAvailable || m_updateUrl.isEmpty()) {
        emit downloadFinished(false, QStringLiteral("No update available or invalid URL"));
        return;
    }

#if defined(Q_OS_WIN)
    // Download the installer to a temp file
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString fileName = m_updateUrl.fileName();
    if (fileName.isEmpty()) {
        emit downloadFinished(false, QStringLiteral("Invalid download URL"));
        return;
    }

    const QString filePath = QDir(tempDir).filePath(fileName);
    m_downloadFile = new QFile(filePath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, QStringLiteral("Cannot create temp file for download"));
        delete m_downloadFile;
        m_downloadFile = nullptr;
        return;
    }

    QNetworkRequest request(m_updateUrl);
    request.setRawHeader("User-Agent", "BLACK-Browser-UpdateChecker/1.0");
    m_downloadReply = m_network.get(request);
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);
#else
    emit downloadFinished(false, QStringLiteral("Auto-install only supported on Windows"));
#endif
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateChecker::onDownloadFinished()
{
    if (!m_downloadReply) {
        emit downloadFinished(false, QStringLiteral("No download reply"));
        return;
    }

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        const QString error = m_downloadReply->errorString();
        if (m_downloadFile) {
            m_downloadFile->close();
            m_downloadFile->remove();
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
        emit downloadFinished(false, error);
        return;
    }

    // Write remaining data
    if (m_downloadFile) {
        m_downloadFile->write(m_downloadReply->readAll());
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;

    // Launch the installer
#if defined(Q_OS_WIN)
    const QString filePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(m_updateUrl.fileName());
    if (QFile::exists(filePath)) {
        QProcess::startDetached(filePath, QStringList() << QStringLiteral("/VERYSILENT") << QStringLiteral("/SUPPRESSMSGBOXES"));
        emit downloadFinished(true, QString());
    } else {
        emit downloadFinished(false, QStringLiteral("Downloaded file not found"));
    }
#else
    emit downloadFinished(false, QStringLiteral("Auto-install only supported on Windows"));
#endif
}