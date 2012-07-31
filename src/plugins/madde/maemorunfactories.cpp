/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#include "maemorunfactories.h"

#include "maemoapplicationrunnerhelperactions.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <remotelinux/remotelinuxruncontrol.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

namespace {

QString pathFromId(Core::Id id)
{
    QString idStr = id.toString();
    const QString prefix = QLatin1String(MAEMO_RC_ID_PREFIX);
    if (!idStr.startsWith(prefix))
        return QString();
    return idStr.mid(prefix.size());
}

} // namespace

MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("MaemoRunConfigurationFactory")); }

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return static_cast<Qt4Project *>(parent->project())->hasApplicationProFile(pathFromId(id));
}

bool MaemoRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    return canHandle(parent)
        && ProjectExplorer::idFromMap(map).toString().startsWith(QLatin1String(MAEMO_RC_ID_PREFIX));
}

bool MaemoRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    const RemoteLinuxRunConfiguration * const rlrc
            = qobject_cast<RemoteLinuxRunConfiguration *>(source);
    QString idStr = QString::fromLatin1(source->id().name()) + QLatin1Char('.') + rlrc->proFilePath();
    return rlrc && canCreate(parent, Core::Id(idStr));
}

QList<Core::Id> MaemoRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;
    QStringList proFiles = static_cast<Qt4Project *>(parent->project())->applicationProFilePathes(QLatin1String(MAEMO_RC_ID_PREFIX));
    foreach (const QString &pf, proFiles)
        result << Core::Id(pf);
    return result;
}

QString MaemoRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName()
        + QLatin1String(" (on remote Maemo device)");
}

RunConfiguration *MaemoRunConfigurationFactory::create(Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new MaemoRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *MaemoRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id id = ProjectExplorer::idFromMap(map);
    MaemoRunConfiguration *rc
        = new MaemoRunConfiguration(parent, id, pathFromId(id));
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *MaemoRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    MaemoRunConfiguration *old = static_cast<MaemoRunConfiguration *>(source);
    return new MaemoRunConfiguration(parent, old);
}

bool MaemoRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    if (!qobject_cast<Qt4Project *>(t->project()))
        return false;
    Core::Id devType = DeviceTypeProfileInformation::deviceTypeId(t->profile());
    return devType == Core::Id(Maemo5OsType) || devType == Core::Id(HarmattanOsType);
}

QList<RunConfiguration *> MaemoRunConfigurationFactory::runConfigurationsForNode(Target *t, Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations())
        if (MaemoRunConfiguration *mrc = qobject_cast<MaemoRunConfiguration *>(rc))
            if (mrc->proFilePath() == n->path())
                result << rc;
    return result;
}


MaemoRunControlFactory::MaemoRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

MaemoRunControlFactory::~MaemoRunControlFactory()
{
}

bool MaemoRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    const MaemoRunConfiguration * const maemoRunConfig
        = qobject_cast<MaemoRunConfiguration *>(runConfiguration);
    if (!maemoRunConfig || !maemoRunConfig->isEnabled())
        return false;
    return maemoRunConfig->hasEnoughFreePorts(mode);
}

RunControl* MaemoRunControlFactory::create(RunConfiguration *runConfig, RunMode mode)
{
    Q_ASSERT(canRun(runConfig, mode));

    MaemoRunConfiguration *rc = qobject_cast<MaemoRunConfiguration *>(runConfig);
    Q_ASSERT(rc);

    if (mode == NormalRunMode)
        return new RemoteLinuxRunControl(rc);

    const DebuggerStartParameters params = LinuxDeviceDebugSupport::startParameters(rc);
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc);
    if (!runControl)
        return 0;
    LinuxDeviceDebugSupport * const debugSupport
            = new LinuxDeviceDebugSupport(rc, runControl->engine());
    const Profile * const profile = runConfig->target()->profile();
    MaemoPreRunAction * const preRunAction = new MaemoPreRunAction(
            DeviceProfileInformation::device(profile), MaemoGlobal::maddeRoot(profile),
            rc->remoteMounts()->mountSpecs(), rc);
    MaemoPostRunAction * const postRunAction = new MaemoPostRunAction(preRunAction->mounter(), rc);
    debugSupport->setApplicationRunnerPreRunAction(preRunAction);
    debugSupport->setApplicationRunnerPostRunAction(postRunAction);
    connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
    return runControl;
}

QString MaemoRunControlFactory::displayName() const
{
    return tr("Run on device");
}

    } // namespace Internal
} // namespace Madde
