#include "storage.h"

#include "app_paths.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace {

QString settingsFilePath()
{
    return AppPaths::dataPath("settings.json");
}

QString scoresFilePath()
{
    return AppPaths::dataPath("leaderboard.json");
}

QJsonObject scoreToJson(const ScoreRecord &record)
{
    QJsonObject object;
    object["name"] = record.name;
    object["score"] = record.score;
    object["length"] = record.length;
    object["mode"] = modeKey(record.mode);
    object["nValue"] = record.nValue;
    object["finishedAt"] = record.finishedAt;
    return object;
}

ScoreRecord scoreFromJson(const QJsonObject &object)
{
    ScoreRecord record;
    record.name = object.value("name").toString("Player");
    record.score = object.value("score").toInt();
    record.length = object.value("length").toInt();
    record.mode = object.value("mode").toString() == "endless" ? GameMode::Endless
                                                                : GameMode::Classic;
    record.nValue = object.value("nValue").toInt(8);
    record.finishedAt = object.value("finishedAt").toString();
    return record;
}

bool betterScore(const ScoreRecord &left, const ScoreRecord &right)
{
    if (left.score != right.score) {
        return left.score > right.score;
    }
    if (left.length != right.length) {
        return left.length > right.length;
    }
    return left.finishedAt > right.finishedAt;
}

}

GameSettings DataStore::loadSettings() const
{
    AppPaths::ensureDataDirectories();

    QFile file(settingsFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return GameSettings();
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject object = document.object();

    GameSettings settings;
    settings.selectedN = object.value("selectedN").toInt(settings.selectedN);
    settings.musicVolume = object.value("musicVolume").toInt(settings.musicVolume);
    settings.sfxVolume = object.value("sfxVolume").toInt(settings.sfxVolume);
    settings.baseStepMs = object.value("baseStepMs").toInt(settings.baseStepMs);
    settings.tileSize = object.value("tileSize").toInt(settings.tileSize);
    settings.showGrid = object.value("showGrid").toBool(settings.showGrid);
    settings.lastPlayerName = object.value("lastPlayerName").toString(settings.lastPlayerName);
    return settings;
}

void DataStore::saveSettings(const GameSettings &settings) const
{
    AppPaths::ensureDataDirectories();

    QJsonObject object;
    object["selectedN"] = settings.selectedN;
    object["musicVolume"] = settings.musicVolume;
    object["sfxVolume"] = settings.sfxVolume;
    object["baseStepMs"] = settings.baseStepMs;
    object["tileSize"] = settings.tileSize;
    object["showGrid"] = settings.showGrid;
    object["lastPlayerName"] = settings.lastPlayerName;

    QFile file(settingsFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    }
}

QVector<ScoreRecord> DataStore::loadScores() const
{
    AppPaths::ensureDataDirectories();

    QFile file(scoresFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray array = document.array();

    QVector<ScoreRecord> scores;
    scores.reserve(array.size());
    for (const auto &item : array) {
        scores.push_back(scoreFromJson(item.toObject()));
    }

    std::sort(scores.begin(), scores.end(), betterScore);
    return scores;
}

void DataStore::saveScores(const QVector<ScoreRecord> &scores) const
{
    AppPaths::ensureDataDirectories();

    QJsonArray array;
    for (const auto &record : scores) {
        array.push_back(scoreToJson(record));
    }

    QFile file(scoresFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void DataStore::addScore(const ScoreRecord &record) const
{
    QVector<ScoreRecord> scores = loadScores();
    scores.push_back(record);
    std::sort(scores.begin(), scores.end(), betterScore);
    saveScores(scores);
}
