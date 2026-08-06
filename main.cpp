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
#include "BrowserWindow.h"
#include "BrowserSettings.h"
#include "TrackerBlocker.h"

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

void markFirstRunComplete() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(dataDir);
    }
    QFile markerFile(dataDir + "/.first_run_done");
    markerFile.open(QIODevice::WriteOnly);
    markerFile.write("1");
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
        "--enable-features=VizDisplayCompositor "
        "--enable-features=Accelerated2dCanvas "
        "--enable-features=NativeGpuMemoryBuffers "
        "--enable-quic "
        "--dns-prefetch-disable=false "
        "--disk-cache-size=104857600 "
        "--enable-smooth-scrolling "
        "--enable-precise-memory-info "
        "--enable-webgl-developer-extensions "
    );

    // Enable high DPI scaling for system default graphics
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName("BLACK");
    app.setOrganizationName("BLACK");
    app.setApplicationDisplayName("BLACK");
    app.setWindowIcon(QIcon(":/app.png"));

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
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    
    // Rendering optimizations
    settings->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
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

    BrowserWindow window;

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
        // First run: Show login page
        window.loadLoginPage();
        markFirstRunComplete();
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

    return app.exec();
}