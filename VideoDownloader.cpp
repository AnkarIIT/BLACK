#include "VideoDownloader.h"

#include <QMetaType>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
#include <QDateTime>
#include <QPointer>
#include <QTimer>
#include <QScopedPointer>
#include <vector>

namespace VideoDownloader {

// ── M3U8Parser ───────────────────────────────────────────────────────────────

QString M3U8Parser::resolveUrl(const QString &baseUrl, const QString &relativeUrl)
{
    if (relativeUrl.isEmpty())
        return QString();
    const QUrl rel(relativeUrl);
    if (rel.isValid() && !rel.isRelative())
        return rel.toString();
    return QUrl(baseUrl).resolved(rel).toString();
}

bool M3U8Parser::isMasterPlaylist(const QString &content)
{
    return content.contains(QStringLiteral("#EXT-X-STREAM-INF"))
        || content.contains(QStringLiteral("#EXT-X-SESSION-DATA"));
}

M3U8Parser::Playlist M3U8Parser::parse(const QString &content, const QString &baseUrl)
{
    Playlist pl;
    pl.url = baseUrl;
    QString baseDir = baseUrl;
    const int slash = baseUrl.lastIndexOf(QLatin1Char('/'));
    if (slash >= 0)
        baseDir = baseUrl.left(slash + 1);
    pl.baseUrl = baseDir;
    pl.isMaster = isMasterPlaylist(content);

    const QStringList lines = content.split(QLatin1Char('\n'));
    double pendingDuration = 0.0;
    QString pendingTitle;
    QString pendingStreamQuality;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith(QStringLiteral("#EXT-X-STREAM-INF"))) {
            // Optional BANDWIDTH / RESOLUTION give a quality label.
            QString label = QStringLiteral("auto");
            const QRegularExpression bwRe(QStringLiteral("BANDWIDTH=(\\d+)"));
            const QRegularExpression resRe(QStringLiteral("RESOLUTION=(\\d+x\\d+)"));
            const auto bwMatch = bwRe.match(line);
            const auto resMatch = resRe.match(line);
            if (resMatch.hasMatch())
                label = resMatch.captured(1);
            else if (bwMatch.hasMatch())
                label = bwMatch.captured(1);
            pendingStreamQuality = label;
        } else if (line.startsWith(QStringLiteral("#EXT-X-MEDIA"))) {
            const QRegularExpression typeRe(QStringLiteral("TYPE=([^,]+)"));
            const QRegularExpression groupRe(QStringLiteral("GROUP-ID=\"([^\"]+)\""));
            const QRegularExpression uriRe(QStringLiteral("URI=\"([^\"]+)\""));
            const auto typeMatch = typeRe.match(line);
            const auto groupMatch = uriRe.match(line);
            const auto uriMatch = uriRe.match(line);
            Q_UNUSED(groupMatch);
            const QString type = typeMatch.hasMatch() ? typeMatch.captured(1) : QString();
            if (uriMatch.hasMatch()) {
                const QString abs = resolveUrl(baseDir, uriMatch.captured(1));
                if (type == QLatin1String("AUDIO"))
                    pl.audioPlaylists.insert(uriMatch.captured(1), abs);
                else if (type == QLatin1String("SUBTITLES"))
                    pl.subtitlePlaylists.insert(uriMatch.captured(1), abs);
            }
        } else if (line.startsWith(QStringLiteral("#EXTINF"))) {
            const int colon = line.indexOf(QLatin1Char(':'));
            const int comma = line.indexOf(QLatin1Char(','));
            if (colon >= 0) {
                const QString durStr = (comma > colon)
                    ? line.mid(colon + 1, comma - colon - 1)
                    : line.mid(colon + 1);
                pendingDuration = durStr.toDouble();
                pendingTitle = (comma >= 0) ? line.mid(comma + 1) : QString();
            }
        } else if (!line.startsWith(QLatin1Char('#'))) {
            const QString abs = resolveUrl(baseDir, line);
            if (pl.isMaster) {
                const QString key = pendingStreamQuality.isEmpty()
                    ? QStringLiteral("auto-%1").arg(pl.mediaPlaylists.size())
                    : pendingStreamQuality;
                if (!pl.mediaPlaylists.contains(key))
                    pl.mediaPlaylists.insert(key, abs);
                else
                    pl.mediaPlaylists.insert(key + QStringLiteral("-%1").arg(pl.mediaPlaylists.size()), abs);
                pendingStreamQuality.clear();
            } else {
                Segment seg;
                seg.url = abs;
                seg.duration = pendingDuration;
                seg.title = pendingTitle;
                pl.segments.append(seg);
                pendingDuration = 0.0;
                pendingTitle.clear();
            }
        }
    }
    return pl;
}

