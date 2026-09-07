#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QPointer>
#include <QUrl>
#include <QDateTime>

class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QToolButton;
class QWidget;

// Represents a single download entry tracked by the manager.
struct DownloadItemInfo {
    QString fileName;
    QString filePath;
    QString url;
    qint64 receivedBytes = 0;
    qint64 totalBytes = -1;
    qint64 speedBytesPerSec = 0; // Current download speed
    qint64 etaSeconds = -1;       // Estimated time remaining
    int state = 0; // 0 = in progress, 1 = completed, 2 = failed/cancelled, 3 = paused
    QPointer<QWebEngineDownloadRequest> request;
    QDateTime startTime;
    QDateTime lastUpdateTime;
};

// Owns the download lifecycle decoupled from the browser window: it listens to
// QWebEngineProfile::downloadRequested, prompts for a location when the user
// chooses "Ask each time", tracks progress/completion, auto-opens safe files,
// and notifies the UI via signals. The window renders the menu/rows from
// items() without holding the download bookkeeping inline.
class DownloadManager : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManager(QWebEngineProfile *profile,
                             QWidget *parentWidget,
                             QToolButton *button,
                             QObject *parent = nullptr);

    const QList<DownloadItemInfo> &items() const { return m_items; }

    void clearCompleted();
    void clearAll();

    int completedCount() const;

signals:
    // The retry action in the UI asks the window to reopen the URL in a tab.
    void openInNewTab(const QUrl &url);
    // Tooltip on the Downloads button reflects the latest state.
    void buttonTooltipChanged(const QString &tip);
    // Download list changed (for UI updates)
    void itemsChanged();

public:
    // Pause/resume a download by index
    Q_INVOKABLE void pauseDownload(int index);
    Q_INVOKABLE void resumeDownload(int index);
    Q_INVOKABLE void cancelDownload(int index);
    Q_INVOKABLE void retryDownload(int index);
    Q_INVOKABLE void showInFolder(int index);

private:
    void updateButtonTooltip();
    void calculateSpeedAndEta(int idx);

    QWebEngineProfile *m_profile;
    QPointer<QWidget> m_parentWidget;
    QPointer<QToolButton> m_button;
    QList<DownloadItemInfo> m_items;
};

#endif // DOWNLOADMANAGER_H
