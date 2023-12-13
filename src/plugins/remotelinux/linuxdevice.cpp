// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevice.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdevicetester.h"
#include "linuxprocessinterface.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"
#include "remotelinuxsignaloperation.h"
#include "remotelinuxtr.h"
#include "sshdevicewizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/filetransferinterface.h>
#include <projectexplorer/devicesupport/processlist.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/process.h>
#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>

#include <QDateTime>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QMutex>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const QByteArray s_pidMarker = "__qtc";

static Q_LOGGING_CATEGORY(linuxDeviceLog, "qtc.remotelinux.device", QtWarningMsg);
#define DEBUG(x) qCDebug(linuxDeviceLog) << x << '\n'

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
    void emitDisconnected();
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
    m_timer.start(SshSettings::connectionSharingTimeout() * 1000 * 60);
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

    const FilePath sshBinary = SshSettings::sshFilePath();
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
    if (!m_sshParameters.x11DisplayName.isEmpty()) {
        args.prepend("-X");
        Environment env = m_masterProcess->environment();
        env.set("DISPLAY", m_sshParameters.x11DisplayName);
        m_masterProcess->setEnvironment(env);
    }
    m_masterProcess->setCommand(CommandLine(sshBinary, args));
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

void SshSharedConnection::emitDisconnected()
{
    m_state = QProcess::NotRunning;
    emit disconnected(m_masterProcess->resultData());
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

// SshConnectionHandle

class SshConnectionHandle : public QObject
{
    Q_OBJECT
public:
    SshConnectionHandle(const IDevice::ConstPtr &device) : m_device(device) {}
    ~SshConnectionHandle() override { emit detachFromSharedConnection(); }

signals:
    // direction: connection -> caller
    void connected(const QString &socketFilePath);
    void disconnected(const ProcessResultData &result);
    // direction: caller -> connection
    void detachFromSharedConnection();

private:
    // Store the IDevice::ConstPtr in order to extend the lifetime of device for as long
    // as this object is alive.
    IDevice::ConstPtr m_device;
};

// LinuxDevicePrivate

class ShellThreadHandler;
class LinuxDevicePrivate;

class LinuxDeviceFileAccess : public UnixDeviceFileAccess
{
public:
    LinuxDeviceFileAccess(LinuxDevicePrivate *dev)
        : m_dev(dev)
    {}

    RunResult runInShell(const CommandLine &cmdLine,
                         const QByteArray &stdInData) const override;

    Environment deviceEnvironment() const override;

    LinuxDevicePrivate *m_dev;
};

class LinuxDeviceSettings : public DeviceSettings
{
public:
    LinuxDeviceSettings() { displayName.setDefaultValue(Tr::tr("Remote Linux Device")); }
};

class LinuxDevicePrivate
{
public:
    explicit LinuxDevicePrivate(LinuxDevice *parent);
    ~LinuxDevicePrivate();

    bool setupShell();
    RunResult runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    void attachToSharedConnection(SshConnectionHandle *connectionHandle,
                                  const SshParameters &sshParameters);

    Environment getEnvironment();
    void invalidateEnvironmentCache();

    void checkOsType();
    void queryOsType(std::function<RunResult(const CommandLine &)> run);

    LinuxDevice *q = nullptr;
    QThread m_shellThread;
    ShellThreadHandler *m_handler = nullptr;
    mutable QMutex m_shellMutex;
    LinuxDeviceFileAccess m_fileAccess{this};

    QReadWriteLock m_environmentCacheLock;
    std::optional<Environment> m_environmentCache;
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

    Process getEnvProc;
    getEnvProc.setCommand({q->filePath("env"), {}});
    getEnvProc.runBlocking();

    const QString remoteOutput = getEnvProc.cleanedStdOut();
    m_environmentCache = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());
    return m_environmentCache.value();
}

RunResult LinuxDeviceFileAccess::runInShell(const CommandLine &cmdLine,
                                            const QByteArray &stdInData) const
{
    return m_dev->runInShell(cmdLine, stdInData);
}

