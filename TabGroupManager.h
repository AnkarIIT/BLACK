#ifndef TABGROUPMANAGER_H
#define TABGROUPMANAGER_H

#include <QObject>

class BrowserWindow;

// Placeholder implementation. Saved/restoreable tab GROUPS are NOT implemented
// in this build. The UI still lets the user group tabs for the current session
// (BrowserWindow owns that state); this manager exists only to satisfy the
// wiring and reports "no persisted groups".
class TabGroupManager : public QObject
{
    Q_OBJECT
public:
    explicit TabGroupManager(QObject *parent = nullptr);

    void setBrowserWindow(BrowserWindow *window);

    // No persisted groups in this build.
    Q_INVOKABLE int groupCount() const;                       // 0
    Q_INVOKABLE bool saveGroups() const;                      // false
    Q_INVOKABLE bool restoreGroups();                         // false
};

#endif // TABGROUPMANAGER_H