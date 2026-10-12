#ifndef AIAGENTMANAGER_H
#define AIAGENTMANAGER_H

#include <QObject>

// Placeholder implementation. Autonomous in-page AI agents (page summarising,
// form filling, task automation driven by an LLM) is not implemented in this
// build; calls are no-ops.
//
// isEnabled() always reports false and no agent is ever dispatched.
class AIAgentManager : public QObject
{
    Q_OBJECT

public:
    explicit AIAgentManager(QObject *parent = nullptr);
    ~AIAgentManager() override;

    // Always false: no agent back-end is wired up in this build.
    Q_INVOKABLE bool isEnabled() const;

private:
    // Never set true: the feature is absent.
    bool m_enabled = false;
};

#endif // AIAGENTMANAGER_H