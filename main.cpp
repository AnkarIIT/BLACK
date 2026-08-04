#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "BrowserWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Safari");
    app.setOrganizationName("Apple");
    app.setApplicationDisplayName("Safari");

    QWebEngineProfile::defaultProfile()->settings()->setAttribute(
        QWebEngineSettings::FullScreenSupportEnabled, true);

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
