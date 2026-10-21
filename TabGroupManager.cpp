#include "TabGroupManager.h"

TabGroupManager::TabGroupManager(QObject *parent)
    : QObject(parent)
{
}

void TabGroupManager::setBrowserWindow(BrowserWindow *window)
{
    Q_UNUSED(window);
}

int TabGroupManager::groupCount() const
{
    return 0;
}

bool TabGroupManager::saveGroups() const
{
    return false;
}

bool TabGroupManager::restoreGroups()
{
    return false;
}
