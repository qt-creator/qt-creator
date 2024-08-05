// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevice.h"

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevicewidget.h"
#include "dockersettings.h"
#include "dockertr.h"
#include "kitdetector.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/devicesupport/processlist.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/basetreeview.h>
#include <utils/clangutils.h>
#include <utils/devicefileaccess.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
#include <utils/expected.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/overridecursor.h>
#include <utils/pathlisteditor.h>
#include <utils/port.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/sortfiltermodel.h>
#include <utils/synchronizedvalue.h>
#include <utils/temporaryfile.h>
#include <utils/terminalhooks.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardItem>
#include <QTextBrowser>
#include <QThread>
#include <QToolButton>

#include <optional>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

Q_LOGGING_CATEGORY(dockerDeviceLog, "qtc.docker.device", QtWarningMsg);

namespace Docker::Internal {

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceUseOutsideUser[] = "DockerDeviceUseUidGid";
const char DockerDeviceMappedPaths[] = "DockerDeviceMappedPaths";
const char DockerDeviceKeepEntryPoint[] = "DockerDeviceKeepEntryPoint";
const char DockerDeviceEnableLldbFlags[] = "DockerDeviceEnableLldbFlags";
const char DockerDeviceClangDExecutable[] = "DockerDeviceClangDExecutable";
const char DockerDeviceExtraArgs[] = "DockerDeviceExtraCreateArguments";

class DockerDeviceFileAccess : public CmdBridge::FileAccess
{
public:
    DockerDeviceFileAccess(DockerDevicePrivate *dev)
        : m_dev(dev)
    {}

    QString mapToDevicePath(const QString &hostPath) const override;

    DockerDevicePrivate *m_dev = nullptr;
};

class DockerFallbackFileAccess : public UnixDeviceFileAccess
{
    const FilePath m_rootPath;

public:
    DockerFallbackFileAccess(const FilePath &rootPath)
        : m_rootPath(rootPath)
    {}

    RunResult runInShell(const CommandLine &cmdLine, const QByteArray &stdInData) const override
    {
        Process proc;
        proc.setWriteData(stdInData);
        proc.setCommand(
            {m_rootPath.withNewPath(cmdLine.executable().path()), cmdLine.splitArguments()});
        proc.runBlocking();

        return {
            proc.resultData().m_exitCode,
            proc.readAllRawStandardOutput(),
            proc.readAllRawStandardError(),
        };
    }
};

// Used for "docker run"
QString DockerDevice::repoAndTag() const
{
    if (repo() == "<none>")
        return imageId();

    if (tag() == "<none>")
        return repo();

    return repo() + ':' + tag();
}

QString DockerDevice::repoAndTagEncoded() const
{
    return repoAndTag().replace(':', '.');
}

FilePath DockerDevice::rootPath() const
{
    return FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME, repoAndTagEncoded(), u"/");
}

class DockerDevicePrivate : public QObject
{
public:
    DockerDevicePrivate(DockerDevice *parent)
        : q(parent)
    {
        QObject::connect(q, &DockerDevice::applied, this, [this] {
            if (!m_container.isEmpty()) {
                stopCurrentContainer();
            }
        });
    }

    ~DockerDevicePrivate() { stopCurrentContainer(); }

    CommandLine createCommandLine();

    expected_str<void> updateContainerAccess();
    void changeMounts(QStringList newMounts);
    bool ensureReachable(const FilePath &other);
    void shutdown();
    expected_str<FilePath> localSource(const FilePath &other) const;

    QString containerId() { return m_container; }

    expected_str<QPair<Utils::OsType, Utils::OsArch>> osTypeAndArch() const;

    expected_str<Environment> environment();

    CommandLine withDockerExecCmd(const CommandLine &cmd,
                                  const std::optional<Environment> &env = std::nullopt,
                                  const std::optional<FilePath> &workDir = std::nullopt,
                                  bool interactive = false,
                                  bool withPty = false,
                                  bool withMarker = true);

    bool prepareForBuild(const Target *target);
    Tasks validateMounts() const;

    expected_str<QString> createContainer();
    expected_str<void> startContainer();
    void stopCurrentContainer();
    expected_str<void> fetchSystemEnviroment();

    expected_str<FilePath> getCmdBridgePath() const;

    std::optional<FilePath> clangdExecutable() const
    {
        if (q->clangdExecutableAspect().isEmpty())
            return std::nullopt;
        if (!q->clangdExecutableAspect().needsDevice())
            return q->rootPath().withNewMappedPath(q->clangdExecutableAspect());
        return q->clangdExecutableAspect();
    }

    QStringList createMountArgs() const;

    bool isImageAvailable() const;

    DockerDevice *const q;

    struct MountPair
    {
        FilePath path;
        FilePath containerPath;
    };

    QString m_container;

    std::optional<Environment> m_cachedEnviroment;
    bool m_isShutdown = false;
    SynchronizedValue<std::unique_ptr<DeviceFileAccess>> m_fileAccess;
};

class DockerProcessImpl : public ProcessInterface
{
public:
    DockerProcessImpl(IDevice::ConstPtr device, DockerDevicePrivate *devicePrivate);
    virtual ~DockerProcessImpl();

private:
    void start() override;
    qint64 write(const QByteArray &data) override;
    void sendControlSignal(ControlSignal controlSignal) final;

private:
    DockerDevicePrivate *m_devicePrivate = nullptr;
    // Store the IDevice::ConstPtr in order to extend the lifetime of device for as long
    // as this object is alive.
    IDevice::ConstPtr m_device;

