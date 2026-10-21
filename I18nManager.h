#ifndef I18NMANAGER_H
#define I18NMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

// Placeholder implementation. Internationalisation (loading .qm catalogues,
// installing a QTranslator, re-translating the native chrome and reloading
// pages after a locale change) is not implemented in this build.
//
// English is the only language: availableLanguages() returns just "en" and
// setLanguage() only accepts "en" (anything else fails). initialize() installs
// no translator, so no string in the UI is actually translated.
class I18nManager : public QObject
{
    Q_OBJECT

public:
    // Process-wide instance.
    static I18nManager &instance();

    // BrowserWindow also constructs its own instance directly.
    explicit I18nManager(QObject *parent = nullptr);
    ~I18nManager() override;

    // Placeholder: no-op. No .qm file is loaded and no QTranslator is
    // installed, so the UI stays in the source language.
    void initialize();

    // Always "en".
    Q_INVOKABLE QString currentLanguage() const;

    // Placeholder: succeeds only for "en" (and "en_US"-style English tags).
    // Any other code returns false and the current language is unchanged.
    Q_INVOKABLE bool setLanguage(const QString &code);

    // Always { "en" }.
    Q_INVOKABLE QStringList availableLanguages() const;

private:
    QString m_current = QStringLiteral("en");
};

#endif // I18NMANAGER_H
