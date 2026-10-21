#include "RewardsManager.h"

RewardsManager::RewardsManager(QObject *parent)
    : QObject(parent)
{
}

RewardsManager::~RewardsManager() = default;

QString RewardsManager::displayName() const
{
    // Placeholder: empty means "no account", which is the truthful state here.
    return QString();
}

bool RewardsManager::isEnabled() const
{
    return false;
}
