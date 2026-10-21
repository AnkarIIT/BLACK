#include "WebRTCManager.h"

WebRTCManager::WebRTCManager(QObject *parent)
    : QObject(parent)
{
}

WebRTCManager::~WebRTCManager() = default;

void WebRTCManager::setSignalingServer(const QString &url)
{
    // Placeholder: a real implementation would apply the URL as the ICE
    // server list / signalling endpoint. Here it is only remembered.
    m_signalingServer = url;
}

QString WebRTCManager::signalingServer() const
{
    return m_signalingServer;
}
