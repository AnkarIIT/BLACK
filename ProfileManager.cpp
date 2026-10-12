#include "ProfileManager.h"

ProfileManager::ProfileManager()
    : QObject(nullptr)
{
    // Placeholder: a real implementation would scan a profiles directory and
    // populate m_profiles. This build hard-codes the single default profile.
    ProfileInfo def;
    def.id = QStringLiteral("default");
    def.name = QStringLiteral("Default");
    m_profiles.append(def);
    m_current = def;
}

ProfileManager::~ProfileManager() = default;

ProfileManager &ProfileManager::instance()
{
    static ProfileManager s_instance;
    return s_instance;
}

QWebEngineProfile *ProfileManager::webEngineProfile(const QString &profileId) const
{
    Q_UNUSED(profileId)
    // Placeholder: multi-profile builds would create/lookup an off-the-record
    // QWebEngineProfile per id. This build has one profile, Qt's default one.
    return QWebEngineProfile::defaultProfile();
}

ProfileInfo ProfileManager::currentProfile() const
{
    return m_current;
}

QStringList ProfileManager::availableProfiles() const
{
    QStringList ids;
    ids.reserve(m_profiles.size());
    for (const ProfileInfo &p : m_profiles)
        ids.append(p.id);
    return ids;
}

bool ProfileManager::createProfile(const QString &name)
{
    Q_UNUSED(name)
    // Placeholder: would allocate an off-the-record profile and persist it.
    // Returning false (not true) so callers do not believe a profile exists.
    return false;
}

bool ProfileManager::switchProfile(const QString &id)
{
    Q_UNUSED(id)
    // Placeholder: would tear down and rebuild the WebEngine profile. There is
    // only one profile, so switching is never possible.
    return false;
}