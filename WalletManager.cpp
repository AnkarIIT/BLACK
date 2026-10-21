#include "WalletManager.h"

#include <QWebChannel>

WalletManager::WalletManager(QObject *parent)
    : QObject(parent)
{
}

WalletManager::~WalletManager() = default;

void WalletManager::registerWeb3Provider(QWebChannel *channel)
{
    // Placeholder: a real implementation would register a QObject exposing the
    // EIP-1193 `request()` API and inject the corresponding script into the
    // page. Nothing is registered and nothing is injected here, so page
    // scripts cannot reach a wallet through this object.
    m_channel = channel;
}
