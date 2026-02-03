// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevice.h"

#include "linuxdevicetester.h"
#include "linuxprocessinterface.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"
#include "remotelinuxfiletransfer.h"
#include "remotelinuxtr.h"
#include "sshdevicewizard.h"
#include "sshkeycreationdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <gocmdbridge/client/bridgedfileaccess.h>
#include <gocmdbridge/client/cmdbridgeclient.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/devicesupport/processlist.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/devicefileaccess.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/globaltasktree.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/synchronizedvalue.h>
#include <utils/temporaryfile.h>
#include <utils/threadutils.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QMutex>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

using namespace RemoteLinux::Internal;

using namespace QtTaskTree;

namespace RemoteLinux {

const QByteArray s_pidMarker = "__qtc";

static Q_LOGGING_CATEGORY(linuxDeviceLog, "qtc.remotelinux.device", QtWarningMsg);
#define DEBUG(x) qCDebug(linuxDeviceLog) << x << '\n'

static QString killCommandForPath(const FilePath &filePath)
{
    return QString::fromLatin1(R"(
        pid=
        cd /proc
        for p in `ls -d [0123456789]*`
        do
          if [ "`readlink /proc/$p/exe`" = "%1" ]
          then
            pid=$p
            break
          fi
        done
        if [ -n "$pid" ]
        then
          kill -15 -$pid $pid
          i=0
          while ps -p $pid
          do
            sleep 1
            test $i -lt %2 || break
            i=$((i+1))
          done
          ps -p $pid && kill -9 -$pid $pid
          true
        else
          false
        fi)").arg(filePath.path()).arg(globalProjectExplorerSettings().reaperTimeoutInSeconds());
}

class SshSharedConnection : public QObject
{
    Q_OBJECT

public:
    explicit SshSharedConnection(const SshParameters &sshParameters, QObject *parent = nullptr);
    ~SshSharedConnection() override;

    SshParameters sshParameters() const { return m_sshParameters; }
    void ref();
    void deref();
    void makeStale();

    void connectToHost();
    void disconnectFromHost();

    QProcess::ProcessState state() const;
    QString socketFilePath() const
    {
        QTC_ASSERT(m_masterSocketDir, return {});
        return m_masterSocketDir->path() + "/cs";
    }

signals:
    void connected(const QString &socketFilePath);
    void disconnected(const ProcessResultData &result);

    void autoDestructRequested();

private:
    void emitConnected();
    void emitError(QProcess::ProcessError processError, const QString &errorString);
    QString fullProcessError() const;
    QStringList connectionArgs(const FilePath &binary) const;

    const SshParameters m_sshParameters;
    std::unique_ptr<Process> m_masterProcess;
    std::unique_ptr<QTemporaryDir> m_masterSocketDir;
    QTimer m_timer;
    int m_ref = 0;
    bool m_stale = false;
    QProcess::ProcessState m_state = QProcess::NotRunning;
};

SshSharedConnection::SshSharedConnection(const SshParameters &sshParameters, QObject *parent)
    : QObject(parent), m_sshParameters(sshParameters)
{
}

SshSharedConnection::~SshSharedConnection()
{
    QTC_CHECK(m_ref == 0);
    disconnect();
    disconnectFromHost();
}

void SshSharedConnection::ref()
{
    ++m_ref;
    m_timer.stop();
}

void SshSharedConnection::deref()
{
    QTC_ASSERT(m_ref, return);
    if (--m_ref)
        return;
    if (m_stale) // no one uses it
        deleteLater();
    // not stale, so someone may reuse it
    m_timer.start(sshSettings().connectionSharingTimeoutInMinutes() * 1000 * 60);
}

void SshSharedConnection::makeStale()
{
    m_stale = true;
    if (!m_ref) // no one uses it
        deleteLater();
}

void SshSharedConnection::connectToHost()
{
    if (state() != QProcess::NotRunning)
        return;

    const FilePath sshBinary = sshSettings().sshFilePath();
    if (!sshBinary.exists()) {
        emitError(QProcess::FailedToStart, Tr::tr("Cannot establish SSH connection: ssh binary "
                  "\"%1\" does not exist.").arg(sshBinary.toUserOutput()));
        return;
    }

    m_masterSocketDir.reset(new QTemporaryDir);
    if (!m_masterSocketDir->isValid()) {
        emitError(QProcess::FailedToStart,
                    Tr::tr("Cannot establish SSH connection: Failed to create temporary "
                           "directory for control socket: %1")
                  .arg(m_masterSocketDir->errorString()));
        m_masterSocketDir.reset();
        return;
    }

    m_masterProcess.reset(new Process);
    SshParameters::setupSshEnvironment(m_masterProcess.get());
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SshSharedConnection::autoDestructRequested);
    connect(m_masterProcess.get(), &Process::readyReadStandardOutput, this, [this] {
        const QByteArray reply = m_masterProcess->readAllRawStandardOutput();
        if (reply == "\n")
            emitConnected();
        // TODO: otherwise emitError and finish master process?
    });
    // TODO: in case of refused connection we are getting the following on stdErr:
    // ssh: connect to host 127.0.0.1 port 22: Connection refused\r\n
    connect(m_masterProcess.get(), &Process::done, this, [this] {
        m_state = QProcess::NotRunning;
        const ProcessResult result = m_masterProcess->result();
        const ProcessResultData resultData = m_masterProcess->resultData();
        if (result == ProcessResult::StartFailed) {
            emitError(QProcess::FailedToStart, Tr::tr("Cannot establish SSH connection.\n"
                                                      "Control process failed to start."));
            return;
        } else if (result == ProcessResult::FinishedWithError) {
            emitError(resultData.m_error, fullProcessError());
            return;
        }
        emit disconnected(resultData);
    });

    QStringList args = QStringList{"-M", "-N", "-o", "ControlPersist=no",
            "-o", "ServerAliveInterval=10", // TODO: Make configurable?
            "-o", "PermitLocalCommand=yes", // Enable local command
            "-o", "LocalCommand=echo"}      // Local command is executed after successfully
                                            // connecting to the server. "echo" will print "\n"
                                            // on the process output if everything went fine.
            << connectionArgs(sshBinary);
    if (!m_sshParameters.x11DisplayName().isEmpty()) {
        args.prepend("-X");
        Environment env = m_masterProcess->environment();
        env.set("DISPLAY", m_sshParameters.x11DisplayName());
        m_masterProcess->setEnvironment(env);
    }
    m_masterProcess->setCommand(CommandLine(sshBinary, args));
    m_state = QProcess::Starting;
    m_masterProcess->start();
}

void SshSharedConnection::disconnectFromHost()
{
    m_masterProcess.reset();
    m_masterSocketDir.reset();
}

QProcess::ProcessState SshSharedConnection::state() const
{
    return m_state;
}

void SshSharedConnection::emitConnected()
{
    m_state = QProcess::Running;
    emit connected(socketFilePath());
}

void SshSharedConnection::emitError(QProcess::ProcessError error, const QString &errorString)
{
    m_state = QProcess::NotRunning;
    ProcessResultData resultData{-1, QProcess::CrashExit, QProcess::UnknownError, {}};
    if (m_masterProcess)
        resultData = m_masterProcess->resultData();
    resultData.m_error = error;
    resultData.m_errorString = errorString;
    emit disconnected(resultData);
}

