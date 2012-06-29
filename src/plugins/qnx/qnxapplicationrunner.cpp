/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxapplicationrunner.h"
#include "qnxrunconfiguration.h"
#include "qnxconstants.h"

#include <remotelinux/remotelinuxusedportsgatherer.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxApplicationRunner::QnxApplicationRunner(QnxRunConfiguration *runConfig, QObject *parent)
    : RemoteLinux::AbstractRemoteLinuxApplicationRunner(runConfig, parent)
    , m_debugMode(false)
{
    usedPortsGatherer()->setCommand(QLatin1String(Constants::QNX_PORT_GATHERER_COMMAND));
}

void QnxApplicationRunner::setDebugMode(bool debugMode)
{
    m_debugMode = debugMode;
}

void QnxApplicationRunner::doDeviceSetup()
{
    handleDeviceSetupDone(true);
}

void QnxApplicationRunner::doAdditionalInitialCleanup()
{
    handleInitialCleanupDone(true);
}

void QnxApplicationRunner::doAdditionalInitializations()
{
    handleInitializationsDone(true);
}

void QnxApplicationRunner::doPostRunCleanup()
{
    handlePostRunCleanupDone();
}

void QnxApplicationRunner::doAdditionalConnectionErrorHandling()
{
}

QString QnxApplicationRunner::killApplicationCommandLine() const
{
    QString executable = m_debugMode ? QLatin1String(Constants::QNX_DEBUG_EXECUTABLE) : remoteExecutable();
    executable.replace(QLatin1String("/"), QLatin1String("\\/"));
    return QString::fromLatin1("for PID in $(ps -f -o pid,comm | grep %1 | awk '/%1/ {print $1}'); "
                                          "do "
                                              "kill $PID; sleep 1; kill -9 $PID; "
                                          "done").arg(executable);
}
