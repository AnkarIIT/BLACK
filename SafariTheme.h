#ifndef SAFARITHEME_H
#define SAFARITHEME_H

#include <QObject>
#include <QString>

class SafariTheme : public QObject
{
    Q_OBJECT

public:
    enum class Scheme { Light, Dark };
    enum class Preference { System, Light, Dark };

    static SafariTheme &instance();

    Q_PROPERTY(int themeScheme READ themeScheme NOTIFY themeSchemeChanged)
    Q_PROPERTY(int themePreference READ themePreference NOTIFY themeSchemeChanged)

    Scheme scheme() const { return m_scheme; }
    int themeScheme() const { return static_cast<int>(m_scheme); }
    int themePreference() const { return static_cast<int>(m_preference); }
    Preference preference() const { return m_preference; }

    void setPreference(Preference preference);
    Q_INVOKABLE void setThemePreference(int preference);
    void refreshScheme();
    void setScheme(Scheme scheme);

    QString bgWindow;
    QString bgToolbar;
    QString bgTabBar;
    QString bgUrlBar;
    QString bgSidebar;
    QString tabActive;
    QString tabInactive;
    QString tabHover;
    QString cardBg;
    QString textPrimary;
    QString textSecondary;
    QString textTertiary;
    QString accent;
    QString accentHover;
    QString border;
    QString borderLight;
    QString hover;
    QString searchBg;
    QString selectedBg;
    QString scrim;
    QString pageBackground;

signals:
    void schemeChanged();
    void themeSchemeChanged();

private:
    explicit SafariTheme(QObject *parent = nullptr);
    void loadPalette();

    Scheme m_scheme;
    Preference m_preference;
    bool m_paletteInitialized = false;
};

#endif
