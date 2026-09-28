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

#include <utils/algorithm.h>
#include <utils/elfreader.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/qtasktree.h>

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <optional>

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

FilePaths libraryDirectories(const FilePath &deploymentSettings)
{
    const Result<QByteArray> contents = deploymentSettings.fileContents();
    if (!contents)
        return {};
    const QJsonObject object = QJsonDocument::fromJson(*contents).object();

    FilePaths directories;
    const QString qtLibs = object.value("qtLibsDirectory").toString();
    if (!qtLibs.isEmpty())
        directories.append(FilePath::fromUserInput(qtLibs));
    for (const QJsonValue &value : object.value("extra-libs-dirs").toArray())
        directories.append(FilePath::fromUserInput(value.toString()));
    return directories;
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
    for (const QJsonValue &value : object.value("launch-schemes").toArray())
        extras.launchSchemes.append(value.toString());
    extras.deviceToolchain = object.value("device-toolchain").toBool();
    return extras;
}

// Lengths no application can have, so what follows one of them is not one.
constexpr quint32 argumentsAnnouncement = 0xffffffff;
constexpr quint32 qtAnnouncement = 0xfffffffe;

static void announce(QTcpSocket *socket, quint32 size)
{
    const char header[4] = {char((size >> 24) & 0xff), char((size >> 16) & 0xff),
                            char((size >> 8) & 0xff), char(size & 0xff)};
    socket->write(header, sizeof(header));
}

static void announceText(QTcpSocket *socket, const QByteArray &text)
{
    announce(socket, quint32(text.size()));
    socket->write(text);
}

FilePath findLibrary(const QString &name, const FilePaths &directories)
{
    const qsizetype versioned = name.indexOf(".so.");
    for (const FilePath &directory : directories) {
        const FilePath exact = directory.pathAppended(name);
        if (exact.isFile())
            return exact;
        if (versioned < 0)
            continue;
        // The file behind the name, for where the link to it cannot be read, as on the
        // device's shared storage.
        const FilePaths full
            = directory.dirEntries(FileFilter({name + ".*"}, DirFilterFlag::Files));
        if (!full.isEmpty())
            return full.first();
        const FilePath unversioned = directory.pathAppended(name.left(versioned + 3));
        if (unversioned.isFile())
            return unversioned;
    }
    return {};
}

// The plugin the platform is asked for by name, which on this platform is the only one the
// device can use: the others in that directory are for displays it does not have.
FilePaths qtPlugins(const FilePath &deploymentSettings)
{
    const Result<QByteArray> contents = deploymentSettings.fileContents();
    if (!contents)
        return {};
    const QString plugins = QJsonDocument::fromJson(*contents)
                                .object().value("qtPluginsDirectory").toString();
    if (plugins.isEmpty())
        return {};
    const FilePath platform = FilePath::fromUserInput(plugins)
                                  .pathAppended("platforms/libqohos.so");
    return platform.isFile() ? FilePaths{platform} : FilePaths();
}

QtLibraries qtLibraries(const FilePath &library, const FilePaths &directories,
                        const FilePaths &plugins)
{
    if (directories.isEmpty())
        return {};
    QSet<QString> met;
    FilePaths pending{library};
    pending.append(plugins);
    QList<QtLibrary> found;
    for (const FilePath &plugin : plugins) {
        found.append({plugin.parentDir().fileName() + '/' + plugin.fileName(), plugin});
    }
    while (!pending.isEmpty()) {
        const FilePath binary = pending.takeFirst();
        for (const QString &name : ElfReader(binary).neededLibraries()) {
            if (met.contains(name))
                continue;
            met.insert(name);
            const FilePath file = findLibrary(name, directories);
            if (file.isEmpty())
                continue;
            found.append({name, file});
            pending.append(file);
        }
    }
    if (found.isEmpty())
        return {};
    Utils::sort(found, [](const QtLibrary &first, const QtLibrary &second) {
        return first.name < second.name;
    });

    QCryptographicHash hash(QCryptographicHash::Sha1);
    for (const QtLibrary &entry : found) {
        hash.addData(QString("%1 %2 %3").arg(entry.name).arg(entry.file.fileSize())
                         .arg(entry.file.lastModified().toMSecsSinceEpoch()).toUtf8());
    }
    return {QString::fromLatin1(hash.result().toHex().left(16)), found};
}

