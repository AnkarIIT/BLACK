#ifndef PERFORMANCEMANAGER_H
#define PERFORMANCEMANAGER_H

#include <QObject>
#include <QString>

class QWebEngineProfile;

// Placeholder implementation. Browser performance tuning (QWebEngineSettings
// attribute tuning, process/tab prioritisation, background throttling, memory
// profiling) is not implemented in this build; calls are no-ops.
//
// setOptimizationLevel() records the level but applies no setting to the
// profile, and startProfiling() starts no timer and records no samples. Note
// that setWebEngineProfile() is deliberately *not* applied: callers passing a
// profile get no behaviour change whatsoever.
class PerformanceManager : public QObject
{
    Q_OBJECT

public:
    enum class OptimizationLevel {
        None,
        Balanced,
        Aggressive,
        BatterySaver
    };
    Q_ENUM(OptimizationLevel)

    explicit PerformanceManager(QObject *parent = nullptr);
    ~PerformanceManager() override;

    // Placeholder: stores the level only. No QWebEngineSettings attribute is
    // changed.
    void setOptimizationLevel(OptimizationLevel level);
    OptimizationLevel optimizationLevel() const;

    // Placeholder: stores the pointer only. Nothing is installed on the
    // profile.
    void setWebEngineProfile(QWebEngineProfile *profile);

    // Placeholder: no-op. No profiling timer is started.
    void startProfiling();

    // Placeholder: no-op. No profiling timer is running.
    void stopProfiling();

private:
    OptimizationLevel m_level = OptimizationLevel::None;
    QWebEngineProfile *m_profile = nullptr; // Borrowed, may be null.
    bool m_profiling = false;                // Never set true.
};

#endif // PERFORMANCEMANAGER_H
