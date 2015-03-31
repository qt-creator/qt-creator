/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androiddebugsupport.h"

#include "androidglobal.h"
#include "androidrunner.h"
#include "androidmanager.h"
#include "androidqtsupport.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <QDirIterator>
#include <QTcpServer>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {

static const char * const qMakeVariables[] = {
         "QT_INSTALL_LIBS",
         "QT_INSTALL_PLUGINS",
         "QT_INSTALL_IMPORTS"
};

static QStringList qtSoPaths(QtSupport::BaseQtVersion *qtVersion)
{
    if (!qtVersion)
        return QStringList();

    QSet<QString> paths;
    for (uint i = 0; i < sizeof qMakeVariables / sizeof qMakeVariables[0]; ++i) {
        QString path = qtVersion->qmakeProperty(qMakeVariables[i]);
        if (path.isNull())
            continue;
        QDirIterator it(path, QStringList() << QLatin1String("*.so"), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            paths.insert(it.fileInfo().absolutePath());
        }
    }
    return paths.toList();
}

RunControl *AndroidDebugSupport::createDebugRunControl(AndroidRunConfiguration *runConfig, QString *errorMessage)
{
    Target *target = runConfig->target();

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.displayName = AndroidManager::packageName(target);
    params.remoteSetupNeeded = true;
    params.runConfiguration = runConfig;

    DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
    if (aspect->useCppDebugger()) {
        params.languages |= CppLanguage;
        Kit *kit = target->kit();
        params.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
        params.debuggerCommand = DebuggerKitInformation::debuggerCommand(kit).toString();
        if (ToolChain *tc = ToolChainKitInformation::toolChain(kit))
            params.toolChainAbi = tc->targetAbi();
        params.executable = target->activeBuildConfiguration()->buildDirectory().toString() + QLatin1String("/app_process");
        params.skipExecutableValidation = true;
        params.remoteChannel = runConfig->remoteChannel();
        params.solibSearchPath = AndroidManager::androidQtSupport(target)->soLibSearchPath(target);
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        params.solibSearchPath.append(qtSoPaths(version));
    }
    if (aspect->useQmlDebugger()) {
        params.languages |= QmlLanguage;
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        params.qmlServerAddress = server.serverAddress().toString();
        params.remoteSetupNeeded = true;
        //TODO: Not sure if these are the right paths.
        params.projectSourceDirectory = target->project()->projectDirectory().toString();
        params.projectSourceFiles = target->project()->files(Project::ExcludeGeneratedFiles);
        params.projectBuildDirectory = target->activeBuildConfiguration()->buildDirectory().toString();
    }

    DebuggerRunControl * const debuggerRunControl
        = DebuggerRunControlFactory::doCreate(params, errorMessage);
    new AndroidDebugSupport(runConfig, debuggerRunControl);
    return debuggerRunControl;
}

AndroidDebugSupport::AndroidDebugSupport(AndroidRunConfiguration *runConfig,
    DebuggerRunControl *runControl)
    : QObject(runControl),
      m_runControl(runControl),
      m_runner(new AndroidRunner(this, runConfig, runControl->runMode()))
{
    QTC_ASSERT(runControl, return);

    connect(m_runControl, SIGNAL(finished()),
            m_runner, SLOT(stop()));

    DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
    Q_ASSERT(aspect->useCppDebugger() || aspect->useQmlDebugger());
    Q_UNUSED(aspect)

    connect(m_runControl, &DebuggerRunControl::requestRemoteSetup,
            m_runner, &AndroidRunner::start);

    // FIXME: Move signal to base class and generalize handling.
    connect(m_runControl, &DebuggerRunControl::aboutToNotifyInferiorSetupOk,
            m_runner, &AndroidRunner::handleRemoteDebuggerRunning);

    connect(m_runner, &AndroidRunner::remoteServerRunning,
        [this](const QByteArray &serverChannel, int pid) {
            QTC_ASSERT(m_runControl, return);
            m_runControl->notifyEngineRemoteServerRunning(serverChannel, pid);
        });

    connect(m_runner, &AndroidRunner::remoteProcessStarted,
            this, &AndroidDebugSupport::handleRemoteProcessStarted);

    connect(m_runner, &AndroidRunner::remoteProcessFinished,
        [this](const QString &errorMsg) {
            QTC_ASSERT(m_runControl, return);
            m_runControl->appendMessage(errorMsg, Utils::DebugFormat);
            QMetaObject::invokeMethod(m_runControl, "notifyInferiorExited", Qt::QueuedConnection);
        });

    connect(m_runner, &AndroidRunner::remoteErrorOutput,
        [this](const QByteArray &output) {
            QTC_ASSERT(m_runControl, return);
            m_runControl->showMessage(QString::fromUtf8(output), AppError);
        });

    connect(m_runner, &AndroidRunner::remoteOutput,
        [this](const QByteArray &output) {
            QTC_ASSERT(m_runControl, return);
            m_runControl->showMessage(QString::fromUtf8(output), AppOutput);
        });
}

void AndroidDebugSupport::handleRemoteProcessStarted(int gdbServerPort, int qmlPort)
{
    disconnect(m_runner, &AndroidRunner::remoteProcessStarted,
               this, &AndroidDebugSupport::handleRemoteProcessStarted);
    QTC_ASSERT(m_runControl, return);
    RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = gdbServerPort;
    result.qmlServerPort = qmlPort;
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

} // namespace Internal
} // namespace Android
