// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctfstatisticsmodel.h"

#include "ctfvisualizertr.h"
#include "json/json.hpp"

#include <tracing/timelineformattime.h>

namespace CtfVisualizer::Internal {

using json = nlohmann::json;

CtfStatisticsModel::CtfStatisticsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void CtfStatisticsModel::beginLoading()
{
    beginResetModel();
    m_data.clear();
}

void CtfStatisticsModel::addEvent(const QString &title, qint64 durationInNs)
{
    EventData &data = m_data[title];
    ++data.count;
    if (durationInNs >= 0) {
        data.totalDuration += durationInNs;
        data.minDuration = std::min(data.minDuration, durationInNs);
        data.maxDuration = std::max(data.maxDuration, durationInNs);
    }
}

void CtfStatisticsModel::setMeasurementDuration(qint64 timeInNs)
{
    m_measurementDurationInNs = timeInNs;
}

void CtfStatisticsModel::endLoading()
{
    endResetModel();
}

int CtfStatisticsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int CtfStatisticsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : Column::COUNT;
}

QVariant CtfStatisticsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto it = m_data.cbegin();
    std::advance(it, index.row());
    const QString &title = it.key();

    switch (role) {
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case Column::Title:
            return Qt::AlignLeft;
        case Column::Count:
        case Column::TotalDuration:
        case Column::RelativeDuration:
        case Column::MinDuration:
        case Column::AvgDuration:
        case Column::MaxDuration:
            return Qt::AlignRight;
        default:
            Q_UNREACHABLE();
            return {};
        }
    case SortRole:
        switch (index.column()) {
        case Column::Title:
            return title;
        case Column::Count:
            return m_data.value(title).count;
        case Column::TotalDuration:
            return m_data.value(title).totalDuration;
        case Column::RelativeDuration:
            return m_data.value(title).totalDuration;
        case Column::MinDuration:
        {
            auto minDuration = m_data.value(title).minDuration;
            return minDuration != std::numeric_limits<qint64>::max() ? minDuration : 0;
        }
        case Column::AvgDuration:
        {
            auto data = m_data.value(title);
            if (data.totalDuration > 0 && data.count > 0)
                return double(data.totalDuration) / data.count;
            return 0;
        }
        case Column::MaxDuration:
            return m_data.value(title).maxDuration;
        default:
            return {};
        }
    case Qt::DisplayRole:
        switch (index.column()) {
        case Column::Title:
            return title;
        case Column::Count:
            return m_data.value(title).count;
        case Column::TotalDuration:
        {
            auto totalDuration = m_data.value(title).totalDuration;
            if (totalDuration > 0)
                return Timeline::formatTime(totalDuration);
            else
                return "-";
        }
        case Column::RelativeDuration:
        {
            auto totalDuration = m_data.value(title).totalDuration;
            if (m_measurementDurationInNs > 0 && totalDuration > 0) {
                const double percent = (totalDuration / double(m_measurementDurationInNs)) * 100;
                return QString("%1 %").arg(percent, 0, 'f', 2);
            } else {
                return "-";
            }
        }
        case Column::MinDuration:
        {
            auto minDuration = m_data.value(title).minDuration;
            if (minDuration != std::numeric_limits<qint64>::max())
                return Timeline::formatTime(minDuration);
            else
                return "-";
        }
        case Column::AvgDuration:
        {
            auto data = m_data.value(title);
            if (data.totalDuration > 0 && data.count > 0)
                return Timeline::formatTime(data.totalDuration / data.count);
            return "-";
        }
        case Column::MaxDuration:
        {
            auto maxDuration = m_data.value(title).maxDuration;
            if (maxDuration > 0)
                return Timeline::formatTime(maxDuration);
            else
                return "-";
        }
        }
    }

    return {};
}

QVariant CtfStatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case Column::Title:
        return Tr::tr("Title");
    case Column::Count:
        return Tr::tr("Count");
    case Column::TotalDuration:
        return Tr::tr("Total Time");
    case Column::RelativeDuration:
        return Tr::tr("Percentage");
    case Column::MinDuration:
        return Tr::tr("Minimum Time");
    case Column::AvgDuration:
        return Tr::tr("Average Time");
    case Column::MaxDuration:
        return Tr::tr("Maximum Time");
    default:
        return "";
    }
}

}  // CtfVisualizer::Internal
