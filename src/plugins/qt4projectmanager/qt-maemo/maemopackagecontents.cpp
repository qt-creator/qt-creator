/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "maemopackagecontents.h"

#include "maemopackagecreationstep.h"

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageContents::MaemoPackageContents(MaemoPackageCreationStep *packageStep)
    : QAbstractTableModel(packageStep),
      m_packageStep(packageStep),
      m_modified(true) // TODO: Has to come from settings
{
}

MaemoPackageContents::Deployable MaemoPackageContents::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return row == 0
        ? Deployable(m_packageStep->localExecutableFilePath(),
                     m_packageStep->remoteExecutableFilePath())
        : m_deployables.at(row - 1);
}

int MaemoPackageContents::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployables.count() + 1;
}

int MaemoPackageContents::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant MaemoPackageContents::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole
        || index.row() >= rowCount())
        return QVariant();

    const Deployable &d = deployableAt(index.row());
    return index.column() == 0 ? d.localFilePath : d.remoteFilePath;
}

QVariant MaemoPackageContents::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote File Path");
}

} // namespace Qt4ProjectManager
} // namespace Internal
