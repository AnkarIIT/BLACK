#include <QtTest>
#include <QUrl>

#include "../VideoDownloader.h"

using namespace VideoDownloader;

class TestVideoDownloader : public QObject
{
    Q_OBJECT

private slots:
    void testIsMasterPlaylist_data()
    {
        QTest::addColumn<QString>("content");
        QTest::addColumn<bool>("expected");
        QTest::newRow("master-stream-inf")
            << "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000\nlow.m3u8\n" << true;
        QTest::newRow("master-session-data")
            << "#EXTM3U\n#EXT-X-SESSION-DATA:DATA-ID=\"x\"\n" << true;
        QTest::newRow("media-playlist")
            << "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#EXTINF:10.0,\nseg0.ts\n" << false;
        QTest::newRow("empty") << QString() << false;
    }

    void testIsMasterPlaylist()
    {
        QFETCH(QString, content);
        QFETCH(bool, expected);
        QCOMPARE(M3U8Parser::isMasterPlaylist(content), expected);
    }

    void testResolveUrl()
    {
        QCOMPARE(M3U8Parser::resolveUrl("https://cdn.example.com/hls/master.m3u8", "low.m3u8"),
                 QString("https://cdn.example.com/hls/low.m3u8"));
        // Absolute URLs pass through untouched.
        QCOMPARE(M3U8Parser::resolveUrl("https://cdn.example.com/hls/master.m3u8",
                                        "https://other.example.com/v.m3u8"),
                 QString("https://other.example.com/v.m3u8"));
        QCOMPARE(M3U8Parser::resolveUrl("https://cdn.example.com/hls/master.m3u8", QString()),
                 QString());
    }

    void testParseMasterPlaylist()
    {
        const QString base = QStringLiteral("https://cdn.example.com/hls/master.m3u8");
        const QString content = QStringLiteral(
            "#EXTM3U\n"
            "#EXT-X-STREAM-INF:BANDWIDTH=800000,RESOLUTION=640x360\n"
            "low.m3u8\n"
            "#EXT-X-STREAM-INF:BANDWIDTH=2000000,RESOLUTION=1280x720\n"
            "hi.m3u8\n"
            "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"audio\",URI=\"audio.m3u8\"\n");
        const M3U8Parser::Playlist pl = M3U8Parser::parse(content, base);
        QVERIFY(pl.isMaster);
        QCOMPARE(pl.segments.size(), 0);
        QCOMPARE(pl.mediaPlaylists.size(), 2);
        QVERIFY(pl.mediaPlaylists.value(QStringLiteral("640x360"))
                == QStringLiteral("https://cdn.example.com/hls/low.m3u8"));
        QVERIFY(pl.mediaPlaylists.value(QStringLiteral("1280x720"))
                == QStringLiteral("https://cdn.example.com/hls/hi.m3u8"));
        QCOMPARE(pl.audioPlaylists.size(), 1);
    }

    void testParseMediaPlaylist()
    {
        const QString base = QStringLiteral("https://cdn.example.com/hls/low.m3u8");
        const QString content = QStringLiteral(
            "#EXTM3U\n"
            "#EXT-X-TARGETDURATION:10\n"
            "#EXTINF:9.5,First\n"
            "seg0.ts\n"
            "#EXTINF:10.0,\n"
            "seg1.ts\n"
            "#EXT-X-ENDLIST\n");
        const M3U8Parser::Playlist pl = M3U8Parser::parse(content, base);
        QVERIFY(!pl.isMaster);
        QCOMPARE(pl.segments.size(), 2);
        QCOMPARE(pl.segments.at(0).url, QString("https://cdn.example.com/hls/seg0.ts"));
        QVERIFY(qAbs(pl.segments.at(0).duration - 9.5) < 0.0001);
        QCOMPARE(pl.segments.at(0).title, QString("First"));
        QCOMPARE(pl.segments.at(1).url, QString("https://cdn.example.com/hls/seg1.ts"));
    }

    void testParseIgnoresCommentsAndBlankLines()
    {
        const QString base = QStringLiteral("https://cdn.example.com/a.m3u8");
        const QString content = QStringLiteral(
            "#EXTM3U\n\n#EXT-X-VERSION:3\n#EXTINF:5.0,\nseg.ts\n\n");
        const M3U8Parser::Playlist pl = M3U8Parser::parse(content, base);
        QCOMPARE(pl.segments.size(), 1);
        QCOMPARE(pl.segments.at(0).url, QString("https://cdn.example.com/seg.ts"));
    }
};

QTEST_MAIN(TestVideoDownloader)
#include "test_videodownloader.moc"
