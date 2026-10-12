#include "CookiePartition.h"

#include <QWebEngineProfile>

CookiePartition::CookiePartition(QObject *parent)
    : QObject(parent)
{
}

void CookiePartition::install(QWebEngineProfile *profile)
{
    Q_UNUSED(profile);
    // Not implemented: see the header.
}

bool CookiePartition::isEnabled() const
{
    return false;
}