#include "game_screen.h"

#include "app_paths.h"
#include "audio_manager.h"
#include "block_title_widget.h"

#include <QDateTime>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace {

QPixmap loadPixmapScaled(const QString &path, int size)
{
    QPixmap pixmap(path);
    return pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

}

GameScreen::GameScreen(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent)
    , m_audioManager(audioManager)
    , m_tickTimer(new QTimer(this))
    , m_rng(std::random_device{}())
{
    setFocusPolicy(Qt::StrongFocus);
    buildUi();

    connect(m_tickTimer, &QTimer::timeout, this, &GameScreen::stepGame);
}

void GameScreen::startGame(GameMode mode, const GameSettings &settings)
{
    m_mode = mode;
    m_settings = settings;
    m_tickTimer->setInterval(m_settings.baseStepMs);

    if (m_mode == GameMode::Classic) {
        m_audioManager->playClassicMusic();
    } else {
        m_audioManager->playEndlessMusic();
    }

    resetWorld();
    updateHud();
    m_overlay->setGeometry(rect());
    m_overlay->hide();
    m_running = true;
    m_gameOver = false;
    m_tickTimer->start();
    setFocus(Qt::OtherFocusReason);
    update();
}

void GameScreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF area = playAreaRect();
    painter.fillRect(area, QColor("#0a1324"));

    const int tile = m_settings.tileSize;
    const QPixmap grass = loadPixmapScaled(AppPaths::assetPath("grass.png"), tile);
    const QPixmap wall = loadPixmapScaled(AppPaths::assetPath("wall.png"), tile);
    const QPixmap body = loadPixmapScaled(AppPaths::assetPath("body.png"), tile);
    const QPixmap food = loadPixmapScaled(AppPaths::assetPath("food.png"), tile);
    const QPixmap head = loadPixmapScaled(headSpritePath(), tile);

    const QPointF cameraCenter = m_mode == GameMode::Classic
                                     ? QPointF((m_minX + m_maxX + 1) / 2.0, (m_minY + m_maxY + 1) / 2.0)
                                     : QPointF(m_snake.front().x + 0.5, m_snake.front().y + 0.5);
    const QPointF origin(area.center().x() - cameraCenter.x() * tile,
                         area.center().y() - cameraCenter.y() * tile);

    const int visibleCols = static_cast<int>(area.width() / tile) + 4;
    const int visibleRows = static_cast<int>(area.height() / tile) + 4;
    const int startX = static_cast<int>((area.left() - origin.x()) / tile) - 2;
    const int endX = startX + visibleCols;
    const int startY = static_cast<int>((area.top() - origin.y()) / tile) - 2;
    const int endY = startY + visibleRows;

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            const QRectF rect(origin.x() + x * tile, origin.y() + y * tile, tile, tile);
            if (rect.right() < area.left() || rect.left() > area.right() ||
                rect.bottom() < area.top() || rect.top() > area.bottom()) {
                continue;
            }
            painter.drawPixmap(rect.toRect(), grass);
            if (m_settings.showGrid) {
                painter.setPen(QColor(255, 255, 255, 14));
                painter.drawRect(rect);
            }
        }
    }

    if (m_mode == GameMode::Classic) {
        for (int x = m_minX; x <= m_maxX; ++x) {
            for (int y = m_minY; y <= m_maxY; ++y) {
                if (x != m_minX && x != m_maxX && y != m_minY && y != m_maxY) {
                    continue;
                }
                const QRectF rect(origin.x() + x * tile, origin.y() + y * tile, tile, tile);
                painter.drawPixmap(rect.toRect(), wall);
            }
        }
    }

    for (const Cell &cell : m_obstacles) {
        const QRectF rect(origin.x() + cell.x * tile, origin.y() + cell.y * tile, tile, tile);
        painter.drawPixmap(rect.toRect(), wall);
    }

    painter.drawPixmap(QRectF(origin.x() + m_food.x * tile, origin.y() + m_food.y * tile, tile, tile).toRect(), food);

    for (int i = static_cast<int>(m_snake.size()) - 1; i >= 0; --i) {
        const Cell &cell = m_snake[static_cast<size_t>(i)];
        const QRectF rect(origin.x() + cell.x * tile, origin.y() + cell.y * tile, tile, tile);
        painter.drawPixmap(rect.toRect(), i == 0 ? head : body);
    }

    painter.setPen(QColor(255, 255, 255, 25));
    painter.drawRoundedRect(area.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
}

