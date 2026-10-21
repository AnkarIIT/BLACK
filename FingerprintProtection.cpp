#include "FingerprintProtection.h"

#include <QWebEngineProfile>

FingerprintProtection::FingerprintProtection(QObject *parent)
    : QObject(parent)
{
}

void FingerprintProtection::install(QWebEngineProfile *profile)
{
    Q_UNUSED(profile);
    // Not implemented: see the header.
}

bool FingerprintProtection::isEnabled() const
{
    return false;
}
