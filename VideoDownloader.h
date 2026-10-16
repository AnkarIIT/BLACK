#ifndef VIDEODOWNLOADER_H
#define VIDEODOWNLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QMetaType>
#include <functional>
#include <memory>

// Forward declarations
class QNetworkAccessManager;
class QNetworkReply;
class QProcess;

namespace VideoDownloader {

// Video quality options
enum class VideoQuality {
    Best,
    Worst,
    HD1080p,
    HD720p,
    SD480p,
    SD360p,
    AudioOnly
};

// Download status
enum class DownloadStatus {
    Pending,
    Preparing,
    Downloading,
    Merging,
    Completed,
    Failed,
    Cancelled
};

// Video metadata
struct VideoInfo {
    QString url;
    QString title;
    QString thumbnailUrl;
    QString duration;
    QString author;
    QString platform;  // "facebook", "instagram", "twitter", "youtube", "generic"
    QMap<VideoQuality, QString> qualityUrls;  // quality -> m3u8/mp4 URL
    QString audioUrl;
    QString subtitleUrl;
    QVariantMap extraData;  // platform-specific extra data
};

// Download progress
struct DownloadProgress {
    QString downloadId;
    qint64 bytesDownloaded;
    qint64 totalBytes;
    double progress;  // 0.0 - 1.0
    double speed;     // bytes/second
    DownloadStatus status;
    QString errorMessage;
};

// Platform-specific video extractor interface
class IVideoExtractor {
public:
    virtual ~IVideoExtractor() = default;
    virtual QString platformName() const = 0;
    virtual bool canHandle(const QUrl& url) const = 0;
    virtual bool extractVideoInfo(const QString& url, std::function<void(const VideoInfo&)> onSuccess,
                                  std::function<void(const QString&)> onError) = 0;
};

// M3U8/HLS playlist parser
class M3U8Parser {
public:
    struct Segment {
        QString url;
        double duration;
        QString title;
        QMap<QString, QString> attributes;
    };

    struct Playlist {
        QString url;
        QString baseUrl;
        bool isMaster = false;
        QList<Segment> segments;
        QMap<QString, QString> mediaPlaylists;  // quality -> playlist URL
        QMap<QString, QString> audioPlaylists;  // group-id -> playlist URL
        QMap<QString, QString> subtitlePlaylists;
        QMap<QString, QString> attributes;
    };

    static Playlist parse(const QString& content, const QString& baseUrl);
    static QString resolveUrl(const QString& baseUrl, const QString& relativeUrl);
    static bool isMasterPlaylist(const QString& content);
};

// FFmpeg wrapper for audio/video merging
class FFmpegWrapper {
public:
    struct MergeOptions {
        QString videoFile;
        QString audioFile;
        QString outputFile;
        QString videoCodec = "copy";
        QString audioCodec = "copy";
        QString outputFormat = "mp4";
        bool overwrite = true;
    };

    static bool isAvailable();
    static QString getVersion();
    static bool merge(const MergeOptions& options,
                      std::function<void(int progress)> onProgress,
                      std::function<void(const QString&)> onError,
                      std::function<void()> onSuccess);
    static bool convert(const QString& input, const QString& output,
                        const QString& codec = "libx264",
                        std::function<void(int progress)> onProgress = nullptr,
                        std::function<void(const QString&)> onError = nullptr,
                        std::function<void()> onSuccess = nullptr);
    static QString getFFmpegPath();
    static void setFFmpegPath(const QString& path);
};

// Main video downloader coordinator
class VideoDownloader : public QObject {
    Q_OBJECT

public:
    explicit VideoDownloader(QObject* parent = nullptr);
    ~VideoDownloader() override;

    // Register a platform-specific extractor
    void registerExtractor(std::unique_ptr<IVideoExtractor> extractor);

    // Start a video download
    // Returns download ID for tracking
    QString downloadVideo(const QUrl& url,
                          VideoQuality quality = VideoQuality::Best,
                          const QString& outputDir = QString(),
                          const QString& customFilename = QString());

    // Cancel a download
    void cancelDownload(const QString& downloadId);

    // Get download progress
    DownloadProgress getProgress(const QString& downloadId) const;

    // Get all active downloads
    QList<DownloadProgress> getAllProgress() const;

    // Set custom FFmpeg path
    void setFFmpegPath(const QString& path);

    // Check if FFmpeg is available
    bool isFFmpegAvailable() const;

    // Register platform extractors (call during initialization)
    static void registerDefaultExtractors(VideoDownloader* downloader);

signals:
    void downloadStarted(const QString& downloadId, const VideoInfo& info);
    void downloadProgress(const QString& downloadId, const DownloadProgress& progress);
    void downloadFinished(const QString& downloadId, const QString& outputFile);
    void downloadFailed(const QString& downloadId, const QString& error);
    void downloadCancelled(const QString& downloadId);
    void ffmpegStatusChanged(bool available);

private:
    struct DownloadTask {
        QString id;
        VideoInfo info;
        VideoQuality quality;
        QString outputDir;
        QString outputFile;
        DownloadProgress progress;
        QNetworkReply* reply = nullptr;
        QProcess* ffmpegProcess = nullptr;
        QString tempVideoFile;
        QString tempAudioFile;
        QList<M3U8Parser::Segment> segments;
        int currentSegment = 0;
    };

    class Private;
    Private* d;

    void downloadNextSegment(const QString &downloadId);
};

} // namespace VideoDownloader

Q_DECLARE_METATYPE(VideoDownloader::VideoInfo)
Q_DECLARE_METATYPE(VideoDownloader::DownloadProgress)

#endif // VIDEODOWNLOADER_H