#ifndef TOOLBARCONTROLLER_H
#define TOOLBARCONTROLLER_H

#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>

class BrowserWindow;

class ToolbarController : public QObject
{
    Q_OBJECT
public:
    explicit ToolbarController(BrowserWindow *window, QObject *parent = nullptr);
    ~ToolbarController();

    void setupToolbar(QWidget *central, QVBoxLayout *centralLayout);
    void setupUrlBar();
    void setupNavigationButtons();
    void setupActionButtons();

    void applyTheme();
    void applyUiLayout(bool chrome);

    void updateNavigationState();
    void updateUrlContainerStyle();
    void animateUrlBar(int targetWidth);

    QLineEdit *urlBar() const { return m_urlBar; }
    QToolButton *backButton() const { return m_backButton; }
    QToolButton *forwardButton() const { return m_forwardButton; }
    QToolButton *reloadButton() const { return m_reloadButton; }
    QToolButton *sidebarButton() const { return m_sidebarButton; }
    QProgressBar *loadingBar() const { return m_loadingBar; }

signals:
    void navigateRequested();
    void backRequested();
    void forwardRequested();
    void reloadRequested();
    void stopRequested();
    void sidebarToggled();
    void shareRequested();
    void downloadsRequested();
    void tabOverviewRequested();
    void newTabRequested();
    void readerModeRequested();
    void translateRequested();
    void settingsRequested();
    void pipRequested();

private:
    BrowserWindow *m_window;
    QWidget *m_toolbar = nullptr;
    QHBoxLayout *m_toolbarLayout = nullptr;
    QHBoxLayout *m_trafficLayout = nullptr;

    QFrame *m_urlContainer = nullptr;
    QParallelAnimationGroup *m_urlAnim = nullptr;
    QToolButton *m_shieldInside = nullptr;
    QToolButton *m_lockButton = nullptr;

    QLineEdit *m_urlBar = nullptr;
    QToolButton *m_backButton = nullptr;
    QToolButton *m_forwardButton = nullptr;
    QToolButton *m_sidebarButton = nullptr;
    QToolButton *m_reloadButton = nullptr;
    QToolButton *m_shareButton = nullptr;
    QToolButton *m_downloadsButton = nullptr;
    QToolButton *m_tabOverviewButton = nullptr;
    QToolButton *m_addTabButton = nullptr;
    QToolButton *m_readerModeButton = nullptr;
    QToolButton *m_translateButton = nullptr;
    QToolButton *m_pipButton = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QToolButton *m_extensionsButton = nullptr;
    QToolButton *m_profileButton = nullptr;

    QProgressBar *m_loadingBar = nullptr;

    QWidget *m_urlSuggest = nullptr;
    QVBoxLayout *m_urlSuggestLayout = nullptr;
    QList<QPushButton*> m_urlSuggestRows;
    QList<QUrl> m_urlSuggestionUrls;
    int m_urlSuggestionIndex = -1;

    bool m_urlFocused = false;
    bool m_urlMouseFocusPending = false;

    void setupUrlSuggestions();
    void rebuildUrlSuggestions();
    void showUrlSuggestions();
    void hideUrlSuggestions();
    void selectUrlSuggestion(int index);
    void activateUrlSuggestion(int index);

    void updateButtonIcons(bool chrome);
    void updateUrlBarStyle(bool chrome);
    void updateTrafficLights(bool chrome);
    void updateLoadingBarStyle(bool chrome);
};

#endif