QString SshSharedConnection::fullProcessError() const
{
    const QString errorString = m_masterProcess->exitStatus() == QProcess::CrashExit
            ? m_masterProcess->errorString() : QString();
    const QString standardError = m_masterProcess->cleanedStdErr();
    const QString errorPrefix = errorString.isEmpty() && standardError.isEmpty()
            ? Tr::tr("SSH connection failure.") : Tr::tr("SSH connection failure:");
    QStringList allErrors {errorPrefix, errorString, standardError};
    allErrors.removeAll({});
    return allErrors.join('\n');
}

QStringList SshSharedConnection::connectionArgs(const FilePath &binary) const
{
    return m_sshParameters.connectionOptions(binary) << "-o" << ("ControlPath=" + socketFilePath())
                                                     << m_sshParameters.host();
}

// LinuxDeviceConfigurationWidget

class LinuxDeviceConfigurationWidget final : public IDeviceWidget
{
public:
    explicit LinuxDeviceConfigurationWidget(const IDevicePtr &device);

private:
    void createNewKey();
    void updateDeviceFromUi() override {}
};

LinuxDeviceConfigurationWidget::LinuxDeviceConfigurationWidget(
    const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    auto createKeyButton = new QPushButton(Tr::tr("Create New..."));

    const QString machineType = device->machineType() == IDevice::Hardware
                                    ? Tr::tr("Physical Device")
                                    : Tr::tr("Emulator");
    auto linuxDevice = std::dynamic_pointer_cast<LinuxDevice>(device);
    QTC_ASSERT(linuxDevice, return);

    using namespace Layouting;

    auto portWarningLabel = new QLabel(
        QString("<font color=\"red\">%1</font>").arg(Tr::tr("You will need at least one port.")));

    auto updatePortWarningLabel = [portWarningLabel, device]() {
        portWarningLabel->setVisible(device->freePortsAspect.volatileValue().isEmpty());
    };

    updatePortWarningLabel();

    connect(&device->freePortsAspect, &PortListAspect::volatileValueChanged, this, updatePortWarningLabel);

    auto autoDetectButton = new QPushButton(Tr::tr("Run Auto-Detection Now"));

    connect(autoDetectButton, &QPushButton::clicked, this, [linuxDevice, autoDetectButton] {
        autoDetectButton->setEnabled(false);
        linuxDevice->tryToConnect(
            {linuxDevice.get(), [linuxDevice, autoDetectButton](const Result<> &res) {
                 if (!res) {
                     autoDetectButton->setEnabled(true);
                     return;
                 }

                 linuxDevice->requestToolDetection(linuxDevice->toolSearchPaths());
                 const auto onDone = [btn = QPointer<QWidget>(autoDetectButton)] {
                     if (btn)
                         btn->setEnabled(true);
                 };
                 GlobalTaskTree::start(linuxDevice->autoDetectDeviceToolsRecipe(), {}, onDone);
             }});
    });

    // clang-format off
    Form {
        Tr::tr("Machine type:"), machineType, st, br,
        device->sshParametersAspectContainer().host, device->sshParametersAspectContainer().port,
            device->sshParametersAspectContainer().hostKeyCheckingMode, st, br,
        device->freePortsAspect, portWarningLabel, device->sshParametersAspectContainer().timeout, st, br,
        device->sshParametersAspectContainer().userName, st, br,
        device->sshParametersAspectContainer().useKeyFile, st, br,
        device->sshParametersAspectContainer().privateKeyFile, createKeyButton, br,
        linuxDevice->autoConnectOnStartup, br,
        linuxDevice->sourceProfile, br,
        device->sshForwardDebugServerPort, br,
        device->linkDevice, br,
        device->deviceToolsGui(),
        Row { autoDetectButton, st, },
    }.attachTo(this);
    // clang-format on

    connect(
        createKeyButton,
        &QAbstractButton::clicked,
        this,
        &LinuxDeviceConfigurationWidget::createNewKey);

    connect(&device->sshParametersAspectContainer(), &AspectContainer::volatileValueChanged, [] {
        markSettingsDirty(true);
    });
}

void LinuxDeviceConfigurationWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        device()->sshParametersAspectContainer().privateKeyFile.setValue(
            dialog.privateKeyFilePath());
    }
}

IDeviceWidget *createLinuxDeviceWidget(const IDevicePtr &device)
{
    return new LinuxDeviceConfigurationWidget(device);
}

// LinuxDevicePrivate

class ShellThreadHandler;

class LinuxDeviceAccess final : public UnixDeviceFileAccess
{
public:
    explicit LinuxDeviceAccess(LinuxDevicePrivate *devicePrivate);
    ~LinuxDeviceAccess();

    Result<RunResult> runInShellImpl(const CommandLine &cmdLine,
                                     const QByteArray &stdInData) const final;

    Result<Environment> deviceEnvironment() const final;

    void attachToSharedConnection(SshConnectionHandle *connectionHandle,
                                  const SshParameters &sshParameters);

    LinuxDevicePrivate *m_devicePrivate = nullptr;
    ShellThreadHandler *m_handler = nullptr;

private:
    QThread m_shellThread;
};

class LinuxDevicePrivate
{
public:
    explicit LinuxDevicePrivate(LinuxDevice *parent)
        : q(parent)
    {}

    void setOsType(OsType osType)
    {
        qCDebug(linuxDeviceLog) << "Setting OS type to" << osType << "for" << q->displayName();
        q->setOsType(osType);
    }

    void setupShell(const SshParameters &sshParameters, const Continuation<> &cont);
    void setupShellPhase2(const Result<> &result, const Continuation<> &cont);
    void setupShellPhase3(
        const Result<DeviceFileAccessPtr> &res,
        const Continuation<> &cont);
    void setupShellFinalize(const Result<> &result, const Continuation<> &cont);

    RunResult runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    bool checkDisconnectedWithWarning();

    void announceConnectionAttempt();
    void unannounceConnectionAttempt();
    void announceConnectionLoss();
    Id announceId() const { return q->id().withPrefix("announce_"); }

    CommandLine unameCommand() const { return {"uname", {"-s"}, OsType::OsTypeLinux}; }
    void setOsTypeFromUnameResult(const RunResult &result);

    Environment getEnvironment();
    void invalidateEnvironmentCache();

    void closeConnection(bool announce)
    {
        QMutexLocker locker(&m_shellMutex);
        DeviceManager::setDeviceState(q->id(), IDevice::DeviceDisconnected, announce);
        q->setFileAccess(nullptr, announce);
        m_scriptAccess.reset();
    }

    LinuxDevice *q = nullptr;

    std::shared_ptr<LinuxDeviceAccess> m_scriptAccess;

    QRecursiveMutex m_shellMutex;
    QReadWriteLock m_environmentCacheLock;
    std::optional<Environment> m_environmentCache;
    KillCommandForPathFunction m_killCommandForPathFunction;
};

void LinuxDevicePrivate::invalidateEnvironmentCache()
{
    QWriteLocker locker(&m_environmentCacheLock);
    m_environmentCache.reset();
}

