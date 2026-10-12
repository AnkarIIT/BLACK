#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include <QWebEngineProfile>

struct ProfileInfo
{
    QString id;
    QString name;
};

// Placeholder implementation. Multi-profile support (creating, switching and
// persisting separate browsing profiles) is not implemented in this build;
// calls are no-ops.
//
// There is exactly one profile: "default". createProfile() and switchProfile()
// always fail, and webEngineProfile() always hands back Qt's single default
// profile.
class ProfileManager : public QObject
{
    Q_OBJECT

public:
    static ProfileManager &instance();

    // Always returns QWebEngineProfile::defaultProfile() regardless of
    // profileId: no secondary profiles exist in this build.
    QWebEngineProfile *webEngineProfile(const QString &profileId) const;

    // Always { id: "default", name: "Default" }.
    ProfileInfo currentProfile() const;

    // Always { "default" }.
    Q_INVOKABLE QStringList availableProfiles() const;

    // Placeholder: always returns false. No profile directory is created.
    Q_INVOKABLE bool createProfile(const QString &name);

    // Placeholder: always returns false. Only "default" exists, so there is
    // nothing to switch to.
    Q_INVOKABLE bool switchProfile(const QString &id);

private:
    ProfileManager();
    ~ProfileManager() override;

    Q_DISABLE_COPY(ProfileManager)

    // The single (and only) profile this build supports.
    ProfileInfo m_current;
    QList<ProfileInfo> m_profiles;
};

#endif // PROFILEMANAGER_H