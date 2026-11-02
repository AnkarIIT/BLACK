#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QWidget>

class BrowserSettings;
class BookmarksDao;
class HistoryDao;
class PasswordStore;
class TrackerBlocker;
class DownloadManager;
class EnhancedDownloadManager;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(BrowserSettings* settings,
                            BookmarksDao* bookmarksDao,
                            HistoryDao* historyDao,
                            PasswordStore* passwordStore,
                            TrackerBlocker* trackerBlocker,
                            EnhancedDownloadManager* downloadManager,
                            QWidget* parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onTabChanged(int index);
    void onSaveClicked();
    void onCancelClicked();
    void onResetToDefaults();
    void onImportData();
    void onExportData();
    void onClearBrowsingData();
    void onManagePasswords();
    void onManageBookmarks();
    void onManageHistory();
    void onManageDownloads();
    void onManageExtensions();
    void onShortcutChanged(const QString& action, const QString& shortcut);

private:
    void setupTabs();
    QWidget* createGeneralTab();
    QWidget* createAppearanceTab();
    QWidget* createPrivacyTab();
    QWidget* createSecurityTab();
    QWidget* createDownloadsTab();
    QWidget* createAdvancedTab();
    QWidget* createShortcutsTab();
    QWidget* createAboutTab();
    
    void loadSettings();
    void saveSettings();
    void applyTheme();
    
    BrowserSettings* m_settings = nullptr;
    BookmarksDao* m_bookmarksDao = nullptr;
    HistoryDao* m_historyDao = nullptr;
    PasswordStore* m_passwordStore = nullptr;
    TrackerBlocker* m_trackerBlocker = nullptr;
    EnhancedDownloadManager* m_downloadManager = nullptr;
    
    QTabWidget* m_tabWidget = nullptr;
    QWidget* m_generalTab = nullptr;
    QWidget* m_appearanceTab = nullptr;
    QWidget* m_privacyTab = nullptr;
    QWidget* m_securityTab = nullptr;
    QWidget* m_downloadsTab = nullptr;
    QWidget* m_advancedTab = nullptr;
    QWidget* m_shortcutsTab = nullptr;
    QWidget* m_aboutTab = nullptr;
};

#endif // SETTINGSDIALOG_H