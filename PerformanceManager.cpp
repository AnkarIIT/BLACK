#include "PerformanceManager.h"

#include <QWebEngineProfile>

PerformanceManager::PerformanceManager(QObject *parent)
    : QObject(parent)
{
}

PerformanceManager::~PerformanceManager() = default;

void PerformanceManager::setOptimizationLevel(OptimizationLevel level)
{
    // Placeholder: a real implementation would map the level onto
    // QWebEngineSettings attributes and process priorities. The level is only
    // recorded so callers can read back what they asked for.
    m_level = level;
}

PerformanceManager::OptimizationLevel PerformanceManager::optimizationLevel() const
{
    return m_level;
}

void PerformanceManager::setWebEngineProfile(QWebEngineProfile *profile)
{
    // Placeholder: the profile is retained but never configured.
    m_profile = profile;
}

void PerformanceManager::startProfiling()
{
    // Placeholder: a real implementation would start a sampling timer and
    // aggregate CPU / memory figures. No timer exists.
    m_profiling = false;
}

void PerformanceManager::stopProfiling()
{
    m_profiling = false;
}
