#ifndef APPEARANCEMANAGER_H
#define APPEARANCEMANAGER_H

#include <QObject>
#include <QString>

class BrowserSettings;

// Lightweight QWebChannel bridge for the dual UI-layout engine.
//
// Exposes:
//  - uiLayout current mode
//  - setUiLayout(mode) for live switching from settings/about pages
//
// Layout changes are applied by BrowserWindow, not here, so the bridge
// stays isolated from widget/geometry concerns.
class AppearanceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int uiLayout READ uiLayout NOTIFY uiLayoutChanged)

public:
    explicit AppearanceManager(QObject *parent = nullptr);

    int uiLayout() const;

public slots:
    void setUiLayout(int mode);

signals:
    void uiLayoutChanged();

private:
    BrowserSettings *settings() const;
};

#endif // APPEARANCEMANAGER_H
