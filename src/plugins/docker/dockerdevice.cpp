// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
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

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

Q_LOGGING_CATEGORY(dockerDeviceLog, "qtc.docker.device", QtWarningMsg);

namespace Docker::Internal {

const QString s_pidMarker = "__qtc$$qtc__";

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
    bool runInShellSuccess(const CommandLine &cmd, const QByteArray &stdInData = {}) {
        return runInShell(cmd, stdInData).exitCode == 0;
    }

    std::optional<QByteArray> fileContents(const FilePath &filePath, qint64 limit, qint64 offset);

    void updateContainerAccess();
    void changeMounts(QStringList newMounts);
    bool ensureReachable(const FilePath &other);
    void shutdown();

    QString containerId() { return m_container; }
    DockerDeviceData data() { return m_data; }
    void setData(const DockerDeviceData &data);
    DockerSettings *settings() { return m_settings; }

    QString repoAndTag() const { return m_data.repoAndTag(); }
    QString dockerImageId() const { return m_data.imageId; }

    Environment environment();

    CommandLine withDockerExecCmd(const CommandLine &cmd, bool interactive = false);

    bool prepareForBuild(const Target *target);
    Tasks validateMounts() const;

    bool createContainer();
    void startContainer();
    void stopCurrentContainer();
    void fetchSystemEnviroment();

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

    Environment m_cachedEnviroment;
    bool m_useFind = true; // prefer find over ls and hacks, but be able to use ls as fallback
    bool m_isShutdown = false;
};

class DockerProcessImpl : public Utils::ProcessInterface
{
public:
    DockerProcessImpl(IDevice::ConstPtr device, DockerDevicePrivate *devicePrivate);
    virtual ~DockerProcessImpl();

private:
    void start() override;
    qint64 write(const QByteArray &data) override;
    void sendControlSignal(ControlSignal controlSignal) override;

private:
    CommandLine fullLocalCommandLine(bool interactive);

private:
    DockerDevicePrivate *m_devicePrivate = nullptr;
    // Store the IDevice::ConstPtr in order to extend the lifetime of device for as long
    // as this object is alive.
    IDevice::ConstPtr m_device;

    QtcProcess m_process;
    qint64 m_remotePID = 0;
    bool m_hasReceivedFirstOutput = false;
};

CommandLine DockerProcessImpl::fullLocalCommandLine(bool interactive)
{
    QStringList args;

    if (!m_setup.m_workingDirectory.isEmpty()) {
        QTC_CHECK(DeviceManager::deviceForPath(m_setup.m_workingDirectory) == m_device);
        args.append({"cd", m_setup.m_workingDirectory.path()});
        args.append("&&");
    }

    args.append({"echo", s_pidMarker, "&&"});

    const Environment &env = m_setup.m_environment;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it)
        args.append(env.key(it) + "='" + env.expandedValueForKey(env.key(it)) + '\'');

    args.append("exec");
    args.append({m_setup.m_commandLine.executable().path(), m_setup.m_commandLine.arguments()});

    CommandLine shCmd("/bin/sh", {"-c", args.join(" ")});
    return m_devicePrivate->withDockerExecCmd(shCmd, interactive);
}

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
            QByteArray output = m_process.readAllStandardOutput();
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
        emit readyRead(m_process.readAllStandardOutput(), {});
    });

    connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
        emit readyRead({}, m_process.readAllStandardError());
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

    m_process.setCommand(fullLocalCommandLine(m_setup.m_processMode == ProcessMode::Writer));
    m_process.start();
}

qint64 DockerProcessImpl::write(const QByteArray &data)
{
    return m_process.writeRaw(data);
}

