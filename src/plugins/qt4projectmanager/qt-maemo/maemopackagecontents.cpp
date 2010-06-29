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
#include "profilewrapper.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace {
    QString pathVar(const QString &var)
    {
        return var + QLatin1String(".path");
    }

    QString filesVar(const QString &var)
    {
        return var + QLatin1String(".files");
    }

    const QLatin1String InstallsVar("INSTALLS");
    const QLatin1String TargetVar("target");
}

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageContents::MaemoPackageContents(MaemoPackageCreationStep *packageStep)
    : QAbstractTableModel(packageStep),
      m_packageStep(packageStep),
      m_modified(false),
      m_initialized(false)
{
}

MaemoPackageContents::~MaemoPackageContents() {}

bool MaemoPackageContents::buildModel() const
{
    if (m_initialized)
        return true;

    m_deployables.clear();
    QSharedPointer<ProFileWrapper> proFileWrapper
        = m_packageStep->proFileWrapper();
    const QStringList &elemList = proFileWrapper->varValues(InstallsVar);
    bool targetFound = false;
    foreach (const QString &elem, elemList) {
        const QStringList &paths = proFileWrapper->varValues(pathVar(elem));
        if (paths.count() != 1) {
            qWarning("Error: Variable %s has %d values.",
                qPrintable(pathVar(elem)), paths.count());
            continue;
        }

        const QStringList &files = proFileWrapper->varValues(filesVar(elem));
        if (files.isEmpty() && elem != TargetVar) {
            qWarning("Error: Variable %s has no RHS.",
                qPrintable(filesVar(elem)));
            continue;
        }

        if (elem == TargetVar) {
            m_deployables.prepend(MaemoDeployable(m_packageStep->localExecutableFilePath(),
                paths.first()));
            targetFound = true;
        } else {
            foreach (const QString &file, files)
                m_deployables << MaemoDeployable(
                    proFileWrapper->absFilePath(file), paths.first());
        }
    }

    if (!targetFound) {
        const Qt4ProFileNode * const proFileNode
            = m_packageStep->qt4BuildConfiguration()->qt4Target()
                ->qt4Project()->rootProjectNode();
        const QString remoteDir = proFileNode->projectType() == LibraryTemplate
            ? QLatin1String("/usr/local/lib")
            : QLatin1String("/usr/local/bin");
        m_deployables.prepend(MaemoDeployable(m_packageStep->localExecutableFilePath(),
            remoteDir));

        if (!proFileWrapper->addVarValue(pathVar(TargetVar), remoteDir)
            || !proFileWrapper->addVarValue(InstallsVar, TargetVar)) {
            qWarning("Error updating .pro file.");
            return false;
        }
    }

    m_initialized = true;
    m_modified = true;
    return true;
}

MaemoDeployable MaemoPackageContents::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

bool MaemoPackageContents::addDeployable(const MaemoDeployable &deployable,
    QString *error)
{
    if (m_deployables.contains(deployable)) {
        *error = tr("File already in list.");
        return false;
    }
    const QSharedPointer<ProFileWrapper> proFileWrapper
        = m_packageStep->proFileWrapper();
    QCryptographicHash elemHash(QCryptographicHash::Md5);
    elemHash.addData(deployable.localFilePath.toUtf8());
    const QString elemName = QString::fromAscii(elemHash.result().toHex());
    proFileWrapper->addFile(filesVar(elemName), deployable.localFilePath);
    proFileWrapper->addVarValue(pathVar(elemName), deployable.remoteDir);
    proFileWrapper->addVarValue(InstallsVar, elemName);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << deployable;
    endInsertRows();
    return true;
}

bool MaemoPackageContents::removeDeployableAt(int row, QString *error)
{
    Q_ASSERT(row > 0 && row < rowCount());

    const MaemoDeployable &deployable = deployableAt(row);
    const QString elemToRemove = findInstallsElem(deployable);
    if (elemToRemove.isEmpty()) {
        *error = tr("Inconsistent model: Deployable not found in  .pro file.");
        return false;
    }

    const QString &filesVarName = filesVar(elemToRemove);
    QSharedPointer<ProFileWrapper> proFileWrapper
        = m_packageStep->proFileWrapper();
    const bool isOnlyElem
        = proFileWrapper->varValues(filesVarName).count() == 1;
    bool success
        = proFileWrapper->removeFile(filesVarName, deployable.localFilePath);
    if (success && isOnlyElem) {
        success = proFileWrapper->removeVarValue(pathVar(elemToRemove),
            deployable.remoteDir);
        if (success)
            success = proFileWrapper->removeVarValue(InstallsVar, elemToRemove);
    }
    if (!success) {
        *error = tr("Could not remove deployable from .pro file.");
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_deployables.removeAt(row - 1);
    endRemoveRows();
    return true;
}

int MaemoPackageContents::rowCount(const QModelIndex &parent) const
{
    buildModel();
    return parent.isValid() ? 0 : m_deployables.count();
}

int MaemoPackageContents::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant MaemoPackageContents::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const MaemoDeployable &d = deployableAt(index.row());
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

    MaemoDeployable &deployable = m_deployables[index.row()];
    const QString elemToChange = findInstallsElem(deployable);
    if (elemToChange.isEmpty()) {
        qWarning("Error: Inconsistent model. "
            "INSTALLS element not found in .pro file");
        return false;
    }

    const QString &newRemoteDir = value.toString();
    QSharedPointer<ProFileWrapper> proFileWrapper
        = m_packageStep->proFileWrapper();
    if (!proFileWrapper->replaceVarValue(pathVar(elemToChange),
        deployable.remoteDir, newRemoteDir)) {
        qWarning("Error: Could not change remote path in .pro file.");
        return false;
    }

    deployable.remoteDir = newRemoteDir;
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

QString MaemoPackageContents::remoteExecutableFilePath() const
{
    return buildModel() ? deployableAt(0).remoteDir + '/'
        + m_packageStep->executableFileName() : QString();
}

QString MaemoPackageContents::findInstallsElem(const MaemoDeployable &deployable) const
{
    QSharedPointer<ProFileWrapper> proFileWrapper
        = m_packageStep->proFileWrapper();
    const QStringList &elems = proFileWrapper->varValues(InstallsVar);
    foreach (const QString &elem, elems) {
        const QStringList elemPaths = proFileWrapper->varValues(pathVar(elem));
        if (elemPaths.count() != 1 || elemPaths.first() != deployable.remoteDir)
            continue;
        if (elem == TargetVar)
            return elem;
        const QStringList elemFiles = proFileWrapper->varValues(filesVar(elem));
        foreach (const QString &file, elemFiles) {
            if (proFileWrapper->absFilePath(file) == deployable.localFilePath)
                return elem;
        }
    }
    return QString();
}

} // namespace Qt4ProjectManager
} // namespace Internal
