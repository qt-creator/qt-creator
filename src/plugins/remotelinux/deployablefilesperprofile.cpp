/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "deployablefilesperprofile.h"

#include "deployablefile.h"

#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QBrush>

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
        QObject *parent)
    : QAbstractTableModel(parent), m_d(new DeployableFilesPerProFilePrivate(proFileNode))
{
    if (m_d->projectType == ApplicationTemplate) {
        m_d->deployables.prepend(DeployableFile(localExecutableFilePath(),
            m_d->installsList.targetPath));
    } else if (m_d->projectType == LibraryTemplate) {
        foreach (const QString &filePath, localLibraryFilePaths()) {
            m_d->deployables.prepend(DeployableFile(filePath,
                m_d->installsList.targetPath));
        }
    }
    foreach (const InstallsItem &elem, m_d->installsList.items) {
        foreach (const QString &file, elem.files)
            m_d->deployables << DeployableFile(file, elem.path);
    }
}

DeployableFilesPerProFile::~DeployableFilesPerProFile()
{
    delete m_d;
}

DeployableFile DeployableFilesPerProFile::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_d->deployables.at(row);
}

int DeployableFilesPerProFile::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_d->deployables.count();
}

int DeployableFilesPerProFile::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant DeployableFilesPerProFile::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    if (m_d->projectType != AuxTemplate && !hasTargetPath() && index.row() == 0
            && index.column() == 1) {
        if (role == Qt::DisplayRole)
            return tr("<no target path set>");
        if (role == Qt::ForegroundRole) {
            QBrush brush;
            brush.setColor("red");
            return brush;
        }
    }

    const DeployableFile &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
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
    if (!m_d->targetInfo.valid || m_d->projectType != ApplicationTemplate)
        return QString();
    return QDir::cleanPath(m_d->targetInfo.workingDir + '/' + m_d->targetInfo.target);
}

QStringList DeployableFilesPerProFile::localLibraryFilePaths() const
{
    if (!m_d->targetInfo.valid || m_d->projectType != LibraryTemplate)
        return QStringList();
    QString basePath = m_d->targetInfo.workingDir + QLatin1String("/lib");
    const bool isStatic = m_d->config.contains(QLatin1String("static"))
            || m_d->config.contains(QLatin1String("staticlib"));
    basePath += m_d->targetInfo.target + QLatin1String(isStatic ? ".a" : ".so");
    basePath = QDir::cleanPath(basePath);
    const QChar dot(QLatin1Char('.'));
    const QString filePathMajor = basePath + dot
        + QString::number(m_d->projectVersion.major);
    const QString filePathMinor = filePathMajor + dot
         + QString::number(m_d->projectVersion.minor);
    const QString filePathPatch  = filePathMinor + dot
         + QString::number(m_d->projectVersion.patch);
    return QStringList() << filePathPatch << filePathMinor << filePathMajor
        << basePath;
}

QString DeployableFilesPerProFile::remoteExecutableFilePath() const
{
    return hasTargetPath() && m_d->projectType == ApplicationTemplate
        ? deployableAt(0).remoteDir + QLatin1Char('/')
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString DeployableFilesPerProFile::projectDir() const
{
    return QFileInfo(m_d->proFilePath).dir().path();
}

bool DeployableFilesPerProFile::hasTargetPath() const
{
    return !m_d->installsList.targetPath.isEmpty();
}

bool DeployableFilesPerProFile::isModified() const { return m_d->modified; }
void DeployableFilesPerProFile::setUnModified() { m_d->modified = false; }
QString DeployableFilesPerProFile::projectName() const { return m_d->projectName; }
QString DeployableFilesPerProFile::proFilePath() const { return m_d->proFilePath; }
Qt4ProjectType DeployableFilesPerProFile::projectType() const { return m_d->projectType; }
QString DeployableFilesPerProFile::applicationName() const { return m_d->targetInfo.target; }
} // namespace RemoteLinux