Environment LinuxDevicePrivate::getEnvironment()
{
    QReadLocker locker(&m_environmentCacheLock);
    if (m_environmentCache.has_value())
        return m_environmentCache.value();

    locker.unlock();
    QWriteLocker writeLocker(&m_environmentCacheLock);
    if (m_environmentCache.has_value())
        return m_environmentCache.value();

    if (q->deviceState() == IDevice::DeviceDisconnected)
        return {};

    char separator = '\0';
    Process getEnvProc;
    getEnvProc.setCommand({q->filePath("env"), {"-0"}});
    using namespace std::chrono;
    getEnvProc.runBlocking(5s);

    if (getEnvProc.result() != ProcessResult::FinishedWithSuccess) {
        qCWarning(linuxDeviceLog) << "Failed to get environment variables from device:"
                                  << getEnvProc.verboseExitMessage()
                                  << "Trying again without -0 option";
        separator = '\n';
        getEnvProc.setCommand({q->filePath("env"), {}});
        getEnvProc.runBlocking(5s);
    }
    if (getEnvProc.result() != ProcessResult::FinishedWithSuccess) {
        qCWarning(linuxDeviceLog) << "Failed to get environment variables from device:"
                                  << getEnvProc.verboseExitMessage();
        return {};
    }

    const QString remoteOutput = getEnvProc.cleanedStdOut();
    m_environmentCache = Environment(remoteOutput.split(separator, Qt::SkipEmptyParts), q->osType());
    return m_environmentCache.value();
}

Result<RunResult> LinuxDeviceAccess::runInShellImpl(
    const CommandLine &cmdLine, const QByteArray &stdInData) const
{
    if (m_devicePrivate->checkDisconnectedWithWarning())
        return RunResult{-1, {}, Tr::tr("Device is disconnected.").toUtf8()};
    return m_devicePrivate->runInShell(cmdLine, stdInData);
}

Result<Environment> LinuxDeviceAccess::deviceEnvironment() const
{
    if (m_devicePrivate->checkDisconnectedWithWarning())
        return {};

    return m_devicePrivate->getEnvironment();
}

// SshProcessImpl

class SshProcessInterfacePrivate : public QObject
{
public:
    SshProcessInterfacePrivate(SshProcessInterface *sshInterface, const IDevice::ConstPtr &device);

    void start();

    void handleConnected(const QString &socketFilePath);
    void handleDisconnected(const ProcessResultData &result);

    void handleStarted();
    void handleDone();
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();

    void doStart();
    CommandLine fullLocalCommandLine() const;

    SshProcessInterface *q = nullptr;

    qint64 m_processId = 0;
    // Store the IDevice::ConstPtr in order to extend the lifetime of device for as long
    // as this object is alive.
    IDevice::ConstPtr m_device;
    std::unique_ptr<SshConnectionHandle> m_connectionHandle;
    Process m_process;

    QString m_socketFilePath;
    SshParameters m_sshParameters;

    bool m_connecting = false;
    bool m_killed = false;

    QByteArray m_output;
    QByteArray m_error;
    bool m_pidParsed = false;
};

SshProcessInterface::SshProcessInterface(const IDevice::ConstPtr &device)
    : d(new SshProcessInterfacePrivate(this, device))
{}

SshProcessInterface::~SshProcessInterface()
{
    killIfRunning();
    delete d;
}

void SshProcessInterface::emitStarted(qint64 processId)
{
    d->m_processId = processId;
    emit started(processId);
}

void SshProcessInterface::killIfRunning()
{
    if (d->m_killed || d->m_process.state() != QProcess::Running || d->m_processId == 0)
        return;
    sendControlSignal(ControlSignal::Kill);
    d->m_killed = true;
}

qint64 SshProcessInterface::processId() const
{
    return d->m_processId;
}

ProcessResult SshProcessInterface::runInShell(const CommandLine &command, const QByteArray &data)
{
    CommandLine cmd{d->m_device->filePath("/bin/sh"), {"-c"}};
    cmd.addCommandLineAsSingleArg(command);

    Process process;
    process.setCommand(cmd);
    process.setWriteData(data);
    using namespace std::chrono_literals;
    process.runBlocking(2s);
    if (process.result() == ProcessResult::Canceled) {
        Core::MessageManager::writeFlashing(Tr::tr("Cannot send control signal to the %1 device. "
                                                   "The device might have been disconnected.")
                                                .arg(d->m_device->displayName()));
    }
    return process.result();
}

void SshProcessInterface::start()
{
    d->start();
}

qint64 SshProcessInterface::write(const QByteArray &data)
{
    return d->m_process.writeRaw(data);
}

void SshProcessInterface::sendControlSignal(ControlSignal controlSignal)
{
    if (controlSignal == ControlSignal::CloseWriteChannel) {
        d->m_process.closeWriteChannel();
        return;
    }
    if (d->m_process.usesTerminal() || d->m_process.ptyData().has_value()) {
        switch (controlSignal) {
        case ControlSignal::Terminate: d->m_process.terminate();      break;
        case ControlSignal::Kill:      d->m_process.kill();           break;
        case ControlSignal::Interrupt: d->m_process.interrupt();      break;
        case ControlSignal::KickOff:   d->m_process.kickoffProcess(); break;
        case ControlSignal::CloseWriteChannel: break;
        }
        return;
    }
    handleSendControlSignal(controlSignal);
}

void SshProcessInterface::handleSendControlSignal(ControlSignal controlSignal)
{
    QTC_ASSERT(controlSignal != ControlSignal::KickOff, return);
    QTC_ASSERT(controlSignal != ControlSignal::CloseWriteChannel, return);
    const qint64 pid = processId();
    QTC_ASSERT(pid, return); // TODO: try sending a signal based on process name
    const QString args = QString::fromLatin1("-%1 -%2")
            .arg(controlSignalToInt(controlSignal)).arg(pid);
    const CommandLine command{"kill", args, CommandLine::Raw};

    // Killing by using the pid as process group didn't work
    // Fallback to killing the pid directly
    // Note: This blocking call takes up to 2 ms for local remote.
    if (runInShell(command, {}) != ProcessResult::FinishedWithSuccess) {
        const QString args = QString::fromLatin1("-%1 %2")
                                 .arg(controlSignalToInt(controlSignal)).arg(pid);
        const CommandLine command{"kill" , args, CommandLine::Raw};
        runInShell(command, {});
    }
}

void SshProcessInterfacePrivate::handleStarted()
{
    const qint64 processId = m_process.usesTerminal() ? m_process.processId() : 0;

    // Don't emit started() when terminal is off,
    // it's being done later inside handleReadyReadStandardOutput().
    if (q->m_setup.m_terminalMode == Utils::TerminalMode::Off && !q->m_setup.m_ptyData)
        return;

    m_pidParsed = true;
    q->emitStarted(processId);
}

void SshProcessInterfacePrivate::handleDone()
{
    if (m_connectionHandle) // TODO: should it disconnect from signals first?
        m_connectionHandle.release()->deleteLater();

    ProcessResultData finalData = m_process.resultData();
    if (!m_pidParsed) {
        finalData.m_error = QProcess::FailedToStart;
        finalData.m_errorString = Utils::joinStrings({finalData.m_errorString,
                                                      QString::fromLocal8Bit(m_error)}, '\n');
    }
    if (finalData.m_exitCode == 255) {
        finalData.m_exitStatus = QProcess::CrashExit;
        finalData.m_error = QProcess::Crashed;
        finalData.m_errorString = Tr::tr("The process crashed.");
    }
    emit q->done(finalData);
}

