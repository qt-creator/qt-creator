// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdebugsupport.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyosrunconfiguration.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/qtcprocess.h>

#include <QtTaskTree/qbarriertask.h>

#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>

using namespace std::chrono_literals;

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace HarmonyOs::Internal {

// The device refuses to let anything be launched under a debugger, so the
// application starts the debug server itself and this attaches to it.
static DebuggerRunParameters debuggerRunParameters(RunControl *runControl)
{
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setStartMode(AttachToRemoteServer);
    rp.setSkipDebugServer(true);
    // The application starts a platform, not a bare server, so that the debugger can ask
    // it what the process has mapped: nothing else on this device can say.
    rp.setLldbPlatform("remote-ohos");
    rp.setUseContinueInsteadOfRun(true);
    rp.setRemoteChannel(QString("127.0.0.1:%1").arg(Constants::HARMONYOS_DEBUG_PORT));

    if (BuildConfiguration * const bc = runControl->buildConfiguration()) {
        // The application is a module the platform loads, so that is where its symbols are.
        const FilePath application
            = bc->buildDirectory().pathAppended("lib" + bc->activeBuildKey() + ".so");
        if (application.exists()) {
            rp.setInferiorExecutable(application);
            rp.setSymbolFile(application);
        }

        FilePaths solibSearchPath = {bc->buildDirectory(),
                                     bc->buildDirectory().pathAppended(
                                         "harmonyos-build/entry/libs/arm64-v8a")};
        if (QtSupport::QtVersion * const qt = QtSupport::QtKitAspect::qtVersion(runControl->kit()))
            solibSearchPath.append(qt->qtSoPaths());
        FilePath::removeDuplicates(solibSearchPath);
        rp.setSolibSearchPath(solibSearchPath);
    }
    return rp;
}

class HarmonyOsDebugWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsDebugWorkerFactory()
    {
        setId("HarmonyOsDebugWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
            if (hdc.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("No HarmonyOS SDK is configured; cannot debug on the device."));
            }

            QString serial;
            if (auto device = std::dynamic_pointer_cast<const HarmonyOsDevice>(runControl->device()))
                serial = device->serialNumber();

            const auto command = [hdc, serial](const QStringList &args) {
                CommandLine cmd{hdc};
                if (!serial.isEmpty())
                    cmd.addArgs({"-t", serial});
                cmd.addArgs(args);
                return cmd;
            };

            BuildConfiguration * const bc = runControl->buildConfiguration();
            QTC_ASSERT(bc, return runControl->errorTask(
                                Tr::tr("No build configuration; cannot debug on the device.")));
            const QString bundle = bundleName(bc->buildDirectory(), bc->activeBuildKey());
            if (bundle.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("Could not determine the application bundle name. "
                           "Build and deploy the package first."));
            }

            // Nothing starts the application in debug mode, and the debugger can only attach
            // once it runs and has started the server, so this launches it and waits for it.
            const Storage<QString> pidStorage;
            // How a launch is told it is being debugged. The library a debug build carries
            // asks the device's own loopback, because nothing reaches an application this
            // early: the framework passes no environment and the parameter store is
            // root-only. A reverse forward puts this server on the device's loopback, so a
            // connect that succeeds there is the answer.
            const Storage<std::unique_ptr<QTcpServer>> gateStorage;

            const auto openGate = [gateStorage] {
                auto server = std::make_unique<QTcpServer>();
                if (!server->listen(QHostAddress::LocalHost))
                    return false;
                QTcpServer * const gate = server.get();
                QObject::connect(gate, &QTcpServer::newConnection,
                                 gate, [gate] {
                    // The connect is the whole message; nothing is sent over it.
                    while (QTcpSocket * const socket = gate->nextPendingConnection())
                        socket->deleteLater();
                });
                *gateStorage = std::move(server);
                return true;
            };
            // A forward outlives the run that made it when Qt Creator does not get to clean
            // up, and the port on the device is the one the library was built to ask, so a
            // leftover has to go before this run can claim it. Which host port it points at
            // is only in the list.
            const Storage<QString> staleStorage;

            const auto onGateListSetup = [command](Process &process) {
                process.setCommand(command({"fport", "ls"}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };
            const auto onGateListDone = [staleStorage](const Process &process) {
                const QString gate = QString("tcp:%1").arg(Constants::HARMONYOS_GATE_PORT);
                const QStringList lines
                    = process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (!line.contains("[Reverse]"))
                        continue;
                    const QStringList fields = line.simplified().split(' ');
                    const qsizetype at = fields.indexOf(gate);
                    if (at >= 0 && at + 1 < fields.size())
                        *staleStorage = fields.at(at + 1);
                }
            };
            const auto onGateDropSetup = [command, staleStorage](Process &process) {
                if (staleStorage->isEmpty())
                    return SetupResult::StopWithSuccess;
                const QString gate = QString("tcp:%1").arg(Constants::HARMONYOS_GATE_PORT);
                process.setCommand(command({"fport", "rm", gate, *staleStorage}));
                process.setEnvironment(Sdk::hdcEnvironment());
                return SetupResult::Continue;
            };

            const auto onGateForwardSetup = [command, gateStorage](Process &process) {
                if (!*gateStorage)
                    return SetupResult::StopWithSuccess;
                process.setCommand(command({"rport",
                                            QString("tcp:%1").arg(Constants::HARMONYOS_GATE_PORT),
                                            QString("tcp:%1").arg((*gateStorage)->serverPort())}));
                process.setEnvironment(Sdk::hdcEnvironment());
                return SetupResult::Continue;
            };
            const auto onGateForwardDone = [runControl](const Process &process) {
                // Without the gate the application does not wait, which only means that
                // the debugger arrives after it has started.
                if (process.allOutput().contains("[Fail]")) {
                    runControl->postMessage(
                        Tr::tr("Could not tell the device that this launch is being debugged: "
                               "%1").arg(process.allOutput().trimmed()), ErrorMessageFormat);
                }
            };

            // An application that still runs is brought to the front rather than started
            // again, and the debugger would attach to one that passed main() long ago.
            const auto onStopSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "aa", "force-stop", bundle}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };

            const auto onStartSetup = [command, bundle](Process &process) {
                const QString spec = QString("%1:%2").arg(Constants::HARMONYOS_DEBUG_PLUGIN)
                                         .arg(Constants::HARMONYOS_DEBUG_PORT);
                process.setCommand(command({"shell", "aa", "start",
                                            "-a", Constants::HARMONYOS_ABILITY_NAME,
                                            "-b", bundle,
                                            "-m", Constants::HARMONYOS_MODULE_NAME,
                                            "--ps", "io.qt.appArgsJson",
                                            // hdc runs this through a shell on the device,
                                            // which would eat the quotes JSON needs.
                                            QString(R"('["-plugin","%1"]')").arg(spec)}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };

            const auto onPidSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "pidof", "-s", bundle}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };
            const auto onPidDone = [pidStorage](const Process &process) {
                *pidStorage = process.cleanedStdOut().trimmed();
                return toDoneResult(!pidStorage->isEmpty());
            };

            // The application starts the platform a little after its own process appears, and
            // attaching before it listens fails, so the port is waited for, not the process.
            const auto onListeningSetup = [command](Process &process) {
                process.setCommand(command({"shell", "netstat", "-ln"}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };
            const auto onListeningDone = [](const Process &process) {
                const QString port = QString(":%1").arg(Constants::HARMONYOS_DEBUG_PORT);
                const QStringList lines = process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (line.contains(port) && line.contains("LISTEN"))
                        return toDoneResult(true);
                }
                return toDoneResult(false);
            };

            const QString forward = QString("tcp:%1").arg(Constants::HARMONYOS_DEBUG_PORT);
            const auto onForwardSetup = [command, forward](Process &process) {
                process.setCommand(command({"fport", forward, forward}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };
            const auto onForwardDone = [runControl](const Process &process) {
                // hdc reports a refused forward in its output, not in its exit code.
                if (process.allOutput().contains("[Fail]")) {
                    runControl->postMessage(process.allOutput().trimmed(), ErrorMessageFormat);
                    return false;
                }
                return true;
            };

            // The engine attaches to a process, so it needs the pid the launch produced.
            const auto setAttachPid = [pidStorage](DebuggerRunParameters &rp) {
                rp.setAttachPid(ProcessHandle(pidStorage->toLongLong()));
            };

            const QString gate = QString("tcp:%1").arg(Constants::HARMONYOS_GATE_PORT);

            return Group {
                pidStorage,
                gateStorage,
                Group {
                    finishAllAndSuccess,
                    staleStorage,
                    QSyncTask(openGate),
                    ProcessTask(onGateListSetup, onGateListDone),
                    ProcessTask(onGateDropSetup),
                    ProcessTask(onGateForwardSetup, onGateForwardDone)
                },
                ProcessTask(onStopSetup, [](const Process &) { return DoneResult::Success; }),
                ProcessTask(onStartSetup),
                Forever {
                    stopOnSuccess,
                    ProcessTask(onPidSetup, onPidDone),
                    timeoutTask(500ms)
                }.withTimeout(10s),
                Group {
                    Forever {
                        stopOnSuccess,
                        ProcessTask(onListeningSetup, onListeningDone),
                        timeoutTask(500ms)
                    }.withTimeout(30s),
                    onGroupDone([runControl](DoneWith result) {
                        if (result != DoneWith::Success) {
                            runControl->postMessage(
                                Tr::tr("The application did not start a debug server on port %1.")
                                    .arg(Constants::HARMONYOS_DEBUG_PORT),
                                ErrorMessageFormat);
                        }
                    })
                },
                ProcessTask(onForwardSetup, onForwardDone),
                debuggerRecipe(runControl, debuggerRunParameters(runControl), setAttachPid),
                onGroupDone([command, forward, gate, gateStorage] {
                    Process::startDetached(command({"fport", "rm", forward, forward}));
                    if (*gateStorage) {
                        Process::startDetached(command(
                            {"fport", "rm", gate,
                             QString("tcp:%1").arg((*gateStorage)->serverPort())}));
                    }
                })
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::HARMONYOS_RUNCONFIG_ID);
        addSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
    }
};

// A binary a project on the device built cannot be debugged where it runs: the terminal
// application that compiles it may not be traced at all, and Qt Creator may not execute a
// file it wrote itself. Inside Qt Creator's own application both are allowed, so a program
// from its package maps that binary into itself, and the server that traces it is started
// there as well.
class HostGate
{
public:
    std::unique_ptr<QTcpServer> server;
    std::unique_ptr<QTcpSocket> socket;
    QByteArray report;
    qint64 pid = 0;
    QString slide;
};

static Group hostRecipe(const QStoredBarrier &barrier, RunControl *runControl,
                        const Storage<HostGate> &gateStorage)
{
    const auto onGateSetup = [gateStorage, barrier, runControl] {
        HostGate * const gate = gateStorage.activeStorage();
        QBarrier * const reported = barrier.activeStorage();

        auto listener = new QTcpServer;
        gate->server.reset(listener);
        QObject::connect(listener, &QTcpServer::newConnection, listener,
                         [listener, gate, reported, runControl] {
            QTcpSocket * const socket = listener->nextPendingConnection();
            QTC_ASSERT(socket, return);
            listener->close();
            gate->socket.reset(socket);
            // Written once the debugger holds the process, which is when breakpoints in it
            // can be hit; until then the program waits on this connection.
            QObject::connect(runControl, &RunControl::started, socket, [gate] {
                gate->socket->write("g");
                gate->socket->flush();
            });
            QObject::connect(socket, &QIODevice::readyRead, socket, [gate, reported] {
                gate->report += gate->socket->readAll();
                if (!gate->report.contains('\n'))
                    return;
                static const QRegularExpression report("pid (\\d+) slide (0x[0-9a-f]+)");
                const QRegularExpressionMatch match = report.match(QString::fromLatin1(gate->report));
                if (!match.hasMatch())
                    return;
                gate->pid = match.captured(1).toLongLong();
                gate->slide = match.captured(2);
                reported->advance();
            });
        });
        // Any free port: the program is told which one, so nothing has to agree on it
        // beforehand, and two runs on one device do not collide.
        if (!listener->listen(QHostAddress::LocalHost)) {
            runControl->postMessage(
                Tr::tr("Cannot listen for the hosted binary: %1").arg(listener->errorString()),
                ErrorMessageFormat);
            return false;
        }
        listener->setMaxPendingConnections(1);
        return true;
    };

    const auto onServerSetup = [](Process &process) {
        process.setCommand({FilePath::fromString(Constants::HARMONYOS_DEBUG_SERVER_PATH),
                            {"platform", "--listen",
                             QString("*:%1").arg(Constants::HARMONYOS_DEBUG_PORT)}});
    };
    const auto onServerDone = [runControl](const Process &process) {
        runControl->postMessage(Tr::tr("The debug server stopped: %1")
                                    .arg(process.exitMessage()), ErrorMessageFormat);
    };

    // Attaching before the server listens fails, and it says nothing when it is ready.
    const auto isListening = [] {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, Constants::HARMONYOS_DEBUG_PORT);
        return socket.waitForConnected(50);
    };

    const auto onHostSetup = [runControl, gateStorage](Process &process) {
        const CommandLine inferior = runControl->commandLine();
        CommandLine cmd{FilePath::fromString(Constants::HARMONYOS_HOST_PATH),
                        {QString::number(gateStorage->server->serverPort()),
                         inferior.executable().path()}};
        cmd.addArgs(inferior.arguments(), CommandLine::Raw);
        process.setCommand(cmd);
        process.setWorkingDirectory(FilePath::fromString(runControl->workingDirectory().path()));
    };

    return Group {
        QSyncTask(onGateSetup),
        Group {
            parallel,
            ProcessTask(onServerSetup, onServerDone, CallDoneFlag::OnError),
            Group {
                Forever {
                    stopOnSuccess,
                    QSyncTask(isListening),
                    timeoutTask(100ms)
                }.withTimeout(10s),
                runControl->processTaskWithModifier(onHostSetup)
            }
        }
    };
}

