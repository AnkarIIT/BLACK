#include "SyncManager.h"

SyncManager::SyncManager(QObject *parent)
    : QObject(parent)
{
}

SyncManager::~SyncManager() = default;

void SyncManager::initialize(QObject *webChannelOwner,
                             PasswordStore *passwords,
                             ShelfStore *bookmarks,
                             ShelfStore *history,
                             BrowserSettings *settings,
                             ExtensionManager *extensions,
                             Account *account)
{
    // Placeholder: a real implementation would construct a SyncEngine, bind it
    // to these stores and start replicating once the account is authenticated.
    // The pointers are only retained; none of them is read.
    m_webChannelOwner = webChannelOwner;
    m_passwords = passwords;
    m_bookmarks = bookmarks;
    m_history = history;
    m_settings = settings;
    m_extensions = extensions;
    m_account = account;
}

bool SyncManager::isEnabled() const
{
    return m_enabled;
}

bool SyncManager::isSyncing() const
{
    return m_syncing;
}