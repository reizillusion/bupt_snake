#include "menu_screen.h"

#include "block_title_widget.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *createMenuButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(52);
    button->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(22, 34, 52, 220);"
        "  color: white;"
        "  border: 1px solid rgba(255,255,255,40);"
        "  border-radius: 8px;"
        "  font-size: 18px;"
        "  padding: 10px 18px;"
        "}"
        "QPushButton:hover { background-color: rgba(36, 57, 86, 235); }"
        "QPushButton:pressed { background-color: rgba(16, 27, 44, 255); }");
    return button;
}

}

MenuScreen::MenuScreen(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void MenuScreen::setSelectedN(int value)
{
    if (m_nButton) {
        m_nButton->setText(QStringLiteral("选择N值  当前: %1").arg(value));
    }
}

void MenuScreen::mousePressEvent(QMouseEvent *event)
{
    if (!m_hasContinued) {
        m_hasContinued = true;
        setOptionsVisible(true);
        emit continueRequested();
    }
    QWidget::mousePressEvent(event);
}

void MenuScreen::buildUi()
{
    setStyleSheet("background-color: #0b1220;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(48, 36, 48, 36);
    root->setSpacing(24);

    m_titleWidget = new BlockTitleWidget(this);
    m_titleWidget->setTitleText("SNAKE");
    root->addWidget(m_titleWidget, 0, Qt::AlignHCenter);

    m_hintLabel = new QLabel(QStringLiteral("点击屏幕继续"), this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setStyleSheet("color: rgba(255,255,255,210); font-size: 22px; font-weight: 600;");
    root->addWidget(m_hintLabel);

    root->addStretch(1);

    auto *frame = new QFrame(this);
    frame->setStyleSheet("QFrame { background-color: rgba(5,10,18,120); border-radius: 8px; }");
    auto *shadow = new QGraphicsDropShadowEffect(frame);
    shadow->setBlurRadius(28);
    shadow->setColor(QColor(0, 0, 0, 120));
    shadow->setOffset(0, 10);
    frame->setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *classicButton = createMenuButton(QStringLiteral("经典模式"), frame);
    auto *endlessButton = createMenuButton(QStringLiteral("无边界模式"), frame);
    m_nButton = createMenuButton(QStringLiteral("选择N值  当前: 8"), frame);
    auto *leaderboardButton = createMenuButton(QStringLiteral("排行榜"), frame);
    auto *settingsButton = createMenuButton(QStringLiteral("设置"), frame);
    auto *exitButton = createMenuButton(QStringLiteral("退出游戏"), frame);

    connect(classicButton, &QPushButton::clicked, this, &MenuScreen::classicRequested);
    connect(endlessButton, &QPushButton::clicked, this, &MenuScreen::endlessRequested);
    connect(m_nButton, &QPushButton::clicked, this, &MenuScreen::nValueRequested);
    connect(leaderboardButton, &QPushButton::clicked, this, &MenuScreen::leaderboardRequested);
    connect(settingsButton, &QPushButton::clicked, this, &MenuScreen::settingsRequested);
    connect(exitButton, &QPushButton::clicked, this, &MenuScreen::exitRequested);

    layout->addWidget(classicButton);
    layout->addWidget(endlessButton);
    layout->addWidget(m_nButton);
    layout->addWidget(leaderboardButton);
    layout->addWidget(settingsButton);
    layout->addWidget(exitButton);

    m_optionsPanel = frame;
    setOptionsVisible(false);

    root->addWidget(frame, 0, Qt::AlignHCenter);
    root->addStretch(2);
}

void MenuScreen::setOptionsVisible(bool visible)
{
    if (m_optionsPanel) {
        m_optionsPanel->setVisible(visible);
    }
    if (m_hintLabel) {
        m_hintLabel->setText(visible ? QStringLiteral("请选择模式或功能")
                                     : QStringLiteral("点击屏幕继续"));
    }
}
