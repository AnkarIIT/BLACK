#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>

class QObject;
class PasswordStore;
class ShelfStore;
class BrowserSettings;
class ExtensionManager;
class Account;
class SyncEngine;

// Placeholder implementation. Browsing-data synchronisation (encrypted
// bookmark/history/password/settings sync to a remote account) is not
// implemented in this build; calls are no-ops.
//
// initialize() only records the collaborators it is handed. No SyncEngine is
// created, no account is contacted and no data ever leaves the machine.
// isEnabled() and isSyncing() are therefore always false.
class SyncManager : public QObject
{
    Q_OBJECT

public:
    explicit SyncManager(QObject *parent = nullptr);
    ~SyncManager() override;

    // Placeholder: stores the (possibly null) pointers and does nothing else.
    void initialize(QObject *webChannelOwner,
                    PasswordStore *passwords,
                    ShelfStore *bookmarks,
                    ShelfStore *history,
                    BrowserSettings *settings,
                    ExtensionManager *extensions,
                    Account *account);

    // Always false: sync is not implemented.
    Q_INVOKABLE bool isEnabled() const;

    // Always false: nothing is ever uploaded or downloaded.
    Q_INVOKABLE bool isSyncing() const;

private:
    // Borrowed, all may be null. Never dereferenced.
    QObject *m_webChannelOwner = nullptr;
    PasswordStore *m_passwords = nullptr;
    ShelfStore *m_bookmarks = nullptr;
    ShelfStore *m_history = nullptr;
    BrowserSettings *m_settings = nullptr;
    ExtensionManager *m_extensions = nullptr;
    Account *m_account = nullptr;

    // Never created in this build.
    SyncEngine *m_engine = nullptr;

    // Never set true.
    bool m_enabled = false;
    bool m_syncing = false;
};

#endif // SYNCMANAGER_H
