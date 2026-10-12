#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariantList>

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
    Q_PROPERTY(QString currentProfileId READ currentProfileId NOTIFY changed)

public:
    static ProfileManager &instance();

    // Always returns QWebEngineProfile::defaultProfile() regardless of
    // profileId: no secondary profiles exist in this build.
    QWebEngineProfile *webEngineProfile(const QString &profileId) const;

    // Always { id: "default", name: "Default" }.
    ProfileInfo currentProfile() const;

    // Always { "default" }.
    Q_INVOKABLE QStringList availableProfiles() const;

    // Id of the active profile. Always "default" in this build.
    QString currentProfileId() const;

    // UI-shaped profile list consumed by Settings > Profiles. Because only the
    // built-in profile exists here, this returns exactly one honest entry:
    //   { id: "default", name: "Default", isDefault: true }
    // It exists so the settings pane renders a real row instead of throwing on
    // a missing method (the pane was written against a multi-profile API).
    Q_INVOKABLE QVariantList getProfiles() const;

    // Placeholder: renaming/deleting profiles is not implemented; both return
    // false and change nothing.
    Q_INVOKABLE bool renameProfile(const QString &id, const QString &name);
    Q_INVOKABLE bool deleteProfile(const QString &id);

    // Placeholder: always returns false. No profile directory is created.
    Q_INVOKABLE bool createProfile(const QString &name);

    // Placeholder: always returns false. Only "default" exists, so there is
    // nothing to switch to.
    Q_INVOKABLE bool switchProfile(const QString &id);

signals:
    // Emitted when the active profile changes. This build never switches, so
    // it is never emitted; it exists for the Settings > Profiles wiring.
    void changed();

private:
    ProfileManager();
    ~ProfileManager() override;

    Q_DISABLE_COPY(ProfileManager)

    // The single (and only) profile this build supports.
    ProfileInfo m_current;
    QList<ProfileInfo> m_profiles;
};

#endif // PROFILEMANAGER_H