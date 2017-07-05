/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "simulatorinfomodel.h"

#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <QTimer>

namespace Ios {
namespace Internal {

using namespace std::placeholders;

const int colCount = 3;
const int nameCol = 0;
const int runtimeCol = 1;
const int stateCol = 2;
static const int deviceUpdateInterval = 1000; // Update simulator state every 1 sec.

SimulatorInfoModel::SimulatorInfoModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    m_fetchFuture.setCancelOnWait(true);

    requestSimulatorInfo();

    auto updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &SimulatorInfoModel::requestSimulatorInfo);
    updateTimer->setInterval(deviceUpdateInterval);
    updateTimer->start();
}

QVariant SimulatorInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const SimulatorInfo &simInfo = m_simList[index.row()];
    if (role == Qt::EditRole || role == Qt::DisplayRole) {
        switch (index.column()) {
        case nameCol:
            return simInfo.name;
        case runtimeCol:
            return simInfo.runtimeName;
        case stateCol:
            return simInfo.state;
        default:
            return "";
        }
    } else if (role == Qt::ToolTipRole) {
        return tr("UDID: %1").arg(simInfo.identifier);
    } else if (role == Qt::UserRole) {
        return QVariant::fromValue<SimulatorInfo>(simInfo);
    }

    return QVariant();
}

int SimulatorInfoModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_simList.count();
    return 0;
}

int SimulatorInfoModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return colCount;
}

QVariant SimulatorInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || section > colCount)
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (section) {
        case nameCol:
            return tr("Simulator Name");
        case runtimeCol:
            return tr("Runtime");
        case stateCol:
            return tr("Current State");
        default:
            return "";
        }
    }

    return QVariant();
}

QModelIndex SimulatorInfoModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex SimulatorInfoModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

void SimulatorInfoModel::requestSimulatorInfo()
{
    if (!m_fetchFuture.futures().isEmpty() && !m_fetchFuture.futures().at(0).isFinished())
        return; // Ignore the request if the last request is still pending.

    m_fetchFuture.clearFutures();
    m_fetchFuture.addFuture(Utils::onResultReady(SimulatorControl::updateAvailableSimulators(),
                                                 this, &SimulatorInfoModel::populateSimulators));
}

void SimulatorInfoModel::populateSimulators(const SimulatorInfoList &simulatorList)
{
    if (m_simList.isEmpty() || m_simList.count() != simulatorList.count()) {
        // Reset the model in case of addition or deletion.
        beginResetModel();
        m_simList = simulatorList;
        endResetModel();
    } else {
        // update the rows with data chagne. e.g. state changes.
        auto newItr = simulatorList.cbegin();
        int start = -1, end = -1;
        std::list<std::pair<int, int>> updatedIndexes;
        for (auto itr = m_simList.cbegin(); itr < m_simList.cend(); ++itr, ++newItr) {
            if (*itr == *newItr) {
                if (end != -1)
                    updatedIndexes.push_back(std::make_pair(start, end - 1));
                start = std::distance(m_simList.cbegin(), itr);
                end = -1;
            } else {
                end = std::distance(m_simList.cbegin(), itr);
            }
        }
        m_simList = simulatorList;
        for (auto pair: updatedIndexes)
            emit dataChanged(index(pair.first,0), index(pair.second, colCount - 1));
    }
}

} // namespace Internal
} // namespace Ios
