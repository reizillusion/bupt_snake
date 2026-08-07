#include "common.h"

QString modeDisplayName(GameMode mode)
{
    return mode == GameMode::Classic ? QStringLiteral("经典模式")
                                     : QStringLiteral("无边界模式");
}

QString modeKey(GameMode mode)
{
    return mode == GameMode::Classic ? QStringLiteral("classic")
                                     : QStringLiteral("endless");
}

QVector<int> availableNValues()
{
    return {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
}

QVector<QColor> rainbowPalette()
{
    return {
        QColor("#ff595e"),
        QColor("#ff924c"),
        QColor("#ffca3a"),
        QColor("#8ac926"),
        QColor("#52b788"),
        QColor("#1982c4"),
        QColor("#6a4c93"),
        QColor("#f15bb5")
    };
}
