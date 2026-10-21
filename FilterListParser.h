#ifndef FILTERLISTPARSER_H
#define FILTERLISTPARSER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>

// Placeholder implementation. Adblock-style filter list parsing (EasyList /
// AdGuard / uBlock syntax, cosmetic + network rules) is not implemented in this
// build; calls are no-ops.
//
// The rule list is always empty, so matches() never reports a match and
// reload() never succeeds. Filtering in this build comes solely from the
// host-based blocklists in TrackerBlocker and SafeBrowsing.
class FilterListParser : public QObject
{
    Q_OBJECT

public:
    explicit FilterListParser(QObject *parent = nullptr);
    ~FilterListParser() override;

    // Always false: there are no rules.
    bool matches(const QUrl &url, const QString &documentDomain, const QString &resourceType) const;

    // Always 0: the rule list is empty.
    int ruleCount() const;

    // Always false: nothing is downloaded or parsed.
    Q_INVOKABLE bool reload();

private:
    // Always empty in this build. Present so callers can be written against a
    // real container without changing the header later.
    struct Rule
    {
        QString pattern;
        QString domain;
        int resourceType = 0;
        bool blocking = true;
    };

    QList<Rule> m_rules;
};

// Placeholder implementation. Scheduled/background filter list downloading is
// not implemented in this build; start() and stop() are no-ops.
class FilterListUpdater : public QObject
{
    Q_OBJECT

public:
    explicit FilterListUpdater(FilterListParser *parser, QObject *parent = nullptr);
    ~FilterListUpdater() override;

    // No-op. No download timer is started and no network requests are made.
    void start();

    // No-op. Nothing to stop.
    void stop();

private:
    FilterListParser *m_parser = nullptr; // Borrowed, may be null.
    bool m_running = false;               // Never set true: start() is a no-op.
};

#endif // FILTERLISTPARSER_H
