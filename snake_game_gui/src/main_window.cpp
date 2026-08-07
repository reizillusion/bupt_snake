#include "main_window.h"

#include "audio_manager.h"
#include "game_screen.h"
#include "leaderboard_screen.h"
#include "menu_screen.h"
#include "settings_screen.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(m_store.loadSettings())
    , m_scores(m_store.loadScores())
    , m_audioManager(new AudioManager(this))
{
    buildUi();
    applySettings(m_settings, false);
    showMenu();
}

void MainWindow::buildUi()
{
    resize(1180, 820);
    setMinimumSize(980, 700);
    setWindowTitle(QStringLiteral("Snake"));

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_menuScreen = new MenuScreen(this);
    m_leaderboardScreen = new LeaderboardScreen(this);
    m_settingsScreen = new SettingsScreen(this);
    m_gameScreen = new GameScreen(m_audioManager, this);

    m_stack->addWidget(m_menuScreen);
    m_stack->addWidget(m_leaderboardScreen);
    m_stack->addWidget(m_settingsScreen);
    m_stack->addWidget(m_gameScreen);

    connect(m_menuScreen, &MenuScreen::continueRequested, this, [this]() {
        m_audioManager->playMenuMusic();
    });
    connect(m_menuScreen, &MenuScreen::classicRequested, this, [this]() {
        startGame(GameMode::Classic);
    });
    connect(m_menuScreen, &MenuScreen::endlessRequested, this, [this]() {
        startGame(GameMode::Endless);
    });
    connect(m_menuScreen, &MenuScreen::leaderboardRequested, this, &MainWindow::showLeaderboard);
    connect(m_menuScreen, &MenuScreen::settingsRequested, this, &MainWindow::showSettings);
    connect(m_menuScreen, &MenuScreen::nValueRequested, this, &MainWindow::chooseNValue);
    connect(m_menuScreen, &MenuScreen::exitRequested, this, &QWidget::close);

    connect(m_leaderboardScreen, &LeaderboardScreen::backRequested, this, &MainWindow::showMenu);
    connect(m_settingsScreen, &SettingsScreen::backRequested, this, &MainWindow::showMenu);
    connect(m_settingsScreen, &SettingsScreen::settingsSaved, this, [this](const GameSettings &settings) {
        applySettings(settings, true);
        QMessageBox::information(this, QStringLiteral("设置"), QStringLiteral("设置已保存。"));
        showMenu();
    });

    connect(m_gameScreen, &GameScreen::saveRecordRequested, this, [this](const ScoreRecord &record) {
        m_scores.push_back(record);
        std::sort(m_scores.begin(), m_scores.end(), [](const ScoreRecord &left, const ScoreRecord &right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            if (left.length != right.length) {
                return left.length > right.length;
            }
            return left.finishedAt > right.finishedAt;
        });
        m_store.addScore(record);
        m_scores = m_store.loadScores();
        m_leaderboardScreen->setRecords(m_scores);
    });
    connect(m_gameScreen, &GameScreen::backToMenuRequested, this, &MainWindow::showMenu);
}

void MainWindow::showMenu()
{
    m_menuScreen->setSelectedN(m_settings.selectedN);
    m_stack->setCurrentWidget(m_menuScreen);
    m_audioManager->playMenuMusic();
}

void MainWindow::showLeaderboard()
{
    m_leaderboardScreen->setRecords(m_scores);
    m_stack->setCurrentWidget(m_leaderboardScreen);
    m_audioManager->playMenuMusic();
}

void MainWindow::showSettings()
{
    m_settingsScreen->setSettings(m_settings);
    m_stack->setCurrentWidget(m_settingsScreen);
    m_audioManager->playMenuMusic();
}

void MainWindow::startGame(GameMode mode)
{
    m_stack->setCurrentWidget(m_gameScreen);
    m_gameScreen->startGame(mode, m_settings);
}

void MainWindow::chooseNValue()
{
    QStringList items;
    const auto values = availableNValues();
    int currentIndex = 0;
    for (int i = 0; i < values.size(); ++i) {
        items.push_back(QString::number(values[i]));
        if (values[i] == m_settings.selectedN) {
            currentIndex = i;
        }
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this,
        QStringLiteral("选择N值"),
        QStringLiteral("请选择蛇每隔 N 步自动增长一次："),
        items,
        currentIndex,
        false,
        &ok);
    if (!ok || selected.isEmpty()) {
        return;
    }

    m_settings.selectedN = selected.toInt();
    m_store.saveSettings(m_settings);
    m_menuScreen->setSelectedN(m_settings.selectedN);
}

void MainWindow::applySettings(const GameSettings &partial, bool preserveSelectedN)
{
    const int selectedN = m_settings.selectedN;
    m_settings = partial;
    if (preserveSelectedN) {
        m_settings.selectedN = selectedN;
    }
    m_store.saveSettings(m_settings);
    m_audioManager->applySettings(m_settings);
    m_menuScreen->setSelectedN(m_settings.selectedN);
    m_settingsScreen->setSettings(m_settings);
}
