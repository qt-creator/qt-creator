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

#pragma once

#include "json/json.hpp"

#include <QHash>
#include <QStack>
#include <QVector>
#include <QPointer>
#include <QAbstractTableModel>

namespace CtfVisualizer {
namespace Internal {

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

}  // Internal
}  // CtfVisualizer
