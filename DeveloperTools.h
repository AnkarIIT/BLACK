#ifndef DEVELOPERTOOLS_H
#define DEVELOPERTOOLS_H

#include <QObject>

// Placeholder implementation. The DevTools front-end (inspector, console,
// network panel) is not provided by this class in this build; calls are
// no-ops.
//
// open() shows no window and close() closes nothing, so isOpen() is always
// false. Note that BrowserWindow has its own separate DevTools dialog; this
// class is not what drives it.
class DeveloperTools : public QObject
{
    Q_OBJECT

public:
    explicit DeveloperTools(QObject *parent = nullptr);
    ~DeveloperTools() override;

    // Always false: no DevTools window is ever shown by this class.
    Q_INVOKABLE bool isOpen() const;

    // Placeholder: no-op. No QWebEngineView is created for the given page.
    Q_INVOKABLE void open(QObject *page = nullptr);

    // Placeholder: no-op.
    Q_INVOKABLE void close();

private:
    // Never set true: open() is a no-op.
    bool m_open = false;
};

#endif // DEVELOPERTOOLS_H
