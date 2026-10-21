#include "AIManager.h"

AIManager::AIManager(QObject *parent)
    : QObject(parent)
{
}

AIManager::~AIManager() = default;

void AIManager::setProvider(Provider provider)
{
    // Placeholder: a real implementation would validate the endpoint, load an
    // API key and probe the model list. Only the selection is stored here.
    m_provider = provider;
}

AIManager::Provider AIManager::provider() const
{
    return m_provider;
}

bool AIManager::isAvailable() const
{
    // Placeholder: no backend is reachable because none is configured.
    return false;
}
