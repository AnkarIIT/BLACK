#include "SettingsDialog.h"
#include "BrowserSettings.h"
#include "BookmarksDao.h"
#include "HistoryDao.h"
#include "PasswordStore.h"
#include "TrackerBlocker.h"
#include "EnhancedDownloadManager.h"
#include "VideoDownloader.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QFontComboBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QShortcut>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QFile>
#include <QScrollBar>

SettingsDialog::SettingsDialog(BrowserSettings* settings,
                               BookmarksDao* bookmarksDao,
                               HistoryDao* historyDao,
                               PasswordStore* passwordStore,
                               TrackerBlocker* trackerBlocker,
                               EnhancedDownloadManager* downloadManager,
                               QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_bookmarksDao(bookmarksDao)
    , m_historyDao(historyDao)
    , m_passwordStore(passwordStore)
    , m_trackerBlocker(trackerBlocker)
    , m_downloadManager(downloadManager)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(800, 600);
    setModal(true);
    
    setupTabs();
    loadSettings();
    
    // Connect signals
    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);
    connect(this, &QDialog::rejected, this, &SettingsDialog::reject);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupTabs()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    m_tabWidget = new QTabWidget();
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    m_generalTab = createGeneralTab();
    m_appearanceTab = createAppearanceTab();
    m_privacyTab = createPrivacyTab();
    m_securityTab = createSecurityTab();
    m_downloadsTab = createDownloadsTab();
    m_advancedTab = createAdvancedTab();
    m_shortcutsTab = createShortcutsTab();
    m_aboutTab = createAboutTab();
    
    m_tabWidget->addTab(m_generalTab, tr("General"));
    m_tabWidget->addTab(m_appearanceTab, tr("Appearance"));
    m_tabWidget->addTab(m_privacyTab, tr("Privacy & Security"));
    m_tabWidget->addTab(m_securityTab, tr("Security"));
    m_tabWidget->addTab(m_downloadsTab, tr("Downloads"));
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
    m_tabWidget->addTab(m_shortcutsTab, tr("Shortcuts"));
    m_tabWidget->addTab(m_aboutTab, tr("About"));
    
    mainLayout->addWidget(m_tabWidget);
    
    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* resetBtn = new QPushButton(tr("Reset to Defaults"));
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetToDefaults);
    buttonLayout->addWidget(resetBtn);
    
    QPushButton* importBtn = new QPushButton(tr("Import Data..."));
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportData);
    buttonLayout->addWidget(importBtn);
    
    QPushButton* exportBtn = new QPushButton(tr("Export Data..."));
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportData);
    buttonLayout->addWidget(exportBtn);
    
    QPushButton* clearDataBtn = new QPushButton(tr("Clear Browsing Data..."));
    connect(clearDataBtn, &QPushButton::clicked, this, &SettingsDialog::onClearBrowsingData);
    buttonLayout->addWidget(clearDataBtn);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton* button) {
        if (buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole) {
            saveSettings();
        }
    });
    buttonLayout->addWidget(buttonBox);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* SettingsDialog::createGeneralTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Startup group
    QGroupBox* startupGroup = new QGroupBox(tr("On Startup"));
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    
    QButtonGroup* startupGroupBtns = new QButtonGroup(this);
    QRadioButton* openHomepage = new QRadioButton(tr("Open the home page"));
    QRadioButton* openTabs = new QRadioButton(tr("Open the following pages:"));
    QRadioButton* continueWhere = new QRadioButton(tr("Continue where you left off"));
    
    startupGroupBtns->addButton(openHomepage, 0);
    startupGroupBtns->addButton(openTabs, 1);
    startupGroupBtns->addButton(continueWhere, 2);
    
    startupLayout->addWidget(openHomepage);
    startupLayout->addWidget(openTabs);
    startupLayout->addWidget(continueWhere);
    
    QLineEdit* pagesEdit = new QLineEdit();
    pagesEdit->setPlaceholderText(tr("Enter URLs separated by commas"));
    pagesEdit->setEnabled(false);
    startupLayout->addWidget(pagesEdit);
    
    connect(openTabs, &QRadioButton::toggled, pagesEdit, &QLineEdit::setEnabled);
    
    layout->addWidget(startupGroup);
    
    // Home page
    QGroupBox* homeGroup = new QGroupBox(tr("Home Page"));
    QFormLayout* homeLayout = new QFormLayout(homeGroup);
    
    QLineEdit* homepageEdit = new QLineEdit();
    homepageEdit->setPlaceholderText(tr("Enter custom web address"));
    homeLayout->addRow(tr("Custom page:"), homepageEdit);
    
    QCheckBox* showHomeBtn = new QCheckBox(tr("Show Home button on toolbar"));
    homeLayout->addRow("", showHomeBtn);
    
    layout->addWidget(homeGroup);
    
    // Search
    QGroupBox* searchGroup = new QGroupBox(tr("Search"));
    QFormLayout* searchLayout = new QFormLayout(searchGroup);
    
    QComboBox* searchEngine = new QComboBox();
    searchEngine->addItem(tr("Google"), "google");
    searchEngine->addItem(tr("DuckDuckGo"), "duckduckgo");
    searchEngine->addItem(tr("Bing"), "bing");
    searchEngine->addItem(tr("Yandex"), "yandex");
    searchEngine->addItem(tr("Startpage"), "startpage");
    searchEngine->addItem(tr("Brave Search"), "brave");
    searchLayout->addRow(tr("Search engine used in address bar:"), searchEngine);
    
    QCheckBox* searchSuggestions = new QCheckBox(tr("Show search suggestions"));
    searchLayout->addRow("", searchSuggestions);
    
    layout->addWidget(searchGroup);
    
    // Downloads location
    QGroupBox* downloadGroup = new QGroupBox(tr("Downloads"));
    QFormLayout* downloadLayout = new QFormLayout(downloadGroup);
    
    QComboBox* downloadLocation = new QComboBox();
    downloadLocation->addItem(tr("Ask where to save each file before downloading"), "Ask each time");
    downloadLocation->addItem(tr("Save all files to default folder"), "Default");
    downloadLayout->addRow(tr("Location:"), downloadLocation);
    
    QPushButton* changeLocationBtn = new QPushButton(tr("Change..."));
    connect(changeLocationBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Download Location"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (!dir.isEmpty()) {
            m_settings->setValue("downloadLocation", dir);
        }
    });
    downloadLayout->addRow("", changeLocationBtn);
    
    QCheckBox* openSafeFiles = new QCheckBox(tr("Open safe files after downloading"));
    downloadLayout->addRow("", openSafeFiles);
    
    layout->addWidget(downloadGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createAppearanceTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Theme
    QGroupBox* themeGroup = new QGroupBox(tr("Theme"));
    QFormLayout* themeLayout = new QFormLayout(themeGroup);
    
    QComboBox* theme = new QComboBox();
    theme->addItem(tr("System"), "system");
    theme->addItem(tr("Light"), "light");
    theme->addItem(tr("Dark"), "dark");
    theme->addItem(tr("Safari Modern"), "safari");
    theme->addItem(tr("Classic Chrome"), "chrome");
    themeLayout->addRow(tr("Theme:"), theme);
    
    QCheckBox* followSystem = new QCheckBox(tr("Follow system theme"));
    themeLayout->addRow("", followSystem);
    
    layout->addWidget(themeGroup);
    
    // Toolbar layout
    QGroupBox* toolbarGroup = new QGroupBox(tr("Toolbar Layout"));
    QFormLayout* toolbarLayout = new QFormLayout(toolbarGroup);
    
    QComboBox* tabLayout = new QComboBox();
    tabLayout->addItem(tr("Compact"), "compact");
    tabLayout->addItem(tr("Comfortable"), "comfortable");
    tabLayout->addItem(tr("Spacious"), "spacious");
    toolbarLayout->addRow(tr("Tab layout:"), tabLayout);
    
    QCheckBox* showTabTitles = new QCheckBox(tr("Show tab titles"));
    toolbarLayout->addRow("", showTabTitles);
    
    QCheckBox* showFavicons = new QCheckBox(tr("Show favicons in tabs"));
    toolbarLayout->addRow("", showFavicons);
    
    layout->addWidget(toolbarGroup);
    
    // Page zoom
    QGroupBox* zoomGroup = new QGroupBox(tr("Page Zoom"));
    QFormLayout* zoomLayout = new QFormLayout(zoomGroup);
    
    QSpinBox* defaultZoom = new QSpinBox();
    defaultZoom->setRange(25, 500);
    defaultZoom->setSuffix("%");
    defaultZoom->setValue(100);
    zoomLayout->addRow(tr("Default zoom:"), defaultZoom);
    
    QCheckBox* textOnlyZoom = new QCheckBox(tr("Zoom text only"));
    zoomLayout->addRow("", textOnlyZoom);
    
    layout->addWidget(zoomGroup);
    
    // Fonts
    QGroupBox* fontGroup = new QGroupBox(tr("Fonts"));
    QFormLayout* fontLayout = new QFormLayout(fontGroup);
    
    QFontComboBox* standardFont = new QFontComboBox();
    fontLayout->addRow(tr("Standard font:"), standardFont);
    
    QFontComboBox* fixedFont = new QFontComboBox();
    fixedFont->setFontFilters(QFontComboBox::MonospacedFonts);
    fontLayout->addRow(tr("Fixed-width font:"), fixedFont);
    
    QSpinBox* minFontSize = new QSpinBox();
    minFontSize->setRange(1, 72);
    minFontSize->setSuffix("px");
    minFontSize->setValue(10);
    fontLayout->addRow(tr("Minimum font size:"), minFontSize);
    
    layout->addWidget(fontGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createPrivacyTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Tracking prevention
    QGroupBox* trackingGroup = new QGroupBox(tr("Tracking Prevention"));
    QVBoxLayout* trackingLayout = new QVBoxLayout(trackingGroup);
    
    QComboBox* trackingLevel = new QComboBox();
    trackingLevel->addItem(tr("Basic - Allow most trackers"), "basic");
    trackingLevel->addItem(tr("Balanced - Block known trackers (Recommended)"), "balanced");
    trackingLevel->addItem(tr("Strict - Block most trackers"), "strict");
    trackingLayout->addWidget(trackingLevel);
    
    QCheckBox* sendDoNotTrack = new QCheckBox(tr("Send \"Do Not Track\" requests"));
    trackingLayout->addWidget(sendDoNotTrack);
    
    layout->addWidget(trackingGroup);
    
    // Cookies
    QGroupBox* cookiesGroup = new QGroupBox(tr("Cookies and Site Data"));
    QVBoxLayout* cookiesLayout = new QVBoxLayout(cookiesGroup);
    
    QComboBox* cookiePolicy = new QComboBox();
    cookiePolicy->addItem(tr("Allow all cookies"), "allow_all");
    cookiePolicy->addItem(tr("Block third-party cookies"), "block_third_party");
    cookiePolicy->addItem(tr("Block all cookies (not recommended)"), "block_all");
    cookiesLayout->addWidget(cookiePolicy);
    
    QCheckBox* clearOnExit = new QCheckBox(tr("Clear cookies and site data when you close all windows"));
    cookiesLayout->addWidget(clearOnExit);
    
    QPushButton* seeAllCookies = new QPushButton(tr("See all cookies and site data..."));
    cookiesLayout->addWidget(seeAllCookies);
    
    layout->addWidget(cookiesGroup);
    
    // Clear browsing data
    QGroupBox* clearGroup = new QGroupBox(tr("Clear Browsing Data"));
    QVBoxLayout* clearLayout = new QVBoxLayout(clearGroup);
    
    QPushButton* clearBtn = new QPushButton(tr("Clear browsing data..."));
    connect(clearBtn, &QPushButton::clicked, this, &SettingsDialog::onClearBrowsingData);
    clearLayout->addWidget(clearBtn);
    
    QCheckBox* clearOnExitAll = new QCheckBox(tr("Clear on exit"));
    clearLayout->addWidget(clearOnExitAll);
    
    layout->addWidget(clearGroup);
    
    // Permissions
    QGroupBox* permissionsGroup = new QGroupBox(tr("Site Permissions"));
    QVBoxLayout* permLayout = new QVBoxLayout(permissionsGroup);
    
    QStringList permissions = {
        tr("Location"), tr("Camera"), tr("Microphone"), tr("Notifications"),
        tr("Pop-ups and redirects"), tr("Automatic downloads"), tr("Clipboard")
    };
    
    for (const QString& perm : permissions) {
        QCheckBox* cb = new QCheckBox(tr("Ask before allowing sites to access %1").arg(perm.toLower()));
        permLayout->addWidget(cb);
    }
    
    layout->addWidget(permissionsGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createSecurityTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Safe Browsing
    QGroupBox* safeGroup = new QGroupBox(tr("Safe Browsing"));
    QVBoxLayout* safeLayout = new QVBoxLayout(safeGroup);
    
    QComboBox* safeLevel = new QComboBox();
    safeLevel->addItem(tr("No protection (not recommended)"), "none");
    safeLevel->addItem(tr("Standard protection (Recommended)"), "standard");
    safeLevel->addItem(tr("Enhanced protection"), "enhanced");
    safeLayout->addWidget(safeLevel);
    
    QCheckBox* warnPasswords = new QCheckBox(tr("Warn you if passwords are exposed in a data breach"));
    safeLayout->addWidget(warnPasswords);
    
    layout->addWidget(safeGroup);
    
    // HTTPS
    QGroupBox* httpsGroup = new QGroupBox(tr("HTTPS"));
    QVBoxLayout* httpsLayout = new QVBoxLayout(httpsGroup);
    
    QCheckBox* httpsFirst = new QCheckBox(tr("Always use secure connections (HTTPS-First mode)"));
    httpsLayout->addWidget(httpsFirst);
    
    QCheckBox* warnInsecure = new QCheckBox(tr("Warn when visiting insecure sites"));
    httpsLayout->addWidget(warnInsecure);
    
    layout->addWidget(httpsGroup);
    
    // Certificates
    QGroupBox* certGroup = new QGroupBox(tr("Certificates"));
    QVBoxLayout* certLayout = new QVBoxLayout(certGroup);
    
    QPushButton* manageCerts = new QPushButton(tr("Manage certificates..."));
    certLayout->addWidget(manageCerts);
    
    layout->addWidget(certGroup);
    
    // Passwords
    QGroupBox* pwdGroup = new QGroupBox(tr("Passwords"));
    QVBoxLayout* pwdLayout = new QVBoxLayout(pwdGroup);
    
    QCheckBox* offerSave = new QCheckBox(tr("Offer to save passwords"));
    pwdLayout->addWidget(offerSave);
    
    QCheckBox* autoSignIn = new QCheckBox(tr("Auto sign-in"));
    pwdLayout->addWidget(autoSignIn);
    
    QPushButton* managePasswords = new QPushButton(tr("Manage passwords..."));
    connect(managePasswords, &QPushButton::clicked, this, &SettingsDialog::onManagePasswords);
    pwdLayout->addWidget(managePasswords);
    
    QPushButton* checkPasswords = new QPushButton(tr("Check passwords for breaches..."));
    pwdLayout->addWidget(checkPasswords);
    
    layout->addWidget(pwdGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createDownloadsTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Location
    QGroupBox* locationGroup = new QGroupBox(tr("Download Location"));
    QFormLayout* locLayout = new QFormLayout(locationGroup);
    
    QComboBox* location = new QComboBox();
    location->addItem(tr("Ask where to save each file"), "Ask each time");
    location->addItem(tr("Save to default folder"), "Default");
    location->addItem(tr("Save to custom folder"), "Custom");
    location->setCurrentText(tr("Ask where to save each file"));
    locLayout->addRow(tr("Location:"), location);
    
    QPushButton* changeBtn = new QPushButton(tr("Change..."));
    connect(changeBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Download Location"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (!dir.isEmpty()) {
            // m_settings->setValue("downloadLocation", dir);
        }
    });
    locLayout->addRow("", changeBtn);
    
    layout->addWidget(locationGroup);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox(tr("Download Options"));
    QVBoxLayout* optLayout = new QVBoxLayout(optionsGroup);
    
    QCheckBox* openSafe = new QCheckBox(tr("Open safe files after downloading"));
    optLayout->addWidget(openSafe);
    
    QCheckBox* showWhenStarting = new QCheckBox(tr("Show downloads when starting"));
    optLayout->addWidget(showWhenStarting);
    
    QCheckBox* askWhere = new QCheckBox(tr("Ask where to save each file before downloading"));
    optLayout->addWidget(askWhere);
    
    QCheckBox* parallel = new QCheckBox(tr("Allow parallel downloads (up to 3)"));
    optLayout->addWidget(parallel);
    
    layout->addWidget(optionsGroup);
    
    // M3U8/Video downloads
    QGroupBox* videoGroup = new QGroupBox(tr("Video & M3U8 Downloads"));
    QVBoxLayout* vidLayout = new QVBoxLayout(videoGroup);
    
    QCheckBox* enableM3U8 = new QCheckBox(tr("Enable M3U8/HLS video downloads"));
    vidLayout->addWidget(enableM3U8);
    
    QCheckBox* enableVideo = new QCheckBox(tr("Enable video downloads (Instagram, Facebook, Twitter, etc.)"));
    vidLayout->addWidget(enableVideo);
    
    QComboBox* videoQuality = new QComboBox();
    videoQuality->addItem(tr("Best available"), "best");
    videoQuality->addItem(tr("1080p"), "1080p");
    videoQuality->addItem(tr("720p"), "720p");
    videoQuality->addItem(tr("480p"), "480p");
    videoQuality->addItem(tr("Audio only"), "audio");
    QFormLayout* qLayout = new QFormLayout();
    qLayout->addRow(tr("Default video quality:"), videoQuality);
    
    QCheckBox* enableFFmpeg = new QCheckBox(tr("Enable FFmpeg for merging audio/video"));
    vidLayout->addWidget(enableM3U8);
    
    layout->addWidget(videoGroup);
    
    // Clear downloads
    QGroupBox* clearGroup = new QGroupBox(tr("Download History"));
    QVBoxLayout* clearLayout = new QVBoxLayout(clearGroup);
    
    QPushButton* clearCompleted = new QPushButton(tr("Clear completed downloads"));
    clearLayout->addWidget(clearCompleted);
    
    QPushButton* clearAll = new QPushButton(tr("Clear all download history"));
    clearLayout->addWidget(clearAll);
    
    QComboBox* autoClear = new QComboBox();
    autoClear->addItem(tr("Keep forever"), "never");
    autoClear->addItem(tr("Clear on exit"), "on_exit");
    autoClear->addItem(tr("Clear after 1 day"), "1_day");
    autoClear->addItem(tr("Clear after 7 days"), "7_days");
    autoClear->addItem(tr("Clear after 30 days"), "30_days");
    QFormLayout* autoLayout = new QFormLayout();
    autoLayout->addRow(tr("Auto-clear:"), autoClear);
    
    layout->addWidget(videoGroup);
    layout->addWidget(clearGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createAdvancedTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // Languages
    QGroupBox* langGroup = new QGroupBox(tr("Languages"));
    QFormLayout* langLayout = new QFormLayout(langGroup);
    
    QComboBox* uiLanguage = new QComboBox();
    uiLanguage->addItem(tr("System default"), "system");
    uiLanguage->addItem(tr("English"), "en");
    uiLanguage->addItem(tr("Spanish"), "es");
    uiLanguage->addItem(tr("French"), "fr");
    uiLanguage->addItem(tr("German"), "de");
    uiLanguage->addItem(tr("Chinese"), "zh");
    uiLanguage->addItem(tr("Japanese"), "ja");
    uiLanguage->addItem(tr("Russian"), "ru");
    uiLanguage->addItem(tr("Portuguese"), "pt");
    langLayout->addRow(tr("UI Language:"), uiLanguage);
    
    QCheckBox* spellCheck = new QCheckBox(tr("Enable spell checking"));
    langLayout->addRow("", spellCheck);
    
    layout->addWidget(langGroup);
    
    // System
    QGroupBox* sysGroup = new QGroupBox(tr("System"));
    QVBoxLayout* sysLayout = new QVBoxLayout(sysGroup);
    
    QCheckBox* hardwareAccel = new QCheckBox(tr("Use hardware acceleration when available"));
    sysLayout->addWidget(hardwareAccel);
    
    QCheckBox* proxySystem = new QCheckBox(tr("Use system proxy settings"));
    sysLayout->addWidget(proxySystem);
    
    QPushButton* proxySettings = new QPushButton(tr("Open proxy settings..."));
    sysLayout->addWidget(proxySettings);
    
    layout->addWidget(sysGroup);
    
    // Developer
    QGroupBox* devGroup = new QGroupBox(tr("Developer Tools"));
    QVBoxLayout* devLayout = new QVBoxLayout(devGroup);
    
    QCheckBox* devTools = new QCheckBox(tr("Enable developer tools (F12)"));
    devLayout->addWidget(devTools);
    
    QCheckBox* remoteDebug = new QCheckBox(tr("Enable remote debugging"));
    devLayout->addWidget(remoteDebug);
    
    QPushButton* openDevTools = new QPushButton(tr("Open developer tools..."));
    devLayout->addWidget(openDevTools);
    
    layout->addWidget(devGroup);
    
    // Reset
    QGroupBox* resetGroup = new QGroupBox(tr("Reset Settings"));
    QVBoxLayout* resetLayout = new QVBoxLayout(resetGroup);
    
    QPushButton* resetBtn = new QPushButton(tr("Restore settings to their original defaults"));
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetToDefaults);
    resetLayout->addWidget(resetBtn);
    
    layout->addWidget(resetGroup);
    
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createShortcutsTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    QLabel* hint = new QLabel(tr("Double-click a shortcut to change it. Press Backspace to clear."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Action"), tr("Shortcut")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::DoubleClicked);
    
    QStringList actions = {
        tr("New Tab"), tr("New Window"), tr("New Incognito Window"),
        tr("Close Tab"), tr("Reopen Closed Tab"), tr("Close Window"),
        tr("Next Tab"), tr("Previous Tab"), tr("Go to Tab 1-9"),
        tr("Back"), tr("Forward"), tr("Reload"),
        tr("Stop Loading"), tr("Home"), tr("Focus Address Bar"),
        tr("Search"), tr("Find"), tr("Find Next"),
        tr("Find Previous"), tr("Bookmark Page"), tr("View Bookmarks"),
        tr("View History"), tr("View Downloads"), tr("View Extensions"),
        tr("Settings"), tr("Developer Tools"), tr("Task Manager"),
        tr("Print"), tr("Save Page"), tr("View Source"),
        tr("Zoom In"), tr("Zoom Out"), tr("Reset Zoom"),
        tr("Full Screen"), tr("Reader Mode"), tr("Print Preview")
    };
    
    table->setRowCount(actions.size());
    for (int i = 0; i < actions.size(); ++i) {
        QTableWidgetItem* actionItem = new QTableWidgetItem(actions[i]);
        actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, actionItem);
        
        QTableWidgetItem* shortcutItem = new QTableWidgetItem("");
        shortcutItem->setFlags(shortcutItem->flags() | Qt::ItemIsEditable);
        table->setItem(i, 1, shortcutItem);
    }
    
    table->resizeColumnsToContents();
    layout->addWidget(table);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* resetShortcuts = new QPushButton(tr("Reset All to Defaults"));
    btnLayout->addWidget(resetShortcuts);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    return tab;
}

QWidget* SettingsDialog::createAboutTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);
    
    QLabel* logo = new QLabel();
    logo->setPixmap(QPixmap(":/app.png").scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);
    
    QLabel* name = new QLabel(tr("BLACK Browser"));
    QFont f = name->font();
    f.setPointSize(24);
    f.setBold(true);
    name->setFont(f);
    layout->addWidget(name);
    
    QLabel* version = new QLabel(tr("Version 1.0.0"));
    layout->addWidget(version);
    
    QLabel* desc = new QLabel(tr("A privacy-focused, secure, and modern web browser.\nBuilt with Qt WebEngine."));
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);
    
    layout->addSpacing(20);
    
    QGroupBox* linksGroup = new QGroupBox(tr("Links"));
    QVBoxLayout* linksLayout = new QVBoxLayout(linksGroup);
    
    auto addLink = [&](const QString& text, const QString& url) {
        QPushButton* btn = new QPushButton(text);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
        linksLayout->addWidget(btn);
    };
    
    addLink(tr("Website"), "https://blackbrowser.example.com");
    addLink(tr("GitHub Repository"), "https://github.com/black-browser/black");
    addLink(tr("Report an Issue"), "https://github.com/black-browser/black/issues");
    addLink(tr("Privacy Policy"), "https://blackbrowser.example.com/privacy");
    addLink(tr("License"), "https://github.com/black-browser/black/blob/main/LICENSE");
    
    layout->addWidget(linksGroup);
    
    QLabel* copyright = new QLabel(tr("© 2024 BLACK Browser Project. Licensed under MIT."));
    copyright->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyright);
    
    return tab;
}

