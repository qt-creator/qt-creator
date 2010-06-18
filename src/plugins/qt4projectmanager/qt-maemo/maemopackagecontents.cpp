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

#include <qt4projectmanager/profilereader.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <prowriter.h>

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
      m_proFileOption(new ProFileOption),
      m_proFileReader(new ProFileReader(m_proFileOption.data())),
      m_modified(false),
      m_proFile(0)
{
}

MaemoPackageContents::~MaemoPackageContents() {}

bool MaemoPackageContents::init()
{
    return m_proFile ? true : buildModel();
}

bool MaemoPackageContents::buildModel() const
{
    m_deployables.clear();
    const Qt4ProFileNode * const proFileNode = m_packageStep
        ->qt4BuildConfiguration()->qt4Target()->qt4Project()->rootProjectNode();
    if (m_proFileName.isEmpty()) {
        m_proFileName = proFileNode->path();
        m_proDir = QFileInfo(m_proFileName).dir();
    }

    resetProFileContents();
    if (!m_proFile)
        return false;

    const QStringList elemList = m_proFileReader->values(InstallsVar, m_proFile);
    bool targetFound = false;
    foreach (const QString &elem, elemList) {
        const QStringList paths
            = m_proFileReader->values(pathVar(elem), m_proFile);
        if (paths.count() != 1) {
            qWarning("Error: Variable %s has %d values.",
                qPrintable(pathVar(elem)), paths.count());
            continue;
        }

        const QStringList files
            = m_proFileReader->values(filesVar(elem), m_proFile);
        if (files.isEmpty() && elem != TargetVar) {
            qWarning("Error: Variable %s has no RHS.",
                qPrintable(filesVar(elem)));
            continue;
        }

        if (elem == TargetVar) {
            m_deployables.prepend(Deployable(m_packageStep->localExecutableFilePath(),
                paths.first()));
            targetFound = true;
        } else {
            foreach (const QString &file, files)
                m_deployables << Deployable(cleanPath(file), paths.first());
        }
    }

    if (!targetFound) {
        const QString remoteDir = proFileNode->projectType() == LibraryTemplate
            ? QLatin1String("/usr/local/lib")
            : QLatin1String("/usr/local/bin");
        m_deployables.prepend(Deployable(m_packageStep->localExecutableFilePath(),
            remoteDir));
        QString errorString;
        if (!readProFileContents(&errorString)) {
            qWarning("Error reading .pro file: %s", qPrintable(errorString));
            return false;
        }
        addValueToProFile(pathVar(TargetVar), remoteDir);
        addValueToProFile(InstallsVar, TargetVar);
        if (!writeProFileContents(&errorString)) {
            qWarning("Error writing .pro file: %s", qPrintable(errorString));
            return false;
        }
    }

    m_modified = true;
    return true;
}

MaemoPackageContents::Deployable MaemoPackageContents::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

bool MaemoPackageContents::addDeployable(const Deployable &deployable,
    QString *error)
{
    if (m_deployables.contains(deployable) || deployableAt(0) == deployable)
        return false;

    if (!readProFileContents(error))
        return false;

    QCryptographicHash elemHash(QCryptographicHash::Md5);
    elemHash.addData(deployable.localFilePath.toUtf8());
    const QString elemName = QString::fromAscii(elemHash.result().toHex());
    addFileToProFile(filesVar(elemName), deployable.localFilePath);
    addValueToProFile(pathVar(elemName), deployable.remoteDir);
    addValueToProFile(InstallsVar, elemName);

    if (!writeProFileContents(error))
        return false;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << deployable;
    endInsertRows();
    return true;
}

bool MaemoPackageContents::removeDeployableAt(int row, QString *error)
{
    Q_ASSERT(row > 0 && row < rowCount());

    const Deployable &deployable = deployableAt(row);
    const QString elemToRemove = findInstallsElem(deployable);
    if (elemToRemove.isEmpty()) {
        *error = tr("Inconsistent model: Deployable not found in  .pro file.");
        return false;
    }

    if (!readProFileContents(error))
        return false;

    const QString filesVarName = filesVar(elemToRemove);
    const bool isOnlyElem
        = m_proFileReader->values(filesVarName, m_proFile).count() == 1;
    bool success
        = removeFileFromProFile(filesVarName, deployable.localFilePath);
    if (success && isOnlyElem) {
        success = removeValueFromProFile(pathVar(elemToRemove),
            deployable.remoteDir);
        if (success)
            success = removeValueFromProFile(InstallsVar, elemToRemove);
    }
    if (!success) {
        *error = tr("Could not remove deployable from .pro file.");
        return false;
    }

    if (!writeProFileContents(error))
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    m_deployables.removeAt(row - 1);
    endRemoveRows();
    return true;
}

