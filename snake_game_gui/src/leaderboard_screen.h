#pragma once

#include "common.h"

#include <QWidget>

class QComboBox;
class QTableWidget;

class LeaderboardScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LeaderboardScreen(QWidget *parent = nullptr);

    void setRecords(const QVector<ScoreRecord> &records);

signals:
    void backRequested();

private:
    void rebuildTable();

    QVector<ScoreRecord> m_records;
    QComboBox *m_filterBox = nullptr;
    QTableWidget *m_table = nullptr;
};
