// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosrunconfiguration.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QtTaskTree/qtasktree.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

FilePath deploymentSettings(const FilePath &buildDir, const QString &buildKey);
FilePath generatedProjectDir(const FilePath &buildDir, const QString &buildKey);

QString bundleName(const FilePath &buildDir, const QString &buildKey)
{
    const FilePath appJson = generatedProjectDir(buildDir, buildKey)
                                 .pathAppended("AppScope/app.json5");
    const Result<QByteArray> contents = appJson.fileContents();
    if (!contents)
        return {};
    // The file is JSON5, so drop the comments to not pick up a commented-out entry.
    static const QRegularExpression re("\"bundleName\"\\s*:\\s*\"([^\"]+)\"");
    const QString text = QString::fromUtf8(Utils::removeCommentsFromJson(*contents));
    const QRegularExpressionMatch match = re.match(text);
    return match.hasMatch() ? match.captured(1) : QString();
}

// What harmonydeployqt was told, which is also where it puts what it generates. The file
// sits beside the application target, which is only the top of the build directory when
// the application is the whole project.
FilePath deploymentSettings(const FilePath &buildDir, const QString &buildKey)
{
    const QString name = buildKey + "-harmony-deployment-settings.json";
    const FilePath top = buildDir.pathAppended(name);
    if (top.exists())
        return top;
    const FilePaths found = buildDir.dirEntries(
        FileFilter({name}, DirFilterFlag::Files, DirIteratorFlag::Subdirectories));
    return found.isEmpty() ? FilePath() : found.first();
}

// Where harmonydeployqt generates the HAP project: beside the application target, which is
// the top of the build directory only when the application is the whole project. Qt
// Creator's own application lives in src/app.
FilePath generatedProjectDir(const FilePath &buildDir, const QString &buildKey)
{
    const FilePath settings = deploymentSettings(buildDir, buildKey);
    if (!settings.isEmpty())
        return settings.parentDir().pathAppended("harmonyos-build");
    return buildDir.pathAppended("harmonyos-build");
}

// The library the build produced, which is what a run hands to the runner.
FilePath applicationLibrary(const FilePath &buildDir, const QString &buildKey)
{
    const FilePath settings = deploymentSettings(buildDir, buildKey);
    if (settings.isEmpty())
        return {};
    const Result<QByteArray> contents = settings.fileContents();
    if (!contents)
        return {};
    const QString path = QJsonDocument::fromJson(*contents)
                             .object().value("application-binary").toString();
    return path.isEmpty() ? FilePath() : FilePath::fromUserInput(path);
}

// What the project says its package needs beyond what harmonydeployqt knows about: the
// directories whose contents belong in the package as resource files, and the arguments the
// application has to be started with to find them. Qt Creator itself needs both - see
// src/app/CMakeLists.txt - and nothing else in a HAP can supply them.
HarmonyOsExtras harmonyOsExtras(const FilePath &buildDir, const QString &buildKey)
{
    HarmonyOsExtras extras;
    const FilePath settings = deploymentSettings(buildDir, buildKey);
    if (settings.isEmpty())
        return extras;
    const FilePath file = settings.parentDir().pathAppended(buildKey + "-harmonyos-extras.json");
    const Result<QByteArray> contents = file.fileContents();
    if (!contents)
        return extras;

    const QJsonObject object = QJsonDocument::fromJson(*contents).object();
    for (const QJsonValue &value : object.value("resource-directories").toArray())
        extras.resourceDirectories.append(FilePath::fromUserInput(value.toString()));
    for (const QJsonValue &value : object.value("native-package-files").toArray())
        extras.nativePackageFiles.append(FilePath::fromUserInput(value.toString()));
    for (const QJsonValue &value : object.value("launch-arguments").toArray())
        extras.launchArguments.append(value.toString());
    return extras;
}

class HarmonyOsRunConfiguration final : public RunConfiguration
{
public:
    HarmonyOsRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {}
};

class HarmonyOsBuildDeviceRunConfiguration final : public RunConfiguration
{
public:
    HarmonyOsBuildDeviceRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);
        executable.setLabelText(Tr::tr("Executable on device:"));
        executable.setPlaceHolderText(Tr::tr("Path on the device not set"));

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            executable.setExecutable(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.targetFilePath.parentDir());
        });
    }

    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
};

class HarmonyOsBuildDeviceRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    HarmonyOsBuildDeviceRunConfigurationFactory()
    {
        registerRunConfiguration<HarmonyOsBuildDeviceRunConfiguration>(
            Constants::HARMONYOS_BUILD_RUNCONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
        setDecorateDisplayNames(true);
    }
};

class HarmonyOsBuildDeviceRunWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsBuildDeviceRunWorkerFactory()
    {
        setId("HarmonyOsBuildDeviceRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            return runControl->processRecipe(runControl->processTask());
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::HARMONYOS_BUILD_RUNCONFIG_ID);
        addSupportedDeviceType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
    }
};

class HarmonyOsRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    HarmonyOsRunConfigurationFactory()
    {
        registerRunConfiguration<HarmonyOsRunConfiguration>(Constants::HARMONYOS_RUNCONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setDecorateDisplayNames(true);
    }
};

class HarmonyOsRunWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsRunWorkerFactory()
    {
        setId("HarmonyOsRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
            if (hdc.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("No HarmonyOS SDK is configured; cannot launch on the device."));
            }

            BuildConfiguration * const bc = runControl->buildConfiguration();
            QTC_ASSERT(bc, return runControl->errorTask(
                                Tr::tr("No build configuration; cannot launch on the device.")));

            const QString bundle = bundleName(bc->buildDirectory(), bc->activeBuildKey());
            if (bundle.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("Could not determine the application bundle name. "
                           "Build and deploy the package first."));
            }

            // Target the run device explicitly so the right one is used when several
            // are connected.
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

            // Running without installing: the package holds the runner, and this is the
            // channel it asks on. A reverse forward puts this listener on the device's own
            // loopback, the runner connects to it, and the library the build just produced
            // goes over as a length and that many bytes. Read at connect time, so a rebuild
            // between runs needs no new package.
            const bool viaChannel = settings().runWithoutInstalling();
            const FilePath library = viaChannel
                ? applicationLibrary(bc->buildDirectory(), bc->activeBuildKey()) : FilePath();
            if (viaChannel && library.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("Could not find the application library to hand to the runner. "
                           "Build and deploy the package first."));
            }

            const Storage<std::unique_ptr<QTcpServer>> channelStorage;
            const auto openChannel = [channelStorage, runControl, library] {
                auto server = std::make_unique<QTcpServer>();
                if (!server->listen(QHostAddress::LocalHost)) {
                    runControl->postMessage(
                        Tr::tr("Could not open the channel the runner asks on: %1")
                            .arg(server->errorString()), ErrorMessageFormat);
                    return false;
                }
                QTcpServer * const channel = server.get();
                QObject::connect(channel, &QTcpServer::newConnection, channel,
                                 [channel, runControl, library] {
                    while (QTcpSocket * const socket = channel->nextPendingConnection()) {
                        QObject::connect(socket, &QTcpSocket::disconnected,
                                         socket, &QTcpSocket::deleteLater);
                        // Whatever the runner reports about the handover belongs in the
                        // application's own output.
                        QObject::connect(socket, &QTcpSocket::readyRead, socket,
                                         [socket, runControl] {
                            const QString text = QString::fromUtf8(socket->readAll());
                            for (const QString &line : text.split('\n', Qt::SkipEmptyParts))
                                runControl->postMessage(line, StdOutFormat);
                        });

                        const Result<QByteArray> contents = library.fileContents();
                        if (!contents) {
                            runControl->postMessage(contents.error(), ErrorMessageFormat);
                            socket->disconnectFromHost();
                            continue;
                        }
                        const quint32 size = quint32(contents->size());
                        const char header[4] = {char((size >> 24) & 0xff),
                                                char((size >> 16) & 0xff),
                                                char((size >> 8) & 0xff),
                                                char(size & 0xff)};
                        socket->write(header, sizeof(header));
                        socket->write(*contents);
                        runControl->postMessage(
                            Tr::tr("Handed %1 to the runner (%2 bytes).")
                                .arg(library.fileName()).arg(size), NormalMessageFormat);
                    }
                });
                *channelStorage = std::move(server);
                return true;
            };

            // A forward outlives a run that Qt Creator did not get to clean up after, and
            // the port on the device is the one the runner was built to ask, so a leftover
            // has to go before this run can claim it.
            const Storage<QString> staleStorage;
            const QString channelPort = QString("tcp:%1").arg(Constants::HARMONYOS_CHANNEL_PORT);

            const auto onChannelListSetup = [command](Process &process) {
                process.setCommand(command({"fport", "ls"}));
                process.setEnvironment(Sdk::hdcEnvironment());
            };
            const auto onChannelListDone = [staleStorage, channelPort](const Process &process) {
                const QStringList lines = process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (!line.contains("[Reverse]"))
                        continue;
                    const QStringList fields = line.simplified().split(' ');
                    const qsizetype at = fields.indexOf(channelPort);
                    if (at >= 0 && at + 1 < fields.size())
                        *staleStorage = fields.at(at + 1);
                }
            };
            const auto onChannelDropSetup = [command, staleStorage, channelPort](Process &process) {
                if (staleStorage->isEmpty())
                    return SetupResult::StopWithSuccess;
                process.setCommand(command({"fport", "rm", channelPort, *staleStorage}));
                process.setEnvironment(Sdk::hdcEnvironment());
                return SetupResult::Continue;
            };
            const auto onChannelForwardSetup
                = [command, channelStorage, channelPort](Process &process) {
                if (!*channelStorage)
                    return SetupResult::StopWithSuccess;
                const QString host
                    = QString("tcp:%1").arg((*channelStorage)->serverPort());
                process.setCommand(command({"rport", channelPort, host}));
                process.setEnvironment(Sdk::hdcEnvironment());
                return SetupResult::Continue;
            };
            const auto onChannelForwardDone = [runControl](const Process &process) {
                if (process.allOutput().contains("[Fail]")) {
                    runControl->postMessage(
                        Tr::tr("Could not offer the application to the device: %1")
                            .arg(process.allOutput().trimmed()), ErrorMessageFormat);
                }
            };

            const auto channelGroup = [=] {
                if (!viaChannel)
                    return Group { nullItem };
                return Group {
                    finishAllAndSuccess,
                    staleStorage,
                    QSyncTask(openChannel),
                    ProcessTask(onChannelListSetup, onChannelListDone),
                    ProcessTask(onChannelDropSetup),
                    ProcessTask(onChannelForwardSetup, onChannelForwardDone)
                };
            };

            const auto forceStopTask = [command, bundle] {
                const auto onSetup = [command, bundle](Process &process) {
                    process.setCommand(command({"shell", "aa", "force-stop", bundle}));
                };
                return ProcessTask(onSetup) || successItem;
            };

            // What the application has to be started with. The platform plugin turns this
            // Want parameter into the arguments main() gets, so a HAP needs no launch
            // script and the project's own build is what says which ones it needs.
            const QStringList launchArguments
                = harmonyOsExtras(bc->buildDirectory(), bc->activeBuildKey()).launchArguments;

            const auto onStartSetup = [command, bundle, launchArguments](Process &process) {
                QStringList arguments{"shell", "aa", "start",
                                      "-a", Constants::HARMONYOS_ABILITY_NAME,
                                      "-b", bundle,
                                      "-m", Constants::HARMONYOS_MODULE_NAME};
                if (!launchArguments.isEmpty()) {
                    const QByteArray json = QJsonDocument(
                        QJsonArray::fromStringList(launchArguments)).toJson(QJsonDocument::Compact);
                    // hdc hands the whole line to a shell on the device, which would eat the
                    // quotes the JSON is made of and leave the platform plugin with something
                    // it cannot parse - and an application that dies in onCreate.
                    arguments << "--ps" << "io.qt.appArgsJson"
                              << '\'' + QString::fromUtf8(json) + '\'';
                    // Without this the launch URI arrives as the first argument, which an
                    // application that takes file names on the command line tries to open.
                    arguments << "--pb" << "io.qt.useUriAsArg" << "false";
                }
                process.setCommand(command(arguments));
            };

            // "aa start" returns as soon as the ability was launched, so it cannot stand in
            // for the application's lifetime. Follow the application's hilog output instead:
            // that keeps the run alive, feeds the application output pane, and lets the run
            // be stopped. Polling for the process is still needed because hilog keeps
            // waiting once the process is gone.
            const Storage<QString> pidStorage;

            const auto onPidSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "pidof", "-s", bundle}));
            };
            const auto onPidDone = [pidStorage](const Process &process) {
                *pidStorage = process.cleanedStdOut().trimmed();
                return !pidStorage->isEmpty();
            };

            const auto onLogSetup = [command, pidStorage](Process &process) {
                process.setCommand(command({"shell", "hilog", "-P", *pidStorage}));
            };

            const auto onGoneSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "pidof", "-s", bundle}));
            };
            const auto onGoneDone = [](const Process &process) {
                // Succeeding ends the polling loop, and with it the run.
                return toDoneResult(process.cleanedStdOut().trimmed().isEmpty());
            };

            return Group {
                pidStorage,
                channelStorage,
                channelGroup(),
                forceStopTask(),
                Group {
                    ProcessTask(onStartSetup),
                    ProcessTask(onPidSetup, onPidDone),
                    Group {
                        parallel,
                        stopOnSuccessOrError,
                        runControl->processRecipe(onLogSetup),
                        Forever {
                            stopOnSuccess,
                            ProcessTask(onGoneSetup, onGoneDone),
                            timeoutTask(1s)
                        }
                    }
                }.withCancel([runControl] {
                    return makeObjectSignal(runControl, &RunControl::canceled);
                }),
                forceStopTask(),
                onGroupDone([command, channelPort, channelStorage] {
                    if (*channelStorage) {
                        Process::startDetached(command(
                            {"fport", "rm", channelPort,
                             QString("tcp:%1").arg((*channelStorage)->serverPort())}));
                    }
                })
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::HARMONYOS_RUNCONFIG_ID);
        addSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
    }
};

void setupHarmonyOsRunSupport()
{
    static HarmonyOsRunConfigurationFactory theHarmonyOsRunConfigurationFactory;
    static HarmonyOsRunWorkerFactory theHarmonyOsRunWorkerFactory;
    static HarmonyOsBuildDeviceRunConfigurationFactory theBuildDeviceRunConfigurationFactory;
    static HarmonyOsBuildDeviceRunWorkerFactory theBuildDeviceRunWorkerFactory;
}

} // namespace HarmonyOs::Internal
