#include "SafariWebPage.h"

#include <QApplication>
#include <QWebChannel>
#include <QWebEngineScript>

SafariWebPage::SafariWebPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
}

void SafariWebPage::setWebChannelObject(QWebChannel *channel)
{
    m_webChannel = channel;
}

void SafariWebPage::setPasswordChannelObject(QWebChannel *channel)
{
    m_passwordChannel = channel;
}

bool SafariWebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    if (type == NavigationTypeLinkClicked && isMainFrame) {
        const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        if (modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier)) {
            emit newTabRequested(url);
            return false;
        }
    }

    // Apply/remove the QWebChannel transport before the new document is
    // created.
    //   - Internal qrc: pages get the FULL bridge transport in the MAIN world
    //     so their own scripts can use the full bridge (bookmarks, history,
    //     passwords, settings...).
    //   - External sites get only the PASSWORD-ONLY bridge transport, and only
    //     in the private kPasswordWorld where the autofill content script runs.
    //     The site's own JavaScript (main world) and installed extension
    //     content scripts (ApplicationWorld) never see qt.webChannelTransport,
    //     so neither can ever reach the registered objects.
    if (isMainFrame) {
        const bool internal = (url.scheme() == QLatin1String("qrc"));
        QWebChannel *channel = internal ? m_webChannel.data() : m_passwordChannel.data();
        setWebChannel(channel,
                      internal ? QWebEngineScript::MainWorld : kPasswordWorld);
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}