QList<QtLibrary> missingLibraries(const QtLibraries &qt, const QString &listing)
{
    QHash<QString, qint64> present;
    for (const QString &line : listing.split('\n', Qt::SkipEmptyParts)) {
        const qsizetype space = line.indexOf(' ');
        bool ok = false;
        const qint64 size = line.left(space).toLongLong(&ok);
        if (space <= 0 || !ok)
            continue;
        QString name = line.mid(space + 1).trimmed();
        if (name.startsWith("./"))
            name = name.mid(2);
        present.insert(name, size);
    }
    return Utils::filtered(qt.files, [&present](const QtLibrary &entry) {
        return present.value(entry.name, -1) != entry.file.fileSize();
    });
}

// One channel, from the runner's connection to the end of the application it runs. The
// runner is read when it asks rather than when the run starts, so a rebuild between runs
// needs no new package, and it holds the channel open for as long as the application runs
// and reports over it what it did, which belongs in the application's own output.
//
// The libraries go first, because only the runner knows which of them it needs: it is
// offered the whole set by name and size and answers, one letter per library, what it can
// neither get from the platform nor find where an earlier run of the same set left it.
//
// Arguments go next, ahead of the application and only where the launch cannot carry them:
// an empty list there is an answer too, and takes the launch URI the platform passes out of
// the application's way.
class Handover final : public QObject
{
public:
    Handover(QTcpSocket *socket, const FilePath &library,
             const std::optional<QStringList> &arguments, const QtLibraries &qt,
             RunControl *runControl)
        : QObject(socket)
        , m_socket(socket)
        , m_library(library)
        , m_arguments(arguments)
        , m_qt(qt)
        , m_runControl(runControl)
    {
        connect(socket, &QTcpSocket::readyRead, this, &Handover::read);
        if (m_qt.files.isEmpty())
            serveApplication();
        else
            offerLibraries();
    }

private:
    void offerLibraries()
    {
        announce(m_socket, qtAnnouncement);
        announceText(m_socket, m_qt.tag.toUtf8());
        announce(m_socket, quint32(m_qt.files.size()));
        for (const QtLibrary &entry : m_qt.files) {
            announceText(m_socket, entry.name.toUtf8());
            announce(m_socket, quint32(entry.file.fileSize()));
        }
        m_expected = m_qt.files.size();
    }

    bool serveLibraries()
    {
        qint64 sent = 0;
        int count = 0;
        for (qsizetype index = 0; index < m_qt.files.size(); ++index) {
            if (m_answer.at(index) != 'S')
                continue;
            const FilePath &file = m_qt.files.at(index).file;
            const Result<QByteArray> contents = file.fileContents();
            if (!contents) {
                m_runControl->postMessage(contents.error(), ErrorMessageFormat);
                m_socket->disconnectFromHost();
                return false;
            }
            if (contents->size() != file.fileSize()) {
                m_runControl->postMessage(
                    Tr::tr("\"%1\" changed while it was being handed over.")
                        .arg(file.toUserOutput()), ErrorMessageFormat);
                m_socket->disconnectFromHost();
                return false;
            }
            m_socket->write(*contents);
            sent += contents->size();
            ++count;
        }
        if (count == 0) {
            m_runControl->postMessage(Tr::tr("The runner has the %1 libraries it needs.")
                                          .arg(m_qt.files.size()), NormalMessageFormat);
            return true;
        }
        m_runControl->postMessage(Tr::tr("Handed %1 of %2 libraries to the runner (%3 MB). "
                                         "It keeps them for the next run.")
                                      .arg(count).arg(m_qt.files.size())
                                      .arg(sent / (1024 * 1024)), NormalMessageFormat);
        return true;
    }

