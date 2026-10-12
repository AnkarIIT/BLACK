#include "SearchAffiliation.h"

SearchAffiliation::SearchAffiliation(QObject *parent)
    : QObject(parent)
{
}

SearchAffiliation::~SearchAffiliation() = default;

void SearchAffiliation::setProvider(Provider provider)
{
    // Placeholder: a real implementation would store the provider and rewrite
    // search URLs through the corresponding affiliate endpoint.
    m_provider = provider;
}

SearchAffiliation::Provider SearchAffiliation::provider() const
{
    return m_provider;
}