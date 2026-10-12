#include "AIAgentManager.h"

AIAgentManager::AIAgentManager(QObject *parent)
    : QObject(parent)
{
}

AIAgentManager::~AIAgentManager() = default;

bool AIAgentManager::isEnabled() const
{
    return m_enabled;
}