void GameScreen::keyPressEvent(QKeyEvent *event)
{
    if (m_gameOver) {
        QWidget::keyPressEvent(event);
        return;
    }

    const bool isMovementKey =
        event->key() == Qt::Key_W || event->key() == Qt::Key_A ||
        event->key() == Qt::Key_S || event->key() == Qt::Key_D ||
        event->key() == Qt::Key_Up || event->key() == Qt::Key_Left ||
        event->key() == Qt::Key_Down || event->key() == Qt::Key_Right;
    if (!isMovementKey) {
        QWidget::keyPressEvent(event);
        return;
    }

    const Direction requested = directionFromKey(event->key());
    if (requested == m_direction && !m_hasPendingDirection) {
        event->accept();
        return;
    }

    m_pendingDirection = requested;
    m_hasPendingDirection = true;
    event->accept();
}

void GameScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_overlay) {
        m_overlay->setGeometry(rect());
    }
}

void GameScreen::buildUi()
{
    setStyleSheet("background-color: #08111f; color: white;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(10);

    auto *hud = new QWidget(this);
    auto *hudLayout = new QHBoxLayout(hud);
    hudLayout->setContentsMargins(8, 6, 8, 6);

    m_modeLabel = new QLabel(this);
    m_nLabel = new QLabel(this);
    m_scoreLabel = new QLabel(this);

    const QString chipStyle = "background-color: rgba(22,34,52,220); border-radius: 8px; padding: 8px 12px; font-size: 16px;";
    m_modeLabel->setStyleSheet(chipStyle);
    m_nLabel->setStyleSheet(chipStyle);
    m_scoreLabel->setStyleSheet(chipStyle);

    hudLayout->addWidget(m_modeLabel);
    hudLayout->addWidget(m_nLabel);
    hudLayout->addStretch(1);
    hudLayout->addWidget(m_scoreLabel);
    root->addWidget(hud);
    root->addStretch(1);

    m_overlay = new QWidget(this);
    m_overlay->setStyleSheet("background-color: rgba(2, 6, 12, 120);");
    m_overlay->hide();

    auto *overlayLayout = new QVBoxLayout(m_overlay);
    overlayLayout->setContentsMargins(32, 32, 32, 32);
    overlayLayout->setSpacing(18);
    overlayLayout->addStretch(1);

    m_gameOverTitle = new BlockTitleWidget(m_overlay);
    m_gameOverTitle->setTitleText("GAME OVER");
    overlayLayout->addWidget(m_gameOverTitle, 0, Qt::AlignHCenter);

    m_overlayInfo = new QLabel(m_overlay);
    m_overlayInfo->setAlignment(Qt::AlignCenter);
    m_overlayInfo->setStyleSheet("font-size: 18px; font-weight: 600;");
    overlayLayout->addWidget(m_overlayInfo, 0, Qt::AlignHCenter);

    m_backButton = new QPushButton(QStringLiteral("返回主界面"), m_overlay);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setStyleSheet(
        "QPushButton { background-color: rgba(22,34,52,235); color: white; border-radius: 8px; padding: 12px 18px; }"
        "QPushButton:hover { background-color: rgba(36,57,86,235); }");
    connect(m_backButton, &QPushButton::clicked, this, &GameScreen::backToMenuRequested);
    overlayLayout->addWidget(m_backButton, 0, Qt::AlignHCenter);
    overlayLayout->addStretch(2);

    m_overlay->setGeometry(rect());
}

void GameScreen::resetWorld()
{
    m_snake.clear();
    m_obstacles.clear();
    m_generatedChunks.clear();
    m_score = 0;
    m_moveCount = 0;
    m_direction = Direction::Up;
    m_pendingDirection = Direction::Up;
    m_hasPendingDirection = false;
    m_gameOver = false;

    m_minX = 0;
    m_maxX = 19;
    m_minY = 0;
    m_maxY = 19;

    if (m_mode == GameMode::Classic) {
        m_snake.push_back({10, 11});
        m_snake.push_back({10, 12});
        m_snake.push_back({10, 13});

        std::uniform_int_distribution<int> distX(1, 18);
        std::uniform_int_distribution<int> distY(1, 18);
        while (static_cast<int>(m_obstacles.size()) < 10) {
            const Cell cell{distX(m_rng), distY(m_rng)};
            if (!isOccupied(cell) && cell != Cell{10, 10}) {
                m_obstacles.insert(cell);
            }
        }
    } else {
        m_snake.push_back({0, 0});
        m_snake.push_back({0, 1});
        m_snake.push_back({0, 2});
        expandEndlessWorld(m_snake.front());
    }

    spawnFood();
}

void GameScreen::updateHud()
{
    m_modeLabel->setText(modeDisplayName(m_mode));
    m_nLabel->setText(QStringLiteral("N = %1").arg(m_settings.selectedN));
    m_scoreLabel->setText(QStringLiteral("分数 %1   长度 %2").arg(m_score).arg(m_snake.size()));
}

void GameScreen::stepGame()
{
    if (!m_running || m_gameOver) {
        return;
    }

    const GameResult result = advanceOneStep();
    if (result.ateFood) {
        m_audioManager->playEatSound();
    }
    if (result.gameOver) {
        setGameOver();
    }

    updateHud();
    update();
}

GameScreen::GameResult GameScreen::advanceOneStep()
{
    GameResult result;

    Direction nextDirection = m_direction;
    if (m_hasPendingDirection) {
        nextDirection = m_pendingDirection;
        m_hasPendingDirection = false;
        if (isOpposite(nextDirection, m_direction)) {
            result.gameOver = true;
            return result;
        }
    }
    m_direction = nextDirection;

    const Cell previousTail = m_snake.back();
    Cell nextHead = movedCell(m_snake.front(), m_direction);

    if (m_mode == GameMode::Classic) {
        if (nextHead.x <= m_minX || nextHead.x >= m_maxX || nextHead.y <= m_minY || nextHead.y >= m_maxY) {
            result.gameOver = true;
            return result;
        }
    } else {
        ensureEndlessWorldAroundHead();
    }

    const bool ateFood = nextHead == m_food;
    const bool grewByN = ((m_moveCount + 1) % m_settings.selectedN == 0);
    const bool shouldGrow = ateFood || grewByN;

    if (isWall(nextHead) || isOccupied(nextHead, !shouldGrow)) {
        result.gameOver = true;
        return result;
    }

    m_snake.push_front(nextHead);
    if (!shouldGrow) {
        m_snake.pop_back();
    }

    ++m_moveCount;
    result.ateFood = ateFood;
    result.grewByN = grewByN && !ateFood;

    if (ateFood) {
        m_score += 10;
        if (m_mode == GameMode::Endless) {
            expandEndlessWorld(nextHead);
        }
        spawnFood();
    } else if (m_mode == GameMode::Endless && grewByN) {
        expandEndlessWorld(nextHead);
    }

    Q_UNUSED(previousTail);
    return result;
}

GameScreen::Cell GameScreen::movedCell(const Cell &cell, Direction direction) const
{
    Cell moved = cell;
    switch (direction) {
    case Direction::Up: --moved.y; break;
    case Direction::Down: ++moved.y; break;
    case Direction::Left: --moved.x; break;
    case Direction::Right: ++moved.x; break;
    }
    return moved;
}

GameScreen::Direction GameScreen::directionFromKey(int key) const
{
    switch (key) {
    case Qt::Key_W:
    case Qt::Key_Up:
        return Direction::Up;
    case Qt::Key_S:
    case Qt::Key_Down:
        return Direction::Down;
    case Qt::Key_A:
    case Qt::Key_Left:
        return Direction::Left;
    case Qt::Key_D:
    case Qt::Key_Right:
        return Direction::Right;
    default:
        return m_direction;
    }
}

bool GameScreen::isOpposite(Direction next, Direction current) const
{
    return (next == Direction::Up && current == Direction::Down) ||
           (next == Direction::Down && current == Direction::Up) ||
           (next == Direction::Left && current == Direction::Right) ||
           (next == Direction::Right && current == Direction::Left);
}

bool GameScreen::isOccupied(const Cell &cell, bool includeTail) const
{
    const int limit = includeTail ? static_cast<int>(m_snake.size()) : static_cast<int>(m_snake.size()) - 1;
    for (int i = 0; i < limit; ++i) {
        if (m_snake[static_cast<size_t>(i)] == cell) {
            return true;
        }
    }
    return m_obstacles.count(cell) > 0;
}

bool GameScreen::isWall(const Cell &cell) const
{
    if (m_mode != GameMode::Classic) {
        return false;
    }
    return cell.x <= m_minX || cell.x >= m_maxX || cell.y <= m_minY || cell.y >= m_maxY;
}

void GameScreen::spawnFood()
{
    if (m_mode == GameMode::Classic) {
        std::uniform_int_distribution<int> distX(m_minX + 1, m_maxX - 1);
        std::uniform_int_distribution<int> distY(m_minY + 1, m_maxY - 1);
        while (true) {
            const Cell cell{distX(m_rng), distY(m_rng)};
            if (!isOccupied(cell) && !isDeadFoodPosition(cell)) {
                m_food = cell;
                return;
            }
        }
    }

    ensureEndlessWorldAroundHead();
    std::uniform_int_distribution<int> distX(m_minX, m_maxX);
    std::uniform_int_distribution<int> distY(m_minY, m_maxY);
    while (true) {
        const Cell cell{distX(m_rng), distY(m_rng)};
        if (!isOccupied(cell) && !isDeadFoodPosition(cell)) {
            m_food = cell;
            return;
        }
    }
}

bool GameScreen::isDeadFoodPosition(const Cell &cell) const
{
    int blocked = 0;
    const std::array<Cell, 4> neighbors = {
        Cell{cell.x + 1, cell.y},
        Cell{cell.x - 1, cell.y},
        Cell{cell.x, cell.y + 1},
        Cell{cell.x, cell.y - 1}
    };

    for (const Cell &neighbor : neighbors) {
        if (isWall(neighbor) || m_obstacles.count(neighbor) > 0) {
            ++blocked;
        }
    }
    return blocked >= 3;
}

void GameScreen::ensureEndlessWorldAroundHead()
{
    expandEndlessWorld(m_snake.front());
}

void GameScreen::expandEndlessWorld(const Cell &head)
{
    const int chunkSize = 10;
    const int centerChunkX = static_cast<int>(std::floor(head.x / static_cast<double>(chunkSize)));
    const int centerChunkY = static_cast<int>(std::floor(head.y / static_cast<double>(chunkSize)));

    std::uniform_real_distribution<double> chance(0.0, 1.0);
    std::uniform_int_distribution<int> offset(0, chunkSize - 1);

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            const Cell chunk{centerChunkX + dx, centerChunkY + dy};
            if (!m_generatedChunks.insert(chunk).second) {
                continue;
            }

            const int chunkStartX = chunk.x * chunkSize;
            const int chunkStartY = chunk.y * chunkSize;
            m_minX = std::min(m_minX, chunkStartX);
            m_minY = std::min(m_minY, chunkStartY);
            m_maxX = std::max(m_maxX, chunkStartX + chunkSize - 1);
            m_maxY = std::max(m_maxY, chunkStartY + chunkSize - 1);

            const int areaFactor = std::max(std::abs(chunk.x), std::abs(chunk.y));
            const int attempts = 2 + areaFactor * 2;
            const double spawnRate = std::min(0.60, 0.18 + areaFactor * 0.05);

            for (int i = 0; i < attempts; ++i) {
                if (chance(m_rng) > spawnRate) {
                    continue;
                }
                const Cell obstacle{chunkStartX + offset(m_rng), chunkStartY + offset(m_rng)};
                if (std::abs(obstacle.x - head.x) <= 2 && std::abs(obstacle.y - head.y) <= 2) {
                    continue;
                }
                if (!isOccupied(obstacle)) {
                    m_obstacles.insert(obstacle);
                }
            }
        }
    }
}