Environment LinuxDeviceFileAccess::deviceEnvironment() const
{
    return m_dev->getEnvironment();
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

    void clearForStart();
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

    ProcessResultData m_result;

    QByteArray m_output;
    QByteArray m_error;
    bool m_pidParsed = false;
    bool m_useConnectionSharing = false;
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

bool SshProcessInterface::runInShell(const CommandLine &command, const QByteArray &data)
{
    Process process;
    CommandLine cmd = {d->m_device->filePath("/bin/sh"), {"-c"}};
    QString tmp;
    ProcessArgs::addArg(&tmp, command.executable().path());
    ProcessArgs::addArgs(&tmp, command.arguments());
    cmd.addArg(tmp);
    process.setCommand(cmd);
    process.setWriteData(data);
    process.start();
    bool isFinished = process.waitForFinished(2000); // It may freeze on some devices
    if (!isFinished) {
        Core::MessageManager::writeFlashing(tr("Can't send control signal to the %1 device. "
                                               "The device might have been disconnected.")
                                                .arg(d->m_device->displayName()));
    }
    return isFinished;
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
    const CommandLine command = { "kill", args, CommandLine::Raw };
    // Note: This blocking call takes up to 2 ms for local remote.
    runInShell(command);
}

void SshProcessInterfacePrivate::handleStarted()
{
    const qint64 processId = m_process.usesTerminal() ? m_process.processId() : 0;

    // Don't emit started() when terminal is off,
    // it's being done later inside handleReadyReadStandardOutput().
    if (q->m_setup.m_terminalMode == TerminalMode::Off && !q->m_setup.m_ptyData)
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
    clearForStart();
    m_sshParameters = m_device->sshParameters();

    const Id linkDeviceId = Id::fromSetting(m_device->extraData(Constants::LinkDevice));
    if (const IDevice::ConstPtr linkDevice = DeviceManager::instance()->find(linkDeviceId)) {
        CommandLine cmd{linkDevice->filePath("ssh")};
        if (!m_sshParameters.userName().isEmpty()) {
            cmd.addArg("-l");
            cmd.addArg(m_sshParameters.userName());
        }
        cmd.addArg(m_sshParameters.host());

        const bool useTerminal = q->m_setup.m_terminalMode != TerminalMode::Off
                                 || q->m_setup.m_ptyData;
        if (useTerminal)
            cmd.addArg("-tt");

        const CommandLine full = q->m_setup.m_commandLine;
        if (!full.isEmpty()) { // Empty is ok in case of opening a terminal.
            CommandLine inner;
            const QString wd = q->m_setup.m_workingDirectory.path();
            if (!wd.isEmpty())
                inner.addCommandLineWithAnd({"cd", {wd}});
            if (!useTerminal) {
                const QString pidArg = QString("%1\\$\\$%1").arg(QString::fromLatin1(s_pidMarker));
                inner.addCommandLineWithAnd({"echo", pidArg, CommandLine::Raw});
            }
            inner.addCommandLineWithAnd(full);
            cmd.addCommandLineAsSingleArg(inner);
        }

        m_process.setProcessImpl(q->m_setup.m_processImpl);
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

    m_useConnectionSharing = SshSettings::connectionSharingEnabled();

    // TODO: Do we really need it for master process?
    m_sshParameters.x11DisplayName
            = q->m_setup.m_extraData.value("Ssh.X11ForwardToDisplay").toString();
    if (m_useConnectionSharing) {
        m_connecting = true;
        m_connectionHandle.reset(new SshConnectionHandle(m_device));
        m_connectionHandle->setParent(this);
        connect(m_connectionHandle.get(), &SshConnectionHandle::connected,
                this, &SshProcessInterfacePrivate::handleConnected);
        connect(m_connectionHandle.get(), &SshConnectionHandle::disconnected,
                this, &SshProcessInterfacePrivate::handleDisconnected);
        auto linuxDevice = m_device.dynamicCast<const LinuxDevice>();
        QTC_ASSERT(linuxDevice, handleDone(); return);
        linuxDevice->connectionAccess()
            ->attachToSharedConnection(m_connectionHandle.get(), m_sshParameters);
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

void SshProcessInterfacePrivate::clearForStart()
{
    m_result = {};
}

void SshProcessInterfacePrivate::doStart()
{
    m_process.setProcessImpl(q->m_setup.m_processImpl);
    m_process.setProcessMode(q->m_setup.m_processMode);
    m_process.setTerminalMode(q->m_setup.m_terminalMode);
    m_process.setPtyData(q->m_setup.m_ptyData);
    m_process.setReaperTimeout(q->m_setup.m_reaperTimeout);
    m_process.setWriteData(q->m_setup.m_writeData);
    m_process.setCreateConsoleOnWindows(q->m_setup.m_createConsoleOnWindows);

    // TODO: what about other fields from m_setup?
    SshParameters::setupSshEnvironment(&m_process);
    if (!m_sshParameters.x11DisplayName.isEmpty()) {
        Environment env = m_process.controlEnvironment();
        // Note: it seems this is no-op when shared connection is used.
        // In this case the display is taken from master process.
        env.set("DISPLAY", m_sshParameters.x11DisplayName);
        m_process.setControlEnvironment(env);
    }
    m_process.setExtraData(q->m_setup.m_extraData);
    m_process.setCommand(fullLocalCommandLine());
    m_process.start();
}

CommandLine SshProcessInterfacePrivate::fullLocalCommandLine() const
{
    const FilePath sshBinary = SshSettings::sshFilePath();
    const bool useTerminal = q->m_setup.m_terminalMode != TerminalMode::Off || q->m_setup.m_ptyData;
    const bool usePidMarker = !useTerminal;
    const bool sourceProfile = m_device->extraData(Constants::SourceProfile).toBool();
    const bool useX = !m_sshParameters.x11DisplayName.isEmpty();

    CommandLine cmd{sshBinary};

    if (useX)
        cmd.addArg("-X");
    if (useTerminal)
        cmd.addArg("-tt");

    cmd.addArg("-q");

    cmd.addArgs(m_sshParameters.connectionOptions(sshBinary));
    if (!m_socketFilePath.isEmpty())
        cmd.addArgs({"-o", "ControlPath=" + m_socketFilePath});

    cmd.addArg(m_sshParameters.host());

    CommandLine commandLine = q->m_setup.m_commandLine;
    FilePath executable = FilePath::fromParts({}, {}, commandLine.executable().path());
    commandLine.setExecutable(executable);

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

    const FilePath &workingDirectory = q->m_setup.m_workingDirectory;
    if (!workingDirectory.isEmpty()) {
        inner.addArgs({"cd", workingDirectory.path()});
        inner.addArgs("&&", CommandLine::Raw);
    }

    if (usePidMarker)
        inner.addArgs(QString("echo ") + s_pidMarker + "$$" + s_pidMarker + " && ", CommandLine::Raw);

    const Environment &env = q->m_setup.m_environment;
    env.forEachEntry([&](const QString &key, const QString &value, bool enabled) {
        if (enabled)
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
    parameters.x11DisplayName.clear();
    return parameters;
}

class ShellThreadHandler : public QObject
{
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
        m_shell.reset();
    }

    // Call me with shell mutex locked
    bool start(const SshParameters &parameters)
    {
        closeShell();
        setSshParameters(parameters);

        const FilePath sshPath = SshSettings::sshFilePath();
        CommandLine cmd { sshPath };
        cmd.addArg("-q");
        cmd.addArgs(m_displaylessSshParameters.connectionOptions(sshPath)
                    << m_displaylessSshParameters.host());
        cmd.addArg("/bin/sh");

        m_shell.reset(new LinuxDeviceShell(cmd,
            FilePath::fromString(QString("ssh://%1/").arg(parameters.userAtHostAndPort()))));
        connect(m_shell.get(), &DeviceShell::done, this, [this] {
            m_shell.release()->deleteLater();
        });
        auto result = m_shell->start();
        if (!result) {
            qCWarning(linuxDeviceLog) << "Failed to start shell for:" << parameters.userAtHostAndPort()
                                      << ", " << result.error();
        }
        return result.has_value();
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

    // Call me with shell mutex locked, called from other thread
    bool isRunning(const SshParameters &sshParameters) const
    {
        if (!m_shell)
           return false;
        QMutexLocker locker(&m_mutex);
        if (m_displaylessSshParameters != displayless(sshParameters))
           return false;
        return true;
    }
private:
    mutable QMutex m_mutex;
    SshParameters m_displaylessSshParameters;
    QList<SshSharedConnection *> m_connections;
    std::unique_ptr<LinuxDeviceShell> m_shell;
};

// LinuxDevice

LinuxDevice::LinuxDevice()
    : IDevice(std::make_unique<LinuxDeviceSettings>())
    , d(new LinuxDevicePrivate(this))
{
    setFileAccess(&d->m_fileAccess);
    setDisplayType(Tr::tr("Remote Linux"));
    setOsType(OsTypeLinux);

    setupId(IDevice::ManuallyAdded, Utils::Id());
    setType(Constants::GenericLinuxOsType);
    setMachineType(IDevice::Hardware);
    setFreePorts(PortList::fromString(QLatin1String("10000-10100")));
    SshParameters sshParams;
    sshParams.timeout = 10;
    setSshParameters(sshParams);

    addDeviceAction({Tr::tr("Deploy Public Key..."), [](const IDevice::Ptr &device, QWidget *parent) {
        if (auto d = Internal::PublicKeyDeploymentDialog::createDialog(device, parent)) {
            d->exec();
            delete d;
        }
    }});

    setOpenTerminal([this](const Environment &env,
                           const FilePath &workingDir) -> expected_str<void> {
        Process proc;

        // If we will not set any environment variables, we can leave out the shell executable
        // as the "ssh ..." call will automatically launch the default shell if there are
        // no arguments. But if we will set environment variables, we need to explicitly
        // specify the shell executable.
        const QString shell = env.hasChanges() ? env.value_or("SHELL", "/bin/sh") : QString();

        proc.setCommand({filePath(shell), {}});
        proc.setTerminalMode(TerminalMode::Detached);
        proc.setEnvironment(env);
        proc.setWorkingDirectory(workingDir);
        proc.start();

        return {};
    });

    addDeviceAction({Tr::tr("Open Remote Shell"), [](const IDevice::Ptr &device, QWidget *) {
                         expected_str<void> result = device->openTerminal(Environment(), FilePath());

                         if (!result)
                             QMessageBox::warning(nullptr, Tr::tr("Error"), result.error());
                     }});
}

void LinuxDevice::_setOsType(Utils::OsType osType)
{
    qCDebug(linuxDeviceLog) << "Setting OS type to" << osType << "for" << displayName();
    IDevice::setOsType(osType);
}

LinuxDevice::~LinuxDevice()
{
    delete d;
}

IDeviceWidget *LinuxDevice::createWidget()
{
    return new Internal::GenericLinuxDeviceConfigurationWidget(sharedFromThis());
}

DeviceTester *LinuxDevice::createDeviceTester() const
{
    return new GenericLinuxDeviceTester;
}

DeviceProcessSignalOperation::Ptr LinuxDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new RemoteLinuxSignalOperation(sharedFromThis()));
}

bool LinuxDevice::usableAsBuildDevice() const
{
    return true;
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

bool LinuxDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"device" && filePath.host() == id().toString())
        return true;
    if (filePath.scheme() == u"ssh" && filePath.host() == userAtHostAndPort())
        return true;
    return false;
}

ProcessInterface *LinuxDevice::createProcessInterface() const
{
    return new SshProcessInterface(sharedFromThis());
}

LinuxDevicePrivate::LinuxDevicePrivate(LinuxDevice *parent)
    : q(parent)
{
    m_shellThread.setObjectName("LinuxDeviceShell");
    m_handler = new ShellThreadHandler();
    m_handler->moveToThread(&m_shellThread);
    QObject::connect(&m_shellThread, &QThread::finished, m_handler, &QObject::deleteLater);
    m_shellThread.start();
}

LinuxDevicePrivate::~LinuxDevicePrivate()
{
    auto closeShell = [this] {
        m_shellThread.quit();
        m_shellThread.wait();
    };
    if (QThread::currentThread() == m_shellThread.thread())
        closeShell();
    else // We might be in a non-main thread now due to extended lifetime of IDevice::Ptr
        QMetaObject::invokeMethod(&m_shellThread, closeShell, Qt::BlockingQueuedConnection);
}

void LinuxDevicePrivate::queryOsType(std::function<RunResult(const CommandLine &)> runInShell)
{
    const RunResult result = runInShell({"uname", {"-s"}, OsType::OsTypeLinux});
    if (result.exitCode != 0)
        q->_setOsType(OsTypeOtherUnix);
    const QString osName = QString::fromUtf8(result.stdOut).trimmed();
    if (osName == "Darwin")
        q->_setOsType(OsTypeMac);
    if (osName == "Linux")
        q->_setOsType(OsTypeLinux);
}

void LinuxDevicePrivate::checkOsType()
{
    queryOsType([this](const CommandLine &cmd) { return runInShell(cmd); });
}

// Call me with shell mutex locked
bool LinuxDevicePrivate::setupShell()
{
    const SshParameters sshParameters = q->sshParameters();
    if (m_handler->isRunning(sshParameters))
        return true;

    invalidateEnvironmentCache();

    bool ok = false;
    QMetaObject::invokeMethod(m_handler, [this, sshParameters] {
        return m_handler->start(sshParameters);
    }, Qt::BlockingQueuedConnection, &ok);

    if (ok) {
        queryOsType([this](const CommandLine &cmd) { return m_handler->runInShell(cmd); });
    }
    return ok;
}

RunResult LinuxDevicePrivate::runInShell(const CommandLine &cmd, const QByteArray &data)
{
    QMutexLocker locker(&m_shellMutex);
    DEBUG(cmd.toUserOutput());
    const bool isSetup = setupShell();
    QTC_ASSERT(isSetup, return {});
    return m_handler->runInShell(cmd, data);
}

void LinuxDevicePrivate::attachToSharedConnection(SshConnectionHandle *connectionHandle,
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

static FilePaths dirsToCreate(const FilesToTransfer &files)
{
    FilePaths dirs;
    for (const FileToTransfer &file : files) {
        FilePath parentDir = file.m_target.parentDir();
        while (true) {
            if (dirs.contains(parentDir) || QDir(parentDir.path()).isRoot())
                break;
            dirs << parentDir;
            parentDir = parentDir.parentDir();
        }
    }
    return sorted(std::move(dirs));
}

static QByteArray transferCommand(bool link)
{
    return link ? "ln -s" : "put";
}

class SshTransferInterface : public FileTransferInterface
{
    Q_OBJECT

protected:
    SshTransferInterface(const FileTransferSetupData &setup, const IDevice::ConstPtr &device)
        : FileTransferInterface(setup)
        , m_device(device)
        , m_process(this)
    {
        SshParameters::setupSshEnvironment(&m_process);
        connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
            emit progress(QString::fromLocal8Bit(m_process.readAllRawStandardOutput()));
        });
        connect(&m_process, &Process::done, this, &SshTransferInterface::doneImpl);
    }

    IDevice::ConstPtr device() const { return m_device; }

    bool handleError()
    {
        ProcessResultData resultData = m_process.resultData();
        if (resultData.m_error == QProcess::FailedToStart) {
            resultData.m_errorString = Tr::tr("\"%1\" failed to start: %2")
                    .arg(FileTransfer::transferMethodName(m_setup.m_method), resultData.m_errorString);
        } else if (resultData.m_exitStatus != QProcess::NormalExit) {
            resultData.m_errorString = Tr::tr("\"%1\" crashed.")
                    .arg(FileTransfer::transferMethodName(m_setup.m_method));
        } else if (resultData.m_exitCode != 0) {
            resultData.m_errorString = QString::fromLocal8Bit(m_process.readAllRawStandardError());
        } else {
            return false;
        }
        emit done(resultData);
        return true;
    }

    void handleDone()
    {
        if (!handleError())
            emit done(m_process.resultData());
    }

    QStringList fullConnectionOptions() const
    {
        QStringList options = m_sshParameters.connectionOptions(SshSettings::sshFilePath());
        if (!m_socketFilePath.isEmpty())
            options << "-o" << ("ControlPath=" + m_socketFilePath);
        return options;
    }

    QString host() const { return m_sshParameters.host(); }
    QString userAtHost() const { return m_sshParameters.userAtHost(); }

    Process &process() { return m_process; }

private:
    virtual void startImpl() = 0;
    virtual void doneImpl() = 0;

    void start() final
    {
        m_sshParameters = displayless(m_device->sshParameters());
        const Id linkDeviceId = Id::fromSetting(m_device->extraData(Constants::LinkDevice));
        const auto linkDevice = DeviceManager::instance()->find(linkDeviceId);
        const bool useConnectionSharing = !linkDevice && SshSettings::connectionSharingEnabled();

        if (useConnectionSharing) {
            m_connecting = true;
            m_connectionHandle.reset(new SshConnectionHandle(m_device));
            m_connectionHandle->setParent(this);
            connect(m_connectionHandle.get(), &SshConnectionHandle::connected,
                    this, &SshTransferInterface::handleConnected);
            connect(m_connectionHandle.get(), &SshConnectionHandle::disconnected,
                    this, &SshTransferInterface::handleDisconnected);
            auto linuxDevice = m_device.dynamicCast<const LinuxDevice>();
            QTC_ASSERT(linuxDevice, startFailed("No Linux device"); return);
            linuxDevice->connectionAccess()
                ->attachToSharedConnection(m_connectionHandle.get(), m_sshParameters);
        } else {
            startImpl();
        }
    }

    void handleConnected(const QString &socketFilePath)
    {
        m_connecting = false;
        m_socketFilePath = socketFilePath;
        startImpl();
    }

    void handleDisconnected(const ProcessResultData &result)
    {
        ProcessResultData resultData = result;
        if (m_connecting)
            resultData.m_error = QProcess::FailedToStart;

        m_connecting = false;
        if (m_connectionHandle) // TODO: should it disconnect from signals first?
            m_connectionHandle.release()->deleteLater();

        if (resultData.m_error != QProcess::UnknownError || m_process.state() != QProcess::NotRunning)
            emit done(resultData); // TODO: don't emit done() on process finished afterwards
    }

    IDevice::ConstPtr m_device;
    SshParameters m_sshParameters;

    // ssh shared connection related
    std::unique_ptr<SshConnectionHandle> m_connectionHandle;
    QString m_socketFilePath;
    bool m_connecting = false;

    Process m_process;
};

class SftpTransferImpl : public SshTransferInterface
{
public:
    SftpTransferImpl(const FileTransferSetupData &setup, const IDevice::ConstPtr &device)
        : SshTransferInterface(setup, device)
    {}

private:
    void startImpl() final
    {
        FilePath sftpBinary = SshSettings::sftpFilePath();

        // This is a hack. We only test the last hop here.
        const Id linkDeviceId = Id::fromSetting(device()->extraData(Constants::LinkDevice));
        if (const auto linkDevice = DeviceManager::instance()->find(linkDeviceId))
            sftpBinary = linkDevice->filePath(sftpBinary.fileName()).searchInPath();

        if (!sftpBinary.exists()) {
            startFailed(Tr::tr("\"sftp\" binary \"%1\" does not exist.")
                            .arg(sftpBinary.toUserOutput()));
            return;
        }

        QByteArray batchData;

        const FilePaths dirs = dirsToCreate(m_setup.m_files);
        for (const FilePath &dir : dirs) {
            if (!dir.exists())
                batchData += "-mkdir " + ProcessArgs::quoteArgUnix(dir.path()).toLocal8Bit() + '\n';
        }

        for (const FileToTransfer &file : m_setup.m_files) {
            FilePath sourceFileOrLinkTarget = file.m_source;
            bool link = false;

            const QFileInfo fi(file.m_source.toFileInfo());
            if (fi.isSymLink()) {
                link = true;
                batchData += "-rm " + ProcessArgs::quoteArgUnix(
                                          file.m_target.path()).toLocal8Bit() + '\n';
                // see QTBUG-5817.
                sourceFileOrLinkTarget =
                    sourceFileOrLinkTarget.withNewPath(fi.dir().relativeFilePath(fi.symLinkTarget()));
            }

            batchData += transferCommand(link) + ' '
                         + ProcessArgs::quoteArgUnix(sourceFileOrLinkTarget.path()).toLocal8Bit() + ' '
                         + ProcessArgs::quoteArgUnix(file.m_target.path()).toLocal8Bit() + '\n';
        }
        process().setCommand({sftpBinary, fullConnectionOptions() << "-b" << "-" << host()});
        process().setWriteData(batchData);
        process().start();
    }

    void doneImpl() final { handleDone(); }
};

class RsyncTransferImpl : public SshTransferInterface
{
public:
    RsyncTransferImpl(const FileTransferSetupData &setup, const IDevice::ConstPtr &device)
        : SshTransferInterface(setup, device)
    { }

private:
    void startImpl() final
    {
        m_currentIndex = 0;
        startNextFile();
    }

    void doneImpl() final
    {
        if (m_setup.m_files.size() == 0 || m_currentIndex == m_setup.m_files.size() - 1)
            return handleDone();

        if (handleError())
            return;

        ++m_currentIndex;
        startNextFile();
    }

    void startNextFile()
    {
        process().close();

        const QString sshCmdLine = ProcessArgs::joinArgs(
                    QStringList{SshSettings::sshFilePath().toUserOutput()}
                    << fullConnectionOptions(), OsTypeLinux);
        QStringList options{"-e", sshCmdLine, m_setup.m_rsyncFlags};

        if (!m_setup.m_files.isEmpty()) { // NormalRun
            const FileToTransfer file = m_setup.m_files.at(m_currentIndex);
            const FileToTransfer fixedFile = fixLocalFileOnWindows(file, options);
            const auto fixedPaths = fixPaths(fixedFile, userAtHost());

            options << fixedPaths.first << fixedPaths.second;
        } else { // TestRun
            options << "-n" << "--exclude=*" << (userAtHost() + ":/tmp");
        }
        // TODO: Get rsync location from settings?
        process().setCommand(CommandLine("rsync", options));
        process().start();
    }

    // On Windows, rsync is either from msys or cygwin. Neither work with the other's ssh.exe.
    FileToTransfer fixLocalFileOnWindows(const FileToTransfer &file, const QStringList &options) const
    {
        if (!HostOsInfo::isWindowsHost())
            return file;

        QString localFilePath = file.m_source.path();
        localFilePath = '/' + localFilePath.at(0) + localFilePath.mid(2);
        if (anyOf(options, [](const QString &opt) {
                return opt.contains("cygwin", Qt::CaseInsensitive); })) {
            localFilePath.prepend("/cygdrive");
        }

        FileToTransfer fixedFile = file;
        fixedFile.m_source = fixedFile.m_source.withNewPath(localFilePath);
        return fixedFile;
    }

    QPair<QString, QString> fixPaths(const FileToTransfer &file, const QString &remoteHost) const
    {
        FilePath localPath = file.m_source;
        FilePath remotePath = file.m_target;
        const QString local = (localPath.isDir() && localPath.path().back() != '/')
                ? localPath.path() + '/' : localPath.path();
        const QString remote = remoteHost + ':' + remotePath.path();

        return qMakePair(local, remote);
    }

    int m_currentIndex = 0;
};

class GenericTransferImpl : public FileTransferInterface
{
public:
    GenericTransferImpl(const FileTransferSetupData &setup)
        : FileTransferInterface(setup)
    {}

private:
    void start() final
    {
        m_fileCount = m_setup.m_files.size();
        m_currentIndex = 0;
        m_checkedDirectories.clear();
        nextFile();
    }

    void nextFile()
    {
        ProcessResultData result;
        if (m_currentIndex >= m_fileCount) {
            emit done(result);
            return;
        }

        const FileToTransfer &file = m_setup.m_files.at(m_currentIndex);
        const FilePath &source = file.m_source;
        const FilePath &target = file.m_target;
        ++m_currentIndex;

        const FilePath targetDir = target.parentDir();
        if (!m_checkedDirectories.contains(targetDir)) {
            emit progress(Tr::tr("Creating directory: %1\n").arg(targetDir.toUserOutput()));
            if (!targetDir.ensureWritableDir()) {
                result.m_errorString = Tr::tr("Failed.");
                result.m_exitCode = -1; // Random pick
                emit done(result);
                return;
            }
            m_checkedDirectories.insert(targetDir);
        }

        emit progress(Tr::tr("Copying %1/%2: %3 -> %4\n")
                          .arg(m_currentIndex)
                          .arg(m_fileCount)
                          .arg(source.toUserOutput(), target.toUserOutput()));
        expected_str<void> copyResult = source.copyFile(target);
        if (!copyResult) {
            result.m_errorString = Tr::tr("Failed: %1").arg(copyResult.error());
            result.m_exitCode = -1; // Random pick
            emit done(result);
            return;
        }

        // FIXME: Use asyncCopyFile instead
        nextFile();
    }

    int m_currentIndex = 0;
    int m_fileCount = 0;
    QSet<FilePath> m_checkedDirectories;
};

FileTransferInterface *LinuxDevice::createFileTransferInterface(
        const FileTransferSetupData &setup) const
{
    if (Utils::anyOf(setup.m_files,
                     [](const FileToTransfer &f) { return f.m_source.needsDevice(); })) {
        return new GenericTransferImpl(setup);
    }

    switch (setup.m_method) {
    case FileTransferMethod::Sftp:  return new SftpTransferImpl(setup, sharedFromThis());
    case FileTransferMethod::Rsync: return new RsyncTransferImpl(setup, sharedFromThis());
    case FileTransferMethod::GenericCopy: return new GenericTransferImpl(setup);
    }
    QTC_CHECK(false);
    return {};
}

LinuxDevicePrivate *LinuxDevice::connectionAccess() const
{
    return d;
}

void LinuxDevice::checkOsType()
{
    d->checkOsType();
}

namespace Internal {

LinuxDeviceFactory::LinuxDeviceFactory()
    : IDeviceFactory(Constants::GenericLinuxOsType)
{
    setDisplayName(Tr::tr("Remote Linux Device"));
    setIcon(QIcon());
    setConstructionFunction(&LinuxDevice::create);
    setQuickCreationAllowed(true);
    setCreator([]() -> IDevice::Ptr {
        const IDevice::Ptr device = LinuxDevice::create();
        SshDeviceWizard wizard(Tr::tr("New Remote Linux Device Configuration Setup"), device);
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });
}

} // namespace Internal
} // namespace RemoteLinux

#include "linuxdevice.moc"
