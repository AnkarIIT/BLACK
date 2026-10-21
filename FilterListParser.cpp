#include "FilterListParser.h"

FilterListParser::FilterListParser(QObject *parent)
    : QObject(parent)
{
    // No filter list is downloaded or parsed in this build, so m_rules stays
    // empty.
}

FilterListParser::~FilterListParser() = default;

bool FilterListParser::matches(const QUrl &url, const QString &documentDomain, const QString &resourceType) const
{
    Q_UNUSED(url)
    Q_UNUSED(documentDomain)
    Q_UNUSED(resourceType)
    // Placeholder: real implementations evaluate urlFilter / substring /
    // regex / domain-anchor rules here. With no rules loaded there is nothing
    // to match, so the honest answer is "not blocked by this filter list".
    return false;
}

int FilterListParser::ruleCount() const
{
    return m_rules.size();
}

bool FilterListParser::reload()
{
    // Placeholder: a real implementation would fetch the configured lists over
    // the network, parse them and replace m_rules. Reporting failure (rather
    // than silently succeeding with an empty list) keeps callers honest.
    return false;
}

FilterListUpdater::FilterListUpdater(FilterListParser *parser, QObject *parent)
    : QObject(parent)
    , m_parser(parser)
{
}

FilterListUpdater::~FilterListUpdater() = default;

void FilterListUpdater::start()
{
    // Placeholder: a real implementation would arm a periodic timer that
    // downloads the subscribed filter lists. No timer, no network traffic.
    m_running = false;
}

void FilterListUpdater::stop()
{
    // Placeholder: would stop the download timer. Nothing is running.
    m_running = false;
}