    void serveApplication()
    {
        const Result<QByteArray> contents = m_library.fileContents();
        if (!contents) {
            m_runControl->postMessage(contents.error(), ErrorMessageFormat);
            m_socket->disconnectFromHost();
            return;
        }
        if (m_arguments) {
            QByteArray payload;
            for (const QString &argument : *m_arguments)
                payload += argument.toUtf8() + '\0';
            announce(m_socket, argumentsAnnouncement);
            announceText(m_socket, payload);
        }
        const quint32 size = quint32(contents->size());
        announce(m_socket, size);
        m_socket->write(*contents);
        m_runControl->postMessage(Tr::tr("Handed %1 to the runner (%2 bytes).")
                                      .arg(m_library.fileName()).arg(size),
                                  NormalMessageFormat);
    }

    void read()
    {
        if (m_expected > 0) {
            m_answer += m_socket->read(m_expected - m_answer.size());
            if (m_answer.size() < m_expected)
                return;
            m_expected = 0;
            if (!serveLibraries())
                return;
            serveApplication();
        }
        const QString text = QString::fromUtf8(m_socket->readAll());
        for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
            if (line == "runner: staged, start again")
                m_staged = true;
            m_runControl->postMessage(line, StdOutFormat);
        }
    }

public:
    // Whether the runner took the Qt and ended the start it took it in, which is what a
    // runner without a Qt of its own does: the platform does not wait for the time a Qt
    // takes to arrive.
    bool staged() const { return m_staged; }

private:

    QTcpSocket * const m_socket;
    const FilePath m_library;
    const std::optional<QStringList> m_arguments;
    const QtLibraries m_qt;
    RunControl * const m_runControl;
    qsizetype m_expected = 0;
    QByteArray m_answer;
    bool m_staged = false;
};

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