    Process m_process;
    qint64 m_remotePID = 0;
    bool m_hasReceivedFirstOutput = false;
};

DockerProcessImpl::DockerProcessImpl(IDevice::ConstPtr device, DockerDevicePrivate *devicePrivate)
    : m_devicePrivate(devicePrivate)
    , m_device(std::move(device))
    , m_process(this)
{
    connect(&m_process, &Process::started, this, [this] {
        qCDebug(dockerDeviceLog) << "Process started:" << m_process.commandLine();

        if (m_setup.m_ptyData.has_value()) {
            m_hasReceivedFirstOutput = true;
            emit started(m_process.processId(), m_process.applicationMainThreadId());
        }
    });

    connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
        if (m_hasReceivedFirstOutput) {
            emit readyRead(m_process.readAllRawStandardOutput(), {});
            return;
        }

        QByteArray output = m_process.readAllRawStandardOutput();
        qsizetype idx = output.indexOf('\n');
        QByteArray firstLine = output.left(idx).trimmed();
        QByteArray rest = output.mid(idx + 1);
        qCDebug(dockerDeviceLog) << "Process first line received:" << m_process.commandLine()
                                 << firstLine;

        if (!firstLine.startsWith("__qtc")) {
            emit done(ProcessResultData{
                -1,
                QProcess::ExitStatus::CrashExit,
                QProcess::ProcessError::FailedToStart,
                QString::fromUtf8(firstLine),
            });
            return;
        }

        bool ok = false;
        m_remotePID = firstLine.mid(5, firstLine.size() - 5 - 5).toLongLong(&ok);

        if (ok)
            emit started(m_remotePID);
        else {
            emit done(ProcessResultData{
                -1,
                QProcess::ExitStatus::CrashExit,
                QProcess::ProcessError::FailedToStart,
                QString::fromUtf8(firstLine),
            });
            return;
        }

        // In case we already received some error output, send it now.
        const QByteArray stdErr = m_process.readAllRawStandardError();
        if (rest.size() > 0 || stdErr.size() > 0)
            emit readyRead(rest, stdErr);

        m_hasReceivedFirstOutput = true;
    });

    connect(&m_process, &Process::readyReadStandardError, this, [this] {
        if (m_remotePID)
            emit readyRead({}, m_process.readAllRawStandardError());
    });

    connect(&m_process, &Process::done, this, [this] {
        qCDebug(dockerDeviceLog) << "Process exited:" << m_process.commandLine()
                                 << "with code:" << m_process.resultData().m_exitCode;

        ProcessResultData resultData = m_process.resultData();

        if (m_remotePID == 0 && !m_hasReceivedFirstOutput) {
            resultData.m_error = QProcess::FailedToStart;
            qCWarning(dockerDeviceLog) << "Process failed to start:" << m_process.commandLine();
            QByteArray stdOut = m_process.readAllRawStandardOutput();
            QByteArray stdErr = m_process.readAllRawStandardError();
            if (!stdOut.isEmpty())
                qCWarning(dockerDeviceLog) << "stdout:" << stdOut;
            if (!stdErr.isEmpty())
                qCWarning(dockerDeviceLog) << "stderr:" << stdErr;
        }

        emit done(resultData);
    });
}

DockerProcessImpl::~DockerProcessImpl()
{
    if (m_process.state() == QProcess::Running)
        sendControlSignal(ControlSignal::Kill);
}

void DockerProcessImpl::start()
{
    m_process.setProcessImpl(m_setup.m_processImpl);
    m_process.setProcessMode(m_setup.m_processMode);
    m_process.setTerminalMode(m_setup.m_terminalMode);
    m_process.setPtyData(m_setup.m_ptyData);
    m_process.setReaperTimeout(m_setup.m_reaperTimeout);
    m_process.setWriteData(m_setup.m_writeData);
    m_process.setProcessChannelMode(m_setup.m_processChannelMode);
    m_process.setExtraData(m_setup.m_extraData);
    m_process.setStandardInputFile(m_setup.m_standardInputFile);
    m_process.setAbortOnMetaChars(m_setup.m_abortOnMetaChars);
    m_process.setCreateConsoleOnWindows(m_setup.m_createConsoleOnWindows);
    if (m_setup.m_lowPriority)
        m_process.setLowPriority();

    const bool inTerminal = m_setup.m_terminalMode != TerminalMode::Off
                            || m_setup.m_ptyData.has_value();

    const bool interactive = m_setup.m_processMode == ProcessMode::Writer
                             || !m_setup.m_writeData.isEmpty() || inTerminal;

    const CommandLine fullCommandLine
        = m_devicePrivate->withDockerExecCmd(m_setup.m_commandLine,
                                             m_setup.m_environment,
                                             m_setup.m_workingDirectory,
                                             interactive,
                                             inTerminal,
                                             !m_process.ptyData().has_value());

    m_process.setCommand(fullCommandLine);
    m_process.start();
}

qint64 DockerProcessImpl::write(const QByteArray &data)
{
    return m_process.writeRaw(data);
}