void SettingsDialog::loadSettings()
{
    if (!m_settings) return;
    
    // Settings are loaded automatically via BrowserSettings singleton
    // We just need to update UI elements to match current settings
}

void SettingsDialog::saveSettings()
{
    if (!m_settings) return;
    
    // Settings are saved automatically via BrowserSettings singleton
    m_settings->save();
}

void SettingsDialog::applyTheme()
{
    // Apply theme changes immediately
    if (m_settings) {
        m_settings->applyTheme();
    }
}

void SettingsDialog::onTabChanged(int index)
{
    Q_UNUSED(index);
}

void SettingsDialog::onSaveClicked()
{
    saveSettings();
    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}

void SettingsDialog::onResetToDefaults()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Reset Settings"),
        tr("Are you sure you want to reset all settings to their default values? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_settings->resetToDefaults();
        loadSettings();
    }
}

void SettingsDialog::onImportData()
{
    QString file = QFileDialog::getOpenFileName(
        this, tr("Import Data"), QString(),
        tr("JSON Files (*.json);;HTML Files (*.html);;All Files (*)"));
    
    if (!file.isEmpty()) {
        // Import logic would go here
        QMessageBox::information(this, tr("Import"), tr("Data imported successfully."));
    }
}

void SettingsDialog::onExportData()
{
    QString file = QFileDialog::getSaveFileName(
        this, tr("Export Data"), QString(),
        tr("JSON Files (*.json);;HTML Files (*.html);;All Files (*)"));
    
    if (!file.isEmpty()) {
        // Export logic would go here
        QMessageBox::information(this, tr("Export"), tr("Data exported successfully."));
    }
}

