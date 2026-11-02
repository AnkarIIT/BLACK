#ifndef ENHANCEDDOWNLOADMANAGER_H
#define ENHANCEDDOWNLOADMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QPointer>
#include <QUrl>
#include <QDateTime>
#include <QSet>
#include <QMap>

class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QToolButton;
class QWidget;
class M3U8Parser;

namespace EnhancedDownload {

// Extended download item with M3U8/video support
struct EnhancedDownloadItem {
    enum Type {
        Type_Regular = 0,
        Type_M3U8 = 1,
        Type_Video = 2,
        Type_M3U8Segment = 3
    };
    
    enum State {
        State_Pending = 0,
        State_InProgress = 1,
        State_Completed = 2,
        State_Failed = 3,
        State_Cancelled = 4,
        State_Paused = 5,
        State_Merging = 6
    };
    
    // Core info
    QString id;                      // Unique download ID
    Type type = Type_Regular;
    State state = State_Pending;
    
    // Basic info
    QString fileName;
    QString filePath;
    QString url;
    qint64 receivedBytes = 0;
    qint64 totalBytes = -1;
    qint64 speedBytesPerSec = 0;
    qint64 etaSeconds = -1;
    int stateInt = 0;  // For Q_PROPERTY compatibility
    
    // WebEngine integration
    QPointer<QWebEngineDownloadRequest> request;
    
    // M3U8/Video specific
    bool isM3U8 = false;
    bool isVideo = false;
    QString m3u8Url;
    QString videoTitle;
    QString videoQuality;  // "1080p", "720p", "best", etc.
    QString outputDir;
    QString tempDir;
    QString finalFilePath;
    
    // M3U8 parsing
    struct M3U8Info {
        QString masterUrl;
        QString selectedQuality;
        int totalSegments = 0;
        int downloadedSegments = 0;
        QString tempDir;
        QString keyUrl;
        QByteArray encryptionKey;
        QByteArray encryptionIV;
        QString encryptionMethod;
        QList<QString> segmentUrls;
        QMap<int, QString> segmentFiles;  // sequence -> temp file
    };
    M3U8Info m3u8Info;
    
    // Video merging
    bool needsMerge = false;
    QString videoFile;
    QString audioFile;
    QString mergedFile;
    int mergeProgress = 0;
    
    // Timing
    QDateTime startTime;
    QDateTime lastUpdateTime;
    
    // Progress
    double progress = 0.0;  // 0.0 - 1.0
    
    // Metadata
    QString referrerUrl;
    QString referrerTitle;
    QString userAgent;
    QString suggestedFileName;
    QString mimeType;
    
    // Error handling
    QString errorMessage;
    int retryCount = 0;
    int maxRetries = 3;
    QDateTime lastRetryTime;
    
    QDateTime startTime;
    QDateTime lastUpdateTime;
    QDateTime completedTime;
};

} // namespace EnhancedDownload

// Forward declarations
class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QToolButton;
class QWidget;
class QNetworkAccessManager;
class M3U8Parser;

class EnhancedDownloadManager : public QObject
{
    Q_OBJECT
    
public:
    explicit EnhancedDownloadManager(QWebEngineProfile* profile,
                                    QWidget* parentWidget,
                                    QToolButton* button,
                                    QObject* parent = nullptr);
    ~EnhancedDownloadManager() override;
    
    // Types
    using EnhancedItem = EnhancedDownload::EnhancedDownloadItem;
    using ItemList = QList<EnhancedItem>;
    
    const QList<EnhancedItem>& items() const { return m_items; }
    
    // Core management
    void clearCompleted();
    void clearAll();
    int completedCount() const;
    
    // Download control
    Q_INVOKABLE void pauseDownload(int index);
    Q_INVOKABLE void resumeDownload(int index);
    Q_INVOKABLE void cancelDownload(int index);
    Q_INVOKABLE void retryDownload(int index);
    Q_INVOKABLE void showInFolder(int index);
    
    // M3U8/Video downloads
    Q_INVOKABLE QString startM3U8Download(const QString& m3u8Url, const QString& quality = "best", const QString& outputDir = QString());
    Q_INVOKABLE QString startVideoDownload(const QString& videoUrl, const QString& quality = "best", const QString& outputDir = QString());
    Q_INVOKABLE bool cancelM3U8Download(const QString& downloadId);
    
    // M3U8 parsing
    Q_INVOKABLE QStringList fetchM3U8Qualities(const QString& m3u8Url);
    Q_INVOKABLE bool parseM3U8Url(const QString& m3u8Url);
    
    // Video merging
    Q_INVOKABLE bool mergeVideoAudio(const QString& videoFile, const QString& audioFile, const QString& outputFile);
    