// ── FFmpegWrapper ────────────────────────────────────────────────────────────

namespace {
QString g_ffmpegPath;
}

bool FFmpegWrapper::isAvailable()
{
    if (!g_ffmpegPath.isEmpty())
        return QFileInfo(g_ffmpegPath).isFile();
    return !QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty();
}

QString FFmpegWrapper::getFFmpegPath()
{
    if (!g_ffmpegPath.isEmpty())
        return g_ffmpegPath;
    return QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
}

void FFmpegWrapper::setFFmpegPath(const QString &path)
{
    g_ffmpegPath = path;
}

QString FFmpegWrapper::getVersion()
{
    const QString exe = getFFmpegPath();
    if (exe.isEmpty())
        return QString();
    QProcess proc;
    proc.start(exe, { QStringLiteral("-version") });
    if (!proc.waitForFinished(5000))
        return QString();
    const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
    return out.split(QLatin1Char('\n')).value(0);
}

bool FFmpegWrapper::merge(const MergeOptions &options,
                          std::function<void(int)> onProgress,
                          std::function<void(const QString &)> onError,
                          std::function<void()> onSuccess)
{
    const QString exe = getFFmpegPath();
    if (exe.isEmpty()) {
        if (onError)
            onError(QStringLiteral("ffmpeg executable not found"));
        return false;
    }
    QStringList args;
    if (options.overwrite)
        args << QStringLiteral("-y");
    args << QStringLiteral("-i") << options.videoFile
         << QStringLiteral("-i") << options.audioFile
         << QStringLiteral("-c:v") << options.videoCodec
         << QStringLiteral("-c:a") << options.audioCodec
         << options.outputFile;

    auto *proc = new QProcess();
    QObject::connect(proc, &QProcess::finished, proc,
                     [proc, onSuccess, onError](int code, QProcess::ExitStatus) {
                         proc->deleteLater();
                         if (code == 0) {
                             if (onSuccess)
                                 onSuccess();
                         } else if (onError) {
                             onError(QString::fromLocal8Bit(proc->readAllStandardError()));
                         }
                     });
    QObject::connect(proc, &QProcess::errorOccurred, proc,
                     [proc, onError](QProcess::ProcessError) {
                         if (onError)
                             onError(proc->errorString());
                         proc->deleteLater();
                     });
    if (onProgress)
        onProgress(0);
    proc->start(exe, args);
    return proc->waitForStarted(5000);
}

bool FFmpegWrapper::convert(const QString &input, const QString &output,
                            const QString &codec,
                            std::function<void(int)> onProgress,
                            std::function<void(const QString &)> onError,
                            std::function<void()> onSuccess)
{
    const QString exe = getFFmpegPath();
    if (exe.isEmpty()) {
        if (onError)
            onError(QStringLiteral("ffmpeg executable not found"));
        return false;
    }
    auto *proc = new QProcess();
    QObject::connect(proc, &QProcess::finished, proc,
                     [proc, onSuccess, onError](int code, QProcess::ExitStatus) {
                         proc->deleteLater();
                         if (code == 0) {
                             if (onSuccess)
                                 onSuccess();
                         } else if (onError) {
                             onError(QString::fromLocal8Bit(proc->readAllStandardError()));
                         }
                     });
    if (onProgress)
        onProgress(0);
    proc->start(exe, { QStringLiteral("-y"), QStringLiteral("-i"), input,
                       QStringLiteral("-c:v"), codec, output });
    return proc->waitForStarted(5000);
}

