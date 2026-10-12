#include "TranslationManager.h"

#include <QWebEngineView>

TranslationManager::TranslationManager(QObject *parent)
    : QObject(parent)
{
}

TranslationManager::~TranslationManager() = default;

void TranslationManager::translatePage(QWebEngineView *view, const QString &targetLanguage)
{
    Q_UNUSED(view)
    Q_UNUSED(targetLanguage)
    // Placeholder: a real implementation would detect the page language, run
    // it through a translation model and rewrite the DOM. The page is left
    // exactly as it was.
}

QStringList TranslationManager::availableLanguages() const
{
    // Placeholder: empty, because no translation back-end exists here. UI code
    // that builds a language picker from this list will correctly show nothing.
    return QStringList();
}

QString TranslationManager::targetLanguage() const
{
    return m_targetLanguage;
}

bool TranslationManager::setTargetLanguage(const QString &code)
{
    Q_UNUSED(code)
    // Placeholder: refusing is the honest answer -- accepting would make the
    // caller believe a translation target is in effect.
    m_targetLanguage.clear();
    return false;
}