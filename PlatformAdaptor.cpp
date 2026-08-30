#include "PlatformAdaptor.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

PlatformAdaptor::PlatformAdaptor(QObject *parent)
    : QObject(parent)
{
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *screen) {
        if (screen) {
            updateScreenMetrics();
            detectPlatform();
        }
    });

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        updateScreenMetrics();
        detectPlatform();
    }
}

PlatformAdaptor *PlatformAdaptor::instance()
{
    static PlatformAdaptor *inst = new PlatformAdaptor(qApp);
    return inst;
}

void PlatformAdaptor::updateScreenMetrics()
{
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect geom = screen->geometry();
        const qreal dpr = screen->devicePixelRatio();

        m_screenWidth = qRound(geom.width() / dpr);
        m_screenHeight = qRound(geom.height() / dpr);
        m_dpr = dpr;
    }
}

void PlatformAdaptor::detectPlatform()
{
    PlatformType oldPlatform = m_platform;

    const int width = m_screenWidth;
    const int height = m_screenHeight;
    const int minDim = qMin(width, height);

    if (minDim <= 480) {
        m_platform = Phone;
    } else if (minDim <= 1024) {
        m_platform = Tablet;
    } else {
        m_platform = Desktop;
    }

    if (m_platform != oldPlatform) {
        emit platformChanged();
    }
}

int PlatformAdaptor::toolbarHeight() const
{
    switch (m_platform) {
    case Phone:    return 56;
    case Tablet:   return 48;
    case Desktop:  return 44;
    }
    return 44;
}

int PlatformAdaptor::tabBarHeight() const
{
    switch (m_platform) {
    case Phone:    return 48;
    case Tablet:   return 38;
    case Desktop:  return 34;
    }
    return 34;
}

int PlatformAdaptor::sidebarWidth() const
{
    switch (m_platform) {
    case Phone:    return 280;
    case Tablet:   return 320;
    case Desktop:  return 300;
    }
    return 300;
}

int PlatformAdaptor::urlBarHeight() const
{
    switch (m_platform) {
    case Phone:    return 44;
    case Tablet:   return 40;
    case Desktop:  return 36;
    }
    return 36;
}

int PlatformAdaptor::touchTargetSize() const
{
    switch (m_platform) {
    case Phone:    return 48;
    case Tablet:   return 44;
    case Desktop:  return 32;
    }
    return 32;
}

int PlatformAdaptor::cornerRadius() const
{
    switch (m_platform) {
    case Phone:    return 16;
    case Tablet:   return 12;
    case Desktop:  return 10;
    }
    return 10;
}

QString PlatformAdaptor::platformName() const
{
    switch (m_platform) {
    case Phone:    return QStringLiteral("phone");
    case Tablet:   return QStringLiteral("tablet");
    case Desktop:  return QStringLiteral("desktop");
    }
    return QStringLiteral("desktop");
}