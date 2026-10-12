#ifndef ADVANCEDSECURITY_H
#define ADVANCEDSECURITY_H

#include <QObject>

// Placeholder implementation. Enhanced protection mode (HTTPS-only fallback
// blocking, permission tightening, remote code-execution mitigations) is not
// implemented in this build; calls are no-ops.
//
// setEnhancedProtectionEnabled() REJECTS the request and returns false: no
// protection change is applied, so claiming success would misrepresent the
// browser's posture. isEnhancedProtectionEnabled() is therefore always false.
class AdvancedSecurity : public QObject
{
    Q_OBJECT

public:
    explicit AdvancedSecurity(QObject *parent = nullptr);
    ~AdvancedSecurity() override;

    // Always false.
    Q_INVOKABLE bool isEnhancedProtectionEnabled() const;

    // Placeholder: always returns false and changes nothing.
    Q_INVOKABLE bool setEnhancedProtectionEnabled(bool enabled);

private:
    // Never set true: the setter rejects every request.
    bool m_enhancedProtection = false;
};

#endif // ADVANCEDSECURITY_H