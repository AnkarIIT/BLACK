#ifndef COLLABORATIONMANAGER_H
#define COLLABORATIONMANAGER_H

#include <QObject>

class WebRTCManager;
class SyncManager;

// Placeholder implementation. Tab collaboration (shared browsing sessions,
// presence, remote cursor broadcast, session invitations) is not implemented in
// this build; calls are no-ops.
//
// isSupported() always reports false. The setWebRTCManager() / setSyncManager()
// setters only record the pointers: no signalling connection is opened and no
// session document is ever created or joined.
class CollaborationManager : public QObject
{
    Q_OBJECT

public:
    explicit CollaborationManager(QObject *parent = nullptr);
    ~CollaborationManager() override;

    // Always false: collaboration is not available in this build.
    Q_INVOKABLE bool isSupported() const;

    // Placeholder: stores the pointer only. No connection is established.
    void setWebRTCManager(WebRTCManager *manager);

    // Placeholder: stores the pointer only. Nothing is synced.
    void setSyncManager(SyncManager *manager);

private:
    WebRTCManager *m_webRTC = nullptr; // Borrowed, may be null.
    SyncManager *m_sync = nullptr;     // Borrowed, may be null.
};

#endif // COLLABORATIONMANAGER_H
