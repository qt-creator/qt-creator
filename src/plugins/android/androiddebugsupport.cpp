/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddebugsupport.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunner.h"
#include "androidmanager.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>
#include <QTcpServer>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

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
    Qt4Project *project = static_cast<Qt4Project *>(target->project());

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.displayName = AndroidManager::packageName(target);
    params.remoteSetupNeeded = true;

    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useCppDebugger()) {
        params.languages |= CppLanguage;
        Kit *kit = target->kit();
        params.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
        params.debuggerCommand = DebuggerKitInformation::debuggerCommand(kit).toString();
        if (ToolChain *tc = ToolChainKitInformation::toolChain(kit))
            params.toolChainAbi = tc->targetAbi();
        params.executable = project->rootQt4ProjectNode()->buildDir() + QLatin1String("/app_process");
        params.remoteChannel = runConfig->remoteChannel();
        params.solibSearchPath.clear();
        QList<Qt4ProFileNode *> nodes = project->allProFiles();
        foreach (Qt4ProFileNode *node, nodes)
            if (node->projectType() == ApplicationTemplate)
                params.solibSearchPath.append(node->targetInformation().buildDir);
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
        params.projectSourceDirectory = project->projectDirectory();
        params.projectSourceFiles = project->files(Qt4Project::ExcludeGeneratedFiles);
        params.projectBuildDirectory = project->rootQt4ProjectNode()->buildDir();
    }

    DebuggerRunControl * const debuggerRunControl
        = DebuggerPlugin::createDebugger(params, runConfig, errorMessage);
    new AndroidDebugSupport(runConfig, debuggerRunControl);
    return debuggerRunControl;
}

AndroidDebugSupport::AndroidDebugSupport(AndroidRunConfiguration *runConfig,
    DebuggerRunControl *runControl)
    : AndroidRunSupport(runConfig, runControl),
      m_engine(0)
{
    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    Q_ASSERT(aspect->useCppDebugger() || aspect->useQmlDebugger());
    Q_UNUSED(aspect)

    if (runControl)
        m_engine = runControl->engine();

    if (m_engine) {
        connect(m_engine, SIGNAL(requestRemoteSetup()),
                m_runner, SLOT(start()));
        connect(m_engine, SIGNAL(aboutToNotifyInferiorSetupOk()),
                m_runner, SLOT(handleRemoteDebuggerRunning()));
    }
    connect(m_runner, SIGNAL(remoteServerRunning(QByteArray,int)),
            SLOT(handleRemoteServerRunning(QByteArray,int)));
    connect(m_runner, SIGNAL(remoteProcessStarted(int,int)),
            SLOT(handleRemoteProcessStarted(int,int)));
    connect(m_runner, SIGNAL(remoteProcessFinished(QString)),
            SLOT(handleRemoteProcessFinished(QString)));

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
            SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
            SLOT(handleRemoteOutput(QByteArray)));
}

void AndroidDebugSupport::handleRemoteServerRunning(const QByteArray &serverChannel, int pid)
{
    if (m_engine)
        m_engine->notifyEngineRemoteServerRunning(serverChannel, pid);
}

void AndroidDebugSupport::handleRemoteProcessStarted(int gdbServerPort, int qmlPort)
{
    disconnect(m_runner, SIGNAL(remoteProcessStarted(int,int)),
        this, SLOT(handleRemoteProcessStarted(int,int)));
    if (m_engine)
        m_engine->notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);
}

void AndroidDebugSupport::handleRemoteProcessFinished(const QString &errorMsg)
{
    DebuggerRunControl *runControl = qobject_cast<DebuggerRunControl *>(m_runControl);
    if (runControl)
        runControl->showMessage(errorMsg, AppStuff);
    else
        AndroidRunSupport::handleRemoteProcessFinished(errorMsg);
}

void AndroidDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    if (m_engine) {
        m_engine->showMessage(QString::fromUtf8(output), AppOutput);
    } else {
        DebuggerRunControl *runControl = qobject_cast<DebuggerRunControl *>(m_runControl);
        if (runControl)
            runControl->showMessage(QString::fromUtf8(output), AppOutput);
        else
            AndroidRunSupport::handleRemoteOutput(output);
    }
}

void AndroidDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    if (m_engine) {
        m_engine->showMessage(QString::fromUtf8(output), AppError);
    } else {
        DebuggerRunControl *runControl = qobject_cast<DebuggerRunControl *>(m_runControl);
        if (runControl)
            runControl->showMessage(QString::fromUtf8(output), AppError);
        else
            AndroidRunSupport::handleRemoteErrorOutput(output);
    }
}

} // namespace Internal
} // namespace Android
