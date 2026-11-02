#include "EnhancedDownloadManager.h"
#include "M3U8Parser.h"
#include "BrowserSettings.h"
#include "OSPaths.h"

#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QToolButton>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QWidget>
#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QProcess>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCryptographicHash>

EnhancedDownloadManager::EnhancedDownloadManager(QWebEngineProfile* profile,
                                                  QWidget* parentWidget,
                                                  QToolButton* button,
                                                  QObject* parent)
    : QObject(parent)
    , m_profile(profile)
    , m_parentWidget(parentWidget)
    , m_button(button)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_m3u8Parser(new M3U8Parser(this))
{
    connect(m_profile, &QWebEngineProfile::downloadRequested, this, &EnhancedDownloadManager::onDownloadRequested);
    checkFFmpegAvailable();
}

EnhancedDownloadManager::~EnhancedDownloadManager()
{
    clearAll();
    delete m_m3u8Parser;
    delete m_networkManager;
}

bool EnhancedDownloadManager::checkFFmpegAvailable()
{
    m_ffmpegPath = findFFmpegPath();
    m_ffmpegAvailable = !m_ffmpegPath.isEmpty();
    emit ffmpegStatusChanged(m_ffmpegAvailable);
    return m_ffmpegAvailable;
}

QString EnhancedDownloadManager::findFFmpegPath()
{
    // Check common locations
    QStringList paths = {
        "ffmpeg",
        "/usr/bin/ffmpeg",
        "/usr/local/bin/ffmpeg",
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        qgetenv("FFMPEG_PATH")
    };
    
    for (const QString& path : paths) {
        if (path.isEmpty()) continue;
        QProcess proc;
        proc.start(path, QStringList() << "-version");
        if (proc.waitForStarted(1000) && proc.waitForFinished(3000)) {
            if (proc.exitCode() == 0) {
                return path;
            }
        }
    }
    return QString();
}

bool EnhancedDownloadManager::isFFmpegAvailable() const
{
    return m_ffmpegAvailable;
}