    // Settings
    void setMaxConcurrentDownloads(int max) { m_maxConcurrent = max; }
    int maxConcurrentDownloads() const { return m_maxConcurrent; }
    
    // FFmpeg
    void setFFmpegPath(const QString& path) { m_ffmpegPath = path; }
    QString ffmpegPath() const { return m_ffmpegPath; }
    bool isFFmpegAvailable() const;
    
signals:
    void itemsChanged();
    void itemAdded(const EnhancedDownload::EnhancedDownloadItem& item);
    void itemUpdated(const EnhancedDownload::EnhancedDownloadItem& item);
    void itemRemoved(int index);
    void buttonTooltipChanged(const QString& tip);
    void m3u8QualitiesFetched(const QString& m3u8Url, const QStringList& qualities);
    void m3u8ParseFailed(const QString& m3u8Url, const QString& error);
    void m3u8DownloadStarted(const QString& downloadId);
    void m3u8SegmentDownloaded(const QString& downloadId, int segment, int total);
    void m3u8DownloadProgress(const QString& downloadId, int downloaded, int total);
    void m3u8DownloadCompleted(const QString& downloadId, const QString& outputFile);
    void m3u8DownloadFailed(const QString& downloadId, const QString& error);
    void mergeProgress(const QString& downloadId, int progress);
    void mergeCompleted(const QString& downloadId, const QString& outputFile);
    void mergeFailed(const QString& downloadId, const QString& error);
    void buttonTooltipChanged(const QString& tip);
    void ffmpegStatusChanged(bool available);
    
private:
    struct DownloadItem {
        EnhancedDownload::EnhancedDownloadItem item;
        QPointer<QWebEngineDownloadRequest> webEngineRequest;
        QNetworkReply* networkReply = nullptr;
        QString tempFile;
    };
    
    void onDownloadRequested(QWebEngineDownloadRequest* download);
    void onNetworkReplyFinished(QNetworkReply* reply);
    void onM3U8DownloadProgress(const QString& downloadId);
    void onM3U8SegmentFinished(const QString& downloadId, int segmentIndex, const QByteArray& data);
    void onM3U8DownloadComplete(const QString& downloadId);
    void onMergeFinished(const QString& downloadId, bool success, const QString& error);
    void startM3U8SegmentDownload(const QString& downloadId, int segmentIndex);
    void processM3U8Queue();
    void updateItemState(int index, EnhancedDownload::EnhancedDownloadItem::State state);
    void updateItemProgress(int index, double progress);
    void updateSpeedAndEta(int index);
    int indexOfDownload(const QString& id) const;
    int indexOfWebEngineDownload(QWebEngineDownloadRequest* download) const;
    void updateButtonTooltip();
    QString generateDownloadId();
    QString sanitizeFileName(const QString& name);
    QString getUniqueFilePath(const QString& dir, const QString& baseName, const QString& extension);
    bool ensureDirectory(const QString& path);
    void cleanupTempFiles(const QString& downloadId);
    
    // M3U8 handling
    void parseM3U8Content(const QString& downloadId, const QString& content, const QString& baseUrl);
    void selectM3U8Quality(const QString& downloadId, const QString& quality);
    void downloadM3U8Segments(const QString& downloadId);
    void downloadM3U8Segment(const QString& downloadId, int segmentIndex);
    void saveM3U8Segment(const QString& downloadId, int segmentIndex, const QByteArray& data);
    void mergeM3U8Segments(const QString& downloadId);
    
    // Video merging
    void startVideoMerge(const QString& downloadId, const QString& videoFile, const QString& audioFile, const QString& outputFile);
    
    // FFmpeg
    bool checkFFmpegAvailable();
    QString findFFmpegPath();
    
    // Cleanup
    void cleanupDownload(const QString& downloadId);
    
    // Member variables
    QWebEngineProfile* m_profile = nullptr;
    QWidget* m_parentWidget = nullptr;
    QToolButton* m_button = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    M3U8Parser* m_m3u8Parser = nullptr;
    
    QList<DownloadItem> m_items;
    int m_maxConcurrent = 3;
    int m_activeDownloads = 0;
    QString m_ffmpegPath;
    bool m_ffmpegAvailable = false;
    QMap<QString, QString> m_pendingM3U8Urls;  // downloadId -> m3u8Url
    QMap<QString, QString> m_pendingVideoUrls; // downloadId -> videoUrl
    QQueue<QString> m_downloadQueue;
    bool m_processingQueue = false;
    
    int m_maxRetries = 3;
    int m_retryDelayMs = 5000;
};

#endif // ENHANCEDDOWNLOADMANAGER_H