#include "DeveloperTools.h"

DeveloperTools::DeveloperTools(QObject *parent)
    : QObject(parent)
{
}

DeveloperTools::~DeveloperTools() = default;

bool DeveloperTools::isOpen() const
{
    return m_open;
}

void DeveloperTools::open(QObject *page)
{
    // Placeholder: a real implementation would attach a QWebEngineView
    // (showDevTools) to the page. The page pointer is ignored here, so pressing
    // this button in the UI does nothing.
    Q_UNUSED(page)
    m_open = false;
}

void DeveloperTools::close()
{
    m_open = false;
}
