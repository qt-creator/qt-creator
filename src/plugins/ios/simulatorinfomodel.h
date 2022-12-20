// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "simulatorcontrol.h"

#include <utils/futuresynchronizer.h>

#include <QAbstractListModel>

namespace Ios::Internal {

using SimulatorInfoList = QList<SimulatorInfo>;

class SimulatorInfoModel : public QAbstractItemModel
{
public:
    SimulatorInfoModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override;

private:
    void requestSimulatorInfo();
    void populateSimulators(const SimulatorInfoList &simulatorList);

    Utils::FutureSynchronizer m_fetchFuture;
    SimulatorInfoList m_simList;
};

} // Ios::Internal
