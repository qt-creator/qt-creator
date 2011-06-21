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

#include "remotelinuxruncontrolfactory.h"

#include "remotelinuxdebugsupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxruncontrol.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxRunControlFactory::RemoteLinuxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

RemoteLinuxRunControlFactory::~RemoteLinuxRunControlFactory()
{
}

bool RemoteLinuxRunControlFactory::canRun(RunConfiguration *runConfiguration,
    const QString &mode) const
{
    if (mode != QLatin1String(ProjectExplorer::Constants::RUNMODE)
        && mode != QLatin1String(Debugger::Constants::DEBUGMODE))
        return false;

    if (!runConfiguration->isEnabled()
            || !runConfiguration->id().startsWith(RemoteLinuxRunConfiguration::Id)) {
        return false;
    }

    const RemoteLinuxRunConfiguration * const remoteRunConfig
        = qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration);
    if (mode == QLatin1String(Debugger::Constants::DEBUGMODE))
        return remoteRunConfig->portsUsedByDebuggers() <= remoteRunConfig->freePorts().count();
    return true;
}

RunControl* RemoteLinuxRunControlFactory::create(RunConfiguration *runConfig,
    const QString &mode)
{
    Q_ASSERT(canRun(runConfig, mode));

    RemoteLinuxRunConfiguration *rc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return new RemoteLinuxRunControl(rc);

    const DebuggerStartParameters params
        = AbstractRemoteLinuxDebugSupport::startParameters(rc);
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc);
    if (!runControl)
        return 0;
    RemoteLinuxDebugSupport *debugSupport =
        new RemoteLinuxDebugSupport(rc, runControl->engine());
    connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
    return runControl;
}

QString RemoteLinuxRunControlFactory::displayName() const
{
    return tr("Run on remote Linux device");
}

RunConfigWidget *RemoteLinuxRunControlFactory::createConfigurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
