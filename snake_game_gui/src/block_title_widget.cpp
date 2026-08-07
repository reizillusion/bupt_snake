#include "block_title_widget.h"

#include "common.h"

#include <QPainter>
#include <QTimer>
#include <QVector>
#include <algorithm>

BlockTitleWidget::BlockTitleWidget(QWidget *parent)
    : QWidget(parent)
    , m_animTimer(new QTimer(this))
{
    m_animTimer->setInterval(90);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_phase = (m_phase + 1) % 1024;
        update();
    });
    m_animTimer->start();
}

void BlockTitleWidget::setTitleText(const QString &text)
{
    m_text = text.toUpper();
    updateGeometry();
    update();
}

QSize BlockTitleWidget::minimumSizeHint() const
{
    return QSize(460, 110);
}

void BlockTitleWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    static const int rows = 7;
    const auto palette = rainbowPalette();

    QVector<QStringList> glyphs;
    glyphs.reserve(m_text.size());
    int totalColumns = 0;
    for (const QChar character : m_text) {
        const QStringList glyph = patternForCharacter(character);
        glyphs.push_back(glyph);
        totalColumns += glyph.isEmpty() ? 3 : glyph.first().size();
        totalColumns += 1;
    }
    totalColumns = std::max(1, totalColumns - 1);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), Qt::transparent);

    const QRectF area = rect().adjusted(4, 4, -4, -4);
    const qreal cellSize = std::min(area.width() / totalColumns, area.height() / rows);
    const qreal gridWidth = cellSize * totalColumns;
    const qreal gridHeight = cellSize * rows;
    const qreal originX = area.center().x() - (gridWidth / 2.0);
    const qreal originY = area.center().y() - (gridHeight / 2.0);

    int xOffset = 0;
    int colorIndex = 0;
    for (const QStringList &glyph : glyphs) {
        const int glyphWidth = glyph.isEmpty() ? 3 : glyph.first().size();
        for (int row = 0; row < glyph.size(); ++row) {
            for (int col = 0; col < glyph[row].size(); ++col) {
                if (glyph[row][col] != QLatin1Char('1')) {
                    continue;
                }

                const QRectF cell(originX + (xOffset + col) * cellSize,
                                  originY + row * cellSize,
                                  cellSize - 2.0,
                                  cellSize - 2.0);

                const QColor base = palette[(colorIndex + row + col + (m_phase / 4)) % palette.size()];
                QLinearGradient gradient(cell.topLeft(), cell.bottomRight());
                gradient.setColorAt(0.0, base.lighter(140));
                gradient.setColorAt(1.0, base.darker(110));

                painter.setPen(Qt::NoPen);
                painter.setBrush(gradient);
                painter.drawRoundedRect(cell, 4.0, 4.0);
            }
        }
        xOffset += glyphWidth + 1;
        ++colorIndex;
    }
}

QStringList BlockTitleWidget::patternForCharacter(QChar character) const
{
    switch (character.toLatin1()) {
    case 'A':
        return {"01110","10001","10001","11111","10001","10001","10001"};
    case 'E':
        return {"11111","10000","10000","11110","10000","10000","11111"};
    case 'G':
        return {"01111","10000","10000","10011","10001","10001","01111"};
    case 'K':
        return {"10001","10010","10100","11000","10100","10010","10001"};
    case 'M':
        return {"10001","11011","10101","10101","10001","10001","10001"};
    case 'N':
        return {"10001","11001","10101","10011","10001","10001","10001"};
    case 'O':
        return {"01110","10001","10001","10001","10001","10001","01110"};
    case 'R':
        return {"11110","10001","10001","11110","10100","10010","10001"};
    case 'S':
        return {"01111","10000","10000","01110","00001","00001","11110"};
    case 'V':
        return {"10001","10001","10001","10001","01010","01010","00100"};
    case ' ':
        return {"000","000","000","000","000","000","000"};
    default:
        return {"11111","00100","00100","00100","00100","00000","00100"};
    }
}
