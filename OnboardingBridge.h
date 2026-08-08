#ifndef ONBOARDINGBRIDGE_H
#define ONBOARDINGBRIDGE_H

#include <QObject>
#include <QDialog>
#include <QWebEnginePage>

#include "Account.h"

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
    explicit OnboardingBridge(Account *account, QDialog *modal, QObject *parent = nullptr)
        : QObject(parent)
        , m_account(account)
        , m_modal(modal)
    {
    }

    // Full profile: stores the name, writes .first_run_done, closes the modal.
    Q_INVOKABLE void completeOnboarding(const QString &profileName) const
    {
        if (m_account) {
            m_account->signIn(profileName);
            m_account->completeOnboarding();
        }
        if (m_modal)
            m_modal->accept();
    }

    // Guest: just writes the first-run marker and closes the modal.
    Q_INVOKABLE void finishAsGuest() const
    {
        if (m_account)
            m_account->completeOnboarding();
        if (m_modal)
            m_modal->accept();
    }

    // Abandon onboarding entirely (user closes the setup window).
    Q_INVOKABLE void cancel() const
    {
        if (m_modal)
            m_modal->reject();
    }

private:
    Account *m_account;
    QDialog *m_modal;
};

#endif // ONBOARDINGBRIDGE_H
