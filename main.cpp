#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QList>
#include <QUrl>
#include <QLockFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QHash>
#include "BrowserWindow.h"
#include "BrowserSettings.h"
#include "TrackerBlocker.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QWebEngineScript>
#include "Account.h"
#include "OnboardingBridge.h"

// Platform detection for User-Agent
#ifdef Q_OS_WIN
    #define PLATFORM_WINDOWS 1
#elif defined(Q_OS_MAC)
    #define PLATFORM_MACOS 1
#elif defined(Q_OS_LINUX)
    #define PLATFORM_LINUX 1
#else
    #define PLATFORM_UNKNOWN 1
#endif

QString getSafariUserAgent() {
#ifdef PLATFORM_MACOS
    // Native Safari on macOS - Safari 17.5 on macOS Sequoia 15
    return QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 15_0) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15");
#elif defined(PLATFORM_WINDOWS)
    // Safari-style browser on Windows
    return QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15 BLACK/1.0");
#elif defined(PLATFORM_LINUX)
    // Safari-style browser on Linux
    return QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15 BLACK/1.0");
#else
    // Generic Safari UA
    return QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15 BLACK/1.0");
#endif
}

// Check if this is the first run
bool isFirstRun() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(dataDir);
    }
    QFile markerFile(dataDir + "/.first_run_done");
    if (markerFile.exists()) {
        return false;
    }
    return true;
}

// ── Single-instance hand-off state ────────────────────────────────────────
// URLs a secondary instance asked us to open are queued here until the main
// window exists (it may still be inside the first-run onboarding flow when a
// hand-off connection arrives).
static BrowserWindow *g_activeWindow = nullptr;
static QList<QUrl> g_pendingUrls;

// Command-line arguments that look like URLs (BLACK flags such as
// "--open-settings" are skipped).
static QStringList commandLineUrls()
{
    QStringList urls;
    const QStringList args = QApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString a = args.at(i);
        if (a.startsWith(QLatin1Char('-')))
            continue;
        urls.append(a);
    }
    return urls;
}

// Parse a hand-off payload (newline-separated strings) into http/https URLs.
static QList<QUrl> parseHandoffUrls(const QByteArray &payload)
{
    QList<QUrl> urls;
    for (const QByteArray &line : payload.split('\n')) {
        const QString s = QString::fromUtf8(line).trimmed();
        if (s.isEmpty())
            continue;
        const QUrl url = QUrl::fromUserInput(s);
        if (url.isValid()
            && (url.scheme() == QLatin1String("http")
                || url.scheme() == QLatin1String("https")))
            urls.append(url);
    }
    return urls;
}

// Route URLs to the running window, or queue them until it exists.
static void routeOpenRequest(const QList<QUrl> &urls)
{
    if (g_activeWindow) {
        for (const QUrl &url : urls)
            g_activeWindow->addNewTab(url);
    } else {
        g_pendingUrls.append(urls);
    }
}

