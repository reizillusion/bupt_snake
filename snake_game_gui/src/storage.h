#pragma once

#include "common.h"

#include <QVector>

class DataStore
{
public:
    GameSettings loadSettings() const;
    void saveSettings(const GameSettings &settings) const;

    QVector<ScoreRecord> loadScores() const;
    void saveScores(const QVector<ScoreRecord> &scores) const;
    void addScore(const ScoreRecord &record) const;
};
