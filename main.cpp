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
#include "OAuthManager.h"
#include "BookmarkImporter.h"
#include "ShelfStore.h"
#include "AppearanceManager.h"
#include "SafariTheme.h"

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
        "--dns-prefetch-disable "
        "--disk-cache-size=104857600 "
        "--enable-smooth-scrolling "
        "--enable-webgl-developer-extensions "
    );

    // Enable high DPI scaling for system default graphics
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // On Linux, default to the xcb (X11/XWayland) platform. Qt WebEngine's
    // EGL/wayland surface fails to present on common Wayland setups
    // (eglSwapBuffers 0x300d), leaving the window blank and unresponsive.
    // Respect an explicit QT_QPA_PLATFORM so users can still opt into wayland.
#if defined(Q_OS_LINUX)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("BLACK");
    app.setOrganizationName("BLACK");
    app.setApplicationDisplayName("BLACK");
    app.setWindowIcon(QIcon(":/app.png"));

    // ────────────────────────────────────────────────────────────────────
    // Application-wide dark/light QSS so every native Qt dialog follows
    // BLACK's theme instead of the OS default: JavaScript alert/confirm/
    // prompt (QWebEnginePage), certificate-error message boxes, permission
    // prompts, file pickers and any other QDialog/QMessageBox.
    //
    // The sheet is rebuilt from SafariTheme's tokens, so it matches the
    // active browser theme; the per-widget stylesheets used by the chrome
    // (BrowserWindow) win over these generic rules for the widgets they
    // style explicitly.
    // ────────────────────────────────────────────────────────────────────
    {
        const auto buildQss = []() -> QString {
            const SafariTheme &t = SafariTheme::instance();
            return QStringLiteral(
                "QWidget { background-color: %1; color: %2; font-family: -apple-system, 'Segoe UI', 'SF Pro Text', sans-serif; }"
                "QMainWindow, QDialog, QMessageBox, QFileDialog, QColorDialog, QFontDialog, QInputDialog { background-color: %3; }"
                "QDialog > QWidget, QMessageBox > QWidget { background: transparent; }"
                "QLabel { background: transparent; color: %2; }"
                "QPushButton { background: %4; border: 1px solid %5; border-radius: 6px; color: %2; padding: 6px 16px; font-weight: 500; }"
                "QPushButton:hover { background: %6; }"
                "QPushButton:pressed { background: %7; }"
                "QPushButton:disabled { color: %8; background: transparent; }"
                "QPushButton:default { background: %9; border-color: %9; color: #ffffff; }"
                "QPushButton:default:hover { background: %10; }"
                "QLineEdit, QTextEdit, QPlainTextEdit { background: %11; border: 1px solid %5; border-radius: 6px; color: %2; padding: 5px 8px; selection-background-color: %7; }"
                "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus { border-color: %9; }"
                "QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit { background: %11; border: 1px solid %5; border-radius: 6px; color: %2; padding: 4px 8px; }"
                "QComboBox QAbstractItemView { background: %3; color: %2; selection-background-color: %9; border: 1px solid %5; border-radius: 4px; }"
                "QMenu { background: %3; color: %2; border: 1px solid %5; border-radius: 8px; padding: 6px; }"
                "QMenu::item { padding: 6px 22px; border-radius: 6px; }"
                "QMenu::item:selected { background: %9; color: #ffffff; }"
                "QMenu::separator { height: 1px; background: %5; margin: 6px 8px; }"
                "QToolTip { background: %3; color: %2; border: 1px solid %5; padding: 4px 8px; }"
                "QCheckBox, QRadioButton { color: %2; background: transparent; spacing: 8px; }"
                "QGroupBox { color: %2; border: 1px solid %5; border-radius: 8px; margin-top: 12px; }"
                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
                "QTabWidget::pane { border: 1px solid %5; border-radius: 8px; background: transparent; }"
                "QTabBar::tab { background: transparent; color: %8; padding: 6px 16px; border-bottom: 2px solid transparent; }"
                "QTabBar::tab:selected { color: %2; border-bottom: 2px solid %9; }"
                "QProgressBar { background: %4; border: 1px solid %5; border-radius: 5px; text-align: center; color: %2; }"
                "QProgressBar::chunk { background: %9; border-radius: 4px; }"
                "QScrollBar:vertical { background: transparent; width: 10px; }"
                "QScrollBar:horizontal { background: transparent; height: 10px; }"
                "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: %4; border-radius: 5px; min-height: 24px; min-width: 24px; }"
                "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: %8; }"
                "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
                "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
                "QHeaderView { background: %3; color: %2; }"
                "QHeaderView::section { background: %11; color: %2; border: none; border-right: 1px solid %5; padding: 6px 10px; }"
                "QTableView, QListView, QTreeView { background: %3; color: %2; border: 1px solid %5; border-radius: 6px; }"
                "QTableView::item:selected, QListView::item:selected, QTreeView::item:selected { background: %9; color: #ffffff; }"
                "QWebEngineView { background-color: %12; }"
            )
                .arg(t.bgWindow)      // %1
                .arg(t.textPrimary)   // %2
                .arg(t.cardBg)        // %3
                .arg(t.hover)         // %4
                .arg(t.border)        // %5
                .arg(t.tabHover)      // %6
                .arg(t.selectedBg)    // %7
                .arg(t.textTertiary)  // %8
                .arg(t.accent)        // %9
                .arg(t.accentHover)   // %10
                .arg(t.bgUrlBar)      // %11
                .arg(t.pageBackground); // %12
        };

        const QMetaObject::Connection conn = QObject::connect(
            &SafariTheme::instance(), &SafariTheme::schemeChanged, &app,
            [&app, &buildQss]() { app.setStyleSheet(buildQss()); });
        Q_UNUSED(conn);
        app.setStyleSheet(buildQss());
    }

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
        // The primary takes the lock before QLocalServer::listen() starts, so
        // a secondary may connect before the server socket exists; retry a few
        // times so URLs handed off in that window are not silently dropped.
        QLocalSocket socket;
        const QByteArray payload = commandLineUrls().join(QLatin1Char('\n')).toUtf8();
        bool connected = false;
        for (int attempt = 0; attempt < 20 && !connected; ++attempt) {
            if (socket.state() == QLocalSocket::UnconnectedState)
                socket.connectToServer(serverName);
            if (socket.waitForConnected(100))
                connected = true;
        }
        if (connected) {
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
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
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
        // onboarding.setAttribute(Qt::WA_TranslucentBackground);

        QVBoxLayout *layout = new QVBoxLayout(&onboarding);
        layout->setContentsMargins(0, 0, 0, 0);

        QWebEngineView *view = new QWebEngineView(&onboarding);
        OnboardingWebPage *page = new OnboardingWebPage(view);
        view->setPage(page);
        QWebChannel *channel = new QWebChannel(page);
        OAuthManager oauthManager;
        BookmarkImporter importer;
        AppearanceManager appearance;
        ShelfStore bookmarksStore(QStringLiteral("bookmarks.json"));
        OnboardingBridge bridge(&account, &bookmarksStore, &onboarding);
        channel->registerObject(QStringLiteral("onboardingBridge"), &bridge);
        channel->registerObject(QStringLiteral("oauthManager"), &oauthManager);
        channel->registerObject(QStringLiteral("bookmarkImporter"), &importer);
        channel->registerObject(QStringLiteral("appearance"), &appearance);
        channel->registerObject(QStringLiteral("theme"), &SafariTheme::instance());
        page->setWebChannel(channel, QWebEngineScript::MainWorld);
        page->setBridgeChannel(channel);
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

        // Show the modal only after the native-child web view exists; mapping a
        // translucent fullscreen window and then adding a QWebEngineView forces
        // XCB to tear down and re-create the native window, and the recreated
        // window is never mapped, so nothing appears on screen.
        onboarding.showFullScreen();

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

    // First-run startup behavior: if onboarding has never been completed,
    // show the local start page. A separate first-run cinematic is shown
    // earlier if this is the very first launch.
    window.show();
    if (QApplication::arguments().contains(QStringLiteral("--open-settings"))) {
        QTimer::singleShot(1500, &window, &BrowserWindow::openSettingsForTesting);
    }
    if (isFirstRun()) {
        window.loadStartPage();
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