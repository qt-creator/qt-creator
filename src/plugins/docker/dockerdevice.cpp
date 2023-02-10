// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevice.h"

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevicewidget.h"
#include "dockertr.h"
#include "kitdetector.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
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
#include <utils/temporaryfile.h>
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
#include <QNetworkInterface>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QThread>
#include <QToolButton>

#include <numeric>
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

class ContainerShell : public Utils::DeviceShell
{
public:
    ContainerShell(DockerSettings *settings, const QString &containerId, const FilePath &devicePath)
        : m_settings(settings)
        , m_containerId(containerId)
        , m_devicePath(devicePath)
    {}

private:
    void setupShellProcess(QtcProcess *shellProcess) final
    {
        shellProcess->setCommand({m_settings->dockerBinaryPath.filePath(),
                                  {"container", "start", "-i", "-a", m_containerId}});
    }

    CommandLine createFallbackCommand(const CommandLine &cmdLine)
    {
        CommandLine result = cmdLine;
        result.setExecutable(cmdLine.executable().onDevice(m_devicePath));
        return result;
    }

private:
    DockerSettings *m_settings;
    QString m_containerId;
    FilePath m_devicePath;
};

class DockerDeviceFileAccess : public UnixDeviceFileAccess
{
public:
    DockerDeviceFileAccess(DockerDevicePrivate *dev)
        : m_dev(dev)
    {}

    RunResult runInShell(const CommandLine &cmdLine,
                         const QByteArray &stdInData) const override;
    QString mapToDevicePath(const QString &hostPath) const override;
    OsType osType(const FilePath &filePath) const override;

    DockerDevicePrivate *m_dev = nullptr;
};

class DockerDevicePrivate : public QObject
{
public:
    DockerDevicePrivate(DockerDevice *parent, DockerSettings *settings, DockerDeviceData data)
        : q(parent)
        , m_data(std::move(data))
        , m_settings(settings)
    {}

    ~DockerDevicePrivate() { stopCurrentContainer(); }

    RunResult runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    void updateContainerAccess();
    void changeMounts(QStringList newMounts);
    bool ensureReachable(const FilePath &other);
    void shutdown();
    expected_str<FilePath> localSource(const FilePath &other) const;

    QString containerId() { return m_container; }
    DockerDeviceData data() { return m_data; }
    void setData(const DockerDeviceData &data);
    DockerSettings *settings() { return m_settings; }

    QString repoAndTag() const { return m_data.repoAndTag(); }
    QString repoAndTagEncoded() const { return m_data.repoAndTagEncoded(); }
    QString dockerImageId() const { return m_data.imageId; }

    Environment environment();

    CommandLine withDockerExecCmd(const CommandLine &cmd,
                                  Environment *env = nullptr,
                                  FilePath *workDir = nullptr,
                                  bool interactive = false);

    bool prepareForBuild(const Target *target);
    Tasks validateMounts() const;

    bool createContainer();
    void startContainer();
    void stopCurrentContainer();
    void fetchSystemEnviroment();

    std::optional<FilePath> clangdExecutable() const
    {
        if (m_data.clangdExecutable.isEmpty())
            return std::nullopt;
        return m_data.clangdExecutable;
    }

    bool addTemporaryMount(const FilePath &path, const FilePath &containerPath);

    QStringList createMountArgs() const;

    DockerDevice *const q;
    DockerDeviceData m_data;
    DockerSettings *m_settings;

    struct TemporaryMountInfo
    {
        FilePath path;
        FilePath containerPath;
    };

    QList<TemporaryMountInfo> m_temporaryMounts;

    std::unique_ptr<ContainerShell> m_shell;

    QString m_container;

    std::optional<Environment> m_cachedEnviroment;
    bool m_isShutdown = false;
    DockerDeviceFileAccess m_fileAccess{this};
};

class DockerProcessImpl : public Utils::ProcessInterface
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

    QtcProcess m_process;
    qint64 m_remotePID = 0;
    bool m_hasReceivedFirstOutput = false;
};

