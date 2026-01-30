// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevice.h"

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockercontainerthread.h"
#include "dockerdevicewidget.h"
#include "dockersettings.h"
#include "dockertr.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/devicesupport/processlist.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/basetreeview.h>
#include <utils/devicefileaccess.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fsengine.h>
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
#include <QTimer>
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
using namespace Docker::Internal;

Q_LOGGING_CATEGORY(dockerDeviceLog, "qtc.docker.device", QtWarningMsg);

namespace Docker {
namespace Internal {

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceUseOutsideUser[] = "DockerDeviceUseUidGid";
const char DockerDeviceMappedPaths[] = "DockerDeviceMappedPaths";
const char DockerDeviceKeepEntryPoint[] = "DockerDeviceKeepEntryPoint";
const char DockerDeviceEnableLldbFlags[] = "DockerDeviceEnableLldbFlags";
const char DockerDeviceExtraArgs[] = "DockerDeviceExtraCreateArguments";
const char DockerDeviceEnvironment[] = "DockerDeviceEnvironment";

class DockerDeviceFileAccess final : public CmdBridge::FileAccess
{
public:
    explicit DockerDeviceFileAccess(DockerDevicePrivate *dev)
        : m_dev(dev)
    {}

    QString mapToDevicePath(const QString &hostPath) const final;

    DockerDevicePrivate *m_dev = nullptr;
};

class DockerFallbackFileAccess final : public UnixDeviceFileAccess
{
    const FilePath m_rootPath;

public:
    explicit DockerFallbackFileAccess(const FilePath &rootPath)
        : m_rootPath(rootPath)
    {}

    Result<RunResult> runInShellImpl(const CommandLine &cmdLine, const QByteArray &stdInData) const final
    {
        Process proc;
        proc.setWriteData(stdInData);
        proc.setCommand(
            {m_rootPath.withNewPath(cmdLine.executable().path()), cmdLine.splitArguments()});
        proc.runBlocking();

        return RunResult {
            proc.resultData().m_exitCode,
            proc.readAllRawStandardOutput(),
            proc.readAllRawStandardError(),
        };
    }
};

class DockerDevicePrivate final : public QObject
{
public:
    explicit DockerDevicePrivate(DockerDevice *parent)
        : q(parent)
    {
        QObject::connect(q, &DockerDevice::applied, this, [this] { stopCurrentContainer(); });
    }

    ~DockerDevicePrivate() { stopCurrentContainer(); }

    CommandLine createCommandLine();

    Result<QString> updateContainerAccess();
    Result<> ensureReachable(const FilePath &other);
    void shutdown();
    Result<FilePath> localSource(const FilePath &other) const;

    Result<QPair<OsType, OsArch>> osTypeAndArch() const;

    Result<CommandLine> withDockerExecCmd(
        const QString &markerTemplate,
        const CommandLine &cmd,
        const std::optional<Environment> &env = std::nullopt,
        const std::optional<FilePath> &workDir = std::nullopt,
        bool interactive = false,
        bool withPty = false,
        bool withMarker = true);

    bool prepareForBuild(const Target *target);
    Tasks validateMounts() const;

    void stopCurrentContainer();
    Result<Environment> fetchEnvironment() const;

    Result<FilePath> getCmdBridgePath() const;

    QStringList createMountArgs() const;

    bool isImageAvailable() const;

