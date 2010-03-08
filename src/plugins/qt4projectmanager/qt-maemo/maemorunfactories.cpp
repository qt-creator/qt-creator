/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "maemorunfactories.h"

#include "maemoconstants.h"
#include "maemomanager.h"
#include "maemorunconfiguration.h"
#include "maemoruncontrol.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <qt4projectmanager/qt4project.h>

namespace Qt4ProjectManager {
namespace Internal {

using namespace ProjectExplorer;

namespace {

QString targetFromId(const QString &id)
{
    if (!id.startsWith(MAEMO_RC_ID_PREFIX))
        return QString();
    return id.mid(QString(MAEMO_RC_ID_PREFIX).size());
}

} // namespace

MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    connect(explorer->session(), SIGNAL(projectAdded(ProjectExplorer::Project*)),
        this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(explorer->session(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
        this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(explorer->session(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)), this,
        SLOT(projectChanged(ProjectExplorer::Project*)));
}

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canCreate(Target *parent,
    const QString &id) const
{
    Qt4Target *target = qobject_cast<Qt4Target *>(parent);
    if (!target
        || target->id() != QLatin1String(MAEMO_DEVICE_TARGET_ID)) {
        return false;
    }
    return id == MAEMO_RC_ID || id.startsWith(MAEMO_RC_ID_PREFIX);
}

bool MaemoRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

bool MaemoRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QStringList MaemoRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (Qt4Target *t = qobject_cast<Qt4Target *>(parent)) {
        if (t->id() == QLatin1String(MAEMO_DEVICE_TARGET_ID)) {
            return t->qt4Project()->
                applicationProFilePathes(QLatin1String(MAEMO_RC_ID_PREFIX));
        }
    }
    return QStringList();
}

QString MaemoRunConfigurationFactory::displayNameForId(const QString &id) const
{
    const QString &target = targetFromId(id);
    if (target.isEmpty())
        return QString();
    return tr("%1 on Maemo Device").arg(QFileInfo(target).completeBaseName());
}

RunConfiguration *MaemoRunConfigurationFactory::create(Target *parent,
    const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4Target *pqt4parent = static_cast<Qt4Target *>(parent);
    return new MaemoRunConfiguration(pqt4parent, targetFromId(id));

}

RunConfiguration *MaemoRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Target *target = static_cast<Qt4Target *>(parent);
    MaemoRunConfiguration *rc = new MaemoRunConfiguration(target, QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *MaemoRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    MaemoRunConfiguration *old = static_cast<MaemoRunConfiguration *>(source);
    return new MaemoRunConfiguration(static_cast<Qt4Target *>(parent), old);
}

void MaemoRunConfigurationFactory::projectAdded(ProjectExplorer::Project *project)
{
    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetAdded(ProjectExplorer::Target*)));
    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetRemoved(ProjectExplorer::Target*)));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(targetChanged(ProjectExplorer::Target*)));

    foreach (Target *target, project->targets())
        targetAdded(target);
}

void MaemoRunConfigurationFactory::projectRemoved(ProjectExplorer::Project *project)
{
    disconnect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetAdded(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetRemoved(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(targetChanged(ProjectExplorer::Target*)));

    foreach (Target *target, project->targets())
        targetRemoved(target);
}

void MaemoRunConfigurationFactory::projectChanged(ProjectExplorer::Project *project)
{
    if (project)
        updateMaemoEmulatorStarter(project->target(QLatin1String(MAEMO_DEVICE_TARGET_ID)));
}

void MaemoRunConfigurationFactory::targetAdded(ProjectExplorer::Target *target)
{
    if (!target || target->id() != QLatin1String(MAEMO_DEVICE_TARGET_ID))
        return;

    MaemoManager::instance().addQemuSimulatorStarter(target->project());
    foreach (RunConfiguration *runConfig, target->runConfigurations()) {
        if (MaemoRunConfiguration *mrc = qobject_cast<MaemoRunConfiguration *> (runConfig)) {
            connect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
                this, SLOT(targetChanged(ProjectExplorer::Target*)));
        }
    }
    updateMaemoEmulatorStarter(target);
}

void MaemoRunConfigurationFactory::targetRemoved(ProjectExplorer::Target *target)
{
    if (!target || target->id() != QLatin1String(MAEMO_DEVICE_TARGET_ID))
        return;

    MaemoManager::instance().removeQemuSimulatorStarter(target->project());
    foreach (RunConfiguration *runConfig, target->runConfigurations()) {
        if (MaemoRunConfiguration *mrc = qobject_cast<MaemoRunConfiguration *> (runConfig)) {
            disconnect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
                this, SLOT(targetChanged(ProjectExplorer::Target*)));
        }
    }
    updateMaemoEmulatorStarter(target);
}

void MaemoRunConfigurationFactory::targetChanged(ProjectExplorer::Target *target)
{
    if (target)
        updateMaemoEmulatorStarter(target);
}

void MaemoRunConfigurationFactory::updateMaemoEmulatorStarter(Target *target) const
{
    bool enable = false;
    bool isRunning = false;

    if (target) {
        MaemoRunConfiguration *mrc = 0;
        if (target->project()->activeTarget() == target)
            mrc = qobject_cast<MaemoRunConfiguration *> (target->activeRunConfiguration());

        if (mrc) {
            isRunning = mrc->isQemuRunning();
            const MaemoDeviceConfig &config = mrc ? mrc->deviceConfig() : MaemoDeviceConfig();

            ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
            if (config.isValid()
                && !mrc->simulatorPath().isEmpty()
                && config.type == MaemoDeviceConfig::Simulator
                && explorer->startupProject() == target->project()) {
                enable = true;
            }
        }
    }

    MaemoManager::instance().updateQemuSimulatorStarter(isRunning);
    MaemoManager::instance().setQemuSimulatorStarterEnabled(enable);
}


// #pragma mark -- MaemoRunControlFactory


MaemoRunControlFactory::MaemoRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

MaemoRunControlFactory::~MaemoRunControlFactory()
{
}

bool MaemoRunControlFactory::canRun(RunConfiguration *runConfiguration,
    const QString &mode) const
{
    return qobject_cast<MaemoRunConfiguration *>(runConfiguration)
        && (mode == ProjectExplorer::Constants::RUNMODE
        || mode == ProjectExplorer::Constants::DEBUGMODE);
}

RunControl* MaemoRunControlFactory::create(RunConfiguration *runConfig,
    const QString &mode)
{
    MaemoRunConfiguration *rc = qobject_cast<MaemoRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE
        || mode == ProjectExplorer::Constants::DEBUGMODE);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return new MaemoRunControl(rc);
    return new MaemoDebugRunControl(rc);
}

QString MaemoRunControlFactory::displayName() const
{
    return tr("Run on device");
}

QWidget* MaemoRunControlFactory::configurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}

    } // namespace Internal
} // namespace Qt4ProjectManager
