#pragma once

#include "common.h"

#include <QWidget>

class BlockTitleWidget;
class QLabel;
class QPushButton;

class MenuScreen : public QWidget
{
    Q_OBJECT

public:
    explicit MenuScreen(QWidget *parent = nullptr);

    void setSelectedN(int value);

signals:
    void continueRequested();
    void classicRequested();
    void endlessRequested();
    void leaderboardRequested();
    void settingsRequested();
    void exitRequested();
    void nValueRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void buildUi();
    void setOptionsVisible(bool visible);

    bool m_hasContinued = false;
    BlockTitleWidget *m_titleWidget = nullptr;
    QLabel *m_hintLabel = nullptr;
    QWidget *m_optionsPanel = nullptr;
    QPushButton *m_nButton = nullptr;
};
