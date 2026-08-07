#pragma once

#include "common.h"

#include <QWidget>
#include <array>
#include <deque>
#include <random>
#include <set>

class BlockTitleWidget;
class QLabel;
class QPushButton;
class QTimer;

class AudioManager;

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(AudioManager *audioManager, QWidget *parent = nullptr);

    void startGame(GameMode mode, const GameSettings &settings);

signals:
    void saveRecordRequested(const ScoreRecord &record);
    void backToMenuRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Cell {
        int x = 0;
        int y = 0;
        bool operator==(const Cell &other) const { return x == other.x && y == other.y; }
        bool operator!=(const Cell &other) const { return !(*this == other); }
        bool operator<(const Cell &other) const
        {
            return x == other.x ? y < other.y : x < other.x;
        }
    };

    enum class Direction {
        Up,
        Down,
        Left,
        Right
    };

    struct GameResult {
        bool ateFood = false;
        bool grewByN = false;
        bool gameOver = false;
    };

    void buildUi();
    void resetWorld();
    void updateHud();
    void stepGame();
    GameResult advanceOneStep();
    Cell movedCell(const Cell &cell, Direction direction) const;
    Direction directionFromKey(int key) const;
    bool isOpposite(Direction next, Direction current) const;
    bool isOccupied(const Cell &cell, bool includeTail = true) const;
    bool isWall(const Cell &cell) const;
    void spawnFood();
    bool isDeadFoodPosition(const Cell &cell) const;
    void ensureEndlessWorldAroundHead();
    void expandEndlessWorld(const Cell &head);
    void setGameOver();
    QString headSpritePath() const;
    QRectF playAreaRect() const;
    QPointF worldToScreen(const Cell &cell, const QPointF &origin) const;

    AudioManager *m_audioManager = nullptr;
    GameMode m_mode = GameMode::Classic;
    GameSettings m_settings;

    QLabel *m_scoreLabel = nullptr;
    QLabel *m_modeLabel = nullptr;
    QLabel *m_nLabel = nullptr;
    QWidget *m_overlay = nullptr;
    BlockTitleWidget *m_gameOverTitle = nullptr;
    QLabel *m_overlayInfo = nullptr;
    QPushButton *m_backButton = nullptr;
    QTimer *m_tickTimer = nullptr;

    std::deque<Cell> m_snake;
    std::set<Cell> m_obstacles;
    std::set<Cell> m_generatedChunks;
    Cell m_food;
    Direction m_direction = Direction::Up;
    Direction m_pendingDirection = Direction::Up;
    bool m_hasPendingDirection = false;
    bool m_running = false;
    bool m_gameOver = false;
    int m_score = 0;
    int m_moveCount = 0;
    int m_minX = 0;
    int m_maxX = 19;
    int m_minY = 0;
    int m_maxY = 19;
    std::mt19937 m_rng;
};
