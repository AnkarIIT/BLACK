#include "UpdateChecker.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QJsonArray>
#include <QDebug>

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
    if (m_checking || m_downloading) {
        return;
    }

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
    // Custom: { "version": "1.2.3", "url": "...", "notes": "...", "sha256": "..." }

    QString version;
    QUrl downloadUrl;
    QString notes;
    QString sha256;

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
        sha256 = obj.value(QStringLiteral("sha256")).toString();
    }

    if (version.isEmpty()) {
        m_lastError = QStringLiteral("No version found in response");
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    if (downloadUrl.isEmpty()) {
        m_lastError = QStringLiteral("No download URL found for Windows installer");
        emit updateStatusChanged();
        emit checkFinished(false);
        return;
    }

    m_latestVersion = version;
    m_updateUrl = downloadUrl;
    m_releaseNotes = notes;
    m_expectedSha256 = sha256.toLower();

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

    if (m_downloading) {
        emit downloadFinished(false, QStringLiteral("Download already in progress"));
        return;
    }

#if defined(Q_OS_WIN)
    // Create unique temporary file using QTemporaryFile for atomic, unique naming
    m_downloadFile = new QTemporaryFile(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                         .filePath(QStringLiteral("BLACK-Installer-XXXXXX.exe")));
    m_downloadFile->setAutoRemove(false);

    if (!m_downloadFile->open()) {
        m_lastError = QStringLiteral("Cannot create unique temp file for download");
        cleanupDownload();
        emit downloadFinished(false, m_lastError);
        return;
    }

    m_downloadFilePath = m_downloadFile->fileName();

    // Initialize streaming SHA256 hasher
    m_hasher = new QCryptographicHash(QCryptographicHash::Sha256, this);

    QNetworkRequest request(m_updateUrl);
    request.setRawHeader("User-Agent", "BLACK-Browser-UpdateChecker/1.0");
    m_downloadReply = m_network.get(request);

    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::readyRead,
            this, &UpdateChecker::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);

    m_downloading = true;
    m_lastError.clear();
    emit updateStatusChanged();
#else
    emit downloadFinished(false, QStringLiteral("Auto-install only supported on Windows"));
#endif
}

void UpdateChecker::cancelDownload()
{
    if (!m_downloading) {
        return;
    }

    if (m_downloadReply) {
        m_downloadReply->abort();
    }
    cleanupDownload();
    emit downloadFinished(false, QStringLiteral("Download cancelled by user"));
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateChecker::onDownloadReadyRead()
{
    if (!m_downloadReply || !m_downloadFile || !m_hasher) {
        return;
    }

    // Stream data directly to file and hasher - no buffering
    const QByteArray chunk = m_downloadReply->readAll();
    if (chunk.isEmpty()) {
        return;
    }

    if (m_downloadFile->write(chunk) != chunk.size()) {
        m_downloadReply->abort();
        m_lastError = QStringLiteral("Failed to write download chunk to disk");
        cleanupDownload();
        emit downloadFinished(false, m_lastError);
        return;
    }

    m_hasher->addData(chunk);
}

void UpdateChecker::onDownloadFinished()
{
    if (!m_downloadReply) {
        emit downloadFinished(false, QStringLiteral("No download reply"));
        return;
    }

    bool success = false;
    QString error;

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        error = m_downloadReply->errorString();
    } else {
        // Flush any remaining data
        if (m_downloadFile) {
            m_downloadFile->flush();
            m_downloadFile->close();
        }

        // Verify checksum if provided
        if (!m_expectedSha256.isEmpty()) {
            const QString actualSha256 = computeFileSha256(m_downloadFilePath);
            if (!actualSha256.isEmpty() && actualSha256.toLower() != m_expectedSha256) {
                error = QStringLiteral("Checksum verification failed: expected %1, got %2")
                            .arg(m_expectedSha256, actualSha256);
                qWarning() << "Update checksum mismatch:" << error;
                QFile::remove(m_downloadFilePath);
            } else if (actualSha256.isEmpty()) {
                error = QStringLiteral("Failed to compute checksum of downloaded file");
                QFile::remove(m_downloadFilePath);
            } else {
                success = true;
            }
        } else {
            qWarning() << "No SHA256 provided in update manifest - skipping verification";
            success = true;
        }

        if (success) {
            // Launch the installer with proper error handling
            m_installerProcess = new QProcess(this);
            connect(m_installerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, &UpdateChecker::onInstallerFinished);

            QStringList args;
            args << QStringLiteral("/VERYSILENT") << QStringLiteral("/SUPPRESSMSGBOXES")
                 << QStringLiteral("/NORESTART") << QStringLiteral("/LOG");

            if (!m_installerProcess->start(m_downloadFilePath, args)) {
                error = QStringLiteral("Failed to launch installer: %1").arg(m_installerProcess->errorString());
                success = false;
                QFile::remove(m_downloadFilePath);
            }
        }
    }

    if (!success) {
        cleanupDownload();
        emit downloadFinished(false, error);
    }
    // On success, we wait for installer process to finish
}

void UpdateChecker::onInstallerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString error;
    bool success = false;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        success = true;
    } else {
        error = QStringLiteral("Installer exited with code %1 (status: %2)")
                    .arg(exitCode).arg(exitStatus == QProcess::NormalExit ? "NormalExit" : "CrashExit");
    }

    // Clean up downloaded installer after launch attempt
    if (!m_downloadFilePath.isEmpty()) {
        QFile::remove(m_downloadFilePath);
    }

    cleanupDownload();
    emit downloadFinished(success, error);
}

bool UpdateChecker::verifyChecksum(const QString &filePath) const
{
    if (m_expectedSha256.isEmpty()) {
        return true; // No checksum to verify
    }
    const QString actual = computeFileSha256(filePath);
    return !actual.isEmpty() && actual.toLower() == m_expectedSha256;
}

QString UpdateChecker::computeFileSha256(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const qint64 bufferSize = 64 * 1024;
    QByteArray buffer;

    while (!file.atEnd()) {
        buffer = file.read(bufferSize);
        if (buffer.isEmpty()) {
            break;
        }
        hash.addData(buffer);
    }

    return QString::fromLatin1(hash.result().toHex());
}

void UpdateChecker::cleanupDownload()
{
    if (m_downloadReply) {
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    if (m_downloadFile) {
        if (m_downloadFile->isOpen()) {
            m_downloadFile->close();
        }
        // Don't auto-remove here - we handle removal explicitly after verification/launch
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }
    if (m_hasher) {
        m_hasher->deleteLater();
        m_hasher = nullptr;
    }
    if (m_installerProcess) {
        m_installerProcess->deleteLater();
        m_installerProcess = nullptr;
    }

    m_downloading = false;
    m_downloadFilePath.clear();
    emit updateStatusChanged();
}

#include "UpdateChecker.moc"