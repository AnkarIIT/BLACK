#ifndef SAFARITHEME_H
#define SAFARITHEME_H

#include <QObject>
#include <QString>

class SafariTheme : public QObject
{
    Q_OBJECT

public:
    enum class Scheme { Light, Dark };

    static SafariTheme &instance();

    Scheme scheme() const { return m_scheme; }
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

private:
    explicit SafariTheme(QObject *parent = nullptr);
    void loadPalette();

    Scheme m_scheme;
};

#endif