void DockerProcessImpl::sendControlSignal(ControlSignal controlSignal)
{
    QTC_ASSERT(m_remotePID, return);
    int signal = controlSignalToInt(controlSignal);
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

DockerDevice::DockerDevice(DockerSettings *settings, const DockerDeviceData &data)
    : d(new DockerDevicePrivate(this, settings, data))
{
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

CommandLine DockerDevicePrivate::withDockerExecCmd(const CommandLine &cmd, bool interactive)
{
    if (!m_settings)
        return {};

    updateContainerAccess();

    QStringList args;

    args << "exec";
    if (interactive)
        args << "-i";
    args << m_container;

    CommandLine dcmd{m_settings->dockerBinaryPath.filePath(), args};
    dcmd.addCommandLineAsArgs(cmd);
    return dcmd;
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

    m_cachedEnviroment.clear();
}

static QString getLocalIPv4Address()
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (auto &a : addresses) {
        if (a.isInSubnet(QHostAddress("192.168.0.0"), 16))
            return a.toString();
        if (a.isInSubnet(QHostAddress("10.0.0.0"), 8))
            return a.toString();
        if (a.isInSubnet(QHostAddress("172.16.0.0"), 12))
            return a.toString();
    }
    return QString();
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

QStringList toMountArg(const DockerDevicePrivate::TemporaryMountInfo &mi)
{
    QString escapedPath;
    QString escapedContainerPath;

    if (HostOsInfo::isWindowsHost()) {
        escapedPath = escapeMountPathWin(mi.path);
        escapedContainerPath = escapeMountPathWin(mi.containerPath);
    } else {
        escapedPath = escapeMountPathUnix(mi.path);
        escapedContainerPath = escapeMountPathUnix(mi.containerPath);
    }

    const QString mountArg = QString(R"(type=bind,"source=%1","destination=%2")")
                                 .arg(escapedPath)
                                 .arg(escapedContainerPath);

    return QStringList{"--mount", mountArg};
}

bool isValidMountInfo(const DockerDevicePrivate::TemporaryMountInfo &mi)
{
    return !mi.path.isEmpty() && !mi.containerPath.isEmpty() && mi.path.isAbsolutePath()
           && mi.containerPath.isAbsolutePath();
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
                                                      : QString(getLocalIPv4Address() + ":0.0");
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

    m_shell = std::make_unique<ContainerShell>(m_settings,
                                               m_container,
                                               FilePath::fromString(
                                                   QString("device://%1/")
                                                       .arg(this->q->id().toString())));

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

DeviceProcessSignalOperation::Ptr DockerDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

DeviceEnvironmentFetcher::Ptr DockerDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr();
}

bool DockerDevice::usableAsBuildDevice() const
{
    return true;
}

FilePath DockerDevice::mapToGlobalPath(const FilePath &pathOnDevice) const
{
    if (pathOnDevice.needsDevice()) {
        // Already correct form, only sanity check it's ours...
        QTC_CHECK(handlesFile(pathOnDevice));
        return pathOnDevice;
    }

    return FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME,
                               d->repoAndTag(),
                               pathOnDevice.path());

// The following would work, but gives no hint on repo and tag
//   result.setScheme("docker");
//    result.setHost(d->m_data.imageId);

// The following would work, but gives no hint on repo, tag and imageid
//    result.setScheme("device");
//    result.setHost(id().toString());
}

QString DockerDevice::mapToDevicePath(const Utils::FilePath &globalPath) const
{
    // make sure to convert windows style paths to unix style paths with the file system case:
    // C:/dev/src -> /c/dev/src
    const FilePath normalized = FilePath::fromString(globalPath.path()).normalizedPathName();
    QString path = normalized.path();
    if (HostOsInfo::isWindowsHost() && normalized.startsWithDriveLetter()) {
        const QChar lowerDriveLetter = path.at(0).toLower();
        path = '/' + lowerDriveLetter + path.mid(2); // strip C:
    }
    return path;
}

Utils::FilePath DockerDevice::rootPath() const
{
    return FilePath::fromParts(Constants::DOCKER_DEVICE_SCHEME, d->repoAndTag(), u"/");
}

bool DockerDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"device" && filePath.host() == id().toString())
        return true;

    const bool isDockerScheme = filePath.scheme() == Constants::DOCKER_DEVICE_SCHEME;

    if (isDockerScheme && filePath.host() == d->dockerImageId())
        return true;

    if (isDockerScheme && filePath.host() == QString(d->repoAndTag()))
        return true;

    return false;
}

bool DockerDevice::isExecutableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-x", path}});
}

bool DockerDevice::isReadableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-r", path, "-a", "-f", path}});
}

bool DockerDevice::isWritableFile(const Utils::FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-w", path, "-a", "-f", path}});
}

bool DockerDevice::isReadableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-r", path, "-a", "-d", path}});
}

bool DockerDevice::isWritableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-w", path, "-a", "-d", path}});
}

bool DockerDevice::isFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-f", path}});
}

bool DockerDevice::isDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-d", path}});
}

bool DockerDevice::createDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"mkdir", {"-p", path}});
}

bool DockerDevice::exists(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"test", {"-e", path}});
}

bool DockerDevice::ensureExistingFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShellSuccess({"touch", {path}});
}

bool DockerDevice::removeFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    return d->runInShellSuccess({"rm", {filePath.path()}});
}

bool DockerDevice::removeRecursively(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(filePath.path().startsWith('/'), return false);

    const QString path = filePath.cleanPath().path();
    // We are expecting this only to be called in a context of build directories or similar.
    // Chicken out in some cases that _might_ be user code errors.
    QTC_ASSERT(path.startsWith('/'), return false);
    const int levelsNeeded = path.startsWith("/home/") ? 4 : 3;
    QTC_ASSERT(path.count('/') >= levelsNeeded, return false);

    return d->runInShellSuccess({"rm", {"-rf", "--", path}});
}

bool DockerDevice::copyFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    return d->runInShellSuccess({"cp", {filePath.path(), target.path()}});
}

bool DockerDevice::renameFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    return d->runInShellSuccess({"mv", {filePath.path(), target.path()}});
}

QDateTime DockerDevice::lastModified(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const RunResult result = d->runInShell({"stat", {"-L", "-c", "%Y", filePath.path()}});
    qint64 secs = result.stdOut.toLongLong();
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(secs, Qt::UTC);
    return dt;
}

