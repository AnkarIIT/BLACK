#ifndef WEBRTCMANAGER_H
#define WEBRTCMANAGER_H

#include <QObject>
#include <QString>

// Placeholder implementation. WebRTC signalling (custom STUN/TURN server
// configuration, mDNS masking, IP leak protection) is not implemented in this
// build; calls are no-ops.
//
// setSignalingServer() stores the URL as a string and never dials it. No peer
// connection, STUN binding request or TURN allocation is ever made by this
// class.
class WebRTCManager : public QObject
{
    Q_OBJECT

public:
    explicit WebRTCManager(QObject *parent = nullptr);
    ~WebRTCManager() override;

    // Placeholder: records the URL only. No network connection is opened.
    void setSignalingServer(const QString &url);
    QString signalingServer() const;

private:
    QString m_signalingServer;
};

#endif // WEBRTCMANAGER_H
