#include "M3U8Parser.h"

#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace {
const QRegularExpression EXT_X_KEY_RE(R"delim(#EXT-X-KEY:([^,\n]+)(?:,(.*))?)delim");
const QRegularExpression EXT_X_MEDIA_RE(R"delim(#EXT-X-MEDIA:([^,\n]+)(?:,(.*))?)delim");
const QRegularExpression EXT_X_STREAM_INF_RE(R"delim(#EXT-X-STREAM-INF:([^,\n]+)(?:,(.*))?)delim");
const QRegularExpression EXTINF_RE(R"delim(#EXTINF:([0-9.]+)(?:,(.*))?)delim");
const QRegularExpression EXT_X_BYTERANGE_RE(R"delim(#EXT-X-BYTERANGE:([^,\n]+))delim");
const QRegularExpression ATTRIBUTE_RE(R"delim(([A-Z0-9-]+)=(\"(?:[^\"]*)\"|[^,]*))delim");
}

M3U8Parser::Playlist M3U8Parser::parse(const QString& content, const QString& baseUrl)
{
    Playlist playlist;
    playlist.url = baseUrl;
    
    // Extract base URL (directory part)
    QUrl base(baseUrl);
    playlist.baseUrl = base.resolved(QUrl(".")).toString();
    
    QStringList lines = content.split('\n');
    Playlist::Segment currentSegment;
    bool inSegment = false;
    
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;
        
        if (line.startsWith("#EXTM3U")) {
            // M3U header
            continue;
        }
        else if (line.startsWith("#EXT-X-VERSION:")) {
            // Version
            continue;
        }
        else if (line.startsWith("#EXT-X-TARGETDURATION:")) {
            playlist.targetDuration = line.mid(20).toInt();
        }
        else if (line.startsWith("#EXT-X-MEDIA-SEQUENCE:")) {
            playlist.mediaSequence = line.mid(21).toInt();
        }
        else if (line.startsWith("#EXT-X-PLAYLIST-TYPE:")) {
            QString type = line.mid(21);
            playlist.isLive = (type != "VOD");
        }
        else if (line.startsWith("#EXT-X-KEY:")) {
            QString method, keyUrl, iv, keyFormat, keyFormatVersions;
            if (parseKeyLine(line, method, keyUrl, iv, keyFormat, keyFormatVersions)) {
                playlist.encryptionMethod = method;
                playlist.encryptionKeyUrl = keyUrl;
                if (!iv.isEmpty()) {
                    bool ok;
                    playlist.encryptionIV = QByteArray::fromHex(iv.toLatin1());
                    if (!ok && iv.startsWith("0x")) {
                        playlist.encryptionIV = QByteArray::fromHex(iv.mid(2).toLatin1());
                    }
                }
                if (!keyUrl.isEmpty()) {
                    playlist.encryptionKeyUrl = resolveUrl(baseUrl, keyUrl);
                    playlist.encryptionKey = fetchEncryptionKey(playlist.encryptionKeyUrl);
                }
            }
        }
        else if (line.startsWith("#EXT-X-MEDIA:")) {
            QString type, groupId, name, language, autoSelect, defaultFlag, forced, uri;
            if (parseMediaLine(line, type, groupId, name, language, autoSelect, defaultFlag, forced, uri)) {
                QString resolvedUri = uri.isEmpty() ? QString() : resolveUrl(baseUrl, uri);
                if (type == "AUDIO") {
                    playlist.audioPlaylists[groupId] = resolvedUri;
                } else if (type == "SUBTITLES") {
                    playlist.subtitlePlaylists[groupId] = resolvedUri;
                }
            }
        }
        else if (line.startsWith("#EXT-X-STREAM-INF:")) {
            QString bandwidth, avgBandwidth, codecs, resolution, frameRate, hdcpLevel, audio, video, subtitles, closedCaptions;
            if (parseStreamInfLine(line, bandwidth, avgBandwidth, codecs, resolution, frameRate, hdcpLevel, audio, video, subtitles, closedCaptions)) {
                // This is a master playlist entry
                playlist.isMaster = true;
                // The next non-comment line will be the playlist URL
                // We'll handle this in the next iteration
            }
        }
        else if (line.startsWith("#EXTINF:")) {
            double duration = 0;
            QString title;
            QMap<QString, QString> attrs;
            if (parseSegmentLine(line, duration, title, attrs)) {
                currentSegment.duration = duration;
                currentSegment.title = title;
                currentSegment.attributes = attrs;
                inSegment = true;
            }
        }
        else if (line.startsWith("#EXT-X-BYTERANGE:")) {
            if (inSegment) {
                qint64 length = 0, offset = 0;
                parseByteRangeLine(line, currentSegment.attributes["BYTERANGE_LENGTH"].toLongLong(), 
                                  currentSegment.attributes["BYTERANGE_OFFSET"].toLongLong());
            }
        }
        else if (line.startsWith("#EXT-X-KEY:") && inSegment) {
            // Segment-level key
            QString method, keyUrl, iv, keyFormat, keyFormatVersions;
            if (parseKeyLine(line, method, keyUrl, iv, keyFormat, keyFormatVersions)) {
                currentSegment.encryptionMethod = method;
                currentSegment.encryptionKeyUrl = keyUrl.isEmpty() ? QString() : resolveUrl(baseUrl, keyUrl);
                currentSegment.encrypted = !method.isEmpty() && method != "NONE";
                if (!iv.isEmpty()) {
                    bool ok;
                    currentSegment.encryptionIV = QByteArray::fromHex(iv.toLatin1());
                }
                if (!keyUrl.isEmpty()) {
                    currentSegment.encryptionKeyUrl = resolveUrl(baseUrl, keyUrl);
                    currentSegment.encryptionKey = fetchEncryptionKey(currentSegment.encryptionKeyUrl);
                }
            }
        }
        else if (line.startsWith("#")) {
            // Other tags - ignore for now
        }
        else {
            // This is a segment URL
            if (inSegment) {
                currentSegment.url = resolveUrl(baseUrl, line);
                currentSegment.sequenceNumber = playlist.segments.size() + playlist.mediaSequence;
                playlist.segments.append(currentSegment);
                currentSegment = Playlist::Segment();
                inSegment = false;
            }
            else if (playlist.isMaster) {
                // Master playlist - this is a variant playlist URL
                QString resolvedUrl = resolveUrl(baseUrl, line);
                // Determine quality from attributes (would need to store stream-inf attributes)
                playlist.mediaPlaylists[QString("variant_%1").arg(playlist.mediaPlaylists.size())] = line;
            }
        }
    }
    
    return playlist;
}

