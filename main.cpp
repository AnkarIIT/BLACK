#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "BrowserWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BLACK");
    app.setOrganizationName("BLACK");
    app.setApplicationDisplayName("BLACK");

    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
    QWebEngineSettings *settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

    profile->setHttpUserAgent(QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15"));

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
