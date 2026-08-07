#pragma once

#include <QColor>
#include <QDateTime>
#include <QString>
#include <QVector>

enum class GameMode {
    Classic,
    Endless
};

struct GameSettings {
    int selectedN = 8;
    int musicVolume = 60;
    int sfxVolume = 80;
    int baseStepMs = 160;
    int tileSize = 32;
    bool showGrid = false;
    QString lastPlayerName = "Player";
};

struct ScoreRecord {
    QString name;
    int score = 0;
    int length = 0;
    GameMode mode = GameMode::Classic;
    int nValue = 8;
    QString finishedAt;
};

QString modeDisplayName(GameMode mode);
QString modeKey(GameMode mode);
QVector<int> availableNValues();
QVector<QColor> rainbowPalette();