// ── Extractors ───────────────────────────────────────────────────────────────

namespace {

QString sanitizeFilename(const QString &name)
{
    QString out = name.trimmed();
    if (out.isEmpty())
        return QStringLiteral("video");
    static const QRegularExpression bad(QStringLiteral("[\\\\/:*?\"<>|]"));
    out.replace(bad, QStringLiteral("_"));
    return out.left(120);
}

bool looksLikeDirectMedia(const QUrl &url)
{
    const QString path = url.path().toLower();
    return path.endsWith(QStringLiteral(".mp4")) || path.endsWith(QStringLiteral(".m3u8"))
        || path.endsWith(QStringLiteral(".webm")) || path.endsWith(QStringLiteral(".m4a"))
        || path.endsWith(QStringLiteral(".mp3")) || path.endsWith(QStringLiteral(".mov"));
}

// Fetches a page and extracts og:video / og:title / og:image meta tags.
void fetchOpenGraph(const QString &pageUrl,
                    std::function<void(QVariantMap)> onDone,
                    std::function<void(QString)> onError)
{
    auto *nam = new QNetworkAccessManager();
    QNetworkRequest req{QUrl(pageUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) BLACK/1.0"));
    QNetworkReply *reply = nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, nam,
                     [nam, reply, onDone, onError]() {
                         QVariantMap meta;
                         if (reply->error() == QNetworkReply::NoError) {
                             const QString html = QString::fromUtf8(reply->readAll()).left(200000);
                             const auto grab = [&html](const QString &prop) {
                                 QRegularExpression re(
                                     QStringLiteral("<meta[^>]*property=[\"']%1[\"'][^>]*content=[\"']([^\"']+)")
                                         .arg(prop));
                                 const auto m = re.match(html);
                                 if (m.hasMatch())
                                     return m.captured(1);
                                 QRegularExpression re2(
                                     QStringLiteral("<meta[^>]*content=[\"']([^\"']+)[\"'][^>]*property=[\"']%1[\"']")
                                         .arg(prop));
                                 const auto m2 = re2.match(html);
                                 return m2.hasMatch() ? m2.captured(1) : QString();
                             };
                             meta.insert(QStringLiteral("video"), grab(QStringLiteral("og:video")));
                             meta.insert(QStringLiteral("title"), grab(QStringLiteral("og:title")));
                             meta.insert(QStringLiteral("image"), grab(QStringLiteral("og:image")));
                             onDone(meta);
                         } else {
                             onError(reply->errorString());
                         }
                         reply->deleteLater();
                         nam->deleteLater();
                     });
}

class GenericExtractor : public IVideoExtractor {
public:
    QString platformName() const override { return QStringLiteral("generic"); }
    bool canHandle(const QUrl &url) const override
    {
        const QString scheme = url.scheme().toLower();
        return (scheme == QLatin1String("http") || scheme == QLatin1String("https"))
            && looksLikeDirectMedia(url);
    }
    bool extractVideoInfo(const QString &url,
                          std::function<void(const VideoInfo &)> onSuccess,
                          std::function<void(const QString &)> /*onError*/) override
    {
        VideoInfo info;
        info.url = url;
        const QUrl u(url);
        info.title = sanitizeFilename(QFileInfo(u.path()).completeBaseName());
        info.platform = QStringLiteral("generic");
        info.qualityUrls.insert(VideoQuality::Best, url);
        onSuccess(info);
        return true;
    }
};

class OpenGraphExtractor : public IVideoExtractor {
public:
    OpenGraphExtractor(QString platform, QStringList hosts)
        : m_platform(std::move(platform)), m_hosts(std::move(hosts)) {}
    QString platformName() const override { return m_platform; }
    bool canHandle(const QUrl &url) const override
    {
        const QString host = url.host().toLower();
        for (const QString &h : m_hosts) {
            if (host == h || host.endsWith(QLatin1Char('.') + h))
                return true;
        }
        return false;
    }
    bool extractVideoInfo(const QString &url,
                          std::function<void(const VideoInfo &)> onSuccess,
                          std::function<void(const QString &)> onError) override
    {
        fetchOpenGraph(url,
                       [url, plat = m_platform, onSuccess, onError](const QVariantMap &meta) {
                           const QString video = meta.value(QStringLiteral("video")).toString();
                           if (video.isEmpty()) {
                               onError(QStringLiteral("No embeddable og:video found on this page"));
                               return;
                           }
                           VideoInfo info;
                           info.url = url;
                           const QString title = meta.value(QStringLiteral("title")).toString();
                           info.title = sanitizeFilename(title.isEmpty() ? plat : title);
                           info.thumbnailUrl = meta.value(QStringLiteral("image")).toString();
                           info.platform = plat;
                           info.qualityUrls.insert(VideoQuality::Best, video);
                           onSuccess(info);
                       },
                       onError);
        return true;
    }

private:
    QString m_platform;
    QStringList m_hosts;
};

} // namespace

