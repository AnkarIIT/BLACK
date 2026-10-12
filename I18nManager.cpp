#include "I18nManager.h"

I18nManager::I18nManager(QObject *parent)
    : QObject(parent)
{
}

I18nManager::~I18nManager() = default;

I18nManager &I18nManager::instance()
{
    static I18nManager s_instance;
    return s_instance;
}

void I18nManager::initialize()
{
    // Placeholder: a real implementation would locate the .qm files under the
    // installation prefix and install a QTranslator. Nothing is installed, so
    // no tr() call resolves to anything but the source string.
}

QString I18nManager::currentLanguage() const
{
    return m_current;
}

bool I18nManager::setLanguage(const QString &code)
{
    // Placeholder: English is the only bundled language in this build. Failing
    // loudly for anything else keeps callers from believing a translation was
    // applied.
    if (!code.startsWith(QLatin1String("en"), Qt::CaseInsensitive))
        return false;

    m_current = QStringLiteral("en");
    return true;
}

QStringList I18nManager::availableLanguages() const
{
    return QStringList{ QStringLiteral("en") };
}