void SshProcessInterfacePrivate::handleReadyReadStandardOutput()
{
    // By default this forwards readyRead immediately, but only buffers the
    // output in case the start signal is not emitted yet.
    // In case the pid can be parsed now, the delayed started() is
    // emitted, and any previously buffered output emitted now.
    const QByteArray outputData = m_process.readAllRawStandardOutput();

    if (m_pidParsed) {
        emit q->readyRead(outputData, {});
        return;
    }

    m_output.append(outputData);

    static const QByteArray endMarker = s_pidMarker + '\n';
    int endMarkerLength = endMarker.length();
    int endMarkerOffset = m_output.indexOf(endMarker);
    if (endMarkerOffset == -1) {
        static const QByteArray endMarker = s_pidMarker + "\r\n";
        endMarkerOffset = m_output.indexOf(endMarker);
        endMarkerLength = endMarker.length();
        if (endMarkerOffset == -1)
            return;
    }
    const int startMarkerOffset = m_output.indexOf(s_pidMarker);
    if (startMarkerOffset == endMarkerOffset) // Only theoretically possible.
        return;
    const int pidStart = startMarkerOffset + s_pidMarker.length();
    const QByteArray pidString = m_output.mid(pidStart, endMarkerOffset - pidStart);
    m_pidParsed = true;
    const qint64 processId = pidString.toLongLong();

    // We don't want to show output from e.g. /etc/profile.
    m_output = m_output.mid(endMarkerOffset + endMarkerLength);

    q->emitStarted(processId);

    if (!m_output.isEmpty() || !m_error.isEmpty())
        emit q->readyRead(m_output, m_error);

    m_output.clear();
    m_error.clear();
}

void SshProcessInterfacePrivate::handleReadyReadStandardError()
{
    // By default forwards readyRead immediately, but buffers it in
    // case the start signal is not emitted yet.
    const QByteArray errorData = m_process.readAllRawStandardError();

    if (m_pidParsed) {
        emit q->readyRead({}, errorData);
        return;
    }
    m_error.append(errorData);
}

SshProcessInterfacePrivate::SshProcessInterfacePrivate(SshProcessInterface *sshInterface,
                                                       const IDevice::ConstPtr &device)
    : QObject(sshInterface)
    , q(sshInterface)
    , m_device(device)
    , m_process(this)
{
    connect(&m_process, &Process::started, this, &SshProcessInterfacePrivate::handleStarted);
    connect(&m_process, &Process::done, this, &SshProcessInterfacePrivate::handleDone);
    connect(&m_process, &Process::readyReadStandardOutput,
            this, &SshProcessInterfacePrivate::handleReadyReadStandardOutput);
    connect(&m_process, &Process::readyReadStandardError,
            this, &SshProcessInterfacePrivate::handleReadyReadStandardError);
}

void SshProcessInterfacePrivate::start()
{
    m_sshParameters = m_device->sshParameters();

    const Id linkDeviceId = Id::fromSetting(m_device->linkDevice.value());
    if (const IDevice::ConstPtr linkDevice = DeviceManager::find(linkDeviceId)) {
        CommandLine cmd{linkDevice->filePath("ssh")};
        if (!m_sshParameters.userName().isEmpty()) {
            cmd.addArg("-l");
            cmd.addArg(m_sshParameters.userName());
        }
        cmd.addArg(m_sshParameters.host());

        const bool useTerminal = q->m_setup.m_terminalMode != Utils::TerminalMode::Off
                                 || q->m_setup.m_ptyData;
        if (useTerminal)
            cmd.addArg("-tt");

        const CommandLine full = q->m_setup.m_commandLine;
        if (!full.isEmpty()) { // Empty is ok in case of opening a terminal.
            CommandLine inner;
            const QString wd = q->m_setup.rawWorkingDirectory().path();
            if (!wd.isEmpty())
                inner.addCommandLineWithAnd({"cd", {wd}});
            if (!useTerminal) {
                const QString pidArg = QString("%1\\$\\$%1").arg(QString::fromLatin1(s_pidMarker));
                inner.addCommandLineWithAnd({"echo", pidArg, CommandLine::Raw});
            }
            inner.addCommandLineWithAnd(full);
            cmd.addCommandLineAsSingleArg(inner);
        }

        const auto forwardPort = q->m_setup.m_extraData.value(Constants::SshForwardPort).toString();
        if (!forwardPort.isEmpty()) {
            cmd.addArg("-L");
            cmd.addArg(QString("%1:localhost:%1").arg(forwardPort));
        }

        m_process.setProcessMode(q->m_setup.m_processMode);
        m_process.setTerminalMode(q->m_setup.m_terminalMode);
        m_process.setPtyData(q->m_setup.m_ptyData);
        m_process.setReaperTimeout(q->m_setup.m_reaperTimeout);
        m_process.setWriteData(q->m_setup.m_writeData);
        m_process.setCreateConsoleOnWindows(q->m_setup.m_createConsoleOnWindows);
        m_process.setExtraData(q->m_setup.m_extraData);

        m_process.setCommand(cmd);
        m_process.start();
        return;
    }

    auto linuxDevice = std::dynamic_pointer_cast<const LinuxDevice>(m_device);
    QTC_ASSERT(linuxDevice, handleDone(); return);
    if (linuxDevice->isDisconnected() && !linuxDevice->isTesting())
        return handleDone();
    const bool useConnectionSharing = !linuxDevice->isDisconnected()
            && sshSettings().useConnectionSharing()
            && !q->m_setup.m_extraData.value(Constants::DisableSharing).toBool();

    // TODO: Do we really need it for master process?
    m_sshParameters.setX11DisplayName(
        q->m_setup.m_extraData.value("Ssh.X11ForwardToDisplay").toString());

    if (useConnectionSharing) {
        m_connecting = true;
        m_connectionHandle.reset(new SshConnectionHandle(m_device));
        m_connectionHandle->setParent(this);
        connect(m_connectionHandle.get(), &SshConnectionHandle::connected,
                this, &SshProcessInterfacePrivate::handleConnected);
        connect(m_connectionHandle.get(), &SshConnectionHandle::disconnected,
                this, &SshProcessInterfacePrivate::handleDisconnected);
        linuxDevice->attachToSharedConnection(m_connectionHandle.get(), m_sshParameters);
    } else {
        doStart();
    }
}

void SshProcessInterfacePrivate::handleConnected(const QString &socketFilePath)
{
    m_connecting = false;
    m_socketFilePath = socketFilePath;
    doStart();
}

void SshProcessInterfacePrivate::handleDisconnected(const ProcessResultData &result)
{
    ProcessResultData resultData = result;
    if (m_connecting)
        resultData.m_error = QProcess::FailedToStart;

    m_connecting = false;
    if (m_connectionHandle) // TODO: should it disconnect from signals first?
        m_connectionHandle.release()->deleteLater();

    if (resultData.m_error != QProcess::UnknownError || m_process.state() != QProcess::NotRunning)
        emit q->done(resultData); // TODO: don't emit done() on process finished afterwards
}

void SshProcessInterfacePrivate::doStart()
{
    m_process.setProcessMode(q->m_setup.m_processMode);
    m_process.setTerminalMode(q->m_setup.m_terminalMode);
    m_process.setPtyData(q->m_setup.m_ptyData);
    m_process.setReaperTimeout(q->m_setup.m_reaperTimeout);
    m_process.setWriteData(q->m_setup.m_writeData);
    m_process.setCreateConsoleOnWindows(q->m_setup.m_createConsoleOnWindows);

    // TODO: what about other fields from m_setup?
    SshParameters::setupSshEnvironment(&m_process);
    if (!m_sshParameters.x11DisplayName().isEmpty()) {
        Environment env = m_process.controlEnvironment();
        // Note: it seems this is no-op when shared connection is used.
        // In this case the display is taken from master process.
        env.set("DISPLAY", m_sshParameters.x11DisplayName());
        m_process.setControlEnvironment(env);
    }
    m_process.setExtraData(q->m_setup.m_extraData);
    m_process.setCommand(fullLocalCommandLine());
    m_process.start();
}