    Result<DeviceFileAccessPtr> createBridgeFileAccess(
        SynchronizedValue<DeviceFileAccessPtr>::unique_lock &fileAccess)
    {
        Result<FilePath> cmdBridgePath = getCmdBridgePath();

        if (!cmdBridgePath)
            return ResultError(cmdBridgePath.error());

        auto fAccess = std::make_unique<DockerDeviceFileAccess>(this);

        if (auto result = updateContainerAccess(); !result)
            return ResultError(result.error());

        Result<> initResult = ResultOk;
        if (q->mountCmdBridge()
            && cmdBridgePath->isSameDevice(Docker::Internal::settings().dockerBinaryPath())) {
            initResult = fAccess->init(
                q->rootPath().withNewPath("/tmp/_qtc_cmdbridge"), q->environment(), false);
        } else {
            // Prepare a fallback access so we can deploy the CmdBridge ...
            *fileAccess = std::make_unique<DockerFallbackFileAccess>(q->rootPath());
            // We have to unlock here so that recursive calls to getFileAccess() will not deadlock.
            fileAccess.unlock();

            // ... and then deploy the CmdBridge.
            initResult
                = fAccess->deployAndInit(Core::ICore::libexecPath(), q->rootPath(), q->environment());

            // Dont forget to re-lock.
            fileAccess.lock();
        }

        if (!initResult)
            return ResultError(initResult.error());

        return fAccess;
    }

    DeviceFileAccessPtr createFileAccess()
    {
        if (DeviceFileAccessPtr fileAccess = *m_fileAccess.readLocked())
            return fileAccess;

        if (!DockerApi::instance()->imageExists(q->repoAndTag()))
            return nullptr;

        SynchronizedValue<DeviceFileAccessPtr>::unique_lock fileAccess = m_fileAccess.writeLocked();
        if (*fileAccess)
            return *fileAccess;

        Result<DeviceFileAccessPtr> fAccess = createBridgeFileAccess(fileAccess);

        if (fAccess) {
            *fileAccess = std::move(*fAccess);
            return *fileAccess;
        }

        qCWarning(dockerDeviceLog).noquote() << "Failed to start CmdBridge:" << fAccess.error()
                                             << ", falling back to slow direct access";

        *fileAccess = std::make_shared<DockerFallbackFileAccess>(q->rootPath());
        return *fileAccess;
    }

    DockerDevice *const q;

    struct MountPair
    {
        FilePath path;
        FilePath containerPath;
    };

