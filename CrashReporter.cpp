#include "CrashReporter.h"

CrashReporter::CrashReporter()
    : QObject(nullptr)
{
}

CrashReporter::~CrashReporter() = default;

CrashReporter &CrashReporter::instance()
{
    static CrashReporter s_instance;
    return s_instance;
}

bool CrashReporter::initialize()
{
    // Placeholder: a real implementation installs a platform crash handler here
    // (Breakpad/Crashpad). No handler is installed in this build, so there is
    // nothing that would capture a crash. Reporting success keeps callers that
    // branch on it honest about "no error occurred here".
    m_initialized = true;
    return true;
}

void CrashReporter::setUserConsent(bool consent)
{
    m_userConsent = consent;
    if (!consent && m_enabled)
        m_enabled = false;
}

void CrashReporter::setEnabled(bool enabled)
{
    // Consent gates enabling, but even with consent there is no handler, so
    // this only records intent.
    m_enabled = enabled && m_userConsent;
}

void CrashReporter::setAnnotation(const QString &key, const QString &value)
{
    if (key.isEmpty())
        return;
    m_annotations.insert(key, value);
}

bool CrashReporter::isEnabled() const
{
    return m_enabled;
}

bool CrashReporter::userConsent() const
{
    return m_userConsent;
}

QString CrashReporter::annotation(const QString &key) const
{
    return m_annotations.value(key);
}
