#include "VoiceInterface.h"

VoiceInterface::VoiceInterface(QObject *parent)
    : QObject(parent)
{
}

VoiceInterface::~VoiceInterface() = default;

bool VoiceInterface::isAvailable() const
{
    return false;
}

void VoiceInterface::startListening()
{
    // Placeholder: a real implementation would open an audio input device and
    // stream frames to a speech recogniser. No device is touched here.
    m_listening = false;
}

void VoiceInterface::stopListening()
{
    m_listening = false;
}