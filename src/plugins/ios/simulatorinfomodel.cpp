// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simulatorinfomodel.h"

#include "iostr.h"

#include <utils/algorithm.h>
#include <utils/async.h>

#include <QTimer>

namespace Ios::Internal {

using namespace std::placeholders;

const int colCount = 3;
const int nameCol = 0;
const int runtimeCol = 1;
const int stateCol = 2;
const int deviceUpdateInterval = 1000; // Update simulator state every 1 sec.

SimulatorInfoModel::SimulatorInfoModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    requestSimulatorInfo();

    auto updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &SimulatorInfoModel::requestSimulatorInfo);
    updateTimer->setInterval(deviceUpdateInterval);
    updateTimer->start();
}

QVariant SimulatorInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

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
        return Tr::tr("UDID: %1").arg(simInfo.identifier);
    } else if (role == Qt::UserRole) {
        return QVariant::fromValue<SimulatorInfo>(simInfo);
    }

    return {};
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
        return {};

    if (role == Qt::DisplayRole) {
        switch (section) {
        case nameCol:
            return Tr::tr("Simulator Name");
        case runtimeCol:
            return Tr::tr("Runtime");
        case stateCol:
            return Tr::tr("Current State");
        default:
            return {};
        }
    }

    return {};
}

QModelIndex SimulatorInfoModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex SimulatorInfoModel::parent(const QModelIndex &) const
{
    return {};
}

void SimulatorInfoModel::requestSimulatorInfo()
{
    m_fetchFuture.flushFinishedFutures();
    if (!m_fetchFuture.isEmpty())
        return; // Ignore the request if the last request is still pending.

    m_fetchFuture.addFuture(Utils::onResultReady(SimulatorControl::updateAvailableSimulators(this),
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
                    updatedIndexes.emplace_back(start, end - 1);
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

} // Ios::Internal
