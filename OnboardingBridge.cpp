#include "OnboardingBridge.h"
#include "BookmarkImporter.h"
#include "ShelfStore.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

QJsonArray OnboardingBridge::scanBrowsersJson() const
{
    BookmarkImporter importer(nullptr);
    return importer.scanBrowsersJson();
}

QJsonArray OnboardingBridge::accountsJson() const
{
    if (m_account)
        return m_account->accountsJson();
    return QJsonArray();
}

QString OnboardingBridge::saveAccount(const QString &provider, const QString &name,
                                      const QString &email, const QString &avatar) const
{
    if (m_account)
        return m_account->saveAccount(provider, name, email, avatar);
    return QString();
}

void OnboardingBridge::selectAccount(const QString &id) const
{
    if (m_account)
        m_account->selectAccount(id);
}

QJsonArray OnboardingBridge::importBrowserBookmarks(const QString &browserId) const
{
    QJsonArray result;
    if (!m_bookmarks)
        return result;

    BookmarkImporter importer(nullptr);
    const QJsonArray items = importer.importBookmarksFrom(browserId);
    const int imported = m_bookmarks->importBookmarks(items);
    QJsonObject out;
    out.insert(QStringLiteral("browserId"), browserId);
    out.insert(QStringLiteral("imported"), imported);
    out.insert(QStringLiteral("total"), items.size());
    result.append(out);
    return result;
}

void OnboardingBridge::recordOAuthIdentity(const QString &provider,
                                           const QString &name,
                                           const QString &email,
                                           const QString &avatar) const
{
    if (m_account)
        m_account->signInWithOAuth(provider, name, email, avatar);
}

void OnboardingBridge::completeOnboarding(const QString &profileName) const
{
    if (m_account) {
        if (!profileName.isEmpty())
            m_account->signIn(profileName);
        m_account->completeOnboarding();
    }
    if (m_modal)
        m_modal->accept();
}

void OnboardingBridge::finishAsGuest() const
{
    if (m_account)
        m_account->completeOnboarding();
    if (m_modal)
        m_modal->accept();
}

void OnboardingBridge::completeOnboardingWithProfile(const QString &provider,
                                                     const QString &name,
                                                     const QString &email) const
{
    recordOAuthIdentity(provider, name, email, QString());
    completeOnboarding(QString());
}

void OnboardingBridge::cancel() const
{
    if (m_modal)
        m_modal->reject();
}
