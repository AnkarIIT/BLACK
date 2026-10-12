#ifndef VOICEINTERFACE_H
#define VOICEINTERFACE_H

#include <QObject>

// Placeholder implementation. Voice control (speech recognition, dictation,
// voice navigation commands) is not implemented in this build; calls are
// no-ops.
//
// isAvailable() always reports false and no microphone is ever opened. The
// start/stop calls do nothing at all, not even emit a signal, so callers must
// not treat them as evidence that recording started or stopped.
class VoiceInterface : public QObject
{
    Q_OBJECT

public:
    explicit VoiceInterface(QObject *parent = nullptr);
    ~VoiceInterface() override;

    // Always false: no speech backend is linked into this build.
    Q_INVOKABLE bool isAvailable() const;

    // No-op. No microphone device is opened and no audio is captured.
    Q_INVOKABLE void startListening();

    // No-op. Nothing is listening.
    Q_INVOKABLE void stopListening();

private:
    // Never set true: startListening() is a no-op.
    bool m_listening = false;
};

#endif // VOICEINTERFACE_H