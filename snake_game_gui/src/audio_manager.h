#pragma once

#include "common.h"

#include <QObject>

class QMediaPlayer;
class QMediaPlaylist;
class QSoundEffect;

class AudioManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager() override;

    void applySettings(const GameSettings &settings);

    void playMenuMusic();
    void playClassicMusic();
    void playEndlessMusic();
    void stopMusic();

    void playEatSound();
    void playGameOverSound();

private:
    void playLoopingTrack(const QString &filePath);

    int m_musicVolume = 60;
    int m_sfxVolume = 80;
    QString m_currentTrack;
    QMediaPlayer *m_musicPlayer = nullptr;
    QMediaPlaylist *m_playlist = nullptr;
    QSoundEffect *m_eatEffect = nullptr;
    QSoundEffect *m_gameOverEffect = nullptr;
};