// Building for the platform produces an application module rather than a program, and the
// only thing that loads one is the runner in an installed package. So a run offers it over
// the same channel a run from a host does, and needs no forward for it: an application's
// loopback is the device's own, and the port the runner asks on can be bound here. Nothing
// else of the host route is available - hdc is on the other side of it, and from an
// application sandbox the "aa" command can neither start the runner nor stop it again.
static Group handoverRecipe(RunControl *runControl, const FilePath &library,
                            const QStringList &arguments, const QtLibraries &qt)
{
    const Storage<std::unique_ptr<QTcpServer>> channelStorage;
    const QStoredBarrier answeredBarrier;
    const QStoredBarrier finishedBarrier;

    // An implicit want is all an application may send, so the runner is reached through the
    // scheme its package declares rather than by name.
    const auto askForTheRunner = [runControl] {
        const QUrl url(QString("%1://run")
                           .arg(QString::fromLatin1(Constants::HARMONYOS_RUN_SCHEME)));
        if (QDesktopServices::openUrl(url))
            return true;
        runControl->postMessage(Tr::tr("The device did not accept \"%1\".")
                                    .arg(url.toString()), ErrorMessageFormat);
        return false;
    };

    const auto openChannel = [runControl, library, arguments, qt, channelStorage, answeredBarrier,
                              finishedBarrier, askForTheRunner] {
        QBarrier * const answered = answeredBarrier.activeStorage();
        QBarrier * const finished = finishedBarrier.activeStorage();
        auto server = std::make_unique<QTcpServer>();
        if (!server->listen(QHostAddress::LocalHost, Constants::HARMONYOS_CHANNEL_PORT)) {
            runControl->postMessage(Tr::tr("Could not open the channel the runner asks on: %1")
                                        .arg(server->errorString()), ErrorMessageFormat);
            return false;
        }
        server->setMaxPendingConnections(1);
        QTcpServer * const channel = server.get();
        QObject::connect(channel, &QTcpServer::newConnection, channel,
                         [channel, runControl, library, arguments, qt, answered, finished,
                          askForTheRunner] {
            QTcpSocket * const socket = channel->nextPendingConnection();
            QTC_ASSERT(socket, return);
            const Handover * const handover
                = new Handover(socket, library, arguments, qt, runControl);
            QObject::connect(socket, &QTcpSocket::disconnected, socket,
                             [socket, channel, finished, handover, askForTheRunner] {
                socket->deleteLater();
                if (handover->staged()) {
                    QMetaObject::invokeMethod(channel, [askForTheRunner] { askForTheRunner(); },
                                              Qt::QueuedConnection);
                    return;
                }
                // The runner closes the channel when the application's main() has returned,
                // so this is the run being over. Ending it takes the channel down, and with
                // it this socket, which a socket does not survive being emitted from - so
                // leave the emission first. Posting to the channel is what keeps the call
                // from reaching a barrier that a cancel has taken away in the meantime.
                QMetaObject::invokeMethod(channel, [finished] { finished->advance(); },
                                          Qt::QueuedConnection);
            });
            answered->advance();
        });
        *channelStorage = std::move(server);
        return true;
    };

    return Group {
        channelStorage,
        answeredBarrier,
        finishedBarrier,
        QSyncTask(openChannel),
        QSyncTask(askForTheRunner),
        Group {
            parallel,
            stopOnSuccessOrError,
            barrierAwaiterTask(answeredBarrier),
            timeoutTask(30s),
            onGroupDone([runControl](DoneWith result) {
                if (result != DoneWith::Success) {
                    runControl->postMessage(
                        Tr::tr("The runner did not ask for the application. A package that "
                               "holds it has to be installed, and it must not already be "
                               "running: a launch reaches a running instance instead of "
                               "starting one."), ErrorMessageFormat);
                }
            })
        },
        QSyncTask([runControl] { runControl->reportStarted(); }),
        barrierAwaiterTask(finishedBarrier),
        onGroupDone([runControl](DoneWith result) {
            if (result == DoneWith::Cancel) {
                runControl->postMessage(
                    Tr::tr("The channel is closed, but the application keeps running: "
                           "stopping an ability is not something an application may do."),
                    NormalMessageFormat);
            }
        })
    }.withCancel([runControl] {
        return makeObjectSignal(runControl, &RunControl::canceled);
    });
}

class HarmonyOsBuildDeviceRunWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsBuildDeviceRunWorkerFactory()
    {
        setId("HarmonyOsBuildDeviceRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            BuildConfiguration * const bc = runControl->buildConfiguration();
            const FilePath library = HostOsInfo::isHarmonyHost() && bc
                ? applicationLibrary(bc->buildDirectory(), bc->activeBuildKey()) : FilePath();
            if (!library.isEmpty()) {
                const QStringList launchArguments
                    = harmonyOsExtras(bc->buildDirectory(), bc->activeBuildKey()).launchArguments;
                const FilePath settings
                    = deploymentSettings(bc->buildDirectory(), bc->activeBuildKey());
                return handoverRecipe(runControl, library,
                                      launchArguments
                                          + runControl->commandLine().splitArguments(),
                                      qtLibraries(library, libraryDirectories(settings),
                                                  qtPlugins(settings)));
            }
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
            // loopback, and the runner connects to it.
            const bool viaChannel = settings().runWithoutInstalling();
            const FilePath library = viaChannel
                ? applicationLibrary(bc->buildDirectory(), bc->activeBuildKey()) : FilePath();
            if (viaChannel && library.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("Could not find the application library to hand to the runner. "
                           "Build and deploy the package first."));
            }

            const FilePath settings
                = deploymentSettings(bc->buildDirectory(), bc->activeBuildKey());
            const QtLibraries qt = viaChannel
                ? qtLibraries(library, libraryDirectories(settings), qtPlugins(settings))
                : QtLibraries();

            const Storage<std::unique_ptr<QTcpServer>> channelStorage;
            const auto openChannel = [channelStorage, runControl, library, qt] {
                auto server = std::make_unique<QTcpServer>();
                if (!server->listen(QHostAddress::LocalHost)) {
                    runControl->postMessage(
                        Tr::tr("Could not open the channel the runner asks on: %1")
                            .arg(server->errorString()), ErrorMessageFormat);
                    return false;
                }
                QTcpServer * const channel = server.get();
                QObject::connect(channel, &QTcpServer::newConnection, channel,
                                 [channel, runControl, library, qt] {
                    while (QTcpSocket * const socket = channel->nextPendingConnection()) {
                        QObject::connect(socket, &QTcpSocket::disconnected,
                                         socket, &QTcpSocket::deleteLater);
                        new Handover(socket, library, std::nullopt, qt, runControl);
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

            // What the runner's cache lacks of the Qt goes there before the start, not over the
            // channel: a runner without a Qt of its own takes one while the platform is waiting
            // for the ability, and a Qt takes longer than the platform waits. hdc can neither
            // create a directory for a single file nor delete one, but a directory it sends
            // lands in whatever is already there.
            const Storage<QList<QtLibrary>> missingStorage;
            const Storage<std::unique_ptr<TemporaryDirectory>> stageStorage;
            const QString cache = QString("qtcrun/qt/") + qt.tag;

            const auto onCacheListSetup = [command, bundle, cache](Process &process) {
                // The sandbox as the shell sees it, for the device's one user.
                process.setCommand(command(
                    {"shell", QString("cd /data/app/el2/100/base/%1/files/%2 && "
                                      "find . -type f -exec stat -c '%s %n' {} +")
                                  .arg(bundle, cache)}));
            };
            const auto onCacheListDone = [missingStorage, qt](const Process &process, DoneWith) {
                *missingStorage = missingLibraries(qt, process.cleanedStdOut());
                return DoneResult::Success;
            };
            const auto onStageSetup = [command, bundle, qt, missingStorage, stageStorage,
                                       runControl](Process &process) {
                if (missingStorage->isEmpty())
                    return SetupResult::StopWithSuccess;
                auto stage = std::make_unique<TemporaryDirectory>("qtc-harmonyos-qt-XXXXXX");
                const FilePath root = stage->path().pathAppended("qtcrun");
                qint64 size = 0;
                for (const QtLibrary &entry : *missingStorage) {
                    const FilePath link = root.pathAppended("qt/" + qt.tag + '/' + entry.name);
                    Result<> linked = link.parentDir().ensureWritableDir();
                    if (linked)
                        linked = entry.file.createSymLink(link);
                    if (!linked) {
                        runControl->postMessage(linked.error(), ErrorMessageFormat);
                        return SetupResult::StopWithError;
                    }
                    size += entry.file.fileSize();
                }
                runControl->postMessage(Tr::tr("Handing %1 of %2 libraries to the runner "
                                               "(%3 MB). It keeps them for the next run.")
                                            .arg(missingStorage->size()).arg(qt.files.size())
                                            .arg(size / (1024 * 1024)), NormalMessageFormat);
                process.setCommand(command({"file", "send", "-b", bundle, root.nativePath(),
                                            "data/storage/el2/base/files/"}));
                process.setEnvironment(Sdk::hdcEnvironment());
                *stageStorage = std::move(stage);
                return SetupResult::Continue;
            };
            const auto onStageDone = [runControl](const Process &process) {
                if (!process.allOutput().contains("[Fail]"))
                    return DoneResult::Success;
                runControl->postMessage(Tr::tr("Could not hand the Qt to the runner: %1")
                                            .arg(process.allOutput().trimmed()),
                                        ErrorMessageFormat);
                return DoneResult::Error;
            };

            const auto stageGroup = [=] {
                if (qt.files.isEmpty())
                    return Group { nullItem };
                return Group {
                    missingStorage,
                    stageStorage,
                    ProcessTask(onCacheListSetup, onCacheListDone),
                    ProcessTask(onStageSetup, onStageDone)
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
                stageGroup(),
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
