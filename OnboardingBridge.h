#ifndef ONBOARDINGBRIDGE_H
#define ONBOARDINGBRIDGE_H

#include <QObject>
#include <QDialog>
#include <QWebEnginePage>
#include <QJsonArray>

#include "Account.h"

class ShelfStore;

// QWebEnginePage::javaScriptConsoleMessage is a protected virtual, so surface
// page console messages as a signal for first-run diagnostics on the native
// side (see main.cpp).
class OnboardingWebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit OnboardingWebPage(QObject *parent = nullptr)
        : QWebEnginePage(parent)
    {
    }

signals:
    void consoleMessage(const QString &message, int lineNumber);

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                  int lineNumber, const QString &sourceID) override
    {
        emit consoleMessage(QStringLiteral("[%1] %2").arg(sourceID, message), lineNumber);
        QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
    }

    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        if (isMainFrame) {
            const QString scheme = url.scheme();
            if (scheme != QLatin1String("qrc") && scheme != QLatin1String("http") && scheme != QLatin1String("https"))
                return false;
            if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
                if (url.host() != QLatin1String("localhost") && url.host() != QLatin1String("127.0.0.1"))
                    return false;
            }
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }
};

// Bridge between the cinematic first-run page (qrc:/onboarding_experience.html)
// and the native fullscreen setup modal that main() shows before the browser
// window exists. Reaching "Launch Blackhole" (or skipping the tour) writes the
// first-run marker and accepts the modal so the real browser can open.
//
// The name typed during onboarding is stored through Account::signIn so the
// browser's own Account (constructed later) picks the profile up from
// account.json on disk.
class OnboardingBridge : public QObject
{
    Q_OBJECT

public:
    explicit OnboardingBridge(Account *account, ShelfStore *bookmarks, QDialog *modal,
                              QObject *parent = nullptr)
        : QObject(parent)
        , m_account(account)
        , m_bookmarks(bookmarks)
        , m_modal(modal)
    {
    }

    // Full profile: stores the name, writes .first_run_done, closes the modal.
    Q_INVOKABLE void completeOnboarding(const QString &profileName) const;

    // Guest: just writes the first-run marker and closes the modal.
    Q_INVOKABLE void finishAsGuest() const;

    // OAuth sign-in finished: record the provider + verified name/email, write
    // the first-run marker and close the modal so the real browser opens.
    Q_INVOKABLE void completeOnboardingWithProfile(const QString &provider,
                                                   const QString &name,
                                                   const QString &email) const;

    // Stores the OAuth identity on the local Account without finishing
    // onboarding (used before the tour completes).
    Q_INVOKABLE void recordOAuthIdentity(const QString &provider, const QString &name,
                                         const QString &email, const QString &avatar) const;

    // Abandon onboarding entirely (user closes the setup window).
    Q_INVOKABLE void cancel() const;

    // Detected importable browsers for the Import frame.
    Q_INVOKABLE QJsonArray scanBrowsersJson() const;

    // Imports a browser's bookmarks into the local shelf; returns a report
    // array with { browserId, imported, total }.
    Q_INVOKABLE QJsonArray importBrowserBookmarks(const QString &browserId) const;

    // Saved profiles for the page-06 account picker.
    Q_INVOKABLE QJsonArray accountsJson() const;

    // Persists a profile (from OAuth sign-in) and returns its stable id so the
    // page can scope service connections to it.
    Q_INVOKABLE QString saveAccount(const QString &provider, const QString &name,
                                    const QString &email, const QString &avatar) const;

    // Activates a saved profile (writes account.json so the browser uses it).
    Q_INVOKABLE void selectAccount(const QString &id) const;

private:
    Account *m_account;
    ShelfStore *m_bookmarks;
    QDialog *m_modal;
};

#endif // ONBOARDINGBRIDGE_H