FilePath DockerDevice::symLinkTarget(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const RunResult result = d->runInShell({"readlink", {"-n", "-e", filePath.path()}});
    const QString out = QString::fromUtf8(result.stdOut);
    return out.isEmpty() ? FilePath() : filePath.withNewPath(out);
}

qint64 DockerDevice::fileSize(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return -1);
    const RunResult result = d->runInShell({"stat", {"-L", "-c", "%s", filePath.path()}});
    return result.stdOut.toLongLong();
}

QFileDevice::Permissions DockerDevice::permissions(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});

    const RunResult result = d->runInShell({"stat", {"-L", "-c", "%a", filePath.path()}});
    const uint bits = result.stdOut.toUInt(nullptr, 8);
    QFileDevice::Permissions perm = {};
#define BIT(n, p) if (bits & (1<<n)) perm |= QFileDevice::p
    BIT(0, ExeOther);
    BIT(1, WriteOther);
    BIT(2, ReadOther);
    BIT(3, ExeGroup);
    BIT(4, WriteGroup);
    BIT(5, ReadGroup);
    BIT(6, ExeUser);
    BIT(7, WriteUser);
    BIT(8, ReadUser);
#undef BIT
    return perm;
}

bool DockerDevice::setPermissions(const FilePath &filePath,
                                  QFileDevice::Permissions permissions) const
{
    Q_UNUSED(permissions)
    QTC_ASSERT(handlesFile(filePath), return {});
    QTC_CHECK(false); // FIXME: Implement.
    return false;
}

bool DockerDevice::ensureReachable(const FilePath &other) const
{
    if (other.needsDevice())
        return false;

    if (other.isDir())
        return d->ensureReachable(other);
    return d->ensureReachable(other.parentDir());
}

void DockerDevice::iterateDirectory(const FilePath &filePath,
                                    const FilePath::IterateDirCallback &callBack,
                                    const FileFilter &filter) const
{
    QTC_ASSERT(handlesFile(filePath), return);
    auto runInShell = [this](const CommandLine &cmd) { return d->runInShell(cmd); };
    FileUtils::iterateUnixDirectory(filePath, filter, &d->m_useFind, runInShell, callBack);
}

void DockerDevice::iterateDirectory(const FilePath &filePath,
                                    const FilePath::IterateDirWithInfoCallback &callBack,
                                    const FileFilter &filter) const
{
    QTC_ASSERT(handlesFile(filePath), return);
    auto runInShell = [this](const CommandLine &cmd) { return d->runInShell(cmd); };
    FileUtils::iterateUnixDirectory(filePath, filter, &d->m_useFind, runInShell, callBack);
}

std::optional<QByteArray> DockerDevice::fileContents(const FilePath &filePath,
                                                     qint64 limit,
                                                     qint64 offset) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    return d->fileContents(filePath, limit, offset);
}

bool DockerDevice::writeFileContents(const FilePath &filePath,
                                     const QByteArray &data,
                                     qint64 offset) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    CommandLine cmd({"dd", {"of=" + filePath.path()}});
    if (offset != 0) {
        cmd.addArg("bs=1");
        cmd.addArg(QString("seek=%1").arg(offset));
    }
    return d->runInShellSuccess(cmd, data);
}

FilePathInfo DockerDevice::filePathInfo(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const RunResult stat = d->runInShell({"stat", {"-L", "-c", "%f %Y %s", filePath.path()}});
    return FileUtils::filePathInfoFromTriple(QString::fromLatin1(stat.stdOut));
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

std::optional<QByteArray> DockerDevicePrivate::fileContents(const FilePath &filePath,
                                                            qint64 limit,
                                                            qint64 offset)
{
    updateContainerAccess();

    QStringList args = {"if=" + filePath.path(), "status=none"};
    if (limit > 0 || offset > 0) {
        const qint64 gcd = std::gcd(limit, offset);
        args += {QString("bs=%1").arg(gcd),
                 QString("count=%1").arg(limit / gcd),
                 QString("seek=%1").arg(offset / gcd)};
    }

    const RunResult r = m_shell->runInShell({"dd", args});

    if (r.exitCode != 0)
        return {};

    return r.stdOut;
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

        connect(showUnnamedContainers, &QCheckBox::toggled, this, [this]() {
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
                             + QCoreApplication::translate("Debugger", "Process failed to start.");
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

        connect(m_process, &QtcProcess::readyReadStandardOutput, [this] {
            const QString out = QString::fromUtf8(m_process->readAllStandardOutput().trimmed());
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

        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
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
    bool alreadyAdded = anyOf(m_temporaryMounts, [containerPath](const TemporaryMountInfo &info) {
        return info.containerPath == containerPath;
    });
    if (alreadyAdded)
        return false;

    qCDebug(dockerDeviceLog) << "Adding temporary mount:" << path;
    m_temporaryMounts.append({path, containerPath});
    stopCurrentContainer(); // Force re-start with new mounts.
    return true;
}

Environment DockerDevicePrivate::environment()
{
    if (!m_cachedEnviroment.isValid())
        fetchSystemEnviroment();

    QTC_CHECK(m_cachedEnviroment.isValid());
    return m_cachedEnviroment;
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

} // namespace Docker::Internal
