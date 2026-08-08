#include "OSPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QByteArray>
#include <QtGlobal>

QString OSPaths::browserProfilePath(const QString &browserName)
{
    const QString lower = browserName.toLower();
#if defined(Q_OS_WIN)
    const QString localAppData = qgetenv("LOCALAPPDATA");
    if (lower == QLatin1String("chrome"))
        return localAppData + QStringLiteral("/Google/Chrome/User Data/Default");
    if (lower == QLatin1String("edge"))
        return localAppData + QStringLiteral("/Microsoft/Edge/User Data/Default");
    if (lower == QLatin1String("brave"))
        return localAppData + QStringLiteral("/BraveSoftware/Brave-Browser/User Data/Default");
    if (lower == QLatin1String("vivaldi"))
        return localAppData + QStringLiteral("/Vivaldi/User Data/Default");
#elif defined(Q_OS_MAC)
    const QString home = QDir::homePath() + QStringLiteral("/Library/Application Support");
    if (lower == QLatin1String("chrome"))
        return home + QStringLiteral("/Google/Chrome/Default");
    if (lower == QLatin1String("edge"))
        return home + QStringLiteral("/Microsoft Edge/Default");
    if (lower == QLatin1String("brave"))
        return home + QStringLiteral("/BraveSoftware/Brave-Browser/Default");
    if (lower == QLatin1String("vivaldi"))
        return home + QStringLiteral("/Vivaldi/Default");
#elif defined(Q_OS_LINUX)
    const QString config = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (lower == QLatin1String("chrome"))
        return config + QStringLiteral("/google-chrome/Default");
    if (lower == QLatin1String("edge"))
        return config + QStringLiteral("/microsoft-edge/Default");
    if (lower == QLatin1String("brave"))
        return config + QStringLiteral("/BraveSoftware/Brave-Browser/Default");
    if (lower == QLatin1String("vivaldi"))
        return config + QStringLiteral("/vivaldi/Default");
#endif
    return QString();
}

void OSPaths::showInFileManager(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists())
        return;

#if defined(Q_OS_WIN)
    const QStringList args = QStringList{
        QStringLiteral("/select,") + QDir::toNativeSeparators(info.absoluteFilePath())
    };
    QProcess::startDetached(QStringLiteral("explorer.exe"), args);
#elif defined(Q_OS_MAC)
    // Finder reveal: selects the file without launching it.
    QProcess::startDetached(QStringLiteral("open"), QStringList{ QStringLiteral("-R"),
                                                                 info.absoluteFilePath() });
#else
    // Linux: open the parent directory (xdg-open picks Nautilus/Dolphin/etc.).
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
#endif
}

void OSPaths::openDefaultBrowserSettings()
{
#if defined(Q_OS_WIN)
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
#elif defined(Q_OS_MAC)
    QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.preference.general")));
#elif defined(Q_OS_LINUX)
    // xdg-settings has no "open the settings UI" verb, so dispatch on the
    // desktop environment and fall back to the two most common control centers.
    const QByteArray de = qgetenv("XDG_CURRENT_DESKTOP");
    if (de.contains("KDE"))
        QProcess::startDetached(QStringLiteral("systemsettings"), QStringList{ QStringLiteral("kcm_componentchooser") });
    else if (de.contains("XFCE"))
        QProcess::startDetached(QStringLiteral("xfce4-settings-manager"));
    else if (de.contains("GNOME") || de.contains("CINNAMON"))
        QProcess::startDetached(QStringLiteral("gnome-control-center"), QStringList{ QStringLiteral("default-apps") });
    else if (!QProcess::startDetached(QStringLiteral("gnome-control-center"), QStringList{ QStringLiteral("default-apps") }))
        QProcess::startDetached(QStringLiteral("systemsettings"), QStringList{ QStringLiteral("kcm_componentchooser") });
#else
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
#endif
}