void DockerProcessImpl::sendControlSignal(ControlSignal controlSignal)
{
    if (!m_setup.m_ptyData.has_value()) {
        QTC_ASSERT(m_remotePID, return);
        if (controlSignal == ControlSignal::CloseWriteChannel) {
            m_process.closeWriteChannel();
            return;
        }
        auto dfa = dynamic_cast<DockerDeviceFileAccess *>(m_device->fileAccess());
        if (dfa) {
            static_cast<DockerDeviceFileAccess *>(m_device->fileAccess())
                ->signalProcess(m_remotePID, controlSignal);
        } else {
            const int signal = controlSignalToInt(controlSignal);
            Process p;
            p.setCommand(
                {m_device->rootPath().withNewPath("kill"),
                 {QString("-%1").arg(signal), QString("%2").arg(m_remotePID)}});
            p.runBlocking();
        }
    } else {
        // clang-format off
        switch (controlSignal) {
        case ControlSignal::Terminate: m_process.terminate();      break;
        case ControlSignal::Kill:      m_process.kill();           break;
        case ControlSignal::Interrupt: m_process.interrupt();      break;
        case ControlSignal::KickOff:   m_process.kickoffProcess(); break;
        case ControlSignal::CloseWriteChannel: break;
        }
        // clang-format on
    }
}

CommandLine DockerDevice::createCommandLine() const
{
    return d->createCommandLine();
}

IDeviceWidget *DockerDevice::createWidget()
{
    return new DockerDeviceWidget(shared_from_this());
}

Tasks DockerDevice::validate() const
{
    return d->validateMounts();
}

Tasks DockerDevicePrivate::validateMounts() const
{
    Tasks result;

    for (const FilePath &mount : q->mounts()) {
        if (!mount.isDir()) {
            const QString message = Tr::tr("Path \"%1\" is not a directory or does not exist.")
                                        .arg(mount.toUserOutput());

            result.append(Task(Task::Error, message, {}, -1, {}));
        }
    }
    return result;
}

QString DockerDeviceFileAccess::mapToDevicePath(const QString &hostPath) const
{
    // make sure to convert windows style paths to unix style paths with the file system case:
    // C:/dev/src -> /c/dev/src
    const FilePath normalized = FilePath::fromString(hostPath).normalizedPathName();
    QString newPath = normalized.path();
    if (HostOsInfo::isWindowsHost() && normalized.startsWithDriveLetter()) {
        const QChar lowerDriveLetter = newPath.at(0);
        newPath = '/' + lowerDriveLetter + newPath.mid(2); // strip C:
    }
    return newPath;
}

