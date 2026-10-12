#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

class QWebEngineView;

// Placeholder implementation. Page translation (language detection, local
// translation models, in-page text substitution) is not implemented in this
// build; calls are no-ops.
//
// availableLanguages() is empty, translatePage() never translates anything and
// never touches the page, and setTargetLanguage() refuses every code because
// there is no language to target.
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager() override;

    // Placeholder: no-op. The page is neither read nor modified, and no
    // translation service is contacted.
    void translatePage(QWebEngineView *view, const QString &targetLanguage);

    // Always empty: no translation back-ends are bundled.
    Q_INVOKABLE QStringList availableLanguages() const;

    // Always empty: setTargetLanguage() rejects everything, so no target is
    // ever recorded.
    Q_INVOKABLE QString targetLanguage() const;

    // Placeholder: always returns false; no language is accepted.
    Q_INVOKABLE bool setTargetLanguage(const QString &code);

private:
    QString m_targetLanguage;
};

#endif // TRANSLATIONMANAGER_H