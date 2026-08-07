#include "settings_screen.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QSlider *makeSlider(QWidget *parent)
{
    auto *slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, 100);
    return slider;
}

}

SettingsScreen::SettingsScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        "QWidget { background-color: #0b1220; color: white; }"
        "QLineEdit, QSpinBox, QSlider, QPushButton {"
        "  background-color: rgba(22,34,52,220);"
        "  border: 1px solid rgba(255,255,255,40);"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "}"
        "QCheckBox { spacing: 10px; }");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 28, 36, 28);
    layout->setSpacing(18);

    auto *title = new QLabel(QStringLiteral("设置"), this);
    title->setStyleSheet("font-size: 28px; font-weight: 700;");
    layout->addWidget(title);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(18);

    m_musicSlider = makeSlider(this);
    m_sfxSlider = makeSlider(this);

    m_speedSpin = new QSpinBox(this);
    m_speedSpin->setRange(60, 320);
    m_speedSpin->setSingleStep(10);
    m_speedSpin->setSuffix(" ms");

    m_tileSpin = new QSpinBox(this);
    m_tileSpin->setRange(24, 48);
    m_tileSpin->setSingleStep(4);
    m_tileSpin->setSuffix(" px");

    m_gridCheck = new QCheckBox(QStringLiteral("显示辅助网格"), this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setMaxLength(16);

    form->addRow(QStringLiteral("背景音乐音量"), m_musicSlider);
    form->addRow(QStringLiteral("音效音量"), m_sfxSlider);
    form->addRow(QStringLiteral("基础步进速度"), m_speedSpin);
    form->addRow(QStringLiteral("格子尺寸"), m_tileSpin);
    form->addRow(QStringLiteral("默认玩家名"), m_nameEdit);
    form->addRow(QStringLiteral(""), m_gridCheck);

    layout->addLayout(form);
    layout->addStretch(1);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch(1);

    auto *backButton = new QPushButton(QStringLiteral("返回主界面"), this);
    auto *saveButton = new QPushButton(QStringLiteral("保存设置"), this);
    connect(backButton, &QPushButton::clicked, this, &SettingsScreen::backRequested);
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        emit settingsSaved(gatherSettings());
    });

    buttons->addWidget(backButton);
    buttons->addWidget(saveButton);
    layout->addLayout(buttons);
}

void SettingsScreen::setSettings(const GameSettings &settings)
{
    m_musicSlider->setValue(settings.musicVolume);
    m_sfxSlider->setValue(settings.sfxVolume);
    m_speedSpin->setValue(settings.baseStepMs);
    m_tileSpin->setValue(settings.tileSize);
    m_gridCheck->setChecked(settings.showGrid);
    m_nameEdit->setText(settings.lastPlayerName);
}

GameSettings SettingsScreen::gatherSettings() const
{
    GameSettings settings;
    settings.musicVolume = m_musicSlider->value();
    settings.sfxVolume = m_sfxSlider->value();
    settings.baseStepMs = m_speedSpin->value();
    settings.tileSize = m_tileSpin->value();
    settings.showGrid = m_gridCheck->isChecked();
    settings.lastPlayerName = m_nameEdit->text().trimmed().isEmpty()
                                  ? QStringLiteral("Player")
                                  : m_nameEdit->text().trimmed();
    return settings;
}
