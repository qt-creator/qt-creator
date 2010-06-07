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
#include "maemotoolchain.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace {
    const char * const MODIFIED_KEY
        = "Qt4ProjectManager.BuildStep.MaemoPackage.Modified";
    const char * const REMOTE_EXE_DIR_KEY
        = "Qt4ProjectManager.BuildStep.MaemoPackage.RemoteExeDir";
    const char * const LOCAL_FILES_KEY
        = "Qt4ProjectManager.BuildStep.MaemoPackage.LocalFiles";
    const char * const REMOTE_DIRS_KEY
        = "Qt4ProjectManager.BuildStep.MaemoPackage.RemoteDirs";
}

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageContents::MaemoPackageContents(MaemoPackageCreationStep *packageStep)
    : QAbstractTableModel(packageStep),
      m_packageStep(packageStep),
      m_modified(true)
{
}

MaemoPackageContents::Deployable MaemoPackageContents::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return row == 0
        ? Deployable(m_packageStep->localExecutableFilePath(),
                     remoteExecutableDir())
        : m_deployables.at(row - 1);
}

bool MaemoPackageContents::addDeployable(const Deployable &deployable)
{
    if (m_deployables.contains(deployable) || deployableAt(0) == deployable)
        return false;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << deployable;
    endInsertRows();
    m_modified = true;
    return true;
}

void MaemoPackageContents::removeDeployableAt(int row)
{
    Q_ASSERT(row > 0 && row < rowCount());
    beginRemoveRows(QModelIndex(), row, row);
    m_deployables.removeAt(row - 1);
    endRemoveRows();
    m_modified = true;
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
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const Deployable &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return d.localFilePath;
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
    return QVariant();
}

Qt::ItemFlags MaemoPackageContents::flags(const QModelIndex &index) const
{
    Qt::ItemFlags parentFlags = QAbstractTableModel::flags(index);
    if (index.column() == 1)
        return parentFlags | Qt::ItemIsEditable;
    return parentFlags;
}

bool MaemoPackageContents::setData(const QModelIndex &index,
                                   const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() != 1
        || role != Qt::EditRole)
        return false;

    const QString &remoteDir = value.toString();
    if (index.row() == 0)
        m_remoteExecutableDir = remoteDir;
    else
        m_deployables[index.row() - 1].remoteDir = remoteDir;
    m_modified = true;
    emit dataChanged(index, index);
    return true;
}

QVariant MaemoPackageContents::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QVariantMap MaemoPackageContents::toMap() const
{
    QVariantMap map;
    map.insert(MODIFIED_KEY, m_modified);
    map.insert(REMOTE_EXE_DIR_KEY, m_remoteExecutableDir);

    QDir dir;
    QStringList localFiles;
    QStringList remoteDirs;
    foreach (const Deployable &p, m_deployables) {
        localFiles << dir.fromNativeSeparators(p.localFilePath);
        remoteDirs << p.remoteDir;
    }
    map.insert(LOCAL_FILES_KEY, localFiles);
    map.insert(REMOTE_DIRS_KEY, remoteDirs);
    return map;
}

void MaemoPackageContents::fromMap(const QVariantMap &map)
{
    m_modified = map.value(MODIFIED_KEY).toBool();
    m_remoteExecutableDir = map.value(REMOTE_EXE_DIR_KEY).toString();
    const QStringList localFiles = map.value(LOCAL_FILES_KEY).toStringList();
    const QStringList remoteDirs = map.value(REMOTE_DIRS_KEY).toStringList();
    if (localFiles.count() != remoteDirs.count())
        qWarning("%s: serialized data inconsistent", Q_FUNC_INFO);

    QDir dir;
    const int count = qMin(localFiles.count(), remoteDirs.count());
    for (int i = 0; i < count; ++i) {
        m_deployables << Deployable(dir.toNativeSeparators(localFiles.at(i)),
            remoteDirs.at(i));
    }
}

QString MaemoPackageContents::remoteExecutableDir() const
{
    if (m_remoteExecutableDir.isEmpty()) {
        const Qt4ProjectType projectType
            = m_packageStep->qt4BuildConfiguration()->qt4Target()->qt4Project()
              ->rootProjectNode()->projectType();
        m_remoteExecutableDir = projectType == LibraryTemplate
            ? QLatin1String("/usr/local/lib")
            : QLatin1String("/usr/local/bin");
    }
    return m_remoteExecutableDir;
}

QString MaemoPackageContents::remoteExecutableFilePath() const
{
    return remoteExecutableDir() + '/' + m_packageStep->executableFileName();
}

} // namespace Qt4ProjectManager
} // namespace Internal
