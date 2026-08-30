#include "PlatformAdaptor.h"
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

PlatformAdaptor &PlatformAdaptor::instance()
{
    static PlatformAdaptor instance;
    return instance;
}

PlatformAdaptor::PlatformAdaptor(QObject *parent)
    : QObject(parent)
{
    updateScreenMetrics();
    if (auto *screen = QGuiApplication::primaryScreen()) {
        connect(screen, &QScreen::geometryChanged, this, &PlatformAdaptor::updateScreenMetrics);
    }
}

void PlatformAdaptor::updateScreenMetrics()
{
    detectPlatform();
}

void PlatformAdaptor::detectPlatform()
{
    QString detected = QStringLiteral("desktop");
    int target = 32;

    if (auto *screen = QGuiApplication::primaryScreen()) {
        const QRect geom = screen->geometry();
        const int minDim = qMin(geom.width(), geom.height());

        if (minDim < 500) {
            detected = QStringLiteral("phone");
            target = 48;
        } else if (minDim < 900) {
            detected = QStringLiteral("tablet");
            target = 44;
        } else {
            detected = QStringLiteral("desktop");
            target = 32;
        }
    }

    if (m_platformName != detected || m_touchTargetSize != target) {
        m_platformName = detected;
        m_touchTargetSize = target;
        emit platformChanged(m_platformName);
    }
}
