/****************************************************************************
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
#include "deployablefilesperprofile.h"

#include "deployablefile.h"

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDir>
#include <QBrush>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
class DeployableFilesPerProFilePrivate
{
public:
    DeployableFilesPerProFilePrivate(const Qt4ProFileNode *proFileNode)
        : projectType(proFileNode->projectType()),
          proFilePath(proFileNode->path()),
          projectName(proFileNode->displayName()),
          targetInfo(proFileNode->targetInformation()),
          installsList(proFileNode->installsList()),
          projectVersion(proFileNode->projectVersion()),
          config(proFileNode->variableValue(ConfigVar)),
          modified(true)
    {
    }

    const Qt4ProjectType projectType;
    const QString proFilePath;
    const QString projectName;
    const Qt4ProjectManager::TargetInformation targetInfo;
    const Qt4ProjectManager::InstallsList installsList;
    const Qt4ProjectManager::ProjectVersion projectVersion;
    const QStringList config;
    QList<DeployableFile> deployables;
    bool modified;
};

} // namespace Internal

using namespace Internal;

DeployableFilesPerProFile::DeployableFilesPerProFile(const Qt4ProFileNode *proFileNode,
        const QString &installPrefix, QObject *parent)
    : QAbstractTableModel(parent), d(new DeployableFilesPerProFilePrivate(proFileNode))
{
    if (hasTargetPath()) {
        if (d->projectType == ApplicationTemplate) {
            d->deployables.prepend(DeployableFile(localExecutableFilePath(),
                    d->installsList.targetPath, DeployableFile::TypeExecutable));
        } else if (d->projectType == LibraryTemplate) {
            foreach (const QString &filePath, localLibraryFilePaths()) {
                d->deployables.prepend(DeployableFile(filePath,
                        d->installsList.targetPath));
            }
        }
    }

    foreach (const InstallsItem &elem, d->installsList.items) {
        foreach (const QString &file, elem.files)
            d->deployables << DeployableFile(file, elem.path);
    }

    if (!installPrefix.isEmpty()) {
        for (int i = 0; i < d->deployables.count(); ++i)
            d->deployables[i].remoteDir.prepend(installPrefix + QLatin1Char('/'));
    }
}

DeployableFilesPerProFile::~DeployableFilesPerProFile()
{
    delete d;
}

DeployableFile DeployableFilesPerProFile::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return d->deployables.at(row);
}

int DeployableFilesPerProFile::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->deployables.count();
}

int DeployableFilesPerProFile::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant DeployableFilesPerProFile::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const DeployableFile &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole)
        return QDir::cleanPath(d.remoteDir);
    return QVariant();
}

QVariant DeployableFilesPerProFile::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QString DeployableFilesPerProFile::localExecutableFilePath() const
{
    if (!d->targetInfo.valid || d->projectType != ApplicationTemplate)
        return QString();
    return QDir::cleanPath(d->targetInfo.workingDir + '/' + d->targetInfo.target);
}

QStringList DeployableFilesPerProFile::localLibraryFilePaths() const
{
    QStringList list;

    if (!d->targetInfo.valid || d->projectType != LibraryTemplate)
        return list;
    QString basePath = d->targetInfo.workingDir + QLatin1String("/lib");
    const bool isStatic = d->config.contains(QLatin1String("static"))
            || d->config.contains(QLatin1String("staticlib"));
    basePath += d->targetInfo.target + QLatin1String(isStatic ? ".a" : ".so");
    basePath = QDir::cleanPath(basePath);
    if (!isStatic && !d->config.contains(QLatin1String("plugin"))) {
        const QChar dot(QLatin1Char('.'));
        const QString filePathMajor = basePath + dot
                + QString::number(d->projectVersion.major);
        const QString filePathMinor = filePathMajor + dot
                + QString::number(d->projectVersion.minor);
        const QString filePathPatch  = filePathMinor + dot
                + QString::number(d->projectVersion.patch);
        list << filePathPatch << filePathMinor << filePathMajor;
    }
    return list << basePath;
}

QString DeployableFilesPerProFile::remoteExecutableFilePath() const
{
    return hasTargetPath() && d->projectType == ApplicationTemplate
        ? deployableAt(0).remoteDir + QLatin1Char('/')
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString DeployableFilesPerProFile::projectDir() const
{
    return QFileInfo(d->proFilePath).dir().path();
}

bool DeployableFilesPerProFile::hasTargetPath() const
{
    return !d->installsList.targetPath.isEmpty();
}

bool DeployableFilesPerProFile::isModified() const { return d->modified; }
void DeployableFilesPerProFile::setUnModified() { d->modified = false; }
QString DeployableFilesPerProFile::projectName() const { return d->projectName; }
QString DeployableFilesPerProFile::proFilePath() const { return d->proFilePath; }
Qt4ProjectType DeployableFilesPerProFile::projectType() const { return d->projectType; }
QString DeployableFilesPerProFile::applicationName() const { return d->targetInfo.target; }

} // namespace RemoteLinux
