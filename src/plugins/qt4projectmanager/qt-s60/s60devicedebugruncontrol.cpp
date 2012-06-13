/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60devicedebugruncontrol.h"

#include "codaruncontrol.h"
#include "qt4symbiantarget.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "symbianidevice.h"

#include <coreplugin/icore.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;

// Return symbol file which should co-exist with the executable.
// location in debug builds. This can be 'foo.sym' (ABLD) or 'foo.exe.sym' (Raptor)
static inline QString symbolFileFromExecutable(const QString &executable)
{
    // 'foo.exe.sym' (Raptor)
    const QFileInfo raptorSymFi(executable + QLatin1String(".sym"));
    if (raptorSymFi.isFile())
        return raptorSymFi.absoluteFilePath();
    // 'foo.sym' (ABLD)
    const int lastDotPos = executable.lastIndexOf(QLatin1Char('.'));
    if (lastDotPos != -1) {
        const QString symbolFileName = executable.mid(0, lastDotPos) + QLatin1String(".sym");
        const QFileInfo symbolFileNameFi(symbolFileName);
        if (symbolFileNameFi.isFile())
            return symbolFileNameFi.absoluteFilePath();
    }
    return QString();
}

// Create start parameters from run configuration
static Debugger::DebuggerStartParameters s60DebuggerStartParams(const S60DeviceRunConfiguration *rc)
{
    Debugger::DebuggerStartParameters sp;
    QTC_ASSERT(rc, return sp);

    const S60DeployConfiguration *activeDeployConf =
        qobject_cast<S60DeployConfiguration *>(rc->target()->activeDeployConfiguration());
    QTC_ASSERT(activeDeployConf, return sp);

    const QString debugFileName = QString::fromLatin1("%1:\\sys\\bin\\%2.exe")
            .arg(activeDeployConf->installationDrive()).arg(rc->targetName());

    SymbianIDevice::ConstPtr dev = activeDeployConf->device();
    sp.remoteChannel = dev->serialPortName();
    sp.processArgs = rc->commandLineArguments();
    if (rc->debuggerAspect()->useQmlDebugger() && !rc->debuggerAspect()->useCppDebugger()) {
        sp.remoteSetupNeeded = true;
        sp.startMode = Debugger::AttachToRemoteServer;
    } else {
        sp.startMode = Debugger::StartInternal;
    }

    sp.toolChainAbi = rc->abi();
    sp.executable = debugFileName;
    sp.executableUid = rc->executableUid();
    sp.serverAddress = dev->address();
    sp.serverPort = dev->port().toInt();
    sp.displayName = rc->displayName();
    sp.qmlServerAddress = dev->address();
    sp.qmlServerPort = rc->debuggerAspect()->qmlDebugServerPort();
    if (rc->debuggerAspect()->useQmlDebugger()) {
        sp.languages |= Debugger::QmlLanguage;
        QString qmlArgs = rc->qmlCommandLineArguments();
        if (sp.processArgs.length())
            sp.processArgs.prepend(QLatin1Char(' '));
        sp.processArgs.prepend(qmlArgs);
    }
    if (rc->debuggerAspect()->useCppDebugger())
        sp.languages |= Debugger::CppLanguage;

    sp.communicationChannel = dev->communicationChannel() == SymbianIDevice::CommunicationCodaTcpConnection?
                Debugger::DebuggerStartParameters::CommunicationChannelTcpIp:
                Debugger::DebuggerStartParameters::CommunicationChannelUsb;

    if (const ProjectExplorer::Project *project = rc->target()->project()) {
        sp.projectSourceDirectory = project->projectDirectory();
        if (const ProjectExplorer::BuildConfiguration *buildConfig = rc->target()->activeBuildConfiguration()) {
            sp.projectBuildDirectory = buildConfig->buildDirectory();
        }
        sp.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
    }

    QTC_ASSERT(sp.executableUid, return sp);

    // Prefer the '*.sym' file over the '.exe', which should exist at the same
    // location in debug builds. This can be 'foo.exe' (ABLD) or 'foo.exe.sym' (Raptor)
    sp.symbolFileName = symbolFileFromExecutable(rc->localExecutableFileName());
    return sp;
}

