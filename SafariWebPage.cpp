#include "SafariWebPage.h"

#include <QApplication>
#include <QWebChannel>
#include <QWebEngineScript>
#include <QWebEngineView>
#include <QDialog>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "SafariTheme.h"

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

int SafariWebPage::showJsDialog(const QString &message, const QString &okText,
                                const QString &cancelText, QString *input,
                                bool showInput) const
{
    const SafariTheme &t = SafariTheme::instance();

    // The dialog is a child of the view that owns this page, sized to cover it
    // so the translucent scrim dims the whole page behind the card.
    QWidget *anchor = qobject_cast<QWidget *>(parent());
    if (!anchor)
        anchor = QApplication::activeWindow();
    QDialog dialog(anchor);
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setStyleSheet(QStringLiteral("QDialog { background: %1; }").arg(t.scrim));

    QVBoxLayout *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(48, 40, 48, 40);

    QFrame *card = new QFrame(&dialog);
    card->setObjectName(QStringLiteral("jsCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#jsCard { background: %1; border: 1px solid %2; border-radius: 14px; }")
                            .arg(t.cardBg, t.border));
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 22, 24, 22);
    cardLayout->setSpacing(16);

    QLabel *text = new QLabel(message, card);
    text->setWordWrap(true);
    text->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 14px; line-height: 1.4;")
                            .arg(t.textPrimary));
    cardLayout->addWidget(text);

    QLineEdit *field = nullptr;
    if (showInput) {
        field = new QLineEdit(card);
        field->setText(input ? *input : QString());
        field->setStyleSheet(QStringLiteral(
            "QLineEdit { background: %1; border: 1px solid %2; border-radius: 8px; "
            "color: %3; padding: 8px 10px; font-size: 13px; }")
                                 .arg(t.bgUrlBar, t.border, t.textPrimary));
        cardLayout->addWidget(field);
    }

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    if (!cancelText.isEmpty()) {
        QPushButton *cancelBtn = new QPushButton(cancelText, card);
        cancelBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: %1; border: 1px solid %2; border-radius: 8px; "
            "color: %3; padding: 8px 18px; font-weight: 500; }"
            "QPushButton:hover { background: %4; }")
                                     .arg(t.hover, t.border, t.textPrimary, t.tabHover));
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        btnRow->addWidget(cancelBtn);
    }

    QPushButton *okBtn = new QPushButton(okText.isEmpty() ? tr("OK") : okText, card);
    okBtn->setDefault(true);
    okBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: none; border-radius: 8px; color: #ffffff; "
        "padding: 8px 18px; font-weight: 600; }"
        "QPushButton:hover { background: %2; }")
                             .arg(t.accent, t.accentHover));
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnRow->addWidget(okBtn);
    cardLayout->addLayout(btnRow);

    if (showInput && field)
        field->setFocus();

    if (anchor)
        dialog.resize(anchor->size());

    const int result = dialog.exec();
    if (showInput && input && field)
        *input = field->text();
    return result;
}

void SafariWebPage::javaScriptAlert(const QUrl &securityOrigin, const QString &msg)
{
    Q_UNUSED(securityOrigin);
    showJsDialog(msg, tr("OK"), QString(), nullptr, false);
}

bool SafariWebPage::javaScriptConfirm(const QUrl &securityOrigin, const QString &msg)
{
    Q_UNUSED(securityOrigin);
    return showJsDialog(msg, tr("OK"), tr("Cancel"), nullptr, false) == QDialog::Accepted;
}

bool SafariWebPage::javaScriptPrompt(const QUrl &securityOrigin, const QString &msg,
                                     const QString &defaultValue, QString *result)
{
    Q_UNUSED(securityOrigin);
    QString input = defaultValue;
    const bool accepted = showJsDialog(msg, tr("OK"), tr("Cancel"), &input, true)
        == QDialog::Accepted;
    if (result)
        *result = accepted ? input : QString();
    return accepted;
}
