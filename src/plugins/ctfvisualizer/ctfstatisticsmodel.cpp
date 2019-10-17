/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "ctfstatisticsmodel.h"

#include "ctfvisualizerconstants.h"

#include <tracing/timelineformattime.h>

namespace CtfVisualizer {
namespace Internal {

using json = nlohmann::json;
using namespace Constants;

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
        return QVariant();

    QString title = m_data.keys().at(index.row());

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
            return QVariant();
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
            return QVariant();
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

    return QVariant();
}

QVariant CtfStatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case Column::Title:
        return tr("Title");
    case Column::Count:
        return tr("Count");
    case Column::TotalDuration:
        return tr("Total Time");
    case Column::RelativeDuration:
        return tr("Percentage");
    case Column::MinDuration:
        return tr("Minimum Time");
    case Column::AvgDuration:
        return tr("Average Time");
    case Column::MaxDuration:
        return tr("Maximum Time");
    default:
        return "";
    }
}

}  // Internal
}  // CtfVisualizer
