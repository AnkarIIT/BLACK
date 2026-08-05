#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "BrowserWindow.h"
#include "TrackerBlocker.h"

int main(int argc, char *argv[])
{
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
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

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

    window.show();

    return app.exec();
}
