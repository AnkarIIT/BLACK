#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <QObject>
#include <QString>

// Placeholder implementation. LLM provider integration (Ollama / OpenAI /
// Anthropic / local model back-ends, streaming completions, context assembly)
// is not implemented in this build; calls are no-ops.
//
// setProvider() only records the requested provider. isAvailable() always
// returns false because no backend is ever contacted, no endpoint is
// configured and no API key is read.
class AIManager : public QObject
{
    Q_OBJECT

public:
    enum class Provider {
        Ollama,
        OpenAI,
        Anthropic,
        Local,
        None
    };
    Q_ENUM(Provider)

    explicit AIManager(QObject *parent = nullptr);
    ~AIManager() override;

    // Records the requested provider. Does not probe, configure or connect to
    // anything.
    void setProvider(Provider provider);
    Provider provider() const;

    // Always false: no model backend is configured in this build.
    Q_INVOKABLE bool isAvailable() const;

private:
    Provider m_provider = Provider::None;
};

#endif // AIMANAGER_H
