#ifndef WALLETMANAGER_H
#define WALLETMANAGER_H

#include <QObject>

class QWebChannel;

// Placeholder implementation. Web3 / Ethereum wallet integration (injected
// provider object, EIP-1193 request routing, key handling) is not implemented
// in this build; calls are no-ops.
//
// registerWeb3Provider() registers nothing on the channel, so page JavaScript
// finds no `window.ethereum`. No key material is ever requested, derived or
// stored.
class WalletManager : public QObject
{
    Q_OBJECT

public:
    explicit WalletManager(QObject *parent = nullptr);
    ~WalletManager() override;

    // Placeholder: no-op. Nothing is registered on the QWebChannel.
    void registerWeb3Provider(QWebChannel *channel);

private:
    QWebChannel *m_channel = nullptr; // Borrowed, may be null; never populated
                                      // with an EIP-1193 provider object.
};

#endif // WALLETMANAGER_H