CommandLine SshProcessInterfacePrivate::fullLocalCommandLine() const
{
    auto linuxDevice = std::dynamic_pointer_cast<const LinuxDevice>(m_device);
    QTC_ASSERT(linuxDevice, return {});

    const FilePath sshBinary = sshSettings().sshFilePath();
    const bool useTerminal = q->m_setup.m_terminalMode != Utils::TerminalMode::Off || q->m_setup.m_ptyData;
    const bool usePidMarker = !useTerminal;
    const bool sourceProfile = linuxDevice->sourceProfile();
    const bool useX = !m_sshParameters.x11DisplayName().isEmpty();

    CommandLine cmd{sshBinary};

    if (useX)
        cmd.addArg("-X");
    if (useTerminal)
        cmd.addArg("-tt");

    cmd.addArg("-q");

    const auto forwardPort = q->m_setup.m_extraData.value(Constants::SshForwardPort).toString();
    if (!forwardPort.isEmpty()) {
        cmd.addArg("-L");
        cmd.addArg(QString("%1:localhost:%1").arg(forwardPort));
    }

    cmd.addArgs(m_sshParameters.connectionOptions(sshBinary));
    if (!m_socketFilePath.isEmpty())
        cmd.addArgs({"-o", "ControlPath=" + m_socketFilePath});

    cmd.addArg(m_sshParameters.host());

    CommandLine commandLine = q->m_setup.m_commandLine;
    FilePath executable = FilePath::fromParts({}, {}, commandLine.executable().path());
    if (!q->m_setup.m_runAsUser.isEmpty()) {

        // TODO: If downgrading from root, perhaps use su instead? Might be more widely available.
        commandLine.setExecutable(FilePath::fromString("sudo"));

        commandLine.prependArgs({"-E", executable.path()});
        if (q->m_setup.m_runAsUser != "root")
            commandLine.prependArgs({"-u", q->m_setup.m_runAsUser});
    } else {
        commandLine.setExecutable(executable);
    }

    CommandLine inner;

    if (!commandLine.isEmpty() && sourceProfile) {
        const QStringList rcFilesToSource = {"/etc/profile", "$HOME/.profile"};
        for (const QString &filePath : rcFilesToSource) {
            inner.addArgs({"test", "-f", filePath});
            inner.addArgs("&&", CommandLine::Raw);
            inner.addArgs({".", filePath});
            inner.addArgs(";", CommandLine::Raw);
        }
    }

    const FilePath &workingDirectory = q->m_setup.rawWorkingDirectory();
    if (!workingDirectory.isEmpty()) {
        inner.addArgs({"cd", workingDirectory.path()});
        inner.addArgs("&&", CommandLine::Raw);
    }

    if (usePidMarker)
        inner.addArgs(QString("echo ") + s_pidMarker + "$$" + s_pidMarker + " && ", CommandLine::Raw);

    const Environment &env = q->m_setup.m_environment;
    env.forEachEntry([&](const QString &key, const QString &value, bool enabled) {
        if (enabled && !key.trimmed().isEmpty() && !value.contains('\n'))
            inner.addArgs(key + "='" + env.expandVariables(value) + '\'', CommandLine::Raw);
    });

    if (!useTerminal && !commandLine.isEmpty())
        inner.addArg("exec");

    if (!commandLine.isEmpty())
        inner.addCommandLineAsArgs(commandLine, CommandLine::Raw);

    cmd.addArg(inner.arguments());

    return cmd;
}

// ShellThreadHandler

static SshParameters displayless(const SshParameters &sshParameters)
{
    SshParameters parameters = sshParameters;
    parameters.setX11DisplayName({});
    return parameters;
}

class ShellThreadHandler : public QObject
{
    Q_OBJECT

signals:
    void shellExitedIrregularly();

private:
    class LinuxDeviceShell : public DeviceShell
    {
    public:
        LinuxDeviceShell(const CommandLine &cmdLine, const FilePath &devicePath)
            : m_cmdLine(cmdLine)
            , m_devicePath(devicePath)
        {
        }

    private:
        void setupShellProcess(Process *shellProcess) override
        {
            SshParameters::setupSshEnvironment(shellProcess);
            shellProcess->setCommand(m_cmdLine);
        }

        CommandLine createFallbackCommand(const CommandLine &cmdLine) override
        {
            CommandLine result = cmdLine;
            result.setExecutable(m_devicePath.withNewMappedPath(cmdLine.executable())); // Needed?
            return result;
        }

    private:
        const CommandLine m_cmdLine;
        const FilePath m_devicePath;
    };

public:
    ~ShellThreadHandler()
    {
        closeShell();
        qDeleteAll(m_connections);
    }

    void closeShell()
    {
        if (QObject *shell = m_shell.get()) {
            m_shell = nullptr;
            shell->deleteLater();
        }
    }

    // Call me with shell mutex locked
    void startThreadHandler(const SshParameters &parameters)
    {
        closeShell();
        setSshParameters(parameters);

        const FilePath sshPath = sshSettings().sshFilePath();
        CommandLine cmd { sshPath };
        cmd.addArgs(m_displaylessSshParameters.connectionOptions(sshPath)
                    << m_displaylessSshParameters.host());
        cmd.addArg("/bin/sh");

        m_shell = new LinuxDeviceShell(cmd,
            FilePath::fromString(QString("ssh://%1/").arg(parameters.userAtHostAndPort())));
        connect(m_shell.get(), &DeviceShell::exitedIrregularly,
                this, &ShellThreadHandler::shellExitedIrregularly);
        Result<> result = m_shell->start();
        if (!result) {
            qCDebug(linuxDeviceLog) << "Failed to start shell for:" << parameters.userAtHostAndPort()
                                    << ", " << result.error();
        }

        emit threadHandlerStarted(result);
    }

    // Call me with shell mutex locked
    RunResult runInShell(const CommandLine &cmd, const QByteArray &data = {})
    {
        QTC_ASSERT(m_shell, return {});
        return m_shell->runInShell(cmd, data);
    }

    void setSshParameters(const SshParameters &sshParameters)
    {
        QMutexLocker locker(&m_mutex);
        const SshParameters displaylessSshParameters = displayless(sshParameters);

        if (m_displaylessSshParameters == displaylessSshParameters)
            return;

        // If displayless sshParameters don't match the old connections' sshParameters, then stale
        // old connections (don't delete, as the last deref() to each one will delete them).
        for (SshSharedConnection *connection : std::as_const(m_connections))
            connection->makeStale();
        m_connections.clear();
        m_displaylessSshParameters = displaylessSshParameters;
    }

