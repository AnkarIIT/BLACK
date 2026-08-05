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
#include "BrowserWindow.h"
#include "TrackerBlocker.h"

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
    
    // Performance and GPU acceleration settings for system default graphics
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    
    // System default GPU settings for smooth experience
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    
    // Enable smooth animations and transitions
    settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);
    settings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

    profile->setHttpUserAgent(QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.6261.167 Safari/537.36 BLACK/1.0"));

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
    if (isFirstRun()) {
        // First run: Show login page
        window.loadLoginPage();
        markFirstRunComplete();
    } else {
        // Normal run: Show start page
        window.loadStartPage();
    }

    return app.exec();
}