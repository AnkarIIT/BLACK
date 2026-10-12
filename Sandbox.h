#ifndef SANDBOX_H
#define SANDBOX_H

#include <QString>

// Placeholder implementation. The Chromium process sandbox feature is not
// implemented in this build; calls are no-ops.
//
// isSupported() always reports false so main.cpp takes the "sandbox not
// supported on this platform" branch. Nothing is actually enabled: the
// WebEngine child processes are started by Qt with whatever command line Qt
// itself supplies, and BLACK adds no --no-sandbox / --enable-sandbox flags.
class SandboxManager
{
public:
    SandboxManager();

    // Always false: this build never enables the Chromium sandbox.
    static bool isSupported();

    // No-op. Always returns false because isSupported() is false.
    bool enable();

    // Human readable, always-accurate description of the (non-)state.
    QString sandboxStatus() const;

private:
    bool m_enabled = false; // Never set to true: enable() cannot succeed.
};

#endif // SANDBOX_H