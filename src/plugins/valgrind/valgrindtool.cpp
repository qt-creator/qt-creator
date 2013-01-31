/**************************************************************************
**
** Copyright (C) 2013 Kläralvdalens Datakonsult AB, a KDAB Group company.
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
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

#include "valgrindtool.h"

#include <remotelinux/remotelinuxrunconfiguration.h>

#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Valgrind {
namespace Internal {

ValgrindTool::ValgrindTool(QObject *parent) :
    Analyzer::IAnalyzerTool(parent)
{
}

bool ValgrindTool::canRun(RunConfiguration *, RunMode mode) const
{
    return mode == runMode();
}

Analyzer::AnalyzerStartParameters ValgrindTool::createStartParameters(
    RunConfiguration *runConfiguration, RunMode mode) const
{
    Q_UNUSED(mode);

    Analyzer::AnalyzerStartParameters sp;
    sp.displayName = runConfiguration->displayName();
    if (LocalApplicationRunConfiguration *rc1 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp.startMode = Analyzer::StartLocal;
        sp.environment = rc1->environment();
        sp.workingDirectory = rc1->workingDirectory();
        sp.debuggee = rc1->executable();
        sp.debuggeeArgs = rc1->commandLineArguments();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc1->debuggerAspect()->qmlDebugServerPort();
    } else if (RemoteLinuxRunConfiguration *rc2 =
               qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp.startMode = Analyzer::StartRemote;
        sp.debuggee = rc2->remoteExecutableFilePath();
        sp.connParams = ProjectExplorer::DeviceKitInformation::device(rc2->target()->kit())->sshParameters();
        sp.analyzerCmdPrefix = rc2->commandPrefix();
        sp.debuggeeArgs = rc2->arguments();
    } else {
        // Might be S60DeviceRunfiguration, or something else ...
        //sp.startMode = StartRemote;
        sp.startMode = Analyzer::StartRemote;
    }
    return sp;
}

} // namespace Internal
} // namespace Valgrind
