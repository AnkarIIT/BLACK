#include "ProductivityManager.h"

ProductivityManager::ProductivityManager(QObject *parent)
    : QObject(parent)
{
}

ProductivityManager::~ProductivityManager() = default;

bool ProductivityManager::isFocusModeEnabled() const
{
    return m_focusMode;
}

bool ProductivityManager::setFocusModeEnabled(bool enabled)
{
    Q_UNUSED(enabled)
    // Placeholder: focus mode would mute audio and dim non-active tabs. Since
    // none of that exists, the request is refused instead of silently ignored.
    m_focusMode = false;
    return false;
}