int MaemoPackageContents::rowCount(const QModelIndex &parent) const
{
    if (!m_proFile)
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

    QString error;
    if (!readProFileContents(&error)) {
        qWarning("%s", qPrintable(error));
        return false;
    }

    Deployable &deployable = m_deployables[index.row()];
    const QString elemToChange = findInstallsElem(deployable);
    if (elemToChange.isEmpty()) {
        qWarning("Error: Inconsistent model. "
            "INSTALLS element not found in .pro file");
        return false;
    }
    const QString pathElem = pathVar(elemToChange);
    if (!removeValueFromProFile(pathElem, deployable.remoteDir)) {
        qWarning("Error: Could not change remote path in .pro file.");
        return false;
    }
    const QString &newRemoteDir = value.toString();
    addValueToProFile(pathElem, newRemoteDir);

    if (!writeProFileContents(&error)) {
        qWarning("%s", qPrintable(error));
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
    if (!m_proFile)
        buildModel();
    return deployableAt(0).remoteDir + '/' + m_packageStep->executableFileName();
}

bool MaemoPackageContents::readProFileContents(QString *error) const
{
    if (!m_proFileLines.isEmpty())
        return true;

    QFile proFileOnDisk(m_proFileName);
    if (!proFileOnDisk.open(QIODevice::ReadOnly)) {
        *error = tr("Project file '%1' could not be opened for reading.")
            .arg(m_proFileName);
        return false;
    }
    const QString proFileContents
        = QString::fromLatin1(proFileOnDisk.readAll());
    if (proFileOnDisk.error() != QFile::NoError) {
        *error = tr("Project file '%1' could not be read.")
            .arg(m_proFileName);
        return false;
    }
    m_proFileLines = proFileContents.split('\n');
    return true;
}

bool MaemoPackageContents::writeProFileContents(QString *error) const
{
    QFile proFileOnDisk(m_proFileName);
    if (!proFileOnDisk.open(QIODevice::WriteOnly)) {
        *error = tr("Project file '%1' could not be opened for writing.")
            .arg(m_proFileName);
        resetProFileContents();
        return false;
    }

    // TODO: Disconnect and reconnect FS watcher here.
    proFileOnDisk.write(m_proFileLines.join("\n").toLatin1());
    proFileOnDisk.close();
    if (proFileOnDisk.error() != QFile::NoError) {
        *error = tr("Project file '%1' could not be written.")
            .arg(m_proFileName);
        resetProFileContents();
        return false;
    }
    m_modified = true;
    return true;
}

QString MaemoPackageContents::cleanPath(const QString &relFileName) const
{
    // I'd rather use QDir::cleanPath(), but that doesn't work well
    // enough for redundant ".." dirs.
    return QFileInfo(m_proFile->directoryName() + '/'
        + relFileName).canonicalFilePath();
}

QString MaemoPackageContents::findInstallsElem(const Deployable &deployable) const
{
    const QStringList elems = m_proFileReader->values(InstallsVar, m_proFile);
    foreach (const QString &elem, elems) {
        const QStringList elemPaths
            = m_proFileReader->values(pathVar(elem), m_proFile);
        if (elemPaths.count() != 1 || elemPaths.first() != deployable.remoteDir)
            continue;
        if (elem == TargetVar)
            return elem;
        const QStringList elemFiles
            = m_proFileReader->values(filesVar(elem), m_proFile);
        foreach (const QString &file, elemFiles) {
            if (cleanPath(file) == deployable.localFilePath)
                return elem;
        }
    }
    return QString();
}

void MaemoPackageContents::addFileToProFile(const QString &var,
    const QString &absFilePath)
{
    ProWriter::addFiles(m_proFile, &m_proFileLines, m_proDir,
        QStringList(absFilePath), var);
    parseProFile(ParseFromLines);
}

void MaemoPackageContents::addValueToProFile(const QString &var,
    const QString &value) const
{
    ProWriter::addVarValues(m_proFile, &m_proFileLines, m_proDir,
        QStringList(value), var);
    parseProFile(ParseFromLines);
}

bool MaemoPackageContents::removeFileFromProFile(const QString &var,
    const QString &absFilePath)
{
    const bool success = ProWriter::removeFiles(m_proFile, &m_proFileLines,
        m_proDir, QStringList(absFilePath),
        QStringList(var)).isEmpty();
    if (success)
        parseProFile(ParseFromLines);
    else
        resetProFileContents();
    return success;
}

bool MaemoPackageContents::removeValueFromProFile(const QString &var,
    const QString &value)
{
    const bool success = ProWriter::removeVarValues(m_proFile,
        &m_proFileLines, m_proDir, QStringList(value),
        QStringList(var)).isEmpty();
    if (success)
        parseProFile(ParseFromLines);
    else
        resetProFileContents();
    return success;
}

void MaemoPackageContents::parseProFile(ParseType type) const
{
    if (type == ParseFromLines) {
        m_proFile = m_proFileReader->parsedProFile(m_proFileName, false,
            m_proFileLines.join("\n"));
    } else {
        m_proFile = m_proFileReader->readProFile(m_proFileName)
            ? m_proFileReader->proFileFor(m_proFileName) : 0;
    }
}

void MaemoPackageContents::resetProFileContents() const
{
    m_proFileLines.clear();
    parseProFile(ParseFromFile);
    if (!m_proFile)
        qWarning("Fatal: Could not parse .pro file '%s'.",
            qPrintable(m_proFileName));
}

} // namespace Qt4ProjectManager
} // namespace Internal