// ── VideoDownloader ──────────────────────────────────────────────────────────

class VideoDownloader::Private {
public:
    QNetworkAccessManager *nam = nullptr;
    std::vector<std::unique_ptr<IVideoExtractor>> extractors;
    QMap<QString, DownloadTask *> tasks;
    int seq = 0;
};

VideoDownloader::VideoDownloader(QObject *parent)
    : QObject(parent), d(new Private)
{
    d->nam = new QNetworkAccessManager(this);
    qRegisterMetatype<VideoInfo>("VideoInfo");
    qRegisterMetatype<DownloadProgress>("DownloadProgress");
}

VideoDownloader::~VideoDownloader()
{
    for (DownloadTask *t : d->tasks) {
        if (t->reply)
            t->reply->abort();
        if (t->ffmpegProcess) {
            t->ffmpegProcess->kill();
            t->ffmpegProcess->deleteLater();
        }
        delete t;
    }
    delete d;
}

void VideoDownloader::registerExtractor(std::unique_ptr<IVideoExtractor> extractor)
{
    if (extractor)
        d->extractors.push_back(std::move(extractor));
}

void VideoDownloader::registerDefaultExtractors(VideoDownloader *downloader)
{
    if (!downloader)
        return;
    downloader->registerExtractor(std::make_unique<OpenGraphExtractor>(
        QStringLiteral("facebook"),
        QStringList{ QStringLiteral("facebook.com"), QStringLiteral("fb.watch") }));
    downloader->registerExtractor(std::make_unique<OpenGraphExtractor>(
        QStringLiteral("instagram"),
        QStringList{ QStringLiteral("instagram.com") }));
    downloader->registerExtractor(std::make_unique<OpenGraphExtractor>(
        QStringLiteral("twitter"),
        QStringList{ QStringLiteral("twitter.com"), QStringLiteral("x.com") }));
    downloader->registerExtractor(std::make_unique<GenericExtractor>());
}

