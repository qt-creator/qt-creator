// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QList>

#include <chrono>

namespace Profiler::Internal {

class CtfStatisticsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Role {
        SortRole = Qt::UserRole + 1,
    };

    enum Column {
        Title = 0,
        Count,
        TotalDuration,
        RelativeDuration,
        MinDuration,
        AvgDuration,
        MaxDuration,
        COUNT
    };

    struct EventData {
        int count = 0;
        std::chrono::nanoseconds totalDuration{0};
        std::chrono::nanoseconds minDuration = std::chrono::nanoseconds::max();
        std::chrono::nanoseconds maxDuration{0};
    };

    struct Row {
        QString title;
        EventData data;
    };

    explicit CtfStatisticsModel(QObject *parent);
    ~CtfStatisticsModel() override = default;

    void beginLoading();
    void addEvent(const QString &title, std::chrono::nanoseconds duration);
    void setMeasurementDuration(std::chrono::nanoseconds duration);
    void endLoading();

private:

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QHash<QString, EventData> m_data;
    QList<Row> m_rows;
    std::chrono::nanoseconds m_measurementDuration{0};

};

} // namespace Profiler::Internal
