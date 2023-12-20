// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerruncontrol.h"

#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"
#include "appmanagerutilities.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

// AppManagerRunner

class AppManagerRunner : public SimpleTargetRunner
{
public:
    AppManagerRunner(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setId("ApplicationManagerPlugin.Run.TargetRunner");
        connect(this, &RunWorker::stopped, this, [this, runControl] {
            appendMessage(tr("%1 exited").arg(runControl->runnable().command.toUserOutput()),
                          OutputFormat::NormalMessageFormat);
        });

        setStartModifier([this, runControl] {
            const auto targetInformation = TargetInformation(runControl->target());
            if (!targetInformation.isValid())
                return;

            setWorkingDirectory(targetInformation.workingDirectory());
            setCommandLine({FilePath::fromString(getToolFilePath(Constants::APPMAN_CONTROLLER, runControl->kit(),
                                                                 targetInformation.device)),
                            {"start-application", "-eio", targetInformation.manifest.id}});
        });
    }
};


// AppManDebugLauncher

class AppManDebugLauncher : public SimpleTargetRunner
{
public:
    AppManDebugLauncher(RunControl *runControl, Debugger::DebugServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl)
    {
        setId(AppManager::Constants::DEBUG_LAUNCHER_ID);
        QTC_ASSERT(portsGatherer, return);

        setStartModifier([this, runControl, portsGatherer] {

            const auto targetInformation = TargetInformation(runControl->target());
            if (!targetInformation.isValid()) {
                reportFailure();
                return;
            }

            CommandLine cmd{FilePath::fromString(getToolFilePath(Constants::APPMAN_CONTROLLER,
                                                                 runControl->kit(),
                                                                 targetInformation.device))};
            cmd.addArg("debug-application");

            if (portsGatherer->useGdbServer() || portsGatherer->useQmlServer()) {
                QStringList debugArgs;
                if (portsGatherer->useGdbServer()) {
                    debugArgs.append(QString("gdbserver :%1").arg(portsGatherer->gdbServer().port()));
                }
                if (portsGatherer->useQmlServer()) {
                    debugArgs.append(QString("%program% -qmljsdebugger=port:%1,block %arguments%")
                                         .arg(portsGatherer->qmlServer().port()));
                }
                cmd.addArg(debugArgs.join(' '));
            }

            cmd.addArg("-eio");
            cmd.addArg(targetInformation.manifest.id);

            setCommandLine(cmd);
            setWorkingDirectory(targetInformation.workingDirectory());

            appendMessage(tr("Starting AppMan Debugging..."), NormalMessageFormat);
            appendMessage(tr("Using: %1").arg(cmd.toUserOutput()), NormalMessageFormat);
        });
    }
};


// AppManagerDebugSupport

class AppManagerDebugSupport : public Debugger::DebuggerRunTool
{
public:
    AppManagerDebugSupport(RunControl *runControl)
        : DebuggerRunTool(runControl)
    {
        setId("ApplicationManagerPlugin.Debug.Support");

        setUsePortsGatherer(isCppDebugging(), isQmlDebugging());

        auto apmDebugLauncher = new Internal::AppManDebugLauncher(runControl, portsGatherer());
        apmDebugLauncher->addStartDependency(portsGatherer());

        addStartDependency(apmDebugLauncher);

        Target *target = runControl->target();

        const Internal::TargetInformation targetInformation(target);
        if (!targetInformation.isValid())
            return;

        if (targetInformation.manifest.isQmlRuntime()) {
            const Utils::FilePath dir = SysRootKitAspect::sysRoot(target->kit());
            // TODO: get real aspect from deploy configuration
            QString amfolder = Constants::REMOTE_DEFAULT_BIN_PATH;
            m_symbolFile = dir.toString() + amfolder + Constants::APPMAN_LAUNCHER_QML;
        } else if (targetInformation.manifest.isNativeRuntime()) {
            m_symbolFile = Utils::findOrDefault(target->buildSystem()->applicationTargets(), [&](const BuildTargetInfo &ti) {
                               return ti.buildKey == targetInformation.manifest.code || ti.projectFilePath.toString() == targetInformation.manifest.code;
                           }).targetFilePath.toString();
        } else {
            reportFailure(tr("Cannot debug: Only QML and native applications are supported."));
        }
    }

private:
    void start() override;

    QString m_symbolFile;
};

void AppManagerDebugSupport::start()
{
    if (m_symbolFile.isEmpty()) {
        reportFailure(tr("Cannot debug: Local executable is not set."));
        return;
    }

    setStartMode(Debugger::AttachToRemoteServer);
    setCloseMode(Debugger::KillAndExitMonitorAtClose);

    ProcessRunData inferior = runControl()->runnable();

    if (isQmlDebugging())
        setQmlServer(portsGatherer()->qmlServer());

    if (isCppDebugging()) {
        setUseExtendedRemote(false);
        setUseContinueInsteadOfRun(true);
        inferior.command.setArguments({});
        if (isQmlDebugging()) {
            inferior.command.addArg(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                                   portsGatherer()->qmlServer()));
        }
        setRemoteChannel(portsGatherer()->gdbServer());
        setSymbolFile(FilePath::fromString(m_symbolFile));

        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(runControl()->kit());
        if (version) {
            setSolibSearchPath(version->qtSoPaths());
            addSearchDirectory(version->qmlPath());
        }

        auto sysroot = SysRootKitAspect().sysRoot(runControl()->kit());
        if (sysroot.isEmpty())
            setSysRoot("/");
        else
            setSysRoot(sysroot);
    }
    setInferior(inferior);

    DebuggerRunTool::start();
}

// Factories

AppManagerDebugWorkerFactory::AppManagerDebugWorkerFactory()
{
    setProduct<AppManagerDebugSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(Constants::RUNCONFIGURATION_ID);
}

AppManagerRunWorkerFactory::AppManagerRunWorkerFactory()
{
    setProduct<AppManagerRunner>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::RUNCONFIGURATION_ID);
}

} // AppManager::Internal