S60DeviceDebugRunControl::S60DeviceDebugRunControl(S60DeviceRunConfiguration *rc,
                                                   const Debugger::DebuggerStartParameters &sp,
                                                   const QPair<Debugger::DebuggerEngineType, Debugger::DebuggerEngineType> &masterSlaveEngineTypes) :
    Debugger::DebuggerRunControl(rc, sp, masterSlaveEngineTypes),
    m_codaRunControl(NULL),
    m_codaState(ENotUsingCodaRunControl)
{
    if (startParameters().symbolFileName.isEmpty()) {
        const QString msg = tr("Warning: Cannot locate the symbol file belonging to %1.\n").
                               arg(rc->localExecutableFileName());
        appendMessage(msg, Utils::ErrorMessageFormat);
    }
    if (masterSlaveEngineTypes.first == Debugger::QmlEngineType) {
        connect(engine(), SIGNAL(requestRemoteSetup()), this, SLOT(remoteSetupRequested()));
        connect(engine(), SIGNAL(stateChanged(Debugger::DebuggerState)), this, SLOT(qmlEngineStateChanged(Debugger::DebuggerState)));
    }
}

void S60DeviceDebugRunControl::start()
{
    appendMessage(tr("Launching debugger...\n"), Utils::NormalMessageFormat);
    Debugger::DebuggerRunControl::start();
}

bool S60DeviceDebugRunControl::promptToStop(bool *) const
{
    // We override the settings prompt
    return Debugger::DebuggerRunControl::promptToStop(0);
}

void S60DeviceDebugRunControl::remoteSetupRequested()
{
    // This is called from Engine->setupInferior(), ie InferiorSetupRequested state
    QTC_CHECK(runConfiguration()->debuggerAspect()->useQmlDebugger() && !runConfiguration()->debuggerAspect()->useCppDebugger());
    m_codaRunControl = new CodaRunControl(runConfiguration(), DebugRunMode);
    connect(m_codaRunControl, SIGNAL(connected()), this, SLOT(codaConnected()));
    connect(m_codaRunControl, SIGNAL(finished()), this, SLOT(codaFinished()));
    connect(m_codaRunControl, SIGNAL(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)), this, SLOT(handleMessageFromCoda(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)));
    connect(this, SIGNAL(finished()), this, SLOT(handleDebuggingFinished()));
    m_codaState = EWaitingForCodaConnection;
    m_codaRunControl->connect();
}

void S60DeviceDebugRunControl::codaFinished()
{
    if (m_codaRunControl) {
        m_codaRunControl->deleteLater();
        m_codaRunControl = NULL;
    }
    if (m_codaState == EWaitingForCodaConnection) {
        engine()->notifyEngineRemoteSetupFailed(QLatin1String("CODA failed to initialise")); // TODO sort out this error string? Unlikely we'll ever hit this state anyway.
    } else {
        debuggingFinished();
    }
    m_codaState = ENotUsingCodaRunControl;
}

void S60DeviceDebugRunControl::codaConnected()
{
    QTC_ASSERT(m_codaState == EWaitingForCodaConnection, return);
    m_codaState = ECodaConnected;
    engine()->notifyEngineRemoteSetupDone(-1, -1); // calls notifyInferiorSetupOk()
}

void S60DeviceDebugRunControl::qmlEngineStateChanged(Debugger::DebuggerState state)
{
    if (state == Debugger::EngineRunRequested)
        m_codaRunControl->run();
}

void S60DeviceDebugRunControl::handleDebuggingFinished()
{
    if (m_codaRunControl) {
        m_codaRunControl->stop(); // We'll get a callback to our codaFinished() slot when it's done
    }
}

void S60DeviceDebugRunControl::handleMessageFromCoda(ProjectExplorer::RunControl *aCodaRunControl, const QString &msg, Utils::OutputFormat format)
{
    // This only gets used when QmlEngine is the master debug engine. If GDB is running, messages are handled via the gdb adapter
    Q_UNUSED(aCodaRunControl)
    Q_UNUSED(format)
    engine()->showMessage(msg, Debugger::AppOutput);
}

//

S60DeviceDebugRunControlFactory::S60DeviceDebugRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool S60DeviceDebugRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    return mode == DebugRunMode && qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
}

ProjectExplorer::RunControl* S60DeviceDebugRunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode)
{
    S60DeviceRunConfiguration *rc = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc && mode == DebugRunMode, return 0);
    const Debugger::DebuggerStartParameters startParameters = s60DebuggerStartParams(rc);
    const Debugger::ConfigurationCheck check = Debugger::checkDebugConfiguration(startParameters);
    if (!check) {
        Core::ICore::showWarningWithOptions(S60DeviceDebugRunControl::tr("Debugger for Symbian Platform"),
            check.errorMessage, check.errorDetailsString(), check.settingsCategory, check.settingsPage);
        return 0;
    }
    return new S60DeviceDebugRunControl(rc, startParameters, check.masterSlaveEngineTypes);
}

QString S60DeviceDebugRunControlFactory::displayName() const
{
    return S60DeviceDebugRunControl::tr("Debug on Device");
}