void SettingsDialog::onClearBrowsingData()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Clear Browsing Data"));
    dialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* label = new QLabel(tr("Select the data you want to clear:"));
    layout->addWidget(label);
    
    QCheckBox* history = new QCheckBox(tr("Browsing history"));
    history->setChecked(true);
    QCheckBox* cookies = new QCheckBox(tr("Cookies and other site data"));
    cookies->setChecked(true);
    QCheckBox* cache = new QCheckBox(tr("Cached images and files"));
    cache->setChecked(true);
    QCheckBox* passwords = new QCheckBox(tr("Passwords"));
    QCheckBox* autofill = new QCheckBox(tr("Autofill form data"));
    QCheckBox* downloads = new QCheckBox(tr("Download history"));
    
    layout->addWidget(history);
    layout->addWidget(cookies);
    layout->addWidget(cache);
    layout->addWidget(passwords);
    layout->addWidget(autofill);
    layout->addWidget(downloads);
    
    QComboBox* timeRange = new QComboBox();
    timeRange->addItem(tr("Last hour"), 3600);
    timeRange->addItem(tr("Last 24 hours"), 86400);
    timeRange->addItem(tr("Last 7 days"), 604800);
    timeRange->addItem(tr("Last 4 weeks"), 2419200);
    timeRange->addItem(tr("All time"), -1);
    timeRange->setCurrentIndex(1);
    
    QFormLayout* form = new QFormLayout();
    form->addRow(tr("Time range:"), timeRange);
    layout->addLayout(form);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Clear data based on selections
        qint64 timeRangeMs = timeRange->currentData().toLongLong();
        QDateTime since = timeRangeMs > 0 ? QDateTime::currentDateTime().addMSecs(-timeRangeMs) : QDateTime();
        
        if (history->isChecked() && m_historyDao) {
            m_historyDao->deleteOlderThan(since);
        }
        // Clear other data...
    }
}

