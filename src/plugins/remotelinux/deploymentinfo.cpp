/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "deploymentinfo.h"

#include "deployablefile.h"
#include "deployablefilesperprofile.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtprofileinformation.h>

#include <QList>
#include <QTimer>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
class DeploymentInfoPrivate
{
public:
    DeploymentInfoPrivate(const Qt4ProjectManager::Qt4Project *p) : project(p) {}

    QList<DeployableFilesPerProFile *> listModels;
    const Qt4ProjectManager::Qt4Project *const project;
    QString installPrefix;
};
} // namespace Internal

using namespace Internal;

DeploymentInfo::DeploymentInfo(Qt4ProjectManager::Qt4Project *project, const QString &installPrefix)
    : QAbstractListModel(project), d(new DeploymentInfoPrivate(project))
{
    connect(project, SIGNAL(buildSystemEvaluated()), SLOT(createModels()));
    setInstallPrefix(installPrefix);
}

DeploymentInfo::~DeploymentInfo()
{
    delete d;
}

void DeploymentInfo::createModels()
{
    ProjectExplorer::Target *target = d->project->activeTarget();
    if (!target
            || !target->activeDeployConfiguration()
            || !qobject_cast<RemoteLinuxDeployConfiguration *>(target->activeDeployConfiguration()))
        return;

    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target->profile());
    if (!version || !version->isValid()) {
        beginResetModel();
        qDeleteAll(d->listModels);
        d->listModels.clear();
        endResetModel();
        return;
    }
    const Qt4ProFileNode *const rootNode = d->project->rootQt4ProjectNode();
    if (!rootNode || rootNode->parseInProgress()) // Can be null right after project creation by wizard.
        return;
    disconnect(d->project, SIGNAL(buildSystemEvaluated()), this, SLOT(createModels()));
    beginResetModel();
    qDeleteAll(d->listModels);
    d->listModels.clear();
    createModels(rootNode);
    endResetModel();
    connect (d->project, SIGNAL(buildSystemEvaluated()), SLOT(createModels()));
}

void DeploymentInfo::createModels(const Qt4ProFileNode *proFileNode)
{
    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case LibraryTemplate:
    case AuxTemplate:
        d->listModels << new DeployableFilesPerProFile(proFileNode, d->installPrefix, this);
        break;
    case SubDirsTemplate: {
        const QList<Qt4PriFileNode *> &subProjects = proFileNode->subProjectNodesExact();
        foreach (const ProjectExplorer::ProjectNode * const subProject, subProjects) {
            const Qt4ProFileNode * const qt4SubProject
                = qobject_cast<const Qt4ProFileNode *>(subProject);
            if (!qt4SubProject)
                continue;
            if (qt4SubProject->path().endsWith(QLatin1String(".pri")))
                continue;
            if (!proFileNode->isSubProjectDeployable(subProject->path()))
                continue;
            createModels(qt4SubProject);
        }
    }
    default:
        break;
    }
}

void DeploymentInfo::setUnmodified()
{
    foreach (DeployableFilesPerProFile * const model, d->listModels)
        model->setUnModified();
}

bool DeploymentInfo::isModified() const
{
    foreach (const DeployableFilesPerProFile * const model, d->listModels) {
        if (model->isModified())
            return true;
    }
    return false;
}

void DeploymentInfo::setInstallPrefix(const QString &installPrefix)
{
    d->installPrefix = installPrefix;
    createModels();
}

int DeploymentInfo::deployableCount() const
{
    int count = 0;
    foreach (const DeployableFilesPerProFile * const model, d->listModels)
        count += model->rowCount();
    return count;
}

DeployableFile DeploymentInfo::deployableAt(int i) const
{
    foreach (const DeployableFilesPerProFile * const model, d->listModels) {
        Q_ASSERT(i >= 0);
        if (i < model->rowCount())
            return model->deployableAt(i);
        i -= model->rowCount();
    }

    Q_ASSERT(!"Invalid deployable number");
    return DeployableFile(QString(), QString());
}

QString DeploymentInfo::remoteExecutableFilePath(const QString &localExecutableFilePath) const
{
    foreach (const DeployableFilesPerProFile * const model, d->listModels) {
        if (model->localExecutableFilePath() == localExecutableFilePath)
            return model->remoteExecutableFilePath();
    }
    return QString();
}

int DeploymentInfo::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : modelCount();
}

QVariant DeploymentInfo::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= modelCount()
            || index.column() != 0)
        return QVariant();
    const DeployableFilesPerProFile * const model = d->listModels.at(index.row());
    if (role == Qt::ForegroundRole && model->projectType() != AuxTemplate
            && !model->hasTargetPath()) {
        QBrush brush;
        brush.setColor(Qt::red);
        return brush;
    }
    if (role == Qt::DisplayRole)
        return QFileInfo(model->proFilePath()).fileName();
    return QVariant();
}

int DeploymentInfo::modelCount() const { return d->listModels.count(); }
DeployableFilesPerProFile *DeploymentInfo::modelAt(int i) const { return d->listModels.at(i); }

} // namespace RemoteLinux
