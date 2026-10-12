#include "Sandbox.h"

SandboxManager::SandboxManager() = default;

bool SandboxManager::isSupported()
{
    // Placeholder: sandbox support is probed by inspecting the WebEngine
    // command line at runtime in a real implementation. That plumbing does not
    // exist here, so this build never claims sandbox support.
    return false;
}

bool SandboxManager::enable()
{
    m_enabled = false;
    return false;
}

QString SandboxManager::sandboxStatus() const
{
    return QStringLiteral("disabled (sandbox support not implemented in this build)");
}