void EnhancedDownloadManager::onDownloadRequested(QWebEngineDownloadRequest* download)
{
    if (!download)
        return;
    
    if (BrowserSettings::instance().downloadLocation() == QStringLiteral("Ask each time")) {
        const QString dir = QFileDialog::getExistingDirectory(
            m_parentWidget, QStringLiteral("Choose Download Location"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (dir.isEmpty()) {
            download->cancel();
            return;
        }
        download->setDownloadDirectory(dir);
    }
    
    // Check if this is an M3U8 or video URL
    QString url = download->url().toString();
    if (url.endsWith(".m3u8", Qt::CaseInsensitive) || url.contains(".m3u8?")) {
        // This is an M3U8 playlist - handle with our M3U8 downloader
        download->cancel();
        startM3U8Download(download->url().toString());
        return;
    }
    
    // Check for video URLs
    QString lowerUrl = download->url().toString().toLower();
    if (lowerUrl.contains("video") || lowerUrl.contains(".mp4") || lowerUrl.contains(".webm") ||
        lowerUrl.contains(".mkv") || lowerUrl.contains(".m3u8")) {
        // Could be a video - offer to use video downloader
        // For now, let WebEngine handle it normally
    }
    
    // Regular WebEngine download
    struct Item {
        EnhancedDownload::EnhancedDownloadItem item;
        QPointer<QWebEngineDownloadRequest> request;
        QString tempFile;
    };
    
    // We need to add to m_items - but m_items is a QList<DownloadItem> where DownloadItem is a private struct
    // We'll need to refactor this. For now, let's use the existing m_items structure.
    // Actually, looking at the code, m_items is a QList<DownloadItem> where DownloadItem is a private struct.
    // We need to add a proper item to m_items.
    
    // For now, let's just use the existing logic but enhanced
    // We'll need to refactor the DownloadManager to use EnhancedDownloadItem
    // For now, let's just use the existing DownloadManager logic
}

void EnhancedDownloadManager::onDownloadRequested(QWebEngineDownloadRequest* download)
{
    if (!download)
        return;
    
    if (BrowserSettings::instance().downloadLocation() == QStringLiteral("Ask each time")) {
        const QString dir = QFileDialog::getExistingDirectory(
            m_parentWidget, QStringLiteral("Choose Download Location"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (dir.isEmpty()) {
            download->cancel();
            return;
        }
        download->setDownloadDirectory(dir);
    }
    
    // Check for M3U8/video URLs
    QString url = download->url().toString();
    if (url.endsWith(".m3u8", Qt::CaseInsensitive) || url.contains(".m3u8?")) {
        download->cancel();
        startM3U8Download(download->url().toString());
        return;
    }
    
    // Check if it's a video URL that we might want to handle specially
    QString lowerUrl = download->url().toString().toLower();
    QStringList videoExtensions = {".mp4", ".webm", ".mkv", ".mov", ".avi", ".flv", ".m3u8", ".ts"};
    bool isVideo = false;
    for (const QString& ext : videoExtensions) {
        if (lowerUrl.contains(ext)) {
            isVideo = true;
            break;
        }
    }
    
    // For now, let WebEngine handle it
    // We'll enhance this later
}

void EnhancedDownloadManager::onNetworkReplyFinished(QNetworkReply* reply)
{
    // Handle M3U8 segment downloads
    // This would be connected to network replies for M3U8 segment downloads
}

QString EnhancedDownloadManager::startM3U8Download(const QString& m3u8Url, const QString& quality, const QString& outputDir)
{
    QString downloadId = generateDownloadId();
    
    // Create download item
    struct Item {
        EnhancedDownload::EnhancedDownloadItem item;
        QPointer<QWebEngineDownloadRequest> request;
        QString tempFile;
    };
    
    // We need to add to our items list
    // For now, let's create a simple implementation
    
    // Fetch M3U8 playlist
    QNetworkRequest request(m3u8Url);
    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(m3u8Url));
    
    // Store the download ID for this reply
    reply->setProperty("downloadId", generateDownloadId());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString downloadId = reply->property("downloadId").toString();
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit m3u8ParseFailed(m_pendingM3U8Urls.value(downloadId), reply->errorString());
            return;
        }
        
        QString content = reply->readAll();
        QString m3u8Url = m_pendingM3U8Urls.take(downloadId);
        
        if (m3u8Url.isEmpty()) {
            return;
        }
        
        // Parse M3U8
        M3U8Parser::Playlist playlist = M3U8Parser::parse(content, m3u8Url);
        
        if (playlist.isMaster) {
            // Extract qualities
            QStringList qualities = playlist.mediaPlaylists.keys();
            emit m3u8QualitiesFetched(m3u8Url, qualities);
            
            // Auto-select quality if specified
            QString selectedQuality = "best"; // Default
            // For now, just pick the first one
            if (!playlist.mediaPlaylists.isEmpty()) {
                QString bestQuality = playlist.mediaPlaylists.keys().first();
                QString variantUrl = playlist.mediaPlaylists[bestQuality];
                
                // Start downloading the variant playlist
                startM3U8Download(variantUrl);
            }
        } else {
            // Media playlist - start downloading segments
            // Create download item
            QString downloadId = generateDownloadId();
            
            // Store playlist info
            m_pendingM3U8Urls[downloadId] = m3u8Url;
            
            // Start downloading segments
            downloadM3U8Segments(downloadId);
        }
    });
    
    return downloadId;
}

QString EnhancedDownloadManager::generateDownloadId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString EnhancedDownloadManager::sanitizeFileName(const QString& name)
{
    QString result = name;
    // Remove invalid characters
    static const QRegularExpression invalidChars(R"([<>:\"/\\|?*\x00-\x1F])");
    result.replace(invalidChars, "_");
    // Trim length
    if (result.length() > 255) {
        result = result.left(255);
    }
    return result;
}

QString EnhancedDownloadManager::getUniqueFilePath(const QString& dir, const QString& baseName, const QString& extension)
{
    QString base = sanitizeFileName(baseName);
    if (base.isEmpty()) base = "download";
    
    QString ext = extension;
    if (!ext.isEmpty() && !ext.startsWith('.')) {
        ext = "." + ext;
    }
    
    QString path = QDir(dir).filePath(base + ext);
    int counter = 1;
    
    while (QFileInfo::exists(path)) {
        path = QDir(dir).filePath(QString("%1 (%2)%3").arg(base).arg(counter++).arg(ext));
    }
    
    return path;
}

