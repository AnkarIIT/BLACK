#include "AdvancedSecurity.h"

AdvancedSecurity::AdvancedSecurity(QObject *parent)
    : QObject(parent)
{
}

AdvancedSecurity::~AdvancedSecurity() = default;

bool AdvancedSecurity::isEnhancedProtectionEnabled() const
{
    return m_enhancedProtection;
}

bool AdvancedSecurity::setEnhancedProtectionEnabled(bool enabled)
{
    Q_UNUSED(enabled)
    // Placeholder: the real toggle would flip QWebEngineSettings attributes and
    // request/permission policy. Nothing is applied here, so the request is
    // refused rather than falsely acknowledged.
    m_enhancedProtection = false;
    return false;
}
