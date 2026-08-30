#ifndef PLATFORMADAPTOR_H
#define PLATFORMADAPTOR_H

#include <QObject>
#include <QString>
#include <QScreen>
#include <QGuiApplication>

class PlatformAdaptor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString platformName READ platformName NOTIFY platformChanged)
    Q_PROPERTY(int touchTargetSize READ touchTargetSize NOTIFY platformChanged)

public:
    static PlatformAdaptor &instance();

    QString platformName() const { return m_platformName; }
    int touchTargetSize() const { return m_touchTargetSize; }

    Q_INVOKABLE void detectPlatform();

signals:
    void platformChanged(const QString &platform);

private:
    explicit PlatformAdaptor(QObject *parent = nullptr);
    void updateScreenMetrics();

    QString m_platformName = QStringLiteral("desktop");
    int m_touchTargetSize = 32;
};

#endif // PLATFORMADAPTOR_H
