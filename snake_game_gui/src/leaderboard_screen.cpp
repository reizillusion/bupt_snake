#include "leaderboard_screen.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

LeaderboardScreen::LeaderboardScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        "QWidget { background-color: #0b1220; color: white; }"
        "QComboBox, QPushButton {"
        "  background-color: rgba(22,34,52,220);"
        "  border: 1px solid rgba(255,255,255,40);"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  min-height: 18px;"
        "}"
        "QTableWidget {"
        "  background-color: rgba(10,18,32,220);"
        "  border: 1px solid rgba(255,255,255,40);"
        "  border-radius: 8px;"
        "  gridline-color: rgba(255,255,255,25);"
        "}"
        "QHeaderView::section {"
        "  background-color: rgba(22,34,52,255);"
        "  color: white;"
        "  border: none;"
        "  padding: 8px;"
        "}");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 28, 36, 28);
    layout->setSpacing(18);

    auto *title = new QLabel(QStringLiteral("排行榜"), this);
    title->setStyleSheet("font-size: 28px; font-weight: 700;");
    layout->addWidget(title);

    m_filterBox = new QComboBox(this);
    m_filterBox->addItem(QStringLiteral("全部模式"), "");
    m_filterBox->addItem(QStringLiteral("经典模式"), "classic");
    m_filterBox->addItem(QStringLiteral("无边界模式"), "endless");
    connect(m_filterBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        rebuildTable();
    });
    layout->addWidget(m_filterBox, 0, Qt::AlignLeft);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("排名"),
        QStringLiteral("玩家"),
        QStringLiteral("分数"),
        QStringLiteral("长度"),
        QStringLiteral("模式 / N"),
        QStringLiteral("结束时间")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table, 1);

    auto *backButton = new QPushButton(QStringLiteral("返回主界面"), this);
    connect(backButton, &QPushButton::clicked, this, &LeaderboardScreen::backRequested);
    layout->addWidget(backButton, 0, Qt::AlignRight);
}

void LeaderboardScreen::setRecords(const QVector<ScoreRecord> &records)
{
    m_records = records;
    rebuildTable();
}

void LeaderboardScreen::rebuildTable()
{
    const QString filter = m_filterBox->currentData().toString();

    QVector<ScoreRecord> filtered;
    filtered.reserve(m_records.size());
    for (const auto &record : m_records) {
        if (filter.isEmpty() || modeKey(record.mode) == filter) {
            filtered.push_back(record);
        }
    }

    m_table->setRowCount(filtered.size());
    for (int row = 0; row < filtered.size(); ++row) {
        const auto &record = filtered[row];
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_table->setItem(row, 1, new QTableWidgetItem(record.name));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(record.score)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(record.length)));
        m_table->setItem(row, 4, new QTableWidgetItem(QStringLiteral("%1 / %2")
                                                           .arg(modeDisplayName(record.mode))
                                                           .arg(record.nValue)));
        m_table->setItem(row, 5, new QTableWidgetItem(record.finishedAt));
    }
}