    QString attachToSharedConnection(SshConnectionHandle *connectionHandle,
                                     const SshParameters &sshParameters)
    {
        setSshParameters(sshParameters);

        SshSharedConnection *matchingConnection = nullptr;

        // Find the matching connection
        for (SshSharedConnection *connection : std::as_const(m_connections)) {
            if (connection->sshParameters() == sshParameters) {
                matchingConnection = connection;
                break;
            }
        }

        // If no matching connection has been found, create a new one
        if (!matchingConnection) {
            matchingConnection = new SshSharedConnection(sshParameters);
            connect(matchingConnection, &SshSharedConnection::autoDestructRequested,
                    this, [this, matchingConnection] {
                // This slot is just for removing the matchingConnection from the connection list.
                // The SshSharedConnection could have deleted itself otherwise.
                m_connections.removeOne(matchingConnection);
                matchingConnection->deleteLater();
            });
            m_connections.append(matchingConnection);
        }

        matchingConnection->ref();

        connect(matchingConnection, &SshSharedConnection::connected,
                connectionHandle, &SshConnectionHandle::connected);
        connect(matchingConnection, &SshSharedConnection::disconnected,
                connectionHandle, &SshConnectionHandle::disconnected);

        connect(connectionHandle, &SshConnectionHandle::detachFromSharedConnection,
                matchingConnection, &SshSharedConnection::deref,
                Qt::BlockingQueuedConnection); // Ensure the signal is delivered before sender's
                                               // destruction, otherwise we may get out of sync
                                               // with ref count.

        if (matchingConnection->state() == QProcess::Running)
            return matchingConnection->socketFilePath();

        if (matchingConnection->state() == QProcess::NotRunning)
            matchingConnection->connectToHost();

        return {};
    }

signals:
    void threadHandlerStarted(const Result<> &res);

private:
    mutable QMutex m_mutex;
    SshParameters m_displaylessSshParameters;
    QList<SshSharedConnection *> m_connections;
    QPointer<LinuxDeviceShell> m_shell;
};

// LinuxDevice

LinuxDevice::LinuxDevice()
    : d(new LinuxDevicePrivate(this))
{
    setupId(IDevice::ManuallyAdded, Utils::Id());
    setDisplayType(Tr::tr("Remote Linux"));
    setOsType(OsTypeLinux);
    setDefaultDisplayName(Tr::tr("Remote Linux Device"));
    setType(Constants::GenericLinuxOsType);
    setMachineType(IDevice::Hardware);
    setFreePorts(PortList::fromString(QLatin1String("10000-10100")));
    SshParameters sshParams;
    sshParams.setTimeout(10);
    setDefaultSshParameters(sshParams);
    setKillCommandForPathFunction(killCommandForPath);
    offerKitCreation();

    sourceProfile.setSettingsKey("SourceProfile");
    sourceProfile.setDefaultValue(true);
    sourceProfile.setToolTip(Tr::tr("Source profile before executing commands."));
    sourceProfile.setLabelText(Tr::tr("Source %1 and %2").arg("/etc/profile").arg("$HOME/.profile"));
    sourceProfile.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

    autoConnectOnStartup.setSettingsKey("AutoConnectOnStartup");
    autoConnectOnStartup.setDefaultValue(true);
    autoConnectOnStartup.setLabelText(Tr::tr("Auto-connect on startup"));
    autoConnectOnStartup.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

    addDeviceAction({
        Tr::tr("Deploy Public Key..."),
        [](const IDevice::Ptr &device) {
            runPublicKeyDeploymentDialog(device);
        }
    });

    addDeviceAction({
        Tr::tr("Connect"),
        [](const IDevicePtr &device) {
            const auto dev = std::dynamic_pointer_cast<const LinuxDevice>(device);
            QTC_ASSERT(dev, return);
            dev->tryToConnect({const_cast<LinuxDevice *>(dev.get()), [](const Result<> &res) {
               if (!res) {
                   QMessageBox::critical(dialogParent(), Tr::tr("Connection"),
                                         Tr::tr("Connection failed: %1").arg(res.error()));
               }
            }});
        },
        [](const IDeviceConstPtr &device) {
            return device->deviceState() == IDevice::DeviceDisconnected
                || device->deviceState() == IDevice::DeviceStateUnknown;
        }
    });

    addDeviceAction({
        Tr::tr("Disconnect"),
        [](const IDevicePtr &device) {
            const auto dev = std::dynamic_pointer_cast<const LinuxDevice>(device);
            QTC_ASSERT(dev, return);
            dev->closeConnection(true);
        },
        [](const IDeviceConstPtr &device) {
            return device->deviceState() == IDevice::DeviceConnected
                || device->deviceState() == IDevice::DeviceReadyToUse;
        },
    });

    setOpenTerminal([this](const Environment &env,
                           const FilePath &workingDir,
                           const Continuation<> &cont) {
        Process *proc = new Process;

        // If we will not set any environment variables, we can leave out the shell executable
        // as the "ssh ..." call will automatically launch the default shell if there are
        // no arguments. But if we will set environment variables, we need to explicitly
        // specify the shell executable.
        const QString shell = env.hasChanges() ? env.value_or("SHELL", "/bin/sh") : QString();

        proc->setCommand(CommandLine{filePath(shell)});
        proc->setTerminalMode(Utils::TerminalMode::Run);
        proc->setEnvironment(env);
        proc->setWorkingDirectory(workingDir);
        proc->start();

        QObject::connect(proc, &Process::done, proc, [proc](){
            if (proc->exitCode() != 0){
                qCDebug(linuxDeviceLog) << proc->exitMessage();
                Core::MessageManager::writeFlashing(proc->exitMessage());
            }
            proc->deleteLater();
        });

        cont(ResultOk);
    });

    addDeviceAction({Tr::tr("Open Remote Shell"), [](const IDevice::Ptr &device) {
         device->openTerminal(
             Environment(),
             FilePath(),
             Continuation<>([](const Result<> &result) {
                 if (!result)
                     QMessageBox::warning(dialogParent(), Tr::tr("Error"), result.error());
             })
          );
     }});
}

void LinuxDevice::setKillCommandForPathFunction(const KillCommandForPathFunction &handler)
{
    d->m_killCommandForPathFunction = handler;
}

LinuxDevice::~LinuxDevice()
{
    delete d;
}

IDeviceWidget *LinuxDevice::createWidget()
{
    return createLinuxDeviceWidget(shared_from_this());
}

DeviceTester *LinuxDevice::createDeviceTester()
{
    return new GenericLinuxDeviceTester(shared_from_this());
}

static QString signalProcessGroupByPidCommandLine(qint64 pid, int signal)
{
    return QString::fromLatin1("kill -%1 -%2").arg(signal).arg(pid);
}

static QString commandForData(const SignalOperationData &data,
                              const KillCommandForPathFunction &handler)
{
    switch (data.mode) {
    case SignalOperationMode::KillByPid:
        return QString::fromLatin1("%1 && %2")
            .arg(signalProcessGroupByPidCommandLine(data.pid, 15),
                 signalProcessGroupByPidCommandLine(data.pid, 9));
    case ProjectExplorer::SignalOperationMode::InterruptByPid:
        return signalProcessGroupByPidCommandLine(data.pid, 2);
    case ProjectExplorer::SignalOperationMode::KillByPath:
        return handler(data.filePath);
    };
    return {};
}

ExecutableItem LinuxDevice::signalOperationRecipe(const SignalOperationData &data,
                                                  const Storage<Result<>> &resultStorage) const
{
    const auto onSetup = [data, resultStorage] {
        const auto validResult = data.isValid();
        if (validResult)
            return SetupResult::Continue;

        *resultStorage = validResult;
        return SetupResult::StopWithError;
    };

    const auto onProcessSetup = [device = shared_from_this(), data, resultStorage,
                                 handler = d->m_killCommandForPathFunction](Process &process) {
        const QString command = commandForData(data, handler);
        process.setCommand({device->filePath("/bin/sh"), {"-c", command}});
    };
    const auto onProcessDone = [resultStorage](const Process &process, DoneWith result) {
        if (result == DoneWith::Error)
            *resultStorage = ResultError(process.exitMessage());
        else if (result == DoneWith::Cancel)
            *resultStorage = ResultError(Tr::tr("Signal operation canceled."));
    };

    return Group {
        onGroupSetup(onSetup),
        ProcessTask(onProcessSetup, onProcessDone)
    };
}
QString LinuxDevice::userAtHost() const
{
    return sshParameters().userAtHost();
}

