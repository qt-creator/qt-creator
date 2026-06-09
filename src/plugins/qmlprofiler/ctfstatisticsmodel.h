// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractTableModel>
#include <QHash>

namespace CtfVisualizer::Internal {

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
        qint64 totalDuration = 0.0;
        qint64 minDuration = std::numeric_limits<qint64>::max();
        qint64 maxDuration = 0.0;
    };

    explicit CtfStatisticsModel(QObject *parent);
    ~CtfStatisticsModel() override = default;

    void beginLoading();
    void addEvent(const QString &title, qint64 durationInNs);
    void setMeasurementDuration(qint64 timeInNs);
    void endLoading();

private:

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QHash<QString, EventData> m_data;
    qint64 m_measurementDurationInNs = 0;

};

}  // CtfVisualizer::Internal