void GameScreen::setGameOver()
{
    m_running = false;
    m_gameOver = true;
    m_tickTimer->stop();
    m_audioManager->playGameOverSound();

    const QString playerName = QInputDialog::getText(
        this,
        QStringLiteral("保存成绩"),
        QStringLiteral("请输入玩家姓名："),
        QLineEdit::Normal,
        m_settings.lastPlayerName);

    ScoreRecord record;
    record.name = playerName.trimmed().isEmpty() ? QStringLiteral("Player") : playerName.trimmed();
    record.score = m_score;
    record.length = static_cast<int>(m_snake.size());
    record.mode = m_mode;
    record.nValue = m_settings.selectedN;
    record.finishedAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    emit saveRecordRequested(record);

    m_overlayInfo->setText(QStringLiteral("最终得分 %1  长度 %2").arg(m_score).arg(m_snake.size()));
    m_overlay->show();
}

QString GameScreen::headSpritePath() const
{
    switch (m_direction) {
    case Direction::Up:
        return AppPaths::assetPath("head/up.png");
    case Direction::Down:
        return AppPaths::assetPath("head/down.png");
    case Direction::Left:
        return AppPaths::assetPath("head/left.png");
    case Direction::Right:
        return AppPaths::assetPath("head/right.png");
    }
    return AppPaths::assetPath("head/up.png");
}

QRectF GameScreen::playAreaRect() const
{
    const int topInset = 82;
    return QRectF(18.0, static_cast<qreal>(topInset), width() - 36.0, height() - topInset - 18.0);
}

QPointF GameScreen::worldToScreen(const Cell &cell, const QPointF &origin) const
{
    return QPointF(origin.x() + cell.x * m_settings.tileSize,
                   origin.y() + cell.y * m_settings.tileSize);
}
