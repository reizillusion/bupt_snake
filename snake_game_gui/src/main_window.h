#pragma once

#include "common.h"
#include "storage.h"

#include <QMainWindow>

class AudioManager;
class GameScreen;
class LeaderboardScreen;
class MenuScreen;
class SettingsScreen;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void showMenu();
    void showLeaderboard();
    void showSettings();
    void startGame(GameMode mode);
    void chooseNValue();
    void applySettings(const GameSettings &partial, bool preserveSelectedN = true);

    DataStore m_store;
    GameSettings m_settings;
    QVector<ScoreRecord> m_scores;
    AudioManager *m_audioManager = nullptr;
    QStackedWidget *m_stack = nullptr;
    MenuScreen *m_menuScreen = nullptr;
    LeaderboardScreen *m_leaderboardScreen = nullptr;
    SettingsScreen *m_settingsScreen = nullptr;
    GameScreen *m_gameScreen = nullptr;
};