QString VideoDownloader::downloadVideo(const QUrl &url, VideoQuality quality,
                                       const QString &outputDir,
                                       const QString &customFilename)
{
    IVideoExtractor *handler = nullptr;
    for (auto &e : d->extractors) {
        if (e->canHandle(url)) {
            handler = e.get();
            break;
        }
    }
    // Fall back to direct-media handling even without a registered extractor.
    GenericExtractor generic;
    const bool genericOk = generic.canHandle(url);
    if (!handler && !genericOk) {
        emit downloadFailed(QString(),
                            QStringLiteral("Unsupported URL: only direct media files "
                                           "(.mp4/.m3u8/.webm) or Facebook/Instagram/X links "
                                           "with an embeddable og:video are supported"));
        return QString();
    }

    const QString id = QStringLiteral("vd-%1-%2").arg(
        QString::number(++d->seq),
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));

    auto *task = new DownloadTask();
    task->id = id;
    task->quality = quality;
    task->outputDir = outputDir.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        : outputDir;
    QDir().mkpath(task->outputDir);
    task->progress.downloadId = id;
    task->progress.status = DownloadStatus::Preparing;
    d->tasks.insert(id, task);

    QPointer<VideoDownloader> self(this);
    const auto onInfo = [this, self, id, customFilename](const VideoInfo &info) {
        if (!self || !d->tasks.contains(id))
            return;
        DownloadTask *t = d->tasks.value(id);
        t->info = info;
        const QString base = customFilename.isEmpty() ? info.title : sanitizeFilename(customFilename);
        t->outputFile = QDir(t->outputDir).filePath(base + QStringLiteral(".mp4"));
        t->tempVideoFile = QDir(t->outputDir).filePath(base + QStringLiteral(".") + id + QStringLiteral(".part"));
        t->progress.status = DownloadStatus::Downloading;
        emit downloadStarted(id, info);
        // Pick the requested quality, else Best, else first available.
        QString mediaUrl = info.qualityUrls.value(t->quality);
        if (mediaUrl.isEmpty())
            mediaUrl = info.qualityUrls.value(VideoQuality::Best);
        if (mediaUrl.isEmpty() && !info.qualityUrls.isEmpty())
            mediaUrl = info.qualityUrls.first();
        if (mediaUrl.isEmpty()) {
            t->progress.status = DownloadStatus::Failed;
            t->progress.errorMessage = QStringLiteral("Extractor returned no playable URL");
            emit downloadFailed(id, t->progress.errorMessage);
            d->tasks.remove(id);
            delete t;
            return;
        }
        if (mediaUrl.contains(QStringLiteral(".m3u8"))) {
            // Fetch the playlist, then download segments sequentially.
            QNetworkRequest req{QUrl(mediaUrl)};
            QNetworkReply *reply = d->nam->get(req);
            t->reply = reply;
            QObject::connect(reply, &QNetworkReply::finished, this,
                             [this, self, id, mediaUrl]() {
                                 if (!self || !d->tasks.contains(id))
                                     return;
                                 DownloadTask *tt = d->tasks.value(id);
                                 tt->reply = nullptr;
                                 QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(
                                     qobject_cast<QNetworkReply *>(sender()));
                                 if (guard->error() != QNetworkReply::NoError) {
                                     tt->progress.status = DownloadStatus::Failed;
                                     tt->progress.errorMessage = guard->errorString();
                                     emit downloadFailed(id, tt->progress.errorMessage);
                                     d->tasks.remove(id);
                                     delete tt;
                                     return;
                                 }
                                 M3U8Parser::Playlist pl = M3U8Parser::parse(
                                     QString::fromUtf8(guard->readAll()), mediaUrl);
                                 if (pl.isMaster && !pl.mediaPlaylists.isEmpty()) {
                                     // One level of master indirection is enough
                                     // for the extractor-provided playlists.
                                     const QString sub = pl.mediaPlaylists.first();
                                     QNetworkRequest subReq{QUrl(sub)};
                                     QNetworkReply *subReply = d->nam->get(subReq);
                                     tt->reply = subReply;
                                     QObject::connect(subReply, &QNetworkReply::finished, this,
                                                      [this, self, id, sub]() {
                                                          if (!self || !d->tasks.contains(id))
                                                              return;
                                                          DownloadTask *t2 = d->tasks.value(id);
                                                          t2->reply = nullptr;
                                                          QScopedPointer<QNetworkReply,
                                                                         QScopedPointerDeleteLater>
                                                              g2(qobject_cast<QNetworkReply *>(sender()));
                                                          if (g2->error() != QNetworkReply::NoError) {
                                                              t2->progress.status = DownloadStatus::Failed;
                                                              t2->progress.errorMessage = g2->errorString();
                                                              emit downloadFailed(id, t2->progress.errorMessage);
                                                              d->tasks.remove(id);
                                                              delete t2;
                                                              return;
                                                          }
                                                          t2->segments = M3U8Parser::parse(
                                                                                 QString::fromUtf8(g2->readAll()), sub)
                                                                             .segments;
                                                          t2->currentSegment = 0;
                                                          t2->progress.totalBytes = t2->segments.size();
                                                          // Fall through to segment pump below.
                                                          QTimer::singleShot(0, this, [this, self, id]() {
                                                              if (self && d->tasks.contains(id))
                                                                  downloadNextSegment(id);
                                                          });
                                                      });
                                     return;
                                 }
                                 tt->segments = pl.segments;
                                 tt->currentSegment = 0;
                                 tt->progress.totalBytes = tt->segments.size();
                                 downloadNextSegment(id);
                             });
        } else {
            // Direct file download.
            QNetworkRequest req{QUrl(mediaUrl)};
            QNetworkReply *reply = d->nam->get(req);
            t->reply = reply;
            QFile *out = new QFile(t->tempVideoFile, reply);
            if (!out->open(QIODevice::WriteOnly)) {
                t->progress.status = DownloadStatus::Failed;
                t->progress.errorMessage = QStringLiteral("Cannot write output file");
                emit downloadFailed(id, t->progress.errorMessage);
                d->tasks.remove(id);
                delete t;
                return;
            }
            QObject::connect(reply, &QNetworkReply::readyRead, this, [reply, out]() {
                out->write(reply->readAll());
            });
            QObject::connect(reply, &QNetworkReply::downloadProgress, this,
                             [this, self, id](qint64 received, qint64 total) {
                                 if (!self || !d->tasks.contains(id))
                                     return;
                                 DownloadTask *tt = d->tasks.value(id);
                                 tt->progress.bytesDownloaded = received;
                                 tt->progress.totalBytes = total;
                                 tt->progress.progress = (total > 0)
                                     ? (double(received) / double(total))
                                     : 0.0;
                                 emit downloadProgress(id, tt->progress);
                             });
            QObject::connect(reply, &QNetworkReply::finished, this,
                             [this, self, id, out]() {
                                 out->close();
                                 QNetworkReply *r = qobject_cast<QNetworkReply *>(sender());
                                 if (r)
                                     r->deleteLater();
                                 if (!self || !d->tasks.contains(id)) {
                                     return;
                                 }
                                 DownloadTask *tt = d->tasks.value(id);
                                 tt->reply = nullptr;
                                 const bool ok = r && r->error() == QNetworkReply::NoError;
                                 if (!ok) {
                                     tt->progress.status = DownloadStatus::Failed;
                                     tt->progress.errorMessage = r ? r->errorString()
                                                                   : QStringLiteral("Download failed");
                                     QFile::remove(tt->tempVideoFile);
                                     emit downloadFailed(id, tt->progress.errorMessage);
                                     d->tasks.remove(id);
                                     delete tt;
                                     return;
                                 }
                                 QFile::remove(tt->outputFile);
                                 QFile::rename(tt->tempVideoFile, tt->outputFile);
                                 tt->progress.status = DownloadStatus::Completed;
                                 tt->progress.progress = 1.0;
                                 emit downloadFinished(id, tt->outputFile);
                                 d->tasks.remove(id);
                                 delete tt;
                             });
        }
    };
    const auto onError = [this, self, id](const QString &err) {
        if (!self || !d->tasks.contains(id))
            return;
        DownloadTask *t = d->tasks.value(id);
        t->progress.status = DownloadStatus::Failed;
        t->progress.errorMessage = err;
        emit downloadFailed(id, err);
        d->tasks.remove(id);
        delete t;
    };

    if (handler)
        handler->extractVideoInfo(url.toString(), onInfo, onError);
    else
        generic.extractVideoInfo(url.toString(), onInfo, onError);
    return id;
}

