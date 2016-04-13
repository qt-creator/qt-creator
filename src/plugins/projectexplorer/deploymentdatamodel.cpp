/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "deploymentdatamodel.h"

namespace ProjectExplorer {

DeploymentDataModel::DeploymentDataModel(QObject *parent) : QAbstractTableModel(parent)
{ }

void DeploymentDataModel::setDeploymentData(const DeploymentData &deploymentData)
{
    beginResetModel();
    m_deploymentData = deploymentData;
    endResetModel();
}

int DeploymentDataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deploymentData.fileCount();
}

int DeploymentDataModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant DeploymentDataModel::headerData(int section, Qt::Orientation orientation,
        int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QVariant DeploymentDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();

    const DeployableFile &d = m_deploymentData.fileAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return d.localFilePath().toUserOutput();
    if (role == Qt::DisplayRole)
        return d.remoteDirectory();
    return QVariant();
}

} // namespace ProjectExplorer
