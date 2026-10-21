#ifndef SEARCHAFFILIATION_H
#define SEARCHAFFILIATION_H

#include <QObject>
#include <QString>

// Placeholder implementation. Search-engine affiliation / monetised search
// integration (Brave Search API calls, DDG / Google affiliate query rewriting,
// referral attribution) is not implemented in this build; calls are no-ops.
//
// setProvider() only records the selection. Queries are never rewritten and no
// affiliate token is ever attached to a search URL.
class SearchAffiliation : public QObject
{
    Q_OBJECT

public:
    enum class Provider {
        Brave,
        DuckDuckGo,
        Google,
        None
    };
    Q_ENUM(Provider)

    explicit SearchAffiliation(QObject *parent = nullptr);
    ~SearchAffiliation() override;

    // Records the requested provider. No search request is ever issued.
    void setProvider(Provider provider);
    Provider provider() const;

private:
    Provider m_provider = Provider::None;
};

#endif // SEARCHAFFILIATION_H
