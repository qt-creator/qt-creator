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
#include "maemorunfactories.h"

#include "maemoconstants.h"
#include "maemodebugsupport.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"
#include "maemoruncontrol.h"
#include "maemotoolchain.h"
#include "qt4maemotarget.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qt4project.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

namespace {

QString pathFromId(Core::Id id)
{
    QString idStr = QString::fromUtf8(id.name());
    const QString prefix = QLatin1String(MAEMO_RC_ID_PREFIX);
    if (!idStr.startsWith(prefix))
        return QString();
    return idStr.mid(prefix.size());
}

} // namespace

MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    return qobject_cast<Qt4BaseTarget *>(parent)->qt4Project()
        ->hasApplicationProFile(pathFromId(id));
}

bool MaemoRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    Q_UNUSED(parent);
    return qobject_cast<AbstractQt4MaemoTarget *>(parent)
        && QString::fromUtf8(ProjectExplorer::idFromMap(map).name()).startsWith(QLatin1String(MAEMO_RC_ID));
}

bool MaemoRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    const RemoteLinuxRunConfiguration * const rlrc
            = qobject_cast<RemoteLinuxRunConfiguration *>(source);
    QString idStr = QString::fromLatin1(source->id().name()) + QLatin1Char('.') + rlrc->proFilePath();
    return rlrc && canCreate(parent, Core::Id(idStr.toUtf8().constData()));
}

QList<Core::Id> MaemoRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> result;
    if (AbstractQt4MaemoTarget *t = qobject_cast<AbstractQt4MaemoTarget *>(parent)) {
        QStringList proFiles = t->qt4Project()->applicationProFilePathes(QLatin1String(MAEMO_RC_ID_PREFIX));
        foreach (const QString &pf, proFiles)
            result << Core::Id(pf.toUtf8().constData());
    }
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
    return new MaemoRunConfiguration(qobject_cast<AbstractQt4MaemoTarget *>(parent),
        pathFromId(id));
}

RunConfiguration *MaemoRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MaemoRunConfiguration *rc
        = new MaemoRunConfiguration(qobject_cast<AbstractQt4MaemoTarget *>(parent), QString());
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
    return new MaemoRunConfiguration(static_cast<AbstractQt4MaemoTarget *>(parent), old);
}

// #pragma mark -- MaemoRunControlFactory

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
        return new MaemoRunControl(rc);

    const DebuggerStartParameters params
        = AbstractRemoteLinuxDebugSupport::startParameters(rc);
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc);
    if (!runControl)
        return 0;
    MaemoDebugSupport *debugSupport = new MaemoDebugSupport(rc, runControl->engine());
    connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
    return runControl;
}

QString MaemoRunControlFactory::displayName() const
{
    return tr("Run on device");
}

RunConfigWidget *MaemoRunControlFactory::createConfigurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}

    } // namespace Internal
} // namespace Madde
