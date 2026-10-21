#ifndef PRODUCTIVITYMANAGER_H
#define PRODUCTIVITYMANAGER_H

#include <QObject>

// Placeholder implementation. Productivity features (focus mode that silences
// notifications, tab/audio muting, Pomodoro timers, per-site distraction
// blocking) are not implemented in this build; calls are no-ops.
//
// setFocusModeEnabled() REJECTS the request and returns false: focus mode does
// not exist here, so reporting success would make the UI lie about a state
// change that never happened. isFocusModeEnabled() is therefore always false.
class ProductivityManager : public QObject
{
    Q_OBJECT

public:
    explicit ProductivityManager(QObject *parent = nullptr);
    ~ProductivityManager() override;

    // Always false.
    Q_INVOKABLE bool isFocusModeEnabled() const;

    // Placeholder: always returns false and changes nothing.
    Q_INVOKABLE bool setFocusModeEnabled(bool enabled);

private:
    // Never set true: the setter rejects every request.
    bool m_focusMode = false;
};

#endif // PRODUCTIVITYMANAGER_H
