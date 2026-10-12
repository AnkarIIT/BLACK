#include "CollaborationManager.h"

CollaborationManager::CollaborationManager(QObject *parent)
    : QObject(parent)
{
}

CollaborationManager::~CollaborationManager() = default;

bool CollaborationManager::isSupported() const
{
    // Placeholder: no collaboration transport exists in this build.
    return false;
}

void CollaborationManager::setWebRTCManager(WebRTCManager *manager)
{
    // Placeholder: a real implementation would open a data channel here.
    m_webRTC = manager;
}

void CollaborationManager::setSyncManager(SyncManager *manager)
{
    // Placeholder: a real implementation would subscribe to sync changes and
    // mirror them to collaborators.
    m_sync = manager;
}