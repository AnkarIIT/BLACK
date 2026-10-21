#ifndef REWARDSMANAGER_H
#define REWARDSMANAGER_H

#include <QObject>
#include <QString>

// Placeholder implementation. Browser rewards / tokenised loyalty programme
// (balance display, reward history, redemption) is not implemented in this
// build; calls are no-ops.
//
// displayName() is empty and isEnabled() is false, so UI code that guards on
// isEnabled() will correctly hide the feature instead of showing a dead panel.
class RewardsManager : public QObject
{
    Q_OBJECT

public:
    explicit RewardsManager(QObject *parent = nullptr);
    ~RewardsManager() override;

    // Always empty: no rewards account is configured.
    Q_INVOKABLE QString displayName() const;

    // Always false.
    Q_INVOKABLE bool isEnabled() const;
};

#endif // REWARDSMANAGER_H
