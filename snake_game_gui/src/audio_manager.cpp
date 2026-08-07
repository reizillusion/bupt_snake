#include "audio_manager.h"

#include "app_paths.h"

#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSoundEffect>
#include <QUrl>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , m_musicPlayer(new QMediaPlayer(this))
    , m_playlist(new QMediaPlaylist(this))
    , m_eatEffect(new QSoundEffect(this))
    , m_gameOverEffect(new QSoundEffect(this))
{
    m_playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    m_musicPlayer->setPlaylist(m_playlist);

    m_eatEffect->setSource(QUrl::fromLocalFile(AppPaths::assetPath("music/eat.wav")));
    m_gameOverEffect->setSource(QUrl::fromLocalFile(AppPaths::assetPath("music/gameover.wav")));

    applySettings(GameSettings());
}

AudioManager::~AudioManager() = default;

void AudioManager::applySettings(const GameSettings &settings)
{
    m_musicVolume = settings.musicVolume;
    m_sfxVolume = settings.sfxVolume;

    m_musicPlayer->setVolume(m_musicVolume);
    m_eatEffect->setVolume(m_sfxVolume / 100.0);
    m_gameOverEffect->setVolume(m_sfxVolume / 100.0);
}

void AudioManager::playMenuMusic()
{
    playLoopingTrack(AppPaths::assetPath("music/main.mp3"));
}

void AudioManager::playClassicMusic()
{
    playLoopingTrack(AppPaths::assetPath("music/classic.mp3"));
}

void AudioManager::playEndlessMusic()
{
    playLoopingTrack(AppPaths::assetPath("music/ex.mp3"));
}

void AudioManager::stopMusic()
{
    m_musicPlayer->stop();
    m_playlist->clear();
    m_currentTrack.clear();
}

void AudioManager::playEatSound()
{
    if (m_sfxVolume <= 0) {
        return;
    }
    m_eatEffect->play();
}

void AudioManager::playGameOverSound()
{
    if (m_sfxVolume <= 0) {
        return;
    }
    m_gameOverEffect->play();
}

void AudioManager::playLoopingTrack(const QString &filePath)
{
    if (m_currentTrack == filePath && m_musicPlayer->state() == QMediaPlayer::PlayingState) {
        return;
    }

    m_playlist->clear();
    m_playlist->addMedia(QUrl::fromLocalFile(filePath));
    m_playlist->setCurrentIndex(0);
    m_musicPlayer->setVolume(m_musicVolume);
    m_musicPlayer->play();
    m_currentTrack = filePath;
}