    bool m_isShutdown = false;
    SynchronizedValue<DeviceFileAccessPtr> m_fileAccess;
    SynchronizedValue<std::unique_ptr<DockerContainerThread>> m_deviceThread;
};

static WrappedProcessInterface *makeProcessInterface(
    IDevice::ConstPtr device, DockerDevicePrivate *devicePrivate)
{
    std::weak_ptr<const IDevice> weakDevice = device;

    const auto wrapCommandLine =
        [devicePrivate](const ProcessSetupData &setupData, const QString &markerTemplate)
        -> Result<CommandLine> {
        QTC_ASSERT(
            devicePrivate,
            return ResultError(
                Tr::tr("Docker device is not initialized. Cannot create command line.")));

        const bool inTerminal = setupData.m_terminalMode != TerminalMode::Off
                                || setupData.m_ptyData.has_value();

        const bool interactive = setupData.m_processMode == ProcessMode::Writer
                                 || !setupData.m_writeData.isEmpty() || inTerminal;

        return devicePrivate->withDockerExecCmd(
            markerTemplate,
            setupData.m_commandLine,
            setupData.m_environment,
            setupData.rawWorkingDirectory(),
            interactive,
            inTerminal,
            !setupData.m_ptyData);
    };

    const auto controlSignalFunction =
        [weakDevice, devicePrivate](ControlSignal controlSignal, qint64 remotePid) {
            QTC_ASSERT(devicePrivate, return);
            auto device = weakDevice.lock();
            if (!device)
                return;

            auto dfa = std::dynamic_pointer_cast<DockerDeviceFileAccess>(device->fileAccess());
            if (dfa) {
                dfa->signalProcess(remotePid, controlSignal);
            } else {
                const int signal = ProcessInterface::controlSignalToInt(controlSignal);
                Process p;
                p.setCommand(
                    {device->rootPath().withNewPath("kill"),
                     {QString("-%1").arg(signal), QString("%2").arg(remotePid)}});
                p.runBlocking();
            }
        };

    WrappedProcessInterface *processInterface
        = new WrappedProcessInterface(wrapCommandLine, controlSignalFunction);

    QObject::connect(device.get(), &QObject::destroyed, processInterface, [processInterface] {
        processInterface->emitDone(
            ProcessResultData{
                -1,
                QProcess::ExitStatus::CrashExit,
                QProcess::ProcessError::UnknownError,
                Tr::tr("Device is shut down."),
            });
    });

    return processInterface;
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

Result<Environment> DockerDevicePrivate::fetchEnvironment() const
{
    Process envCaptureProcess;

    CommandLine cmdLine{settings().dockerBinaryPath(), {"run", "--rm", "-i"}};
    cmdLine.addArgs(q->extraArgs(), CommandLine::Raw);
    cmdLine.addArg(q->repoAndTag());

    envCaptureProcess.setCommand(cmdLine);
    envCaptureProcess.setWriteData("printenv\n");
    envCaptureProcess.runBlocking();
    if (envCaptureProcess.result() != ProcessResult::FinishedWithSuccess) {
        return ResultError(envCaptureProcess.readAllStandardError());
    }
    const QStringList envLines = QString::fromUtf8(envCaptureProcess.readAllRawStandardOutput())
                                     .split('\n', Qt::SkipEmptyParts);

    const Result<QPair<OsType, OsArch>> osInfo = osTypeAndArch();
    if (!osInfo) {
        qCWarning(dockerDeviceLog).noquote()
            << "Failed to determine OS type and architecture for environment capture:"
            << osInfo.error() << "Falling back to Linux.";
    }

    const OsType osType = osInfo.value_or(qMakePair(OsTypeLinux, OsArchX86)).first;

    const QStringList filteredLines = filtered(envLines, [](const QString &line) {
        // We don't want to capture the following environment variables:
        static const QStringList filterKeys{"_=", "HOSTNAME=", "PWD=", "HOME="};
        return !anyOf(filterKeys, [&line](const QString &key) { return line.startsWith(key); });
    });

    return Environment(NameValueDictionary(filteredLines, osType));
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

Result<CommandLine> DockerDevicePrivate::withDockerExecCmd(
    const QString &markerTemplate,
    const CommandLine &cmd,
    const std::optional<Environment> &env,
    const std::optional<FilePath> &workDir,
    bool interactive,
    bool withPty,
    bool withMarker)
{
    QString containerId;

    if (const Result<QString> result = updateContainerAccess(); !result)
        return make_unexpected(result.error());
    else
        containerId = *result;

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

    dockerCmd.addArg(containerId);

    dockerCmd.addArgs({"/bin/sh", "-c"});

    CommandLine exec("exec");
    exec.addCommandLineAsArgs(cmd, CommandLine::Raw);

    if (withMarker) {
        auto osAndArch = osTypeAndArch();
        if (!osAndArch)
            return make_unexpected(osAndArch.error());

        // Check the executable for existence.
        CommandLine testType({"type", {}});
        testType.addArg(cmd.executable().path(), osAndArch->first);
        testType.addArgs(">/dev/null", CommandLine::Raw);

        // Send PID only if existence was confirmed, so we can correctly notify
        // a failed start.
        CommandLine echo("echo");
        echo.addArgs(markerTemplate.arg("$$"), CommandLine::Raw);
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
    { // scope, so they are unlocked before setDeviceState
        auto fileAccess = m_fileAccess.writeLocked();
        fileAccess->reset();

        auto locked = m_deviceThread.writeLocked();
        locked->reset();
    }

    q->setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
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

Result<> isValidMountInfo(const DockerDevicePrivate::MountPair &mi)
{
    if (!mi.path.isLocal())
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

Result<FilePath> DockerDevicePrivate::getCmdBridgePath() const
{
    auto osAndArch = osTypeAndArch();
    if (!osAndArch)
        return make_unexpected(osAndArch.error());
    return CmdBridge::Client::getCmdBridgePath(
        osAndArch->first, osAndArch->second, Core::ICore::libexecPath());
};

QStringList DockerDevicePrivate::createMountArgs() const
{
    QStringList cmds;
    QList<MountPair> mounts;
    for (const FilePath &m : q->mounts())
        mounts.append({m, m});

    if (q->mountCmdBridge()) {
        const Result<FilePath> cmdBridgePath = getCmdBridgePath();
        QTC_CHECK_RESULT(cmdBridgePath);

        if (cmdBridgePath && cmdBridgePath->isSameDevice(settings().dockerBinaryPath()))
            mounts.append({*cmdBridgePath, FilePath("/tmp/_qtc_cmdbridge")});
    }

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
    if (q->useLocalUidGid()) {
        dockerCreate.addArgs({"-u", QString("%1:%2").arg(getuid()).arg(getgid())});
        dockerCreate.addArgs({"-e", QString("HOME=/tmp/qtc_home/%1").arg(getuid())});
    }
#endif

    if (!q->network().isEmpty()) {
        dockerCreate.addArg("--network");
        dockerCreate.addArg(q->network());
    }

    dockerCreate.addArgs(createMountArgs());
    dockerCreate.addArgs(q->portMappings.createArguments());

    if (!q->keepEntryPoint())
        dockerCreate.addArgs({"--entrypoint", "/bin/sh"});

    if (q->enableLldbFlags())
        dockerCreate.addArgs({"--cap-add=SYS_PTRACE", "--security-opt", "seccomp=unconfined"});

    dockerCreate.addArgs(q->extraArgs(), CommandLine::Raw);

    dockerCreate.addArg(q->repoAndTag());

    return dockerCreate;
}

Result<QString> DockerDevicePrivate::updateContainerAccess()
{
    if (m_isShutdown)
        return make_unexpected(Tr::tr("Device is shut down."));
    if (DockerApi::isDockerDaemonAvailable(false).value_or(false) == false)
        return make_unexpected(Tr::tr("Docker system is not reachable."));
    if (!DockerApi::instance()->imageExists(q->repoAndTag()))
        return make_unexpected(Tr::tr("Docker image \"%1\" not found.").arg(q->repoAndTag()));

    auto lockedThread = m_deviceThread.writeLocked();
    if (*lockedThread)
        return (*lockedThread)->containerId();

    DockerContainerThread::Init init;
    init.dockerBinaryPath = settings().dockerBinaryPath();
    init.createContainerCmd = createCommandLine();

    auto result = DockerContainerThread::create(init);

    if (result)
        lockedThread->reset(result->release());

    if (!result)
        return make_unexpected(result.error());

    return (*lockedThread)->containerId();
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
        QCheckBox *showUnnamedContainers = new QCheckBox(Tr::tr("Show unnamed images"));
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

        const QString fail = QString{"Docker: "} + msgProcessFailedToStart();
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

    using DockerDevicePtr = DockerDevice::Ptr; // trick lupdate, QTBUG-140636
    DockerDevicePtr createDevice() const
    {
        const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
        QTC_ASSERT(selectedRows.size() == 1, return {});
        DockerImageItem *item = m_model.itemForIndex(
            m_proxyModel->mapToSource(selectedRows.front()));
        QTC_ASSERT(item, return {});

        DockerDevicePtr device = DockerDevice::create();
        device->repo.setValue(item->repo);
        device->tag.setValue(item->tag);
        device->imageId.setValue(item->imageId);
        device->setDefaultDisplayName(Tr::tr("Docker Image \"%1\" (%2)")
                                          .arg(device->repoAndTag())
                                          .arg(device->imageId.value()));

        if (const auto env = device->d->fetchEnvironment(); !env)
            qCWarning(dockerDeviceLog) << "Failed to fetch environment:" << env.error();
        else
            device->environment.setRemoteEnvironment(*env);

        return device;
    }

public:
    TreeModel<DockerImageItem> m_model;
    TreeView *m_view = nullptr;
    SortFilterModel *m_proxyModel = nullptr;
    QTextBrowser *m_log = nullptr;
    QDialogButtonBox *m_buttons;

    Process *m_process = nullptr;
};

// Factory

DockerDeviceFactory::DockerDeviceFactory()
    : IDeviceFactory(Constants::DOCKER_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("Docker Device"));
    setCombinedIcon(":/docker/images/dockerdevicesmall.png",
                    ":/docker/images/dockerdevice.png");
    setCreator([this] {
        DockerDeviceSetupWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        DockerDevice::Ptr device = wizard.createDevice();
        m_existingDevices.writeLocked()->push_back(device);
        return std::static_pointer_cast<IDevice>(device);
    });
    setConstructionFunction([this] {
        auto device = DockerDevice::create();
        m_existingDevices.writeLocked()->push_back(device);
        return device;
    });
    setExecutionTypeId(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
}

void DockerDeviceFactory::shutdownExistingDevices()
{
    m_existingDevices.read([](const std::vector<std::weak_ptr<DockerDevice>> &devices) {
        for (const std::weak_ptr<DockerDevice> &weakDevice : devices) {
            if (std::shared_ptr<DockerDevice> device = weakDevice.lock())
                device->shutdown();
        }
    });
}

Result<QPair<OsType, OsArch>> DockerDevicePrivate::osTypeAndArch() const
{
    Process proc;
    proc.setCommand(
        {settings().dockerBinaryPath(),
         {"image", "inspect", q->repoAndTag(), "--format", "{{.Os}}\t{{.Architecture}}"}});
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return make_unexpected(Tr::tr("Failed to inspect image: %1").arg(proc.verboseExitMessage()));

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

    return qMakePair(*os, *arch);
}

void DockerDevicePrivate::shutdown()
{
    m_isShutdown = true;
    stopCurrentContainer();
}

Result<FilePath> DockerDevicePrivate::localSource(const FilePath &other) const
{
    QString fixedPath = other.path();
    /* Try to convert Unix-style paths (originally Windows paths transformed
     * into Unix-style for mounting purposes) back to Windows format:
     * /c/dev/src -> C:/dev/src
     * This is the opposite to DockerDeviceFileAccess::mapToDevicePath.
     */
    if (HostOsInfo::isWindowsHost() &&
            (other.osType() == Utils::OsTypeLinux
             || other.osType() == Utils::OsTypeMac
             || other.osType() == Utils::OsTypeOtherUnix)) {
        QRegularExpression linuxPathRegex("^/([A-Za-z])/(.*)?$");
        QRegularExpressionMatch linuxPathMatch = linuxPathRegex.match(fixedPath);
        if (linuxPathMatch.hasMatch()) {
            QString driveLetter = linuxPathMatch.captured(1);
            QString restOfThePath = linuxPathMatch.captured(2);
            fixedPath = driveLetter.toUpper() + ":/" + restOfThePath;
        }
    }
    const auto devicePath = FilePath::fromString(fixedPath);
    for (const FilePath &mount : q->mounts()) {
        const FilePath mountPoint = mount;
        if (devicePath.isChildOf(mountPoint)) {
            const FilePath relativePath = devicePath.relativeChildPath(mountPoint);
            return mountPoint.pathAppended(relativePath.path());
        }
    }

    return make_unexpected(
        Tr::tr("localSource: No mount point found for %1").arg(other.toUserOutput()));
}

Result<> DockerDevicePrivate::ensureReachable(const FilePath &other)
{
    if (other.isSameDevice(q->rootPath()))
        return ResultOk;

    if (!other.isLocal()) {
        return ResultError(
            Tr::tr("Cannot reach \"%1\" from \"%2\".").arg(other.toUserOutput()).arg(q->displayName()));
    }

    for (const FilePath &mount : q->mounts()) {
        if (other.isChildOf(mount))
            return ResultOk;

        if (mount == other)
            return ResultOk;
    }

    return ResultError(
        Tr::tr("The path \"%1\" is not mounted in the Docker device \"%2\".")
            .arg(other.toUserOutput())
            .arg(q->displayName()));
}

class PortMapping : public AspectContainer
{
public:
    PortMapping();

    StringAspect ip{this};
    IntegerAspect hostPort{this};
    IntegerAspect containerPort{this};
    SelectionAspect protocol{this};
};

PortMapping::PortMapping()
{
    ip.setSettingsKey("HostIp");
    ip.setDefaultValue("0.0.0.0");
    ip.setToolTip(Tr::tr("Host IP address."));
    ip.setLabelText(Tr::tr("Host IP:"));
    ip.setDisplayStyle(StringAspect::LineEditDisplay);

    hostPort.setSettingsKey("HostPort");
    hostPort.setToolTip(Tr::tr("Host port number."));
    hostPort.setRange(1, 65535);
    hostPort.setDefaultValue(8080);
    hostPort.setLabelText(Tr::tr("Host port:"));

    containerPort.setSettingsKey("ContainerPort");
    containerPort.setToolTip(Tr::tr("Container port number."));
    containerPort.setRange(1, 65535);
    containerPort.setDefaultValue(8080);
    containerPort.setLabelText(Tr::tr("Container port:"));

    protocol.setSettingsKey("Protocol");
    protocol.setToolTip(Tr::tr("Protocol to use."));
    protocol.addOption("tcp", "TCP");
    protocol.addOption("udp", "UDP");
    protocol.setDefaultValue("tcp");
    protocol.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    protocol.setLabelText(Tr::tr("Protocol:"));

    setLayouter([this] {
        using namespace Layouting;
        return Row{ip, hostPort, containerPort, protocol};
    });
}

PortMappings::PortMappings(AspectContainer *container)
    : AspectList(container)
{
    setCreateItemFunction([this]() {
        auto mapping = std::make_unique<PortMapping>();
        connect(mapping.get(), &PortMapping::changed, this, &AspectContainer::changed);
        return mapping;
    });
    setLabelText(Tr::tr("Port mappings:"));
}

QStringList PortMappings::createArguments() const
{
    QStringList cmds;

    forEachItem<PortMapping>([&cmds](const std::shared_ptr<PortMapping> &portMapping) {
        if (portMapping->ip().isEmpty()) {
            cmds
                += {"-p",
                    QString("%1:%2/%3")
                        .arg(portMapping->hostPort())
                        .arg(portMapping->containerPort())
                        .arg(portMapping->protocol.stringValue())};
            return;
        }
        cmds
            += {"-p",
                QString("%1:%2:%3/%4")
                    .arg(portMapping->ip())
                    .arg(portMapping->hostPort())
                    .arg(portMapping->containerPort())
                    .arg(portMapping->protocol.stringValue())};
    });

    return cmds;
}

} // namespace Internal

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

    environment.setSettingsKey(DockerDeviceEnvironment);
    environment.setLabelText(Tr::tr("Container environment:"));
    connect(&environment, &DockerDeviceEnvironmentAspect::fetchRequested, this, [this] {
        const Result<Environment> result = d->fetchEnvironment();
        if (!result) {
            QMessageBox::warning(ICore::dialogParent(), Tr::tr("Error"), result.error());
            return;
        }
        environment.setRemoteEnvironment(*result);
    });

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
    mounts.setDefaultValue({"%{Config:DefaultProjectDirectory:NativeFilePath}"});
    mounts.setToolTip(Tr::tr("Maps paths in this list one-to-one to the docker container."));
    mounts.setPlaceHolderText(Tr::tr("Host directories to mount into the container."));
    mounts.addOnChanged(DeviceManager::instance(), [this] {
        DeviceManager::instance()->deviceUpdated(id());
        d->stopCurrentContainer();
    });

    extraArgs.setSettingsKey(DockerDeviceExtraArgs);
    extraArgs.setLabelText(Tr::tr("Extra arguments:"));
    extraArgs.setToolTip(Tr::tr("Extra arguments to pass to docker create."));
    extraArgs.setDisplayStyle(StringAspect::LineEditDisplay);

    network.setSettingsKey("Network");
    network.setLabelText(Tr::tr("Network:"));
    network.setDefaultValue("bridge");
    network.setComboBoxEditable(false);
    network.setFillCallback([](const StringSelectionAspect::ResultCallback &cb) {
        auto networks = DockerApi::instance()->networks();

        if (networks) {
            auto items = transform(*networks, [](const Network &network) {
                QStandardItem *item = new QStandardItem(network.name);
                item->setData(network.name);
                item->setToolTip(network.toString());
                return item;
            });
            cb(items);
        } else {
            QStandardItem *errorItem = new QStandardItem(
                networks.error().mid(0, networks.error().indexOf('\n')));
            errorItem->setToolTip(networks.error());
            cb({errorItem});
        }
    });

    connect(DockerApi::instance(),
            &DockerApi::dockerDaemonAvailableChanged,
            &network,
            &StringSelectionAspect::refill);
    connect(
        DockerApi::instance(),
        &DockerApi::networksChanged,
        &network,
        &StringSelectionAspect::refill);

    allowEmptyCommand.setValue(true);

    portMappings.setSettingsKey("Ports");

    mountCmdBridge.setSettingsKey("MountCmdBridge");
    mountCmdBridge.setLabelText(Tr::tr("Mount Command Bridge:"));
    mountCmdBridge.setDefaultValue(true);
    mountCmdBridge.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    mountCmdBridge.setToolTip(
        Tr::tr("The Command Bridge facilitates communication between Qt Creator and the running "
               "Container. It is mounted into the Container by default. If your Docker server does "
               "not have access to the folder where Qt Creator is installed this can fail. In that "
               "case you can disable this option for a slower workaround."));

    setDisplayType(Tr::tr("Docker"));
    setOsType(OsTypeLinux);
    setupId(IDevice::ManuallyAdded);
    setType(Constants::DOCKER_DEVICE_TYPE);
    setMachineType(IDevice::Hardware);

    setFileAccessFactory([this]() -> DeviceFileAccessPtr {
        if (auto fA = *d->m_fileAccess.readLocked())
            return fA;

        if (DeviceFileAccessPtr fileAccess = d->createFileAccess()) {
            setDeviceState(ProjectExplorer::IDevice::DeviceReadyToUse);
            FSEngine::invalidateFileInfoCache();
            return fileAccess;
        }
        return nullptr;
    });

    setOpenTerminal([this](const Environment &env, const FilePath &workingDir, const Continuation<> &cont) {
        Result<QString> result = d->updateContainerAccess();

        if (!result) {
            cont(ResultError(result.error()));
            return;
        }

        Result<FilePath> shell = Terminal::defaultShellForDevice(rootPath());
        if (!shell) {
            cont(ResultError(shell.error()));
            return;
        }

        Process proc;
        proc.setTerminalMode(TerminalMode::Detached);
        proc.setEnvironment(env);
        proc.setWorkingDirectory(workingDir);
        proc.setCommand(CommandLine{*shell});
        proc.start();

        cont(ResultOk);
    });

    addDeviceAction(
        {Tr::tr("Open Shell in Container"), [](const IDevice::Ptr &device) {
             Result<Environment> env = device->systemEnvironmentWithError();
             if (!env) {
                 QMessageBox::warning(ICore::dialogParent(), Tr::tr("Error"), env.error());
                 return;
             }
             device->openTerminal(
                *env,
                FilePath(),
                Continuation<>(dialogParent(), [](const Result<> &result) {
                     if (!result)
                         QMessageBox::warning(dialogParent(), Tr::tr("Error"), result.error());
                })
             );
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

Result<> DockerDevice::updateContainerAccess() const
{
    Result<QString> result = d->updateContainerAccess();
    return result ? ResultOk : ResultError(result.error());
}

void DockerDevice::fromMap(const Store &map)
{
    ProjectExplorer::IDevice::fromMap(map);

    if (!environment.isRemoteEnvironmentSet()) {
        // Old devices may not have the environment stored yet
        if (const Result<Environment> env = d->fetchEnvironment(); !env)
            qCWarning(dockerDeviceLog) << "Failed to fetch environment:" << env.error();
        else {
            qCDebug(dockerDeviceLog) << "Setting environment for device:" << env->toStringList();
            environment.setRemoteEnvironment(*env);
        }
    }

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
    return makeProcessInterface(shared_from_this(), d);
}

DeviceTester *DockerDevice::createDeviceTester()
{
    return nullptr;
}

Utils::Result<> DockerDevice::handlesFile(const FilePath &filePath) const
{
    const bool isDockerScheme = filePath.scheme() == Constants::DOCKER_DEVICE_SCHEME;

    if (isDockerScheme && filePath.host() == imageId())
        return ResultOk;

    if (isDockerScheme && filePath.host() == repoAndTagEncoded())
        return ResultOk;

    if (isDockerScheme && filePath.host() == repoAndTag())
        return ResultOk;

    return IDevice::handlesFile(filePath);
}

Result<> DockerDevice::ensureReachable(const FilePath &other) const
{
    if (other.isEmpty())
        return ResultError(Tr::tr("Path is empty."));

    if (other.isSameDevice(rootPath()))
        return ResultOk;

    if (!other.isLocal())
        return ResultError(Tr::tr("Cannot reach remote path \"%1\".").arg(other.toUserOutput()));

    if (other.isDir())
        return d->ensureReachable(other);
    return d->ensureReachable(other.parentDir());
}

Result<FilePath> DockerDevice::localSource(const FilePath &other) const
{
    return d->localSource(other);
}

Result<Environment> DockerDevice::systemEnvironmentWithError() const
{
    if (environment.isRemoteEnvironmentSet())
        return environment();

    return make_unexpected(Tr::tr("Environment could not be captured."));
}

void DockerDevice::aboutToBeRemoved() const
{
    QTaskTree tree(
        ProjectExplorer::removeDetectedKitsRecipe(shared_from_this(), [](const QString &msg) {
            MessageManager::writeSilently(msg);
        }));
    tree.runBlocking();
}

bool DockerDevice::prepareForBuild(const Target *target)
{
    return d->prepareForBuild(target);
}

bool DockerDevice::supportsQtTargetDeviceType(const QSet<Id> &targetDeviceTypes) const
{
    return targetDeviceTypes.contains(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
           || IDevice::supportsQtTargetDeviceType(targetDeviceTypes);
}

Result<> DockerDevice::supportsBuildingProject(const FilePath &projectDir) const
{
    return handlesFile(projectDir).or_else([this, projectDir](const QString &) {
        return ensureReachable(projectDir);
    });
}

QString DockerDevice::deviceStateToString() const
{
    switch (deviceState()) {
    case IDevice::DeviceDisconnected:
        if (DockerApi::isDockerDaemonAvailable(false).value_or(false) == false)
            return Tr::tr("Docker system is not reachable.");
        return Tr::tr("Ready (Waiting for access to container...)");
    default:
        return IDevice::deviceStateToString();
    }
}

QPixmap DockerDevice::deviceStateIcon() const
{
    switch (deviceState()) {
    case IDevice::DeviceDisconnected:
        if (DockerApi::isDockerDaemonAvailable(false).value_or(false) == false)
            return ProjectExplorer::Icons::DEVICE_DISCONNECTED_INDICATOR.pixmap();
        return ProjectExplorer::Icons::DEVICE_CONNECTED_INDICATOR.pixmap();
    default:
        return IDevice::deviceStateIcon();
    }
}

} // namespace Docker
