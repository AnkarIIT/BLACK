#ifndef M3U8PARSER_H
#define M3U8PARSER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QUrl>
#include <QUrlQuery>

// M3U8/HLS playlist parser
// Supports master playlists, media playlists, AES-128 encryption
class M3U8Parser
{
public:
    struct Segment {
        QString url;
        double duration = 0.0;
        QString title;
        QMap<QString, QString> attributes;
        bool encrypted = false;
        QString encryptionKeyUrl;
        QString encryptionMethod;
        QByteArray encryptionKey;
        QByteArray encryptionIV;
        int sequenceNumber = -1;
    };

    struct Playlist {
        QString url;
        QString baseUrl;
        bool isMaster = false;
        QList<Segment> segments;
        QMap<QString, QString> mediaPlaylists;   // quality -> playlist URL
        QMap<QString, QString> audioPlaylists;   // group-id -> playlist URL
        QMap<QString, QString> subtitlePlaylists; // group-id -> playlist URL
        QMap<QString, QString> attributes;
        QString encryptionKeyUrl;
        QString encryptionMethod;
        QByteArray encryptionKey;
        QByteArray encryptionIV;
        int targetDuration = 0;
        int mediaSequence = 0;
        bool isLive = false;
    };

    // Parse M3U8 content
    static Playlist parse(const QString& content, const QString& baseUrl);
    
    // Check if content is a master playlist
    static bool isMasterPlaylist(const QString& content);
    
    // Resolve relative URL against base URL
    static QString resolveUrl(const QString& baseUrl, const QString& relativeUrl);
    
    // Parse AES-128 encryption info
    static bool parseEncryption(const QString& line, QString& method, QString& keyUrl, QString& iv, QString& keyFormat, QString& keyFormatVersions);
    
    // Parse EXT-X-KEY line
    static bool parseKeyLine(const QString& line, QString& method, QString& uri, QString& iv, QString& keyFormat, QString& keyFormatVersions);
    
    // Parse EXT-X-MEDIA line
    static bool parseMediaLine(const QString& line, QString& type, QString& groupId, QString& name, QString& language, QString& autoSelect, QString& defaultFlag, QString& forced, QString& uri);
    
    // Parse EXT-X-STREAM-INF line
    static bool parseStreamInfLine(const QString& line, QString& bandwidth, QString& averageBandwidth, QString& codecs, QString& resolution, QString& frameRate, QString& hdcpLevel, QString& audio, QString& video, QString& subtitles, QString& closedCaptions);
    
    // Parse segment line
    static bool parseSegmentLine(const QString& line, double& duration, QString& title, QMap<QString, QString>& attributes);
    
    // Parse EXTINF line
    static bool parseExtInfLine(const QString& line, double& duration, QString& title);
    
    // Parse EXT-X-BYTERANGE line
    static bool parseByteRangeLine(const QString& line, qint64& length, qint64& offset);
    
    // Fetch and decrypt segment
    static QByteArray fetchSegment(const QString& url, const QByteArray& key, const QByteArray& iv, const QString& method);
    
    // Decrypt AES-128 segment
    static QByteArray decryptAES128(const QByteArray& data, const QByteArray& key, const QByteArray& iv);
    
    // Fetch encryption key
    static QByteArray fetchEncryptionKey(const QString& keyUrl);
    
private:
    M3U8Parser() = delete;
};

#endif // M3U8PARSER_H