int main(int argc, char *argv[])
{
    // ============================================================
    // Chromium/Chrome Performance Flags (Qt WebEngine compatible)
    // These flags optimize GPU, memory, and rendering performance
    // ============================================================
    
    // Chromium command-line switches for optimal performance
    // These improve GPU rasterization, memory management, and smoothness
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        "--enable-gpu-rasterization "
        "--enable-zero-copy "
        "--enable-gpu-compositing "
        "--enable-features=VizDisplayCompositor,Accelerated2dCanvas,NativeGpuMemoryBuffers "
        "--enable-quic "
        "--dns-prefetch-disable=false "
        "--disk-cache-size=104857600 "
        "--enable-smooth-scrolling "
        "--enable-webgl-developer-extensions "
    );

    // Enable high DPI scaling for system default graphics
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName("BLACK");
    app.setOrganizationName("BLACK");
    app.setApplicationDisplayName("BLACK");
    app.setWindowIcon(QIcon(":/app.png"));

    // ════════════════════════════════════════════════════════════════════
    // Single-instance guard
    // Only one process may own the shared profile directory (QtWebEngine
    // storage, session.json, settings.json). A second BLACK.exe hands its
    // command-line URLs to the running instance — which opens them in a new
    // tab — and exits immediately, so two processes can never clobber profile
    // data or session history.
    // ════════════════════════════════════════════════════════════════════
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QLockFile lockFile(dataDir + QStringLiteral("/black.lock"));
    lockFile.setStaleLockTime(30000); // reclaim a crashed process's lock after 30s
    const QString serverName = QStringLiteral("BLACK-instance-%1").arg(qHash(dataDir));

    if (!lockFile.tryLock(100)) {
        // Another instance owns the profile: forward our URLs and quit.
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(1000)) {
            const QByteArray payload = commandLineUrls().join(QLatin1Char('\n')).toUtf8();
            socket.write(payload);
            socket.flush();
            socket.waitForBytesWritten(1000);
        }
        return 0;
    }

    // Primary instance: accept URL hand-offs from secondary instances.
    QLocalServer server;
    QLocalServer::removeServer(serverName); // clear a stale pipe from a crash
    if (server.listen(serverName)) {
        QObject::connect(&server, &QLocalServer::newConnection, &server, [&server]() {
            while (QLocalSocket *client = server.nextPendingConnection()) {
                if (!client->waitForReadyRead(1000))
                    client->abort();
                const QByteArray payload = client->readAll();
                client->deleteLater();
                routeOpenRequest(parseHandoffUrls(payload));
            }
        });
    }

    QWebEngineProfile *profile = BrowserWindow::webProfile();
    const QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/QtWebEngine");
    profile->setPersistentStoragePath(storageDir);
    profile->setCachePath(storageDir + QStringLiteral("/Cache"));
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    QWebEngineSettings *settings = profile->settings();
    
    // ============================================================
    // Core WebEngine Settings (Chromium-based optimizations)
    // ============================================================
    
    // Performance and GPU acceleration settings
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    
    // Rendering optimizations
    settings->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, false);
    settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);
    settings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);

    // Set Safari-style User-Agent based on platform detection
    profile->setHttpUserAgent(getSafariUserAgent());

    TrackerBlocker::instance().loadData();
    profile->setUrlRequestInterceptor(&TrackerBlocker::instance());
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        TrackerBlocker::instance().saveData();
    });

    // First run: cinematic BLACKHOLE setup window (frameless fullscreen modal)
    // BEFORE any browser UI exists. Finishing it writes .first_run_done (via
    // Account::completeOnboarding) and accepts the modal; we then fall through
    // to the real browser which honors the "Safari opens with" setting.
    // Closing/cancelling the setup window exits the app.
    if (isFirstRun()) {
        Account account;
        QDialog onboarding;
        onboarding.setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        onboarding.setAttribute(Qt::WA_TranslucentBackground);
        onboarding.showFullScreen();

        QVBoxLayout *layout = new QVBoxLayout(&onboarding);
        layout->setContentsMargins(0, 0, 0, 0);

        QWebEngineView *view = new QWebEngineView(&onboarding);
        OnboardingWebPage *page = new OnboardingWebPage(view);
        view->setPage(page);
        QWebChannel *channel = new QWebChannel(page);
        OnboardingBridge bridge(&account, &onboarding);
        channel->registerObject(QStringLiteral("onboardingBridge"), &bridge);
        page->setWebChannel(channel, QWebEngineScript::MainWorld);
        layout->addWidget(view);

        // Diagnostics so first-run problems surface in the log on any platform.
        QObject::connect(page, &QWebEnginePage::loadFinished, [](bool ok) {
            qInfo() << "[onboarding] load finished:" << ok;
        });
        QObject::connect(page, &OnboardingWebPage::consoleMessage,
                         [](const QString &message, int lineNumber) {
            qWarning() << "[onboarding js]" << message << "line" << lineNumber;
        });

        view->setUrl(QUrl(QStringLiteral("qrc:/onboarding_experience.html")));

        if (onboarding.exec() != QDialog::Accepted) {
            return 0; // Setup window closed without finishing
        }
    }

    BrowserWindow window;
    g_activeWindow = &window;

    const QSize desired(1400, 900);
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect avail = screen->availableGeometry();
        const QSize cap = QSize(avail.width() * 9 / 10, avail.height() * 9 / 10);
        window.resize(desired.boundedTo(cap));
        window.move(avail.center() - QPoint(window.width() / 2, window.height() / 2));
    } else {
        window.resize(desired);
    }

    // Login flow handling
    window.show();
    if (QApplication::arguments().contains(QStringLiteral("--open-settings"))) {
        QTimer::singleShot(1500, &window, &BrowserWindow::openSettingsForTesting);
    }
    if (isFirstRun()) {
        // First run: Show the onboarding/login page. The marker is only
        // written when the user actually finishes onboarding (see
        // Account::completeOnboarding in login.html), so closing the app on
        // the login page will show it again next launch.
        window.loadLoginPage();
    } else {
        // Normal run: honor "Safari opens with"
        const QString openWith = BrowserSettings::instance().opensWith();
        if (openWith == QStringLiteral("All windows from last session")
            || openWith == QStringLiteral("All non-private windows from last session")) {
            // Session was already restored in the constructor; keep those tabs.
        } else if (openWith == QStringLiteral("New private window")) {
            window.loadStartPage();
            auto *priv = new BrowserWindow(true);
            priv->setAttribute(Qt::WA_DeleteOnClose);
            priv->resize(window.size());
            priv->show();
        } else {
            window.loadStartPage();
        }
    }

    // Open URLs passed on this process's own command line, plus any handed off
    // by a secondary instance that started while the window was still coming up.
    routeOpenRequest(parseHandoffUrls(commandLineUrls().join(QLatin1Char('\n')).toUtf8()));
    if (!g_pendingUrls.isEmpty()) {
        for (const QUrl &url : g_pendingUrls)
            window.addNewTab(url);
        g_pendingUrls.clear();
    }

    return app.exec();
}