void VideoDownloader::downloadNextSegment(const QString &downloadId)
{
    // Private segment pump, invoked via singleShot from downloadVideo().
    if (!d->tasks.contains(downloadId))
        return;
    DownloadTask *t = d->tasks.value(downloadId);
    if (t->currentSegment >= t->segments.size()) {
        QFile::remove(t->outputFile);
        QFile::rename(t->tempVideoFile, t->outputFile);
        t->progress.status = DownloadStatus::Completed;
        t->progress.progress = 1.0;
        emit downloadFinished(downloadId, t->outputFile);
        d->tasks.remove(downloadId);
        delete t;
        return;
    }
    const QString segUrl = t->segments.at(t->currentSegment).url;
    QPointer<VideoDownloader> self(this);
    QNetworkReply *reply = d->nam->get(QNetworkRequest{QUrl(segUrl)});
    t->reply = reply;
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, self, downloadId]() {
                         if (!self || !d->tasks.contains(downloadId))
                             return;
                         DownloadTask *tt = d->tasks.value(downloadId);
                         tt->reply = nullptr;
                         QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(
                             qobject_cast<QNetworkReply *>(sender()));
                         if (guard->error() != QNetworkReply::NoError) {
                             tt->progress.status = DownloadStatus::Failed;
                             tt->progress.errorMessage = guard->errorString();
                             QFile::remove(tt->tempVideoFile);
                             emit downloadFailed(downloadId, tt->progress.errorMessage);
                             d->tasks.remove(downloadId);
                             delete tt;
                             return;
                         }
                         QFile out(tt->tempVideoFile);
                         if (!out.open(QIODevice::Append)) {
                             tt->progress.status = DownloadStatus::Failed;
                             tt->progress.errorMessage = QStringLiteral("Cannot write segment file");
                             emit downloadFailed(downloadId, tt->progress.errorMessage);
                             d->tasks.remove(downloadId);
                             delete tt;
                             return;
                         }
                         out.write(guard->readAll());
                         tt->currentSegment++;
                         tt->progress.bytesDownloaded = tt->currentSegment;
                         tt->progress.progress = (tt->progress.totalBytes > 0)
                             ? (double(tt->currentSegment) / double(tt->progress.totalBytes))
                             : 0.0;
                         emit downloadProgress(downloadId, tt->progress);
                         QTimer::singleShot(0, this, [this, self, downloadId]() {
                             if (self && d->tasks.contains(downloadId))
                                 downloadNextSegment(downloadId);
                         });
                     });
}