bool M3U8Parser::isMasterPlaylist(const QString& content)
{
    return content.contains("#EXT-X-STREAM-INF:");
}

QString M3U8Parser::resolveUrl(const QString& baseUrl, const QString& relativeUrl)
{
    QUrl base(baseUrl);
    QUrl relative(relativeUrl);
    return base.resolved(relative).toString();
}

bool M3U8Parser::parseKeyLine(const QString& line, QString& method, QString& keyUrl, QString& iv, QString& keyFormat, QString& keyFormatVersions)
{
    method.clear(); keyUrl.clear(); iv.clear(); keyFormat.clear(); keyFormatVersions.clear();
    
    QString params = line.mid(11); // Skip "#EXT-X-KEY:"
    QRegularExpression attrRe(R"(([A-Z0-9-]+)=("(?:[^"]*)"|[^,]*))");
    QRegularExpressionMatchIterator it = attrRe.globalMatch(params);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);
        
        // Remove quotes if present
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        
        if (key == "METHOD") method = value;
        else if (key == "URI") keyUrl = value;
        else if (key == "IV") iv = value;
        else if (key == "KEYFORMAT") keyFormat = value;
        else if (key == "KEYFORMATVERSIONS") keyFormatVersions = value;
    }
    
    return !method.isEmpty();
}

bool M3U8Parser::parseMediaLine(const QString& line, QString& type, QString& groupId, QString& name, QString& language, QString& autoSelect, QString& defaultFlag, QString& forced, QString& uri)
{
    type.clear(); groupId.clear(); name.clear(); language.clear();
    autoSelect.clear(); defaultFlag.clear(); forced.clear(); uri.clear();
    
    QString params = line.mid(12);
    QRegularExpression attrRe(R"(([A-Z0-9-]+)=("(?:[^"]*)"|[^,]*))");
    QRegularExpressionMatchIterator it = attrRe.globalMatch(params);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);
        
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        
        if (key == "TYPE") type = value;
        else if (key == "GROUP-ID") groupId = value;
        else if (key == "NAME") name = value;
        else if (key == "LANGUAGE") language = value;
        else if (key == "AUTOSELECT") autoSelect = value;
        else if (key == "DEFAULT") defaultFlag = value;
        else if (key == "FORCED") forced = value;
        else if (key == "URI") uri = value;
    }
    
    return !type.isEmpty();
}

