#pragma once

#include "common.h"

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QSlider;
class QSpinBox;

class SettingsScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsScreen(QWidget *parent = nullptr);

    void setSettings(const GameSettings &settings);

signals:
    void settingsSaved(const GameSettings &settings);
    void backRequested();

private:
    GameSettings gatherSettings() const;

    QSlider *m_musicSlider = nullptr;
    QSlider *m_sfxSlider = nullptr;
    QSpinBox *m_speedSpin = nullptr;
    QSpinBox *m_tileSpin = nullptr;
    QCheckBox *m_gridCheck = nullptr;
    QLineEdit *m_nameEdit = nullptr;
};