QString LinuxDevice::userAtHostAndPort() const
{
    return sshParameters().userAtHostAndPort();
}

FilePath LinuxDevice::rootPath() const
{
    return FilePath::fromParts(u"ssh", userAtHostAndPort(), u"/");
}

Result<> LinuxDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"ssh" && filePath.host() == userAtHostAndPort())
        return ResultOk;
    return IDevice::handlesFile(filePath);
}

ProcessInterface *LinuxDevice::createProcessInterface() const
{
    return new SshProcessInterface(shared_from_this());
}

LinuxDeviceAccess::LinuxDeviceAccess(LinuxDevicePrivate *devicePrivate)
    : m_devicePrivate(devicePrivate)
{
    m_shellThread.setObjectName("LinuxDeviceShell");
    m_handler = new ShellThreadHandler();
    m_handler->moveToThread(&m_shellThread);
    QObject::connect(m_handler, &ShellThreadHandler::shellExitedIrregularly,
                     devicePrivate->q, [devicePrivate] {
        devicePrivate->announceConnectionLoss();
    });
    QObject::connect(&m_shellThread, &QThread::finished, m_handler, &QObject::deleteLater);
    m_shellThread.start();
}

LinuxDeviceAccess::~LinuxDeviceAccess()
{
    QMutexLocker locker(&m_devicePrivate->m_shellMutex);
    auto closeShell = [this] {
        m_shellThread.quit();
        m_shellThread.wait();
    };
    if (QThread::currentThread() == m_shellThread.thread())
        closeShell();
    else // We might be in a non-main thread now due to extended lifetime of IDevice::Ptr
        QMetaObject::invokeMethod(&m_shellThread, closeShell, Qt::BlockingQueuedConnection);
}

void LinuxDevicePrivate::setOsTypeFromUnameResult(const RunResult &result)
{
    if (result.exitCode != 0)
        setOsType(OsTypeOtherUnix);
    const QString osName = QString::fromUtf8(result.stdOut).trimmed();
    if (osName == "Darwin")
        setOsType(OsTypeMac);
    if (osName == "Linux")
        setOsType(OsTypeLinux);
}

void LinuxDevicePrivate::setupShell(const SshParameters &sshParameters,
                                    const Continuation<> &cont)
{
    QTC_ASSERT(isMainThread(),
               cont(ResultError(ResultAssert, "setupShell called from wrong thread")));

    announceConnectionAttempt();

    // Remove previous access first.
    closeConnection(true);

    m_scriptAccess = std::make_shared<LinuxDeviceAccess>(this);

    m_shellMutex.lock();

    invalidateEnvironmentCache();

    QObject::connect(
        m_scriptAccess->m_handler,
        &ShellThreadHandler::threadHandlerStarted,
        q,
        [this, cont](const Result<> &res) { setupShellPhase2(res, cont); },
        Qt::SingleShotConnection
    );

    QMetaObject::invokeMethod(m_scriptAccess->m_handler, [this, sshParameters] {
        return m_scriptAccess->m_handler->startThreadHandler(sshParameters);
    });
}

void LinuxDevicePrivate::setupShellPhase2(const Result<> &result,
                                          const Continuation<> &cont)
{
    if (!result) {
        DEBUG("Failed to setup state");
        q->setFileAccess(nullptr);
        m_shellMutex.unlock();
        q->setDeviceState(IDevice::DeviceDisconnected);
        setupShellFinalize(result, cont);
        return;
    }

    // Shell setup is ok.
    q->setFileAccess(m_scriptAccess);
    m_shellMutex.unlock();
    q->setDeviceState(IDevice::DeviceConnected);

    setOsTypeFromUnameResult(m_scriptAccess->m_handler->runInShell(unameCommand()));

    // We have good shell access now, try to get bridge access, too:

    QFuture<Result<DeviceFileAccessPtr>> future = Utils::asyncRun(
        [this, rootPath = q->rootPath()]() -> Result<DeviceFileAccessPtr>{
            auto fileAccess = std::make_unique<CmdBridge::FileAccess>([&] {
                QMetaObject::invokeMethod(
                    this->q, [this] { announceConnectionLoss(); }, Qt::QueuedConnection);
            });
            Result<> deployAndInitResult
                = fileAccess->deployAndInit(Core::ICore::libexecPath(), rootPath, getEnvironment());
            if (deployAndInitResult)
                return DeviceFileAccessPtr(std::move(fileAccess));
            return ResultError(deployAndInitResult.error());
        });
    Utils::onResultReady(future, q, [this, cont](const Result<DeviceFileAccessPtr> &res) {
        setupShellPhase3(res, cont);
    });
    Utils::futureSynchronizer()->addFuture(future);
}

void LinuxDevicePrivate::setupShellPhase3(
    const Result<DeviceFileAccessPtr> &initResult, const Continuation<> &cont)
{
    if (initResult) {
        DEBUG("Bridge ok to use");
        q->setFileAccess(initResult.value());
        q->setDeviceState(IDevice::DeviceReadyToUse);
    } else {
        DEBUG("Failed to start CmdBridge:" << initResult.error()
              << ", falling back to slow shell access");
    }

    // Either "script access" and "cmdbridge access" is good enough to continue.
    setupShellFinalize(ResultOk, cont);
}

void LinuxDevicePrivate::setupShellFinalize(const Result<> &result, const Continuation<> &cont)
{
    unannounceConnectionAttempt();

    cont(result);

    DeviceManager::instance()->deviceUpdated(q->id());
}

RunResult LinuxDevicePrivate::runInShell(const CommandLine &cmd, const QByteArray &data)
{
    DEBUG(cmd.toUserOutput());
    QMutexLocker locker(&m_shellMutex);
    QTC_ASSERT(m_scriptAccess, return RunResult());
    return m_scriptAccess->m_handler->runInShell(cmd, data);
}

void LinuxDevicePrivate::announceConnectionAttempt()
{
    const QString message = Tr::tr("Establishing initial connection to device \"%1\". "
                                   "This might take a moment.").arg(q->displayName());
    DEBUG(message);
    QTC_ASSERT(isMainThread(), return);
    InfoBarEntry info(announceId(), message);
    info.setTitle(Tr::tr("Establishing a Connection"));
    info.setInfoType(InfoLabel::Warning);
    Core::ICore::popupInfoBar()->addInfo(info);
    Core::MessageManager::writeSilently(message);
}

void LinuxDevicePrivate::unannounceConnectionAttempt()
{
    QString message =
        Tr::tr("Connection attempt to device \"%1\" finished.").arg(q->displayName()) + "\n";

    InfoLabel::InfoType infoType = InfoLabel::Ok;
    switch (q->deviceState()) {
        case IDevice::DeviceDisconnected:
            message += Tr::tr("Connection could not be established.");
            infoType = InfoLabel::Error;
            break;
        case IDevice::DeviceReadyToUse:
            message += Tr::tr("Connection in fast mode established.");
            break;
        case IDevice::DeviceConnected:
            message += Tr::tr("Connection in fallback mode established.");
            break;
        case IDevice::DeviceStateUnknown:
            QTC_CHECK(false);
            infoType = InfoLabel::Error;
            break;
    };
    DEBUG(message);
    QTC_ASSERT(isMainThread(), return);

    InfoBarEntry info(announceId(), message);
    info.setTitle(Tr::tr("Connection Attempt Finished"));
    info.setInfoType(infoType);
    InfoBar *infoBar = Core::ICore::popupInfoBar();
    infoBar->removeInfo(announceId());
    infoBar->addInfo(info);
    Core::MessageManager::writeSilently(message);

    QTimer::singleShot(5000, q, [id=announceId(), infoBar] { infoBar->removeInfo(id); });
}