bool M3U8Parser::parseStreamInfLine(const QString& line, QString& bandwidth, QString& averageBandwidth, QString& codecs, QString& resolution, QString& frameRate, QString& hdcpLevel, QString& audio, QString& video, QString& subtitles, QString& closedCaptions)
{
    bandwidth.clear(); averageBandwidth.clear(); codecs.clear(); resolution.clear();
    frameRate.clear(); hdcpLevel.clear(); audio.clear(); video.clear(); subtitles.clear(); closedCaptions.clear();
    
    QString params = line.mid(16);
    QRegularExpression attrRe(R"(([A-Z0-9-]+)=("(?:[^"]*)"|[^,]*))");
    QRegularExpressionMatchIterator it = attrRe.globalMatch(params);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);
        
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        
        if (key == "BANDWIDTH") bandwidth = value;
        else if (key == "AVERAGE-BANDWIDTH") averageBandwidth = value;
        else if (key == "CODECS") codecs = value;
        else if (key == "RESOLUTION") resolution = value;
        else if (key == "FRAME-RATE") frameRate = value;
        else if (key == "HDCP-LEVEL") hdcpLevel = value;
        else if (key == "AUDIO") audio = value;
        else if (key == "VIDEO") video = value;
        else if (key == "SUBTITLES") subtitles = value;
        else if (key == "CLOSED-CAPTIONS") closedCaptions = value;
    }
    
    return !bandwidth.isEmpty();
}

bool M3U8Parser::parseSegmentLine(const QString& line, double& duration, QString& title, QMap<QString, QString>& attributes)
{
    duration = 0;
    title.clear();
    attributes.clear();
    
    QString params = line.mid(8); // Skip "#EXTINF:"
    int commaPos = params.indexOf(',');
    
    if (commaPos >= 0) {
        duration = params.left(commaPos).toDouble();
        title = params.mid(commaPos + 1).trimmed();
    } else {
        duration = params.toDouble();
    }
    
    return true;
}

bool M3U8Parser::parseByteRangeLine(const QString& line, qint64& length, qint64& offset)
{
    length = 0;
    offset = 0;
    
    QString params = line.mid(17); // Skip "#EXT-X-BYTERANGE:"
    QStringList parts = params.split('@');
    
    if (!parts.isEmpty()) {
        length = parts[0].toLongLong();
    }
    if (parts.size() > 1) {
        offset = parts[1].toLongLong();
    }
    
    return length > 0;
}

QByteArray M3U8Parser::fetchSegment(const QString& url, const QByteArray& key, const QByteArray& iv, const QString& method)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch segment:" << url << reply->errorString();
        reply->deleteLater();
        return QByteArray();
    }
    
    QByteArray data = reply->readAll();
    reply->deleteLater();
    
    if (method == "AES-128" && !key.isEmpty()) {
        return decryptAES128(data, key, iv);
    }
    
    return data;
}

QByteArray M3U8Parser::decryptAES128(const QByteArray& data, const QByteArray& key, const QByteArray& iv)
{
    // AES-128 CBC decryption
    // This is a simplified version - in production use QCryptographicHash with proper padding
    // For now, return data as-is (placeholder)
    // Real implementation would use OpenSSL or Qt's QCryptographicHash with proper CBC mode
    return data;
}

QByteArray M3U8Parser::fetchEncryptionKey(const QString& keyUrl)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(keyUrl);
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch encryption key:" << keyUrl << reply->errorString();
        reply->deleteLater();
        return QByteArray();
    }
    
    QByteArray key = reply->readAll();
    reply->deleteLater();
    return key;
}

bool M3U8Parser::parseExtInfLine(const QString& line, double& duration, QString& title)
{
    QString params = line.mid(8); // Skip "#EXTINF:"
    int commaPos = params.indexOf(',');
    
    if (commaPos >= 0) {
        duration = params.left(commaPos).toDouble();
        title = params.mid(commaPos + 1).trimmed();
    } else {
        duration = params.toDouble();
        title.clear();
    }
    
    return true;
}

bool M3U8Parser::parseExtInfLine(const QString& line, double& duration, QString& title, QMap<QString, QString>& attributes)
{
    attributes.clear();
    return parseExtInfLine(line, duration, title);
}