void VideoDownloader::cancelDownload(const QString &downloadId)
{
    if (!d->tasks.contains(downloadId))
        return;
    DownloadTask *t = d->tasks.take(downloadId);
    t->progress.status = DownloadStatus::Cancelled;
    if (t->reply)
        t->reply->abort();
    if (t->ffmpegProcess) {
        t->ffmpegProcess->kill();
        t->ffmpegProcess->deleteLater();
    }
    QFile::remove(t->tempVideoFile);
    QFile::remove(t->tempAudioFile);
    delete t;
    emit downloadCancelled(downloadId);
}

DownloadProgress VideoDownloader::getProgress(const QString &downloadId) const
{
    if (d->tasks.contains(downloadId))
        return d->tasks.value(downloadId)->progress;
    DownloadProgress p;
    p.downloadId = downloadId;
    p.status = DownloadStatus::Failed;
    p.errorMessage = QStringLiteral("Unknown download id");
    return p;
}

QList<DownloadProgress> VideoDownloader::getAllProgress() const
{
    QList<DownloadProgress> out;
    for (DownloadTask *t : d->tasks)
        out.append(t->progress);
    return out;
}

void VideoDownloader::setFFmpegPath(const QString &path)
{
    FFmpegWrapper::setFFmpegPath(path);
    emit ffmpegStatusChanged(isFFmpegAvailable());
}

bool VideoDownloader::isFFmpegAvailable() const
{
    return FFmpegWrapper::isAvailable();
}

} // namespace VideoDownloader
