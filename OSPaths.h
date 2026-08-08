#ifndef OSPATHS_H
#define OSPATHS_H

#include <QString>

// Centralizes every OS-specific integration point so the rest of the app can
// stay portable:
//   * well-known browser profile directories (future bookmark/history importer),
//   * "reveal in file manager" for downloaded files,
//   * the system "default apps" settings panel.
class OSPaths
{
public:
    // Full path to a third-party browser's default profile directory, or an
    // empty string for unsupported browsers. Currently unused by the app; kept
    // as the future foundation for a bookmark/history importer feature.
    static QString browserProfilePath(const QString &browserName);

    // Opens the native file manager with the given file selected/revealed
    // (Windows Explorer, macOS Finder). On Linux it opens the parent directory.
    static void showInFileManager(const QString &filePath);

    // Opens the operating system's default-apps / default browser settings.
    static void openDefaultBrowserSettings();
};

#endif // OSPATHS_H
