/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "deploymentinfo.h"

#include "deployablefile.h"
#include "maemoprofilesupdatedialog.h"

#include <projectexplorer/buildstep.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QTimer>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
using namespace Internal;

DeploymentInfo::DeploymentInfo(const Qt4BaseTarget *target)
    : m_target(target), m_updateTimer(new QTimer(this))
{
    Qt4Project * const pro = m_target->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(startTimer(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
    m_updateTimer->setInterval(1500);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(createModels()));
    createModels();
}

DeploymentInfo::~DeploymentInfo() {}

void DeploymentInfo::startTimer(Qt4ProjectManager::Qt4ProFileNode*, bool success, bool parseInProgress)
{
    Q_UNUSED(success)
    if (!parseInProgress)
        m_updateTimer->start();
}

void DeploymentInfo::createModels()
{
    if (m_target->project()->activeTarget() != m_target)
        return;
    if (!m_target->activeBuildConfiguration() || !m_target->activeBuildConfiguration()->qtVersion()
            || !m_target->activeBuildConfiguration()->qtVersion()->isValid()) {
        beginResetModel();
        m_listModels.clear();
        endResetModel();
        return;
    }
    const Qt4ProFileNode *const rootNode
        = m_target->qt4Project()->rootProjectNode();
    if (!rootNode || rootNode->parseInProgress()) // Can be null right after project creation by wizard.
        return;
    m_updateTimer->stop();
    disconnect(m_target->qt4Project(),
        SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
        this, SLOT(startTimer(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
    beginResetModel();
    qDeleteAll(m_listModels);
    m_listModels.clear();
    createModels(rootNode);
    QList<DeployableFilesPerProFile *> modelsWithoutTargetPath;
    foreach (DeployableFilesPerProFile *const model, m_listModels) {
        if (!model->hasTargetPath()) {
            if (model->proFileUpdateSetting() == DeployableFilesPerProFile::AskToUpdateProFile)
                modelsWithoutTargetPath << model;
        }
    }

    if (!modelsWithoutTargetPath.isEmpty()) {
        MaemoProFilesUpdateDialog dialog(modelsWithoutTargetPath);
        dialog.exec();
        const QList<MaemoProFilesUpdateDialog::UpdateSetting> &settings
            = dialog.getUpdateSettings();
        foreach (const MaemoProFilesUpdateDialog::UpdateSetting &setting, settings) {
            const DeployableFilesPerProFile::ProFileUpdateSetting updateSetting
                = setting.second
                    ? DeployableFilesPerProFile::UpdateProFile
                    : DeployableFilesPerProFile::DontUpdateProFile;
            m_updateSettings.insert(setting.first->proFilePath(),
                updateSetting);
            setting.first->setProFileUpdateSetting(updateSetting);
        }
    }

    endResetModel();
    connect(m_target->qt4Project(),
            SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(startTimer(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
}

void DeploymentInfo::createModels(const Qt4ProFileNode *proFileNode)
{
    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case LibraryTemplate:
    case AuxTemplate: {
        DeployableFilesPerProFile::ProFileUpdateSetting updateSetting;
        if (proFileNode->projectType() == AuxTemplate) {
            updateSetting = DeployableFilesPerProFile::DontUpdateProFile;
        } else {
            UpdateSettingsMap::ConstIterator it
                = m_updateSettings.find(proFileNode->path());
            updateSetting = it != m_updateSettings.end()
                ? it.value() : DeployableFilesPerProFile::AskToUpdateProFile;
        }
        DeployableFilesPerProFile *const newModel
            = new DeployableFilesPerProFile(m_target, proFileNode, updateSetting, this);
        m_listModels << newModel;
        break;
    }
    case SubDirsTemplate: {
        const QList<ProjectExplorer::ProjectNode *> &subProjects
            = proFileNode->subProjectNodes();
        foreach (const ProjectExplorer::ProjectNode *subProject, subProjects) {
            const Qt4ProFileNode * const qt4SubProject
                = qobject_cast<const Qt4ProFileNode *>(subProject);
            if (qt4SubProject && !qt4SubProject->path()
                .endsWith(QLatin1String(".pri")))
                createModels(qt4SubProject);
        }
    }
    default:
        break;
    }
}

void DeploymentInfo::setUnmodified()
{
    foreach (DeployableFilesPerProFile *model, m_listModels)
        model->setUnModified();
}

bool DeploymentInfo::isModified() const
{
    foreach (const DeployableFilesPerProFile *model, m_listModels) {
        if (model->isModified())
            return true;
    }
    return false;
}

int DeploymentInfo::deployableCount() const
{
    int count = 0;
    foreach (const DeployableFilesPerProFile *model, m_listModels)
        count += model->rowCount();
    return count;
}

DeployableFile DeploymentInfo::deployableAt(int i) const
{
    foreach (const DeployableFilesPerProFile *model, m_listModels) {
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
    foreach (const DeployableFilesPerProFile *model, m_listModels) {
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
    const DeployableFilesPerProFile *const model = m_listModels.at(index.row());
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

} // namespace RemoteLinux
