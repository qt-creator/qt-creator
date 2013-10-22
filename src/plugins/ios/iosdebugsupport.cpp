/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "iosdebugsupport.h"

#include "iosrunner.h"
#include "iosmanager.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/target.h>
#include <qt4projectmanager/qmakebuildconfiguration.h>
#include <qt4projectmanager/qmakenodes.h>
#include <qt4projectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

RunControl *IosDebugSupport::createDebugRunControl(IosRunConfiguration *runConfig,
                                                   QString *errorMessage)
{
    //Target *target = runConfig->target();
    //Qt4Project *project = static_cast<Qt4Project *>(target->project());

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    //params.displayName = IosManager::packageName(target);
    params.remoteSetupNeeded = true;

    DebuggerRunControl * const debuggerRunControl
        = DebuggerPlugin::createDebugger(params, runConfig, errorMessage);
    new IosDebugSupport(runConfig, debuggerRunControl);
    return debuggerRunControl;
}

IosDebugSupport::IosDebugSupport(IosRunConfiguration *runConfig,
    DebuggerRunControl *runControl)
    : QObject(runControl), m_runControl(runControl),
      m_runner(new IosRunner(this, runConfig, true)),
      m_gdbServerPort(0), m_qmlPort(0)
{

    connect(m_runControl->engine(), SIGNAL(requestRemoteSetup()),
            m_runner, SLOT(start()));
    connect(m_runControl, SIGNAL(finished()),
            m_runner, SLOT(stop()));

    connect(m_runner, SIGNAL(gotGdbSocket(int)),
        SLOT(handleGdbServerFd(int)));
    connect(m_runner, SIGNAL(finished(bool)),
        SLOT(handleRemoteProcessFinished(bool)));

    connect(m_runner, SIGNAL(errorMsg(QString)),
        SLOT(handleRemoteErrorOutput(QString)));
    connect(m_runner, SIGNAL(appOutput(QString)),
        SLOT(handleRemoteOutput(QString)));
}

void IosDebugSupport::handleGdbServerFd(int gdbServerFd)
{
    Q_UNUSED(gdbServerFd);
    QTC_CHECK(false); // to do transfer fd to debugger
    //m_runControl->engine()->notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);
}

void IosDebugSupport::handleRemoteProcessFinished(bool cleanEnd)
{
    if (!cleanEnd && m_runControl)
        m_runControl->showMessage(tr("Run failed unexpectedly."), AppStuff);
}

void IosDebugSupport::handleRemoteOutput(const QString &output)
{
    if (m_runControl) {
        if (m_runControl->engine())
            m_runControl->engine()->showMessage(output, AppOutput);
        else
            m_runControl->showMessage(output, AppOutput);
    }
}

void IosDebugSupport::handleRemoteErrorOutput(const QString &output)
{
    if (m_runControl) {
        if (m_runControl->engine())
            m_runControl->engine()->showMessage(output + QLatin1Char('\n'), AppError);
        else
            m_runControl->showMessage(output + QLatin1Char('\n'), AppError);
    }
}

} // namespace Internal
} // namespace Ios