bool EnhancedDownloadManager::ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void EnhancedDownloadManager::updateButtonTooltip()
{
    if (!m_button)
        return;
    QString tip;
    int completed = completedCount();
    if (completed > 0)
        tip = QStringLiteral("Downloads (%1 completed)").arg(completed);
    else
        tip = QStringLiteral("Downloads");
    m_button->setToolTip(tip);
    emit buttonTooltipChanged(tip);
}

int EnhancedDownloadManager::completedCount() const
{
    // This would need access to m_items
    return 0;
}

void EnhancedDownloadManager::clearCompleted()
{
    // Implementation would go here
    emit itemsChanged();
}

void EnhancedDownloadManager::clearAll()
{
    // Cancel all active downloads
    for (auto& item : m_items) {
        if (item.networkReply) {
            item.networkReply->abort();
        }
        if (item.webEngineRequest) {
            item.webEngineRequest->cancel();
        }
    }
    // Clean up temp files
    // Clear items
    emit itemsChanged();
}

int EnhancedDownloadManager::completedCount() const
{
    int count = 0;
    // Count completed items
    return count;
}

void EnhancedDownloadManager::pauseDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    // Implementation
    emit itemsChanged();
}

void EnhancedDownloadManager::resumeDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    // Implementation
    emit itemsChanged();
}

void EnhancedDownloadManager::cancelDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    // Implementation
    emit itemsChanged();
}

void EnhancedDownloadManager::retryDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    // Implementation
}

void EnhancedDownloadManager::showInFolder(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    // Implementation
}

QString EnhancedDownloadManager::startM3U8Download(const QString& m3u8Url, const QString& quality, const QString& outputDir)
{
    return startM3U8Download(m3u8Url, quality, outputDir);
}

QString EnhancedDownloadManager::startVideoDownload(const QString& videoUrl, const QString& quality, const QString& outputDir)
{
    // Use the VideoDownloader class for video downloads
    return generateDownloadId();
}

QStringList EnhancedDownloadManager::fetchM3U8Qualities(const QString& m3u8Url)
{
    QStringList qualities;
    // Fetch and parse M3U8
    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(m3u8Url));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QString content = reply->readAll();
        M3U8Parser::Playlist playlist = M3U8Parser::parse(reply->readAll(), m3u8Url);
        if (playlist.isMaster) {
            return playlist.mediaPlaylists.keys();
        }
    }
    reply->deleteLater();
    return qualities;
}

bool EnhancedDownloadManager::parseM3U8Url(const QString& m3u8Url)
{
    // Parse M3U8 in background
    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(m3u8Url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, m3u8Url]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit m3u8ParseFailed(m3u8Url, reply->errorString());
            return;
        }
        
        QString content = reply->readAll();
        M3U8Parser::Playlist playlist = M3U8Parser::parse(content, m3u8Url);
        
        if (playlist.isMaster) {
            emit m3u8QualitiesFetched(m3u8Url, playlist.mediaPlaylists.keys());
        }
    });
    return true;
}

bool EnhancedDownloadManager::cancelM3U8Download(const QString& downloadId)
{
    // Cancel M3U8 download
    return true;
}

bool EnhancedDownloadManager::mergeVideoAudio(const QString& videoFile, const QString& audioFile, const QString& outputFile)
{
    if (!m_ffmpegAvailable) return false;
    
    QProcess proc;
    proc.start(m_ffmpegPath, QStringList() 
        << "-i" << videoFile
        << "-i" << audioFile
        << "-c:v" << "copy"
        << "-c:a" << "copy"
        << "-y" << outputFile);
    
    return proc.waitForFinished(300000) && proc.exitCode() == 0;
}

void EnhancedDownloadManager::setFFmpegPath(const QString& path)
{
    m_ffmpegPath = path;
    m_ffmpegAvailable = !path.isEmpty();
    if (m_ffmpegAvailable) {
        checkFFmpegAvailable();
    }
    emit ffmpegStatusChanged(m_ffmpegAvailable);
}

void EnhancedDownloadManager::cleanupTempFiles(const QString& downloadId)
{
    // Clean up temp files for a download
}

void EnhancedDownloadManager::cleanupDownload(const QString& downloadId)
{
    // Clean up all resources for a download
}