void LinuxDevicePrivate::announceConnectionLoss()
{
    const QString message = Tr::tr("Device \"%1\" unexpectedly lost connection.")
                                .arg(q->displayName());
    const Id id = announceId();
    InfoBarEntry info(id, message);
    info.setTitle(Tr::tr("Connection Lost"));
    info.setInfoType(InfoLabel::Warning);
    InfoBar *infoBar = Core::ICore::popupInfoBar();
    infoBar->addInfo(info);
    QTimer::singleShot(5000, q, [id, infoBar] { infoBar->removeInfo(id); });
    Core::MessageManager::writeSilently(message);
    closeConnection(true);
}

bool LinuxDevicePrivate::checkDisconnectedWithWarning()
{
    if (q->deviceState() != IDevice::DeviceDisconnected)
        return false;

    QMetaObject::invokeMethod(qApp, [id = q->id(), name = q->displayName()] {
        InfoBar *infoBar = Core::ICore::popupInfoBar();
        const Id errorId = id.withPrefix("error_");
        if (!infoBar->canInfoBeAdded(errorId))
            return;
        const QString warnStr
            = Tr::tr("Device \"%1\" is currently marked as disconnected.").arg(name);
        InfoBarEntry info(errorId, warnStr, InfoBarEntry::GlobalSuppression::Enabled);
        info.setTitle(Tr::tr("Device Is Disconnected"));
        info.setDetailsWidgetCreator([] {
            const auto label = new QLabel(Tr::tr(
                "The device was not available when trying to connect previously.<br>"
                "No further connection attempts will be made until the device is manually reset "
                "by running a successful connection test via the "
                "<a href=\"dummy\">settings page</a>."));
            label->setWordWrap(true);
            QObject::connect(label, &QLabel::linkActivated, [] {
                Core::ICore::showSettings(ProjectExplorer::Constants::DEVICE_SETTINGS_PAGE_ID);
            });
            return label;
        });
        infoBar->addInfo(info);
        Core::MessageManager::writeSilently(warnStr);
    });
    return true;
}

void LinuxDeviceAccess::attachToSharedConnection(SshConnectionHandle *connectionHandle,
                                                  const SshParameters &sshParameters)
{
    QString socketFilePath;

    Qt::ConnectionType connectionType = QThread::currentThread() == m_handler->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod(m_handler, [this, connectionHandle, sshParameters] {
        return m_handler->attachToSharedConnection(connectionHandle, sshParameters);
    }, connectionType, &socketFilePath);

    if (!socketFilePath.isEmpty())
        emit connectionHandle->connected(socketFilePath);
}

FileTransferInterface *LinuxDevice::createFileTransferInterface(
        const FileTransferSetupData &setup) const
{
    return Internal::createRemoteLinuxFileTransferInterface(*this, setup);
}

void LinuxDevice::attachToSharedConnection(SshConnectionHandle *sshConnectionHandle,
                                           const SshParameters &sshParams) const
{
    QTC_ASSERT(d->m_scriptAccess, return);
    return d->m_scriptAccess->attachToSharedConnection(sshConnectionHandle, sshParams);
}

void LinuxDevice::checkOsType()
{
    d->setOsTypeFromUnameResult(d->runInShell(d->unameCommand()));
}

QString LinuxDevice::deviceStateToString() const
{
    // We use DeviceConnected if (only) the shell access is usable (either fully,
    // or single-shot fallback only), and Device::ReadyToUse if the gocmdbridge
    // is up and running.
    switch (deviceState()) {
        case IDevice::DeviceDisconnected:
            return Tr::tr("Device is considered unconnected. Re-run device test to reset state.");
        case IDevice::DeviceReadyToUse:
            return Tr::tr("Connected");
        case IDevice::DeviceConnected:
            return Tr::tr("Connected (fallback)");
        case IDevice::DeviceStateUnknown:
            break;
    };
    QTC_CHECK(false);
    return IDevice::deviceStateToString();
}

bool LinuxDevice::isDisconnected() const
{
    return deviceState() == IDevice::DeviceDisconnected;
}

void LinuxDevice::tryToConnect(const Continuation<> &cont) const
{
    if (isDisconnected())
        d->setupShell(sshParameters(), cont);
    else
        cont(ResultOk);
}

void LinuxDevice::closeConnection(bool announce) const
{
    d->closeConnection(announce);
}

Internal::LinuxDeviceFactory::LinuxDeviceFactory()
    : IDeviceFactory(Constants::GenericLinuxOsType)
{
    setDisplayName(Tr::tr("Remote Linux Device"));
    setIcon(QIcon());
    setConstructionFunction(&LinuxDevice::create);
    setQuickCreationAllowed(true);
    setCreator([this]() -> IDevice::Ptr {
        auto device = LinuxDevice::create();
        m_existingDevices.writeLocked()->push_back(device);

        SshDeviceWizard
            wizard(Tr::tr("New Remote Linux Device Configuration Setup"), IDevice::Ptr(device));
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });

    setConstructionFunction([this] {
        auto device = LinuxDevice::create();
        m_existingDevices.writeLocked()->push_back(device);
        return device;
    });
    setExecutionTypeId(Constants::ExecutionType);
}

Internal::LinuxDeviceFactory::~LinuxDeviceFactory()
{
    shutdownExistingDevices();
}

void Internal::LinuxDeviceFactory::shutdownExistingDevices()
{
    m_existingDevices.read([](const std::vector<std::weak_ptr<LinuxDevice>> &devices) {
        for (auto device : devices) {
            if (auto d = device.lock())
                d->closeConnection(false);
        }
    });
}

static const char SourceProfile[] = "RemoteLinux.SourceProfile";

static void backwardsFromExtraData(LinuxDevice *device)
{
    QVariant sourceProfile = device->extraData(SourceProfile);
    if (sourceProfile.isValid())
        device->sourceProfile.setValue(sourceProfile.toBool());
}

static void backwardsToExtraData(LinuxDevice *device)
{
    device->setExtraData(SourceProfile, device->sourceProfile.value());
}

void LinuxDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);
    backwardsFromExtraData(this);
}

void LinuxDevice::toMap(Store &map) const
{
    backwardsToExtraData(const_cast<LinuxDevice *>(this));
    IDevice::toMap(map);
}

void LinuxDevice::postLoad()
{
    if (!autoConnectOnStartup())
        return;

    tryToConnect({this, [this](const Result<> &res) {
        if (res)
            return;
        const QString message =
            Tr::tr("Auto-connection to device \"%1\" failed.").arg(displayName());
            + "\n" + Tr::tr("Switching auto-connection off.");
        DEBUG(message);
        autoConnectOnStartup.setValue(false);
        QTC_ASSERT(isMainThread(), return);
        InfoBarEntry info(d->announceId(), message);
        info.setTitle(Tr::tr("Establishing a Connection"));
        info.setInfoType(InfoLabel::Warning);
        Core::ICore::popupInfoBar()->addInfo(info);
        Core::MessageManager::writeSilently(message);
    }});
}

} // namespace RemoteLinux

#include "linuxdevice.moc"
