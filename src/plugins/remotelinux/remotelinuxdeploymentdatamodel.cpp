/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxdeploymentdatamodel.h"

#include <QDir>

using namespace ProjectExplorer;

namespace RemoteLinux {

RemoteLinuxDeploymentDataModel::RemoteLinuxDeploymentDataModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void RemoteLinuxDeploymentDataModel::setDeploymentData(const DeploymentData &deploymentData)
{
    beginResetModel();
    m_deploymentData = deploymentData;
    endResetModel();
}

int RemoteLinuxDeploymentDataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deploymentData.fileCount();
}

int RemoteLinuxDeploymentDataModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant RemoteLinuxDeploymentDataModel::headerData(int section, Qt::Orientation orientation,
        int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QVariant RemoteLinuxDeploymentDataModel::data(const QModelIndex &index, int role) const
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

} // namespace RemoteLinux
