#include "AppearanceManager.h"
#include "BrowserSettings.h"

AppearanceManager::AppearanceManager(QObject *parent)
    : QObject(parent)
{
}

int AppearanceManager::uiLayout() const
{
    return static_cast<int>(settings()->uiLayout());
}

void AppearanceManager::setUiLayout(int mode)
{
    const int normalized = (mode < 0 || mode > 1) ? 0 : mode;
    if (uiLayout() != normalized) {
        settings()->setUiLayout(static_cast<BrowserSettings::UiLayout>(normalized));
        emit uiLayoutChanged();
    }
}

BrowserSettings *AppearanceManager::settings() const
{
    return &BrowserSettings::instance();
}