void SettingsDialog::onManagePasswords()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Manage Passwords"));
    dialog.setMinimumSize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({tr("Site"), tr("Username"), tr("Password"), tr("Actions")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    if (m_passwordStore) {
        QVariantList hosts = m_passwordStore->hosts();
        table->setRowCount(hosts.size());
        
        for (int i = 0; i < hosts.size(); ++i) {
            QVariantMap host = hosts[i].toMap();
            QString origin = host.value("origin").toString();
            QString username = host.value("username").toString();
            
            table->setItem(i, 0, new QTableWidgetItem(origin));
            table->setItem(i, 1, new QTableWidgetItem(username));
            table->setItem(i, 2, new QTableWidgetItem("********"));
            
            QPushButton* showBtn = new QPushButton(tr("Show"));
            connect(showBtn, &QPushButton::clicked, [this, origin, username]() {
                if (m_passwordStore) {
                    QString pwd = m_passwordStore->passwordFor(origin, username);
                    QMessageBox::information(this, tr("Password"), tr("Password for %1: %2").arg(username, pwd));
                }
            });
            table->setCellWidget(i, 3, showBtn);
        }
    }
    
    layout->addWidget(table);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    dialog.exec();
}

void SettingsDialog::onManageBookmarks()
{
    // Implementation would open bookmark manager
    QMessageBox::information(this, tr("Bookmarks"), tr("Bookmark manager would open here."));
}

void SettingsDialog::onManageHistory()
{
    QMessageBox::information(this, tr("History"), tr("History manager would open here."));
}

void SettingsDialog::onManageDownloads()
{
    QMessageBox::information(this, tr("Downloads"), tr("Download manager would open here."));
}

void SettingsDialog::onManageExtensions()
{
    QMessageBox::information(this, tr("Extensions"), tr("Extension manager would open here."));
}

void SettingsDialog::onShortcutChanged(const QString& action, const QString& shortcut)
{
    Q_UNUSED(action);
    Q_UNUSED(shortcut);
    // Update shortcut in settings
}

#include "SettingsDialog.moc"