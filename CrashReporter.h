#ifndef CRASHREPORTER_H
#define CRASHREPORTER_H

#include <QObject>
#include <QHash>
#include <QString>

// Placeholder implementation. Crash reporting (minidump capture, Breakpad /
// Crashpad handler process, symbol upload) is not implemented in this build;
// calls are no-ops.
//
// No crash handler is installed and no minidumps are written. All state below
// is bookkeeping only: it records what *would* be reported so callers do not
// crash, but nothing is ever transmitted or stored on disk.
class CrashReporter : public QObject
{
    Q_OBJECT

public:
    static CrashReporter &instance();

    // Returns true purely so main.cpp's "was initialisation successful" branch
    // is taken. It does NOT mean a crash handler was installed.
    bool initialize();

    void setUserConsent(bool consent);
    void setEnabled(bool enabled);
    void setAnnotation(const QString &key, const QString &value);

    // Plain getters for the recorded (not acted upon) state.
    bool isEnabled() const;
    bool userConsent() const;
    QString annotation(const QString &key) const;

private:
    CrashReporter();
    ~CrashReporter() override;

    Q_DISABLE_COPY(CrashReporter)

    bool m_initialized = false;
    bool m_userConsent = false;
    bool m_enabled = false;
    QHash<QString, QString> m_annotations;
};

#endif // CRASHREPORTER_H