DockerDevice::DockerDevice()
    : d(new DockerDevicePrivate(this))
{
    imageId.setSettingsKey(DockerDeviceDataImageIdKey);
    imageId.setLabelText(Tr::tr("Image ID:"));
    imageId.setReadOnly(true);

    repo.setSettingsKey(DockerDeviceDataRepoKey);
    repo.setLabelText(Tr::tr("Repository:"));
    repo.setReadOnly(true);

    tag.setSettingsKey(DockerDeviceDataTagKey);
    tag.setLabelText(Tr::tr("Tag:"));
    tag.setReadOnly(true);

    useLocalUidGid.setSettingsKey(DockerDeviceUseOutsideUser);
    useLocalUidGid.setLabelText(Tr::tr("Run as outside user:"));
    useLocalUidGid.setDefaultValue(true);
    useLocalUidGid.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);

    keepEntryPoint.setSettingsKey(DockerDeviceKeepEntryPoint);
    keepEntryPoint.setLabelText(Tr::tr("Do not modify entry point:"));
    keepEntryPoint.setDefaultValue(false);
    keepEntryPoint.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);

    enableLldbFlags.setSettingsKey(DockerDeviceEnableLldbFlags);
    enableLldbFlags.setLabelText(Tr::tr("Enable flags needed for LLDB:"));
    enableLldbFlags.setDefaultValue(false);
    enableLldbFlags.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);

    mounts.setSettingsKey(DockerDeviceMappedPaths);
    mounts.setLabelText(Tr::tr("Paths to mount:"));
    mounts.setDefaultValue({Core::DocumentManager::projectsDirectory().toString()});
    mounts.setToolTip(Tr::tr("Maps paths in this list one-to-one to the docker container."));
    mounts.setPlaceHolderText(Tr::tr("Host directories to mount into the container."));

    extraArgs.setSettingsKey(DockerDeviceExtraArgs);
    extraArgs.setLabelText(Tr::tr("Extra arguments:"));
    extraArgs.setToolTip(Tr::tr("Extra arguments to pass to docker create."));
    extraArgs.setDisplayStyle(StringAspect::LineEditDisplay);

    clangdExecutableAspect.setSettingsKey(DockerDeviceClangDExecutable);
    clangdExecutableAspect.setLabelText(Tr::tr("Clangd Executable:"));
    clangdExecutableAspect.setAllowPathFromDevice(true);

    network.setSettingsKey("Network");
    network.setLabelText(Tr::tr("Network:"));
    network.setDefaultValue("bridge");
    network.setFillCallback([this](const StringSelectionAspect::ResultCallback &cb) {
        auto future = DockerApi::instance()->networks();

        auto watcher = new QFutureWatcher<expected_str<QList<Network>>>(this);
        watcher->setFuture(future);
        QObject::connect(watcher,
                         &QFutureWatcher<expected_str<QList<Network>>>::finished,
                         this,
                         [watcher, cb]() {
                             expected_str<QList<Network>> result = watcher->result();
                             if (result) {
                                 auto items = transform(*result, [](const Network &network) {
                                     QStandardItem *item = new QStandardItem(network.name);
                                     item->setData(network.name);
                                     item->setToolTip(network.toString());
                                     return item;
                                 });
                                 cb(items);
                             } else {
                                 QStandardItem *errorItem = new QStandardItem(Tr::tr("Error"));
                                 errorItem->setToolTip(result.error());
                                 cb({errorItem});
                             }
                         });
    });

    connect(DockerApi::instance(),
            &DockerApi::dockerDaemonAvailableChanged,
            &network,
            &StringSelectionAspect::refill);

    clangdExecutableAspect.setValidationFunction(
        [this](const QString &newValue) -> FancyLineEdit::AsyncValidationFuture {
            const FilePath rootPath = FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME,
                                                          repoAndTagEncoded(),
                                                          u"/");
            return asyncRun([rootPath, newValue]() -> expected_str<QString> {
                QString changedValue = newValue;
                FilePath path = FilePath::fromUserInput(newValue);
                if (!path.needsDevice()) {
                    const FilePath onDevicePath = rootPath.withNewMappedPath(path);
                    if (onDevicePath.exists()) {
                        changedValue = onDevicePath.toUserOutput();
                        path = onDevicePath;
                    } else {
                        return make_unexpected(
                            Tr::tr("The path \"%1\" does not exist.").arg(onDevicePath.toUserOutput()));
                    }
                }
                QString error;
                bool result = checkClangdVersion(path, &error);
                if (!result)
                    return make_unexpected(error);
                return changedValue;
            });
        });

    containerStatus.setText(Tr::tr("stopped"));


    auto createBridgeFileAccess = [this]() -> expected_str<std::unique_ptr<DeviceFileAccess>> {
        expected_str<FilePath> cmdBridgePath = d->getCmdBridgePath();

        if (!cmdBridgePath)
            return make_unexpected(cmdBridgePath.error());

        auto fAccess = std::make_unique<DockerDeviceFileAccess>(d);
        expected_str<void> initResult;
        if (!cmdBridgePath->isSameDevice(Docker::Internal::settings().dockerBinaryPath())) {
            initResult = fAccess->deployAndInit(Core::ICore::libexecPath(), rootPath());
        } else {
            initResult = fAccess->init(rootPath().withNewPath("/tmp/_qtc_cmdbridge"));
        }
        if (!initResult)
            return make_unexpected(initResult.error());

        return fAccess;
    };

    setFileAccess([this, createBridgeFileAccess]() -> DeviceFileAccess * {
        if (DeviceFileAccess *fileAccess = d->m_fileAccess.readLocked()->get())
            return fileAccess;

        SynchronizedValue<std::unique_ptr<DeviceFileAccess>>::unique_lock fileAccess
            = d->m_fileAccess.writeLocked();
        if (*fileAccess)
            return fileAccess->get();

        expected_str<std::unique_ptr<DeviceFileAccess>> fAccess = createBridgeFileAccess();

        if (fAccess) {
            *fileAccess = std::move(*fAccess);
            return fileAccess->get();
        }

        qCWarning(dockerDeviceLog) << "Failed to start CmdBridge:" << fAccess.error()
                                   << ", falling back to slow direct access";

        *fileAccess = std::make_unique<DockerFallbackFileAccess>(rootPath());
        return fileAccess->get();
    });

    setDisplayType(Tr::tr("Docker"));
    setOsType(OsTypeLinux);
    setupId(IDevice::ManuallyAdded);
    setType(Constants::DOCKER_DEVICE_TYPE);
    setMachineType(IDevice::Hardware);
    setAllowEmptyCommand(true);

    setOpenTerminal([this](const Environment &env,
                           const FilePath &workingDir) -> expected_str<void> {
        Q_UNUSED(env); // TODO: That's the runnable's environment in general. Use it via -e below.

        expected_str<void> result = d->updateContainerAccess();

        if (!result)
            return result;

        if (d->containerId().isEmpty())
            return make_unexpected(Tr::tr("Error starting remote shell. No container."));

        expected_str<FilePath> shell = Terminal::defaultShellForDevice(rootPath());
        if (!shell)
            return make_unexpected(shell.error());

        Process proc;
        proc.setTerminalMode(TerminalMode::Detached);
        proc.setEnvironment(env);
        proc.setWorkingDirectory(workingDir);
        proc.setCommand(CommandLine{*shell});
        proc.start();

        return {};
    });

    addDeviceAction(
        {Tr::tr("Open Shell in Container"), [](const IDevice::Ptr &device, QWidget *) {
             expected_str<Environment> env = device->systemEnvironmentWithError();
             if (!env) {
                 QMessageBox::warning(ICore::dialogParent(), Tr::tr("Error"), env.error());
                 return;
             }
             expected_str<void> result = device->openTerminal(*env, FilePath());
             if (!result)
                 QMessageBox::warning(ICore::dialogParent(), Tr::tr("Error"), result.error());
         }});
}

DockerDevice::~DockerDevice()
{
    delete d;
}

void DockerDevice::shutdown()
{
    d->shutdown();
}

expected_str<void> DockerDevice::updateContainerAccess() const
{
    return d->updateContainerAccess();
}

CommandLine DockerDevicePrivate::withDockerExecCmd(const CommandLine &cmd,
                                                   const std::optional<Environment> &env,
                                                   const std::optional<FilePath> &workDir,
                                                   bool interactive,
                                                   bool withPty,
                                                   bool withMarker)
{
    if (!updateContainerAccess())
        return {};

    CommandLine dockerCmd{settings().dockerBinaryPath(), {"exec"}};

    if (interactive)
        dockerCmd.addArg("-i");

    if (withPty)
        dockerCmd.addArg("-t");

    if (env) {
        env->forEachEntry([&](const QString &key, const QString &value, bool enabled) {
            if (enabled) {
                dockerCmd.addArg("-e");
                dockerCmd.addArg(key + "=" + env->expandVariables(value));
            }
        });
    }

    if (workDir && !workDir->isEmpty())
        dockerCmd.addArgs({"-w", q->rootPath().withNewMappedPath(*workDir).nativePath()});

    dockerCmd.addArg(m_container);

    dockerCmd.addArgs({"/bin/sh", "-c"});

    CommandLine exec("exec");
    exec.addCommandLineAsArgs(cmd, CommandLine::Raw);

    if (withMarker) {
        // Check the executable for existence.
        CommandLine testType({"type", {}});
        testType.addArg(cmd.executable().path());
        testType.addArgs(">/dev/null", CommandLine::Raw);

        // Send PID only if existence was confirmed, so we can correctly notify
        // a failed start.
        CommandLine echo("echo");
        echo.addArgs("__qtc$$qtc__", CommandLine::Raw);
        echo.addCommandLineWithAnd(exec);

        testType.addCommandLineWithAnd(echo);

        dockerCmd.addCommandLineAsSingleArg(testType);
    } else {
        dockerCmd.addCommandLineAsSingleArg(exec);
    }

    return dockerCmd;
}

void DockerDevicePrivate::stopCurrentContainer()
{
    if (m_container.isEmpty())
        return;

    if (!DockerApi::isDockerDaemonAvailable(false).value_or(false))
        return;

    auto fileAccess = m_fileAccess.writeLocked();
    if (*fileAccess)
        fileAccess->reset();

    Process proc;
    proc.setCommand({settings().dockerBinaryPath(), {"container", "kill", m_container}});

    m_container.clear();

    proc.runBlocking();

    m_cachedEnviroment.reset();
}

bool DockerDevicePrivate::prepareForBuild(const Target *target)
{
    QTC_ASSERT(QThread::currentThread() == thread(), return false);

    return ensureReachable(target->project()->projectDirectory())
           && ensureReachable(target->activeBuildConfiguration()->buildDirectory());
}

QString escapeMountPathUnix(const FilePath &fp)
{
    return fp.nativePath().replace('\"', "\"\"");
}

QString escapeMountPathWin(const FilePath &fp)
{
    QString result = fp.nativePath().replace('\"', "\"\"").replace('\\', '/');
    if (result.size() >= 2 && result[1] == ':')
        result = "/" + result[0] + "/" + result.mid(3);
    return result;
}

QString escapeMountPath(const FilePath &fp)
{
    if (HostOsInfo::isWindowsHost())
        return escapeMountPathWin(fp);

    return escapeMountPathUnix(fp);
}

QStringList toMountArg(const DockerDevicePrivate::MountPair &mi)
{
    QString escapedPath;
    QString escapedContainerPath;

    escapedPath = escapeMountPath(mi.path);
    escapedContainerPath = escapeMountPath(mi.containerPath);

    const QString mountArg = QString(R"(type=bind,"source=%1","destination=%2")")
                                 .arg(escapedPath)
                                 .arg(escapedContainerPath);

    return QStringList{"--mount", mountArg};
}

expected_str<void> isValidMountInfo(const DockerDevicePrivate::MountPair &mi)
{
    if (mi.path.needsDevice())
        return make_unexpected(QString("The path \"%1\" is not local.").arg(mi.path.toUserOutput()));

    if (mi.path.isEmpty() && mi.containerPath.isEmpty())
        return make_unexpected(QString("Both paths are empty."));

    if (mi.path.isEmpty()) {
        return make_unexpected(QString("The local path is empty, the container path is \"%1\".")
                                   .arg(mi.containerPath.toUserOutput()));
    }

    if (mi.containerPath.isEmpty()) {
        return make_unexpected(
            QString("The container path is empty, the local path is \"%1\".").arg(mi.path.toUserOutput()));
    }

    if (!mi.path.isAbsolutePath() || !mi.containerPath.isAbsolutePath()) {
        return make_unexpected(QString("The path \"%1\" or \"%2\" is not absolute.")
                                   .arg(mi.path.toUserOutput())
                                   .arg(mi.containerPath.toUserOutput()));
    }

    if (mi.containerPath.isRootPath())
        return make_unexpected(QString("The path \"%1\" is root.").arg(mi.containerPath.toUserOutput()));

    if (!mi.path.exists())
        return make_unexpected(QString("The path \"%1\" does not exist.").arg(mi.path.toUserOutput()));

    return {};
}

expected_str<FilePath> DockerDevicePrivate::getCmdBridgePath() const
{
    auto osAndArch = osTypeAndArch();
    if (!osAndArch)
        return make_unexpected(osAndArch.error());
    return CmdBridge::Client::getCmdBridgePath(
        osAndArch->first, osAndArch->second, Core::ICore::libexecPath());
};

QStringList DockerDevicePrivate::createMountArgs() const
{
    const Utils::expected_str<Utils::FilePath> cmdBridgePath = getCmdBridgePath();
    QTC_CHECK_EXPECTED(cmdBridgePath);

    QStringList cmds;
    QList<MountPair> mounts;
    for (const FilePath &m : q->mounts())
        mounts.append({m, m});

    if (cmdBridgePath && cmdBridgePath->isSameDevice(settings().dockerBinaryPath()))
        mounts.append({cmdBridgePath.value(), FilePath("/tmp/_qtc_cmdbridge")});

    for (const MountPair &mi : mounts) {
        if (isValidMountInfo(mi))
            cmds += toMountArg(mi);
    }

    return cmds;
}

bool DockerDevicePrivate::isImageAvailable() const
{
    Process proc;
    proc.setCommand(
        {settings().dockerBinaryPath(),
         {"image", "list", q->repoAndTag(), "--format", "{{.Repository}}:{{.Tag}}"}});
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return false;

    if (proc.stdOut().trimmed() == q->repoAndTag())
        return true;

    return false;
}

CommandLine DockerDevicePrivate::createCommandLine()
{
    const QString display = HostOsInfo::isLinuxHost() ? QString(":0")
                                                      : QString("host.docker.internal:0");
    CommandLine dockerCreate{settings().dockerBinaryPath(),
                             {"create",
                              "-i",
                              "--rm",
                              "-e",
                              QString("DISPLAY=%1").arg(display),
                              "-e",
                              "XAUTHORITY=/.Xauthority"}};

#ifdef Q_OS_UNIX
    // no getuid() and getgid() on Windows.
    if (q->useLocalUidGid())
        dockerCreate.addArgs({"-u", QString("%1:%2").arg(getuid()).arg(getgid())});
#endif

    if (!q->network().isEmpty()) {
        dockerCreate.addArg("--network");
        dockerCreate.addArg(q->network());
    }

    dockerCreate.addArgs(createMountArgs());

    if (!q->keepEntryPoint())
        dockerCreate.addArgs({"--entrypoint", "/bin/sh"});

    if (q->enableLldbFlags())
        dockerCreate.addArgs({"--cap-add=SYS_PTRACE", "--security-opt", "seccomp=unconfined"});

    dockerCreate.addArgs(q->extraArgs(), CommandLine::Raw);

    dockerCreate.addArg(q->repoAndTag());

    return dockerCreate;
}

expected_str<QString> DockerDevicePrivate::createContainer()
{
    if (!isImageAvailable())
        return make_unexpected(Tr::tr("Image \"%1\" is not available.").arg(q->repoAndTag()));

    const CommandLine cmdLine = createCommandLine();

    qCDebug(dockerDeviceLog).noquote() << "RUNNING: " << cmdLine.toUserOutput();
    Process createProcess;
    createProcess.setCommand(cmdLine);
    createProcess.runBlocking();

    if (createProcess.result() != ProcessResult::FinishedWithSuccess) {
        return make_unexpected(Tr::tr("Failed creating Docker container. Exit code: %1, output: %2")
                                   .arg(createProcess.exitCode())
                                   .arg(createProcess.allOutput()));
    }

    m_container = createProcess.cleanedStdOut().trimmed();
    if (m_container.isEmpty())
        return make_unexpected(
            Tr::tr("Failed creating Docker container. No container ID received."));

    qCDebug(dockerDeviceLog) << "ContainerId:" << m_container;
    return m_container;
}

expected_str<void> DockerDevicePrivate::startContainer()
{
    auto createResult = createContainer();
    if (!createResult)
        return make_unexpected(createResult.error());

    Process startProcess;
    startProcess.setCommand({settings().dockerBinaryPath(), {"container", "start", m_container}});
    startProcess.runBlocking();
    if (startProcess.result() != ProcessResult::FinishedWithSuccess) {
        return make_unexpected(Tr::tr("Failed starting Docker container. Exit code: %1, output: %2")
                                   .arg(startProcess.exitCode())
                                   .arg(startProcess.allOutput()));
    }
    return {};
}

expected_str<void> DockerDevicePrivate::updateContainerAccess()
{
    if (!m_container.isEmpty() && DockerApi::instance()->isContainerRunning(m_container))
        return {};

    if (m_isShutdown)
        return make_unexpected(Tr::tr("Device is shut down"));

    if (DockerApi::isDockerDaemonAvailable(false).value_or(false) == false)
        return make_unexpected(Tr::tr("Docker system is not reachable"));

    expected_str<void> result = startContainer();
    QString containerStatus = result ? Tr::tr("Running") : result.error().trimmed();

    if (!result)
        result = make_unexpected(QString("Failed to start container: %1").arg(result.error()));

    QTimer::singleShot(0, this, [this, containerStatus] {
        q->containerStatus.setText(containerStatus);
    });

    return result;
}

void DockerDevice::setMounts(const QStringList &mounts) const
{
    d->changeMounts(mounts);
}

void DockerDevice::fromMap(const Store &map)
{
    ProjectExplorer::IDevice::fromMap(map);

    // This is the only place where we can correctly set the default name.
    // Only here do we know the image id and the repo reliably, no matter
    // where or how we were created.
    if (displayName() == defaultDisplayName()) {
        setDefaultDisplayName(
            Tr::tr("Docker Image \"%1\" (%2)").arg(repoAndTag()).arg(imageId.value()));
    }
}

void DockerDevice::toMap(Store &map) const
{
    IDevice::toMap(map);
}

ProcessInterface *DockerDevice::createProcessInterface() const
{
    return new DockerProcessImpl(shared_from_this(), d);
}

DeviceTester *DockerDevice::createDeviceTester() const
{
    return nullptr;
}

bool DockerDevice::usableAsBuildDevice() const
{
    return true;
}

FilePath DockerDevice::filePath(const QString &pathOnDevice) const
{
    return FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME,
                               repoAndTagEncoded(),
                               pathOnDevice);
}

bool DockerDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"device" && filePath.host() == id().toString())
        return true;

    const bool isDockerScheme = filePath.scheme() == Constants::DOCKER_DEVICE_SCHEME;

    if (isDockerScheme && filePath.host() == imageId())
        return true;

    if (isDockerScheme && filePath.host() == repoAndTagEncoded())
        return true;

    if (isDockerScheme && filePath.host() == repoAndTag())
        return true;

    return false;
}

bool DockerDevice::ensureReachable(const FilePath &other) const
{
    if (other.isEmpty())
        return false;

    if (other.isSameDevice(rootPath()))
        return true;

    if (other.needsDevice())
        return false;

    if (other.isDir())
        return d->ensureReachable(other);
    return d->ensureReachable(other.parentDir());
}

expected_str<FilePath> DockerDevice::localSource(const FilePath &other) const
{
    return d->localSource(other);
}

expected_str<Environment> DockerDevice::systemEnvironmentWithError() const
{
    return d->environment();
}

void DockerDevice::aboutToBeRemoved() const
{
    KitDetector detector(shared_from_this());
    detector.undoAutoDetect(id().toString());
}

expected_str<void> DockerDevicePrivate::fetchSystemEnviroment()
{
    if (m_cachedEnviroment)
        return {};

    if (auto fileAccess = m_fileAccess.readLocked()->get()) {
        m_cachedEnviroment = fileAccess->deviceEnvironment();
        return {};
    }

    expected_str<void> result = updateContainerAccess();

    if (!result)
        return result;

    Process proc;
    proc.setCommand(withDockerExecCmd(CommandLine{"env"}));
    proc.runBlocking();
    const QString remoteOutput = proc.cleanedStdOut();

    m_cachedEnviroment = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());
    QString stdErr = proc.cleanedStdErr();

    if (stdErr.isEmpty())
        return {};

    return make_unexpected("Could not read container environment: " + stdErr);
}

// Factory

class DockerImageItem final : public TreeItem
{
public:
    DockerImageItem() {}

    QVariant data(int column, int role) const final
    {
        switch (column) {
        case 0:
            if (role == Qt::DisplayRole)
                return repo;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return tag;
            break;
        case 2:
            if (role == Qt::DisplayRole)
                return imageId;
            break;
        case 3:
            if (role == Qt::DisplayRole)
                return size;
            break;
        }

        return QVariant();
    }

    QString repo;
    QString tag;
    QString imageId;
    QString size;
};

class DockerDeviceSetupWizard final : public QDialog
{
public:
    DockerDeviceSetupWizard()
        : QDialog(ICore::dialogParent())
    {
        setWindowTitle(Tr::tr("Docker Image Selection"));
        resize(800, 600);

        m_model.setHeader({"Repository", "Tag", "Image", "Size"});

        m_view = new TreeView;
        QCheckBox *showUnnamedContainers = new QCheckBox(Tr::tr("Show Unnamed Images"));
        QLabel *statusLabel = new QLabel();
        statusLabel->setText(Tr::tr("Loading ..."));
        statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        statusLabel->setAlignment(Qt::AlignCenter);

        m_proxyModel = new SortFilterModel(this);

        m_proxyModel->setFilterRowFunction(
            [showUnnamedContainers, this](int source_row, const QModelIndex &parent) {
                if (showUnnamedContainers->isChecked())
                    return true;

                return m_model.index(source_row, 0, parent).data(Qt::DisplayRole) != "<none>";
            });

        connect(showUnnamedContainers, &QCheckBox::toggled, this, [this] {
            m_proxyModel->invalidate();
        });

        m_proxyModel->setSourceModel(&m_model);

        m_view->setModel(m_proxyModel);
        m_view->setEnabled(false);
        m_view->header()->setStretchLastSection(true);
        m_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_view->setSelectionMode(QAbstractItemView::SingleSelection);
        m_view->setSortingEnabled(true);
        m_view->sortByColumn(0, Qt::AscendingOrder);
        m_view->setEnabled(false);

        connect(m_view, &QAbstractItemView::doubleClicked, this, &QDialog::accept);

        m_log = new QTextBrowser;
        m_log->setVisible(dockerDeviceLog().isDebugEnabled());

        const QString fail = QString{"Docker: "}
                             + ::ProjectExplorer::Tr::tr("The process failed to start.");
        auto errorLabel = new InfoLabel(fail, InfoLabel::Error, this);
        errorLabel->setVisible(false);

        m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        using namespace Layouting;

        // clang-format off
        Column {
            Stack {
                statusLabel,
                m_view,
            },
            m_log,
            errorLabel,
            Row{showUnnamedContainers, m_buttons},
        }.attachTo(this);
        // clang-format on
        connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

        CommandLine cmd{settings().dockerBinaryPath(),
                        {"images", "--format", "{{.ID}}\\t{{.Repository}}\\t{{.Tag}}\\t{{.Size}}"}};
        m_log->append(Tr::tr("Running \"%1\"").arg(cmd.toUserOutput()) + "\n");

        m_process = new Process(this);
        m_process->setCommand(cmd);

        connect(m_process, &Process::readyReadStandardOutput, this, [this] {
            const QString out = m_process->readAllStandardOutput().trimmed();
            m_log->append(out);
            for (const QString &line : out.split('\n')) {
                const QStringList parts = line.trimmed().split('\t');
                if (parts.size() != 4) {
                    m_log->append(Tr::tr("Unexpected result: %1").arg(line) + '\n');
                    continue;
                }
                auto item = new DockerImageItem;
                item->imageId = parts.at(0);
                item->repo = parts.at(1);
                item->tag = parts.at(2);
                item->size = parts.at(3);
                m_model.rootItem()->appendChild(item);
            }
            m_log->append(Tr::tr("Done."));
        });

        connect(m_process, &Process::readyReadStandardError, this, [this] {
            const QString out = Tr::tr("Error: %1").arg(m_process->cleanedStdErr());
            m_log->append(Tr::tr("Error: %1").arg(out));
        });

        connect(m_process, &Process::done, errorLabel, [errorLabel, this, statusLabel] {
            delete statusLabel;
            if (m_process->result() == ProcessResult::FinishedWithSuccess)
                m_view->setEnabled(true);
            else
                errorLabel->setVisible(true);
        });

        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
            const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
            QTC_ASSERT(selectedRows.size() == 1, return);
            m_buttons->button(QDialogButtonBox::Ok)->setEnabled(selectedRows.size() == 1);
        });

        m_process->start();
    }

    IDevice::Ptr device() const
    {
        const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
        QTC_ASSERT(selectedRows.size() == 1, return {});
        DockerImageItem *item = m_model.itemForIndex(
            m_proxyModel->mapToSource(selectedRows.front()));
        QTC_ASSERT(item, return {});

        auto device = DockerDevice::create();
        device->repo.setValue(item->repo);
        device->tag.setValue(item->tag);
        device->imageId.setValue(item->imageId);

        return device;
    }

public:
    TreeModel<DockerImageItem> m_model;
    TreeView *m_view = nullptr;
    SortFilterModel *m_proxyModel = nullptr;
    QTextBrowser *m_log = nullptr;
    QDialogButtonBox *m_buttons;

    Process *m_process = nullptr;
    QString m_selectedId;
};

// Factory

DockerDeviceFactory::DockerDeviceFactory()
    : IDeviceFactory(Constants::DOCKER_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("Docker Device"));
    setIcon(QIcon());
    setCreator([] {
        DockerDeviceSetupWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
    setConstructionFunction([this] {
        auto device = DockerDevice::create();
        QMutexLocker lk(&m_deviceListMutex);
        m_existingDevices.push_back(device);
        return device;
    });
}

void DockerDeviceFactory::shutdownExistingDevices()
{
    QMutexLocker lk(&m_deviceListMutex);
    for (const auto &weakDevice : m_existingDevices) {
        if (std::shared_ptr<DockerDevice> device = weakDevice.lock())
            device->shutdown();
    }
}

expected_str<QPair<Utils::OsType, Utils::OsArch>> DockerDevicePrivate::osTypeAndArch() const
{
    Process proc;
    proc.setCommand(
        {settings().dockerBinaryPath(),
         {"image", "inspect", q->repoAndTag(), "--format", "{{.Os}}\t{{.Architecture}}"}});
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return make_unexpected(Tr::tr("Failed to inspect image: %1").arg(proc.allOutput()));

    const QString out = proc.cleanedStdOut().trimmed();
    const QStringList parts = out.split('\t');
    if (parts.size() != 2)
        return make_unexpected(Tr::tr("Could not parse image inspect output: %1").arg(out));

    auto os = osTypeFromString(parts.at(0));
    auto arch = osArchFromString(parts.at(1));
    if (!os)
        return make_unexpected(os.error());
    if (!arch)
        return make_unexpected(arch.error());

    return qMakePair(os.value(), arch.value());
}

expected_str<Environment> DockerDevicePrivate::environment()
{
    if (!m_cachedEnviroment) {
        expected_str<void> result = fetchSystemEnviroment();
        if (!result)
            return make_unexpected(result.error());
    }

    QTC_ASSERT(m_cachedEnviroment, return {});
    return m_cachedEnviroment.value();
}

void DockerDevicePrivate::shutdown()
{
    m_isShutdown = true;
    stopCurrentContainer();
}

void DockerDevicePrivate::changeMounts(QStringList newMounts)
{
    newMounts.removeDuplicates();
    if (q->mounts.value() != newMounts) {
        q->mounts.value() = newMounts;
        stopCurrentContainer(); // Force re-start with new mounts.
    }
}

expected_str<FilePath> DockerDevicePrivate::localSource(const FilePath &other) const
{
    const auto devicePath = FilePath::fromString(other.path());
    for (const FilePath &mount : q->mounts()) {
        const FilePath mountPoint = mount;
        if (devicePath.isChildOf(mountPoint)) {
            const FilePath relativePath = devicePath.relativeChildPath(mountPoint);
            return mountPoint.pathAppended(relativePath.path());
        }
    }

    return make_unexpected(Tr::tr("localSource: No mount point found for %1").arg(other.toString()));
}

bool DockerDevicePrivate::ensureReachable(const FilePath &other)
{
    if (other.isSameDevice(q->rootPath()))
        return true;

    for (const FilePath &mount : q->mounts()) {
        if (other.isChildOf(mount))
            return true;

        if (mount == other)
            return true;
    }

    if (q->filePath(other.path()).exists())
        return false;

    return false;
}

bool DockerDevice::prepareForBuild(const Target *target)
{
    return d->prepareForBuild(target);
}

std::optional<FilePath> DockerDevice::clangdExecutable() const
{
    return d->clangdExecutable();
}

} // namespace Docker::Internal