DockerProcessImpl::DockerProcessImpl(IDevice::ConstPtr device, DockerDevicePrivate *devicePrivate)
    : m_devicePrivate(devicePrivate)
    , m_device(std::move(device))
    , m_process(this)
{
    connect(&m_process, &QtcProcess::started, this, [this] {
        qCDebug(dockerDeviceLog) << "Process started:" << m_process.commandLine();
    });

    connect(&m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
        if (!m_hasReceivedFirstOutput) {
            QByteArray output = m_process.readAllRawStandardOutput();
            qsizetype idx = output.indexOf('\n');
            QByteArray firstLine = output.left(idx);
            QByteArray rest = output.mid(idx + 1);
            qCDebug(dockerDeviceLog)
                << "Process first line received:" << m_process.commandLine() << firstLine;
            if (firstLine.startsWith("__qtc")) {
                bool ok = false;
                m_remotePID = firstLine.mid(5, firstLine.size() - 5 - 5).toLongLong(&ok);

                if (ok)
                    emit started(m_remotePID);

                if (rest.size() > 0)
                    emit readyRead(rest, {});

                m_hasReceivedFirstOutput = true;
                return;
            }
        }
        emit readyRead(m_process.readAllRawStandardOutput(), {});
    });

    connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
        emit readyRead({}, m_process.readAllRawStandardError());
    });

    connect(&m_process, &QtcProcess::done, this, [this] {
        qCDebug(dockerDeviceLog) << "Process exited:" << m_process.commandLine()
                                 << "with code:" << m_process.resultData().m_exitCode;
        emit done(m_process.resultData());
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
    m_process.setReaperTimeout(m_setup.m_reaperTimeout);
    m_process.setWriteData(m_setup.m_writeData);
    m_process.setProcessChannelMode(m_setup.m_processChannelMode);
    m_process.setExtraData(m_setup.m_extraData);
    m_process.setStandardInputFile(m_setup.m_standardInputFile);
    m_process.setAbortOnMetaChars(m_setup.m_abortOnMetaChars);
    if (m_setup.m_lowPriority)
        m_process.setLowPriority();

    const bool interactive = m_setup.m_processMode == ProcessMode::Writer
                             || !m_setup.m_writeData.isEmpty();

    const CommandLine fullCommandLine = m_devicePrivate
                                            ->withDockerExecCmd(m_setup.m_commandLine,
                                                                &m_setup.m_environment,
                                                                &m_setup.m_workingDirectory,
                                                                interactive);

    m_process.setCommand(fullCommandLine);
    m_process.start();
}

qint64 DockerProcessImpl::write(const QByteArray &data)
{
    return m_process.writeRaw(data);
}

void DockerProcessImpl::sendControlSignal(ControlSignal controlSignal)
{
    QTC_ASSERT(m_remotePID, return);
    if (controlSignal == ControlSignal::CloseWriteChannel) {
        m_process.closeWriteChannel();
        return;
    }
    const int signal = controlSignalToInt(controlSignal);
    m_devicePrivate->runInShell(
        {"kill", {QString("-%1").arg(signal), QString("%2").arg(m_remotePID)}});
}

IDeviceWidget *DockerDevice::createWidget()
{
    return new DockerDeviceWidget(sharedFromThis());
}

Tasks DockerDevice::validate() const
{
    return d->validateMounts();
}

Tasks DockerDevicePrivate::validateMounts() const
{
    Tasks result;

    for (const QString &mount : m_data.mounts) {
        const FilePath path = FilePath::fromUserInput(mount);
        if (!path.isDir()) {
            const QString message = Tr::tr("Path \"%1\" is not a directory or does not exist.")
                                        .arg(mount);

            result.append(Task(Task::Error, message, {}, -1, {}));
        }
    }
    return result;
}

RunResult DockerDeviceFileAccess::runInShell(const CommandLine &cmdLine,
                                             const QByteArray &stdInData) const
{
    QTC_ASSERT(m_dev, return {});
    return m_dev->runInShell(cmdLine, stdInData);
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

OsType DockerDeviceFileAccess::osType(const FilePath &filePath) const
{
    QTC_ASSERT(m_dev, return UnixDeviceFileAccess::osType(filePath));
    return m_dev->q->osType();
}

DockerDevice::DockerDevice(DockerSettings *settings, const DockerDeviceData &data)
    : d(new DockerDevicePrivate(this, settings, data))
{
    setFileAccess(&d->m_fileAccess);
    setDisplayType(Tr::tr("Docker"));
    setOsType(OsTypeOtherUnix);
    setDefaultDisplayName(Tr::tr("Docker Image"));

    setDisplayName(Tr::tr("Docker Image \"%1\" (%2)").arg(data.repoAndTag()).arg(data.imageId));
    setAllowEmptyCommand(true);

    setOpenTerminal([this, settings](const Environment &env, const FilePath &workingDir) {
        Q_UNUSED(env); // TODO: That's the runnable's environment in general. Use it via -e below.
        updateContainerAccess();
        if (d->containerId().isEmpty()) {
            MessageManager::writeDisrupting(Tr::tr("Error starting remote shell. No container."));
            return;
        }

        QtcProcess *proc = new QtcProcess(d);
        proc->setTerminalMode(TerminalMode::On);

        QObject::connect(proc, &QtcProcess::done, [proc] {
            if (proc->error() != QProcess::UnknownError && MessageManager::instance()) {
                MessageManager::writeDisrupting(
                    Tr::tr("Error starting remote shell: %1").arg(proc->errorString()));
            }
            proc->deleteLater();
        });

        const QString wd = workingDir.isEmpty() ? "/" : workingDir.path();
        proc->setCommand({settings->dockerBinaryPath.filePath(),
                          {"exec", "-it", "-w", wd, d->containerId(), "/bin/sh"}});
        proc->setEnvironment(Environment::systemEnvironment()); // The host system env. Intentional.
        proc->start();
    });

    addDeviceAction({Tr::tr("Open Shell in Container"), [](const IDevice::Ptr &device, QWidget *) {
                         device->openTerminal(device->systemEnvironment(), FilePath());
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

const DockerDeviceData DockerDevice::data() const
{
    return d->data();
}

DockerDeviceData DockerDevice::data()
{
    return d->data();
}

void DockerDevice::setData(const DockerDeviceData &data)
{
    d->setData(data);
}

void DockerDevice::updateContainerAccess() const
{
    d->updateContainerAccess();
}

CommandLine DockerDevicePrivate::withDockerExecCmd(const CommandLine &cmd,
                                                   Environment *env,
                                                   FilePath *workDir,
                                                   bool interactive)
{
    if (!m_settings)
        return {};

    updateContainerAccess();

    CommandLine dockerCmd{m_settings->dockerBinaryPath.filePath(), {"exec"}};

    if (interactive)
        dockerCmd.addArg("-i");

    if (env) {
        for (auto it = env->constBegin(); it != env->constEnd(); ++it) {
            dockerCmd.addArg("-e");
            dockerCmd.addArg(env->key(it) + "=" + env->expandedValueForKey(env->key(it)));
        }
    }

    if (workDir && !workDir->isEmpty())
        dockerCmd.addArgs({"-w", workDir->path()});

    dockerCmd.addArg(m_container);
    dockerCmd.addArgs({"/bin/sh", "-c"});

    CommandLine exec("exec");
    exec.addCommandLineAsArgs(cmd);

    CommandLine echo("echo");
    echo.addArgs("__qtc$$qtc__", CommandLine::Raw);
    echo.addCommandLineWithAnd(exec);

    dockerCmd.addCommandLineAsSingleArg(echo);

    return dockerCmd;
}

void DockerDevicePrivate::stopCurrentContainer()
{
    if (!m_settings)
        return;
    if (m_container.isEmpty())
        return;
    if (!DockerApi::isDockerDaemonAvailable(false).value_or(false))
        return;

    m_shell.reset();

    QtcProcess proc;
    proc.setCommand({m_settings->dockerBinaryPath.filePath(), {"container", "stop", m_container}});

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

QStringList toMountArg(const DockerDevicePrivate::TemporaryMountInfo &mi)
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

expected_str<void> isValidMountInfo(const DockerDevicePrivate::TemporaryMountInfo &mi)
{
    if (mi.path.needsDevice())
        return make_unexpected(QString("Path \"%1\" is not local").arg(mi.path.toUserOutput()));

    if (mi.path.isEmpty() && mi.containerPath.isEmpty())
        return make_unexpected(QString("Both paths are empty"));

    if (mi.path.isEmpty()) {
        return make_unexpected(QString("Local path is empty, container path is \"%1\"")
                                   .arg(mi.containerPath.toUserOutput()));
    }

    if (mi.containerPath.isEmpty()) {
        return make_unexpected(
            QString("Container path is empty, local path is \"%1\"").arg(mi.path.toUserOutput()));
    }

    if (!mi.path.isAbsolutePath() || !mi.containerPath.isAbsolutePath()) {
        return make_unexpected(QString("Path \"%1\" or \"%2\" is not absolute")
                                   .arg(mi.path.toUserOutput())
                                   .arg(mi.containerPath.toUserOutput()));
    }

    if (mi.containerPath.isRootPath())
        return make_unexpected(QString("Path \"%1\" is root").arg(mi.containerPath.toUserOutput()));

    if (!mi.path.exists())
        return make_unexpected(QString("Path \"%1\" does not exist").arg(mi.path.toUserOutput()));

    return {};
}

QStringList DockerDevicePrivate::createMountArgs() const
{
    QStringList cmds;
    QList<TemporaryMountInfo> mounts = m_temporaryMounts;
    for (const QString &m : m_data.mounts)
        mounts.append({FilePath::fromUserInput(m), FilePath::fromUserInput(m)});

    for (const TemporaryMountInfo &mi : mounts) {
        if (isValidMountInfo(mi))
            cmds += toMountArg(mi);
    }

    return cmds;
}

bool DockerDevicePrivate::createContainer()
{
    if (!m_settings)
        return false;

    const QString display = HostOsInfo::isLinuxHost() ? QString(":0")
                                                      : QString("host.docker.internal:0");
    CommandLine dockerCreate{m_settings->dockerBinaryPath.filePath(),
                             {"create",
                              "-i",
                              "--rm",
                              "-e",
                              QString("DISPLAY=%1").arg(display),
                              "-e",
                              "XAUTHORITY=/.Xauthority",
                              "--net",
                              "host"}};

#ifdef Q_OS_UNIX
    // no getuid() and getgid() on Windows.
    if (m_data.useLocalUidGid)
        dockerCreate.addArgs({"-u", QString("%1:%2").arg(getuid()).arg(getgid())});
#endif
    FilePath dumperPath = FilePath::fromString("/tmp/qtcreator/debugger");
    addTemporaryMount(Core::ICore::resourcePath("debugger/"), dumperPath);
    q->setDebugDumperPath(dumperPath);

    dockerCreate.addArgs(createMountArgs());

    if (!m_data.keepEntryPoint)
        dockerCreate.addArgs({"--entrypoint", "/bin/sh"});

    if (m_data.enableLldbFlags)
        dockerCreate.addArgs({"--cap-add=SYS_PTRACE", "--security-opt", "seccomp=unconfined"});

    dockerCreate.addArg(m_data.repoAndTag());

    qCDebug(dockerDeviceLog).noquote() << "RUNNING: " << dockerCreate.toUserOutput();
    QtcProcess createProcess;
    createProcess.setCommand(dockerCreate);
    createProcess.runBlocking();

    if (createProcess.result() != ProcessResult::FinishedWithSuccess) {
        qCWarning(dockerDeviceLog) << "Failed creating docker container:";
        qCWarning(dockerDeviceLog) << "Exit Code:" << createProcess.exitCode();
        qCWarning(dockerDeviceLog) << createProcess.allOutput();
        return false;
    }

    m_container = createProcess.cleanedStdOut().trimmed();
    if (m_container.isEmpty())
        return false;

    qCDebug(dockerDeviceLog) << "ContainerId:" << m_container;
    return true;
}

void DockerDevicePrivate::startContainer()
{
    if (!createContainer())
        return;

    m_shell = std::make_unique<ContainerShell>(m_settings, m_container, q->rootPath());

    connect(m_shell.get(), &DeviceShell::done, this, [this](const ProcessResultData &resultData) {
        if (resultData.m_error != QProcess::UnknownError
            || resultData.m_exitStatus == QProcess::NormalExit)
            return;

        qCWarning(dockerDeviceLog) << "Container shell encountered error:" << resultData.m_error;
        m_shell.release()->deleteLater();

        DockerApi::recheckDockerDaemon();
        MessageManager::writeFlashing(Tr::tr("Docker daemon appears to be not running. "
                                             "Verify daemon is up and running and reset the "
                                             "docker daemon on the docker device settings page "
                                             "or restart Qt Creator."));
    });

    if (!m_shell->start()) {
        qCWarning(dockerDeviceLog) << "Container shell failed to start";
    }
}

void DockerDevicePrivate::updateContainerAccess()
{
    if (m_isShutdown)
        return;

    if (DockerApi::isDockerDaemonAvailable(false).value_or(false) == false)
        return;

    if (m_shell)
        return;

    startContainer();
}

void DockerDevice::setMounts(const QStringList &mounts) const
{
    d->changeMounts(mounts);
}

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceDataSizeKey[] = "DockerDeviceDataSize";
const char DockerDeviceUseOutsideUser[] = "DockerDeviceUseUidGid";
const char DockerDeviceMappedPaths[] = "DockerDeviceMappedPaths";
const char DockerDeviceKeepEntryPoint[] = "DockerDeviceKeepEntryPoint";
const char DockerDeviceEnableLldbFlags[] = "DockerDeviceEnableLldbFlags";
const char DockerDeviceClangDExecutable[] = "DockerDeviceClangDExecutable";

void DockerDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    DockerDeviceData data;

    data.repo = map.value(DockerDeviceDataRepoKey).toString();
    data.tag = map.value(DockerDeviceDataTagKey).toString();
    data.imageId = map.value(DockerDeviceDataImageIdKey).toString();
    data.size = map.value(DockerDeviceDataSizeKey).toString();
    data.useLocalUidGid = map.value(DockerDeviceUseOutsideUser, HostOsInfo::isLinuxHost()).toBool();
    data.mounts = map.value(DockerDeviceMappedPaths).toStringList();
    data.keepEntryPoint = map.value(DockerDeviceKeepEntryPoint).toBool();
    data.enableLldbFlags = map.value(DockerDeviceEnableLldbFlags).toBool();
    data.clangdExecutable = FilePath::fromSettings(map.value(DockerDeviceClangDExecutable));
    d->setData(data);
}

QVariantMap DockerDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    DockerDeviceData data = d->data();

    map.insert(DockerDeviceDataRepoKey, data.repo);
    map.insert(DockerDeviceDataTagKey, data.tag);
    map.insert(DockerDeviceDataImageIdKey, data.imageId);
    map.insert(DockerDeviceDataSizeKey, data.size);
    map.insert(DockerDeviceUseOutsideUser, data.useLocalUidGid);
    map.insert(DockerDeviceMappedPaths, data.mounts);
    map.insert(DockerDeviceKeepEntryPoint, data.keepEntryPoint);
    map.insert(DockerDeviceEnableLldbFlags, data.enableLldbFlags);
    map.insert(DockerDeviceClangDExecutable, data.clangdExecutable.toSettings());
    return map;
}

ProcessInterface *DockerDevice::createProcessInterface() const
{
    return new DockerProcessImpl(this->sharedFromThis(), d);
}

bool DockerDevice::canAutoDetectPorts() const
{
    return true;
}

PortsGatheringMethod DockerDevice::portsGatheringMethod() const
{
    return {[this](QAbstractSocket::NetworkLayerProtocol protocol) -> CommandLine {
                // We might encounter the situation that protocol is given IPv6
                // but the consumer of the free port information decides to open
                // an IPv4(only) port. As a result the next IPv6 scan will
                // report the port again as open (in IPv6 namespace), while the
                // same port in IPv4 namespace might still be blocked, and
                // re-use of this port fails.
                // GDBserver behaves exactly like this.

                Q_UNUSED(protocol)

                // /proc/net/tcp* covers /proc/net/tcp and /proc/net/tcp6
                return {filePath("sed"),
                        "-e 's/.*: [[:xdigit:]]*:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp*",
                        CommandLine::Raw};
            },

            &Port::parseFromSedOutput};
};

DeviceProcessList *DockerDevice::createProcessListModel(QObject *) const
{
    return nullptr;
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
                               d->repoAndTagEncoded(),
                               pathOnDevice);

// The following would work, but gives no hint on repo and tag
//   result.setScheme("docker");
//    result.setHost(d->m_data.imageId);

// The following would work, but gives no hint on repo, tag and imageid
//    result.setScheme("device");
//    result.setHost(id().toString());
}

Utils::FilePath DockerDevice::rootPath() const
{
    return FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME, d->repoAndTagEncoded(), u"/");
}

bool DockerDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"device" && filePath.host() == id().toString())
        return true;

    const bool isDockerScheme = filePath.scheme() == Constants::DOCKER_DEVICE_SCHEME;

    if (isDockerScheme && filePath.host() == d->dockerImageId())
        return true;

    if (isDockerScheme && filePath.host() == d->repoAndTagEncoded())
        return true;

    if (isDockerScheme && filePath.host() == d->repoAndTag())
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

expected_str<FilePath> DockerDevice::localSource(const Utils::FilePath &other) const
{
    return d->localSource(other);
}

Environment DockerDevice::systemEnvironment() const
{
    return d->environment();
}

void DockerDevice::aboutToBeRemoved() const
{
    KitDetector detector(sharedFromThis());
    detector.undoAutoDetect(id().toString());
}

void DockerDevicePrivate::fetchSystemEnviroment()
{
    updateContainerAccess();

    if (m_shell && m_shell->state() == DeviceShell::State::Succeeded) {
        const RunResult result = runInShell({"env", {}});
        const QString out = QString::fromUtf8(result.stdOut);
        m_cachedEnviroment = Environment(out.split('\n', Qt::SkipEmptyParts), q->osType());
        return;
    }

    QtcProcess proc;
    proc.setCommand(withDockerExecCmd({"env", {}}));
    proc.start();
    proc.waitForFinished();
    const QString remoteOutput = proc.cleanedStdOut();

    m_cachedEnviroment = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());

    const QString remoteError = proc.cleanedStdErr();
    if (!remoteError.isEmpty())
        qCWarning(dockerDeviceLog) << "Cannot read container environment:", qPrintable(remoteError);
}

RunResult DockerDevicePrivate::runInShell(const CommandLine &cmd, const QByteArray &stdInData)
{
    updateContainerAccess();
    QTC_ASSERT(m_shell, return {});
    return m_shell->runInShell(cmd, stdInData);
}

// Factory

class DockerImageItem final : public TreeItem, public DockerDeviceData
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
};

