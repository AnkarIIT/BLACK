#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

class PermissionsBridge : public QObject
{
    Q_OBJECT

public:
    explicit PermissionsBridge(QObject *parent = nullptr);

    Q_INVOKABLE QJsonArray getPermissions();
    Q_INVOKABLE void removePermission(const QString &origin, int type);
    Q_INVOKABLE void clearPermissions();

    // Test helper: write permissions directly (bypassing encryption for test setup)
    Q_INVOKABLE void savePermissionsTest(const QJsonObject &obj);

signals:
    void changed();

private:
    friend class TestPermissionsBridge;
    QString keyFor(const QString &origin, int type) const;
    QString typeName(int type) const;
};