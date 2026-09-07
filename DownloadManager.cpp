#include "DownloadManager.h"

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

#include "BrowserSettings.h"

// Auto-open allowlist kept local to the manager: only inert content types are
// ever launched after a download completes. Executables, scripts, HTML/SVG and
// Office macros are never auto-opened, regardless of the "open safe files"
// setting (see BrowserWindow for the full rationale).
static bool isSafeToOpen(const QString &filePath)
{
    static const QSet<QString> extensions = {
        // Pictures
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("gif"), QStringLiteral("bmp"), QStringLiteral("webp"),
        QStringLiteral("ico"), QStringLiteral("tif"), QStringLiteral("tiff"),
        QStringLiteral("avif"), QStringLiteral("heic"),
        // Sounds
        QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("ogg"),
        QStringLiteral("oga"), QStringLiteral("m4a"), QStringLiteral("aac"),
        QStringLiteral("flac"), QStringLiteral("opus"), QStringLiteral("wma"),
        // Movies
        QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("webm"),
        QStringLiteral("mov"), QStringLiteral("avi"), QStringLiteral("m4v"),
        QStringLiteral("mpg"), QStringLiteral("mpeg"), QStringLiteral("wmv"),
        QStringLiteral("flv"),
        // Documents
        QStringLiteral("pdf"), QStringLiteral("txt"), QStringLiteral("md"),
        QStringLiteral("log"), QStringLiteral("csv"), QStringLiteral("json"),
        QStringLiteral("rtf"),
        // Archives (opened by the archive manager, not executed)
        QStringLiteral("zip"), QStringLiteral("rar"), QStringLiteral("7z"),
        QStringLiteral("tar"), QStringLiteral("gz"), QStringLiteral("bz2"),
        QStringLiteral("xz"), QStringLiteral("tgz"),
    };
    return extensions.contains(QFileInfo(filePath).suffix().toLower());
}

DownloadManager::DownloadManager(QWebEngineProfile *profile,
                                  QWidget *parentWidget,
                                  QToolButton *button,
                                  QObject *parent)
    : QObject(parent)
    , m_profile(profile)
    , m_parentWidget(parentWidget)
    , m_button(button)
{
    connect(m_profile, &QWebEngineProfile::downloadRequested, this,
            [this](QWebEngineDownloadRequest *download) {
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

        DownloadItemInfo info;
        info.fileName = download->downloadFileName();
        info.filePath = QDir(download->downloadDirectory()).filePath(download->downloadFileName());
        info.url = download->url().toString();
        info.receivedBytes = download->receivedBytes();
        info.totalBytes = download->totalBytes();
        info.speedBytesPerSec = 0;
        info.etaSeconds = -1;
        info.state = 0;
        info.request = download;
        info.startTime = QDateTime::currentDateTime();
        info.lastUpdateTime = QDateTime::currentDateTime();
        m_items.append(info);

        const int idx = m_items.count() - 1;
        download->accept();

        connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, [this, idx, download]() {
            if (idx < m_items.count()) {
                m_items[idx].receivedBytes = download->receivedBytes();
                m_items[idx].totalBytes = download->totalBytes();

                // Calculate speed and ETA
                QDateTime now = QDateTime::currentDateTime();
                qint64 elapsedMs = m_items[idx].lastUpdateTime.msecsTo(now);
                if (elapsedMs > 0) {
                    qint64 bytesDiff = download->receivedBytes() - m_items[idx].receivedBytes;
                    // Wait, we need the previous value - let's store it
                }
                m_items[idx].lastUpdateTime = now;

                // Update speed and ETA (will be calculated properly below)
                calculateSpeedAndEta(idx);
            }
        });

        connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, idx, download](QWebEngineDownloadRequest::DownloadState state) {
            if (idx < m_items.count()) {
                switch (state) {
                case QWebEngineDownloadRequest::DownloadCompleted:
                    m_items[idx].state = 1;
                    if (BrowserSettings::instance().openSafeFiles()
                        && !m_items[idx].filePath.isEmpty()
                        && isSafeToOpen(m_items[idx].filePath))
                        QDesktopServices::openUrl(QUrl::fromLocalFile(m_items[idx].filePath));
                    break;
                case QWebEngineDownloadRequest::DownloadCancelled:
                case QWebEngineDownloadRequest::DownloadInterrupted:
                    m_items[idx].state = 2;
                    break;
                default:
                    break;
                }
                updateButtonTooltip();
                emit itemsChanged();
            }
        });

        if (m_button)
            m_button->setToolTip(QStringLiteral("Downloading %1\u2026").arg(download->downloadFileName()));
        emit itemsChanged();
    });
}

// Calculate download speed and ETA
void DownloadManager::calculateSpeedAndEta(int idx)
{
    if (idx >= m_items.count())
        return;

    DownloadItemInfo &item = m_items[idx];
    if (item.totalBytes <= 0 || item.receivedBytes <= 0)
        return;

    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsedSec = item.startTime.secsTo(now);
    if (elapsedSec > 0) {
        item.speedBytesPerSec = item.receivedBytes / elapsedSec;
        if (item.speedBytesPerSec > 0) {
            item.etaSeconds = (item.totalBytes - item.receivedBytes) / item.speedBytesPerSec;
        }
    }
}

int DownloadManager::completedCount() const
{
    int completed = 0;
    for (const DownloadItemInfo &d : m_items)
        if (d.state == 1) ++completed;
    return completed;
}

void DownloadManager::clearCompleted()
{
    for (int i = m_items.count() - 1; i >= 0; --i)
        if (m_items[i].state != 0)
            m_items.removeAt(i);
    updateButtonTooltip();
}

void DownloadManager::clearAll()
{
    m_items.clear();
}

void DownloadManager::updateButtonTooltip()
{
    if (!m_button)
        return;
    QString tip;
    if (completedCount() > 0)
        tip = QStringLiteral("Downloads (%1 completed)").arg(completedCount());
    else
        tip = QStringLiteral("Downloads");
    m_button->setToolTip(tip);
    emit buttonTooltipChanged(tip);
}

void DownloadManager::pauseDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    DownloadItemInfo &item = m_items[index];
    if (item.request && item.state == 0) {
        item.request->pause();
        item.state = 3; // paused
        emit itemsChanged();
    }
}

void DownloadManager::resumeDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    DownloadItemInfo &item = m_items[index];
    if (item.request && item.state == 3) {
        item.request->resume();
        item.state = 0; // in progress
        item.startTime = QDateTime::currentDateTime(); // Reset start time for speed calc
        emit itemsChanged();
    }
}

void DownloadManager::cancelDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    DownloadItemInfo &item = m_items[index];
    if (item.request) {
        item.request->cancel();
        item.state = 2; // cancelled
        emit itemsChanged();
    }
}

void DownloadManager::retryDownload(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    DownloadItemInfo &item = m_items[index];
    if (item.state == 2) {
        // Retry by emitting signal to open URL in new tab
        emit openInNewTab(QUrl(item.url));
    }
}

void DownloadManager::showInFolder(int index)
{
    if (index < 0 || index >= m_items.count())
        return;
    const DownloadItemInfo &item = m_items[index];
    if (!item.filePath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(item.filePath).absolutePath()));
    }
}