class DockerDeviceSetupWizard final : public QDialog
{
public:
    DockerDeviceSetupWizard(DockerSettings *settings)
        : QDialog(ICore::dialogParent())
        , m_settings(settings)
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
                             + QCoreApplication::translate("::Debugger", "Process failed to start.");
        auto errorLabel = new Utils::InfoLabel(fail, Utils::InfoLabel::Error, this);
        errorLabel->setVisible(false);

        m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        using namespace Layouting;

        Column {
            Stack {
                statusLabel,
                m_view,
            },
            m_log,
            errorLabel,
            Row{showUnnamedContainers, m_buttons},
        }
            .attachTo(this);

        connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

        CommandLine cmd{m_settings->dockerBinaryPath.filePath(),
                        {"images", "--format", "{{.ID}}\\t{{.Repository}}\\t{{.Tag}}\\t{{.Size}}"}};
        m_log->append(Tr::tr("Running \"%1\"\n").arg(cmd.toUserOutput()));

        m_process = new QtcProcess(this);
        m_process->setCommand(cmd);

        connect(m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
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

        connect(m_process, &Utils::QtcProcess::readyReadStandardError, this, [this] {
            const QString out = Tr::tr("Error: %1").arg(m_process->cleanedStdErr());
            m_log->append(Tr::tr("Error: %1").arg(out));
        });

        connect(m_process, &QtcProcess::done, errorLabel, [errorLabel, this, statusLabel] {
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

        auto device = DockerDevice::create(m_settings, *item);
        device->setupId(IDevice::ManuallyAdded);
        device->setType(Constants::DOCKER_DEVICE_TYPE);
        device->setMachineType(IDevice::Hardware);

        return device;
    }

public:
    TreeModel<DockerImageItem> m_model;
    TreeView *m_view = nullptr;
    SortFilterModel *m_proxyModel = nullptr;
    QTextBrowser *m_log = nullptr;
    QDialogButtonBox *m_buttons;
    DockerSettings *m_settings;

    QtcProcess *m_process = nullptr;
    QString m_selectedId;
};

// Factory

DockerDeviceFactory::DockerDeviceFactory(DockerSettings *settings)
    : IDeviceFactory(Constants::DOCKER_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("Docker Device"));
    setIcon(QIcon());
    setCreator([settings] {
        DockerDeviceSetupWizard wizard(settings);
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
    setConstructionFunction([settings, this] {
        auto device = DockerDevice::create(settings, {});
        QMutexLocker lk(&m_deviceListMutex);
        m_existingDevices.push_back(device);
        return device;
    });
}

void DockerDeviceFactory::shutdownExistingDevices()
{
    QMutexLocker lk(&m_deviceListMutex);
    for (const auto &weakDevice : m_existingDevices) {
        if (QSharedPointer<DockerDevice> device = weakDevice.lock())
            device->shutdown();
    }
}

bool DockerDevicePrivate::addTemporaryMount(const FilePath &path, const FilePath &containerPath)
{
    const bool alreadyAdded = anyOf(m_temporaryMounts,
                                    [containerPath](const TemporaryMountInfo &info) {
                                        return info.containerPath == containerPath;
                                    });

    if (alreadyAdded)
        return false;

    const bool alreadyManuallyAdded = anyOf(m_data.mounts, [path](const QString &mount) {
        return mount == path.path();
    });

    if (alreadyManuallyAdded)
        return false;

    const TemporaryMountInfo newMount{path, containerPath};

    const expected_str<void> result = isValidMountInfo(newMount);
    QTC_ASSERT_EXPECTED(result, return false);

    qCDebug(dockerDeviceLog) << "Adding temporary mount:" << path;
    m_temporaryMounts.append(newMount);
    stopCurrentContainer(); // Force re-start with new mounts.
    return true;
}

Environment DockerDevicePrivate::environment()
{
    if (!m_cachedEnviroment)
        fetchSystemEnviroment();

    QTC_ASSERT(m_cachedEnviroment, return {});
    return m_cachedEnviroment.value();
}

void DockerDevicePrivate::shutdown()
{
    m_isShutdown = true;
    m_settings = nullptr;
    stopCurrentContainer();
}

void DockerDevicePrivate::changeMounts(QStringList newMounts)
{
    newMounts.removeDuplicates();
    if (m_data.mounts != newMounts) {
        m_data.mounts = newMounts;
        stopCurrentContainer(); // Force re-start with new mounts.
    }
}

expected_str<FilePath> DockerDevicePrivate::localSource(const FilePath &other) const
{
    const auto devicePath = FilePath::fromString(other.path());
    for (const TemporaryMountInfo &info : m_temporaryMounts) {
        if (devicePath.isChildOf(info.containerPath)) {
            const FilePath relativePath = devicePath.relativeChildPath(info.containerPath);
            return info.path.pathAppended(relativePath.path());
        }
    }

    for (const QString &mount : m_data.mounts) {
        const FilePath mountPoint = FilePath::fromString(mount);
        if (devicePath.isChildOf(mountPoint)) {
            const FilePath relativePath = devicePath.relativeChildPath(mountPoint);
            return mountPoint.pathAppended(relativePath.path());
        }
    }

    return make_unexpected(Tr::tr("localSource: No mount point found for %1").arg(other.toString()));
}

bool DockerDevicePrivate::ensureReachable(const FilePath &other)
{
    for (const QString &mount : m_data.mounts) {
        const FilePath fMount = FilePath::fromString(mount);
        if (other.isChildOf(fMount))
            return true;
    }

    for (const auto &[path, containerPath] : m_temporaryMounts) {
        if (path.path() != containerPath.path())
            continue;

        if (other.isChildOf(path))
            return true;
    }

    addTemporaryMount(other, other);
    return true;
}

void DockerDevicePrivate::setData(const DockerDeviceData &data)
{
    if (m_data != data) {
        m_data = data;

        // Force restart if the container is already running
        if (!m_container.isEmpty()) {
            stopCurrentContainer();
        }
    }
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
