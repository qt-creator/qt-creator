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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemodeployables.h"

#include "maemoprofilesupdatedialog.h"
#include "qt4maemotarget.h"

#include <profileevaluator.h>
#include <projectexplorer/buildstep.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <QtCore/QTimer>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployables::MaemoDeployables(const AbstractQt4MaemoTarget *target)
    : m_target(target), m_updateTimer(new QTimer(this))
{
    QTimer::singleShot(0, this, SLOT(init()));
    m_updateTimer->setInterval(1500);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(createModels()));
}

MaemoDeployables::~MaemoDeployables() {}

void MaemoDeployables::init()
{
    Qt4Project * const pro = m_target->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            m_updateTimer, SLOT(start()));

    // TODO do we want to disable the view

    createModels();
}

void MaemoDeployables::createModels()
{
    if (m_target->project()->activeTarget() != m_target)
        return;
    const Qt4ProFileNode *const rootNode
        = m_target->qt4Project()->rootProjectNode();
    if (!rootNode) // Happens on project creation by wizard.
        return;
    m_updateTimer->stop();
    disconnect(m_target->qt4Project(),
        SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
        m_updateTimer, SLOT(start()));
    beginResetModel();
    qDeleteAll(m_listModels);
    m_listModels.clear();
    createModels(rootNode);
    QList<MaemoDeployableListModel *> modelsWithoutTargetPath;
    foreach (MaemoDeployableListModel *const model, m_listModels) {
        if (!model->hasTargetPath()) {
            if (model->proFileUpdateSetting() == MaemoDeployableListModel::AskToUpdateProFile)
                modelsWithoutTargetPath << model;
        }
    }

    if (!modelsWithoutTargetPath.isEmpty()) {
        MaemoProFilesUpdateDialog dialog(modelsWithoutTargetPath);
        dialog.exec();
        const QList<MaemoProFilesUpdateDialog::UpdateSetting> &settings
            = dialog.getUpdateSettings();
        foreach (const MaemoProFilesUpdateDialog::UpdateSetting &setting, settings) {
            const MaemoDeployableListModel::ProFileUpdateSetting updateSetting
                = setting.second
                    ? MaemoDeployableListModel::UpdateProFile
                    : MaemoDeployableListModel::DontUpdateProFile;
            m_updateSettings.insert(setting.first->proFilePath(),
                updateSetting);
            setting.first->setProFileUpdateSetting(updateSetting);
        }
    }

    endResetModel();
    connect(m_target->qt4Project(),
            SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            m_updateTimer, SLOT(start()));
}

void MaemoDeployables::createModels(const Qt4ProFileNode *proFileNode)
{
    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case LibraryTemplate:
    case ScriptTemplate: {
        UpdateSettingsMap::ConstIterator it
            = m_updateSettings.find(proFileNode->path());
        const MaemoDeployableListModel::ProFileUpdateSetting updateSetting
            = it != m_updateSettings.end()
                  ? it.value() : MaemoDeployableListModel::AskToUpdateProFile;
        MaemoDeployableListModel *const newModel
            = new MaemoDeployableListModel(proFileNode, updateSetting, this);
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

void MaemoDeployables::setUnmodified()
{
    foreach (MaemoDeployableListModel *model, m_listModels)
        model->setUnModified();
}

bool MaemoDeployables::isModified() const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        if (model->isModified())
            return true;
    }
    return false;
}

int MaemoDeployables::deployableCount() const
{
    int count = 0;
    foreach (const MaemoDeployableListModel *model, m_listModels)
        count += model->rowCount();
    return count;
}

MaemoDeployable MaemoDeployables::deployableAt(int i) const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        Q_ASSERT(i >= 0);
        if (i < model->rowCount())
            return model->deployableAt(i);
        i -= model->rowCount();
    }

    Q_ASSERT(!"Invalid deployable number");
    return MaemoDeployable(QString(), QString());
}

QString MaemoDeployables::remoteExecutableFilePath(const QString &localExecutableFilePath) const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        if (model->localExecutableFilePath() == localExecutableFilePath)
            return model->remoteExecutableFilePath();
    }
    return QString();
}

int MaemoDeployables::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : modelCount();
}

QVariant MaemoDeployables::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= modelCount()
            || index.column() != 0)
        return QVariant();
    const MaemoDeployableListModel *const model = m_listModels.at(index.row());
    if (role == Qt::ForegroundRole && !model->hasTargetPath()) {
        QBrush brush;
        brush.setColor(Qt::red);
        return brush;
    }
    if (role == Qt::DisplayRole)
        return QFileInfo(model->proFilePath()).fileName();
    return QVariant();
}

} // namespace Qt4ProjectManager
} // namespace Internal