static DebuggerRunParameters hostDebuggerRunParameters(RunControl *runControl)
{
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setStartMode(AttachToRemoteServer);
    rp.setSkipDebugServer(true);
    rp.setCloseMode(KillAtClose);
    rp.setLldbPlatform("remote-ohos");
    rp.setUseContinueInsteadOfRun(true);
    rp.setContinueAfterAttach(false);
    rp.setRemoteChannel(QString("127.0.0.1:%1").arg(Constants::HARMONYOS_DEBUG_PORT));
    // The debugger runs where the terminal that built the binary does, which is not where
    // the process is, so the path is what both see and not a device path.
    rp.setSymbolFile(FilePath::fromString(rp.inferior().command.executable().path()));
    return rp;
}

class HarmonyOsBuildDeviceDebugWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsBuildDeviceDebugWorkerFactory()
    {
        setId("HarmonyOsBuildDeviceDebugWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath host = FilePath::fromString(Constants::HARMONYOS_HOST_PATH);
            if (!host.isExecutableFile()) {
                return runControl->errorTask(
                    Tr::tr("Debugging what was built on the device needs \"%1\", which only a "
                           "Qt Creator installed on the device has.").arg(host.toUserOutput()));
            }

            const Storage<HostGate> gateStorage;
            const auto kicker = [runControl, gateStorage](const QStoredBarrier &barrier) {
                return hostRecipe(barrier, runControl, gateStorage);
            };
            // Neither is known before the program reports where it mapped the binary.
            const auto modifier = [gateStorage](DebuggerRunParameters &rp) {
                rp.setAttachPid(ProcessHandle(gateStorage->pid));
                const FilePath binary = rp.symbolFile();
                rp.setCommandsAfterConnect(
                    QString("target modules add %1\ntarget modules load --file %2 --slide %3")
                        .arg(binary.path(), binary.fileName(), gateStorage->slide));
            };
            return Group {
                gateStorage,
                When (kicker) >> Do {
                    debuggerRecipe(runControl, hostDebuggerRunParameters(runControl), modifier)
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::HARMONYOS_BUILD_RUNCONFIG_ID);
        addSupportedDeviceType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
    }
};

void setupHarmonyOsDebugSupport()
{
    static HarmonyOsDebugWorkerFactory theHarmonyOsDebugWorkerFactory;
    static HarmonyOsBuildDeviceDebugWorkerFactory theHarmonyOsBuildDeviceDebugWorkerFactory;
}

} // namespace HarmonyOs::Internal
