#include "WebAuthn.h"

#include <QWebEnginePage>
#include <QWebEngineProfile>

WebAuthnManager::WebAuthnManager(QObject *parent)
    : QObject(parent)
{
}

WebAuthnManager::~WebAuthnManager() = default;

void WebAuthnManager::registerPage(QWebEnginePage *page)
{
    // Placeholder: a real implementation would install a
    // QWebEngineUrlRequestInterceptor to answer WebAuthn attestation/signing
    // requests. No interceptor is installed and the page is never touched.
    if (!page)
        return;
    if (!m_pages.contains(page))
        m_pages.append(page);
}

void WebAuthnManager::unregisterPage(QWebEnginePage *page)
{
    if (!page)
        return;
    m_pages.removeAll(page);
}