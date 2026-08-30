#ifndef PLATFORMADAPTOR_H
#define PLATFORMADAPTOR_H

#include <QObject>
#include <QGuiApplication>
#include <QScreen>

class PlatformAdaptor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PlatformType platform READ platform NOTIFY platformChanged)
    Q_PROPERTY(int screenWidth READ screenWidth NOTIFY screenChanged)
    Q_PROPERTY(int screenHeight READ screenHeight NOTIFY screenChanged)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY screenChanged)

public:
    enum PlatformType {
        Desktop,
        Tablet,
        Phone
    };
    Q_ENUM(PlatformType)

    static PlatformAdaptor *instance();

    PlatformType platform() const { return m_platform; }
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    qreal devicePixelRatio() const { return m_dpr; }

    bool isPhone() const { return m_platform == Phone; }
    bool isTablet() const { return m_platform == Tablet; }
    bool isDesktop() const { return m_platform == Desktop; }

    int toolbarHeight() const;
    int tabBarHeight() const;
    int sidebarWidth() const;
    int urlBarHeight() const;
    int touchTargetSize() const;
    int cornerRadius() const;

    Q_INVOKABLE QString platformName() const;

signals:
    void platformChanged();
    void screenChanged();

private:
    explicit PlatformAdaptor(QObject *parent = nullptr);
    void detectPlatform();
    void updateScreenMetrics();

    PlatformType m_platform = Desktop;
    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    qreal m_dpr = 1.0;
};

#endif