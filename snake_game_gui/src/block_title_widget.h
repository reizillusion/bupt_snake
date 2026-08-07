#pragma once

#include <QWidget>

class QTimer;

class BlockTitleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BlockTitleWidget(QWidget *parent = nullptr);

    void setTitleText(const QString &text);
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList patternForCharacter(QChar character) const;
    QString m_text = "SNAKE";
    int m_phase = 0;
    QTimer *m_animTimer = nullptr;
};
