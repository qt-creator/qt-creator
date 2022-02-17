/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "linuxdevice.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "genericlinuxdeviceconfigurationwizard.h"
#include "linuxdeviceprocess.h"
#include "linuxdevicetester.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"
#include "remotelinuxsignaloperation.h"
#include "remotelinuxenvironmentreader.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/sshdeviceprocesslist.h>
#include <projectexplorer/runcontrol.h>

#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <ssh/sshsettings.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>

#include <QDateTime>
#include <QLoggingCategory>
#include <QMutex>
#include <QRegularExpression>
#include <QThread>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace RemoteLinux {

const char Delimiter0[] = "x--";
const char Delimiter1[] = "---";

static Q_LOGGING_CATEGORY(linuxDeviceLog, "qtc.remotelinux.device", QtWarningMsg);
#define LOG(x) qCDebug(linuxDeviceLog) << x << '\n'
//#define DEBUG(x) qDebug() << x;
//#define DEBUG(x) LOG(x)
#define DEBUG(x)

static QString visualizeNull(QString s)
{
    return s.replace(QLatin1Char('\0'), QLatin1String("<null>"));
}

class LinuxDeviceProcessList : public SshDeviceProcessList
{
public:
    LinuxDeviceProcessList(const IDevice::ConstPtr &device, QObject *parent)
            : SshDeviceProcessList(device, parent)
    {
    }

private:
    QString listProcessesCommandLine() const override
    {
        return QString::fromLatin1(
            "for dir in `ls -d /proc/[0123456789]*`; do "
                "test -d $dir || continue;" // Decrease the likelihood of a race condition.
                "echo $dir;"
                "cat $dir/cmdline;echo;" // cmdline does not end in newline
                "cat $dir/stat;"
                "readlink $dir/exe;"
                "printf '%1''%2';"
            "done").arg(QLatin1String(Delimiter0)).arg(QLatin1String(Delimiter1));
    }

    QList<DeviceProcessItem> buildProcessList(const QString &listProcessesReply) const override
    {
        QList<DeviceProcessItem> processes;
        const QStringList lines = listProcessesReply.split(QString::fromLatin1(Delimiter0)
                + QString::fromLatin1(Delimiter1), Qt::SkipEmptyParts);
        foreach (const QString &line, lines) {
            const QStringList elements = line.split(QLatin1Char('\n'));
            if (elements.count() < 4) {
                qDebug("%s: Expected four list elements, got %d. Line was '%s'.", Q_FUNC_INFO,
                       int(elements.count()), qPrintable(visualizeNull(line)));
                continue;
            }
            bool ok;
            const int pid = elements.first().mid(6).toInt(&ok);
            if (!ok) {
                qDebug("%s: Expected number in %s. Line was '%s'.", Q_FUNC_INFO,
                       qPrintable(elements.first()), qPrintable(visualizeNull(line)));
                continue;
            }
            QString command = elements.at(1);
            command.replace(QLatin1Char('\0'), QLatin1Char(' '));
            if (command.isEmpty()) {
                const QString &statString = elements.at(2);
                const int openParenPos = statString.indexOf(QLatin1Char('('));
                const int closedParenPos = statString.indexOf(QLatin1Char(')'), openParenPos);
                if (openParenPos == -1 || closedParenPos == -1)
                    continue;
                command = QLatin1Char('[')
                        + statString.mid(openParenPos + 1, closedParenPos - openParenPos - 1)
                        + QLatin1Char(']');
            }

            DeviceProcessItem process;
            process.pid = pid;
            process.cmdLine = command;
            process.exe = elements.at(3);
            processes.append(process);
        }

        Utils::sort(processes);
        return processes;
    }
};

class LinuxPortsGatheringMethod : public PortsGatheringMethod
{
    CommandLine commandLine(QAbstractSocket::NetworkLayerProtocol protocol) const override
    {
        // We might encounter the situation that protocol is given IPv6
        // but the consumer of the free port information decides to open
        // an IPv4(only) port. As a result the next IPv6 scan will
        // report the port again as open (in IPv6 namespace), while the
        // same port in IPv4 namespace might still be blocked, and
        // re-use of this port fails.
        // GDBserver behaves exactly like this.

        Q_UNUSED(protocol)

        // /proc/net/tcp* covers /proc/net/tcp and /proc/net/tcp6
        return {"sed", "-e 's/.*: [[:xdigit:]]*:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp*",
                CommandLine::Raw};
    }

    QList<Utils::Port> usedPorts(const QByteArray &output) const override
    {
        QList<Utils::Port> ports;
        QList<QByteArray> portStrings = output.split('\n');
        foreach (const QByteArray &portString, portStrings) {
            if (portString.size() != 4)
                continue;
            bool ok;
            const Utils::Port port(portString.toInt(&ok, 16));
            if (ok) {
                if (!ports.contains(port))
                    ports << port;
            } else {
                qWarning("%s: Unexpected string '%s' is not a port.",
                         Q_FUNC_INFO, portString.data());
            }
        }
        return ports;
    }
};

// ShellThreadHandler

class ShellThreadHandler : public QObject
{
public:
    ~ShellThreadHandler()
    {
        if (m_shell)
            delete m_shell;
    }

    bool startFailed(const SshConnectionParameters &parameters)
    {
        delete m_shell;
        m_shell = nullptr;
        qCDebug(linuxDeviceLog) << "Failed to connect to" << parameters.host();
        return false;
    }

    bool start(const SshConnectionParameters &parameters)
    {
        // TODO: start here shared ssh connection if needed (take it from settings)
        // connect to it
        // wait for connected
        m_shell = new SshRemoteProcess("/bin/sh",
                  parameters.connectionOptions(SshSettings::sshFilePath()) << parameters.host());
        m_shell->setProcessMode(ProcessMode::Writer);
        m_shell->start();
        const bool startOK = m_shell->waitForStarted();
        if (!startOK)
            return startFailed(parameters);

        m_shell->write("echo\n");
        const bool readOK = m_shell->waitForReadyRead();
        if (!readOK)
            return startFailed(parameters);

        const QByteArray output = m_shell->readAllStandardOutput();
        if (output != "\n")
            return startFailed(parameters);

        return true;
    }

    bool runInShell(const CommandLine &cmd, const QByteArray &data = {})
    {
        QTC_ASSERT(m_shell, return false);
        const QByteArray prefix = !data.isEmpty() ? QByteArray("echo " + data + " | ")
                                                  : QByteArray("");

        m_shell->readAllStandardOutput(); // clean possible left-overs
        m_shell->write(prefix + cmd.toUserOutput().toUtf8() + "\necho $?\n");
        DEBUG("RUN1 " << cmd.toUserOutput());
        m_shell->waitForReadyRead();
        const QByteArray output = m_shell->readAllStandardOutput();
        DEBUG("GOT1 " << output);
        bool ok = false;
        const int result = output.toInt(&ok);
        LOG("Run command in shell:" << cmd.toUserOutput() << "result: " << output << " ==>" << result);
        return ok && result == 0;
    }

    QByteArray outputForRunInShell(const QString &cmd)
    {
        QTC_ASSERT(m_shell, return {});

        static int val = 0;
        const QByteArray delim = QString::number(++val, 16).toUtf8();

        DEBUG("RUN2 " << cmd);
        m_shell->readAllStandardOutput(); // clean possible left-overs
        const QByteArray marker = "___QTC___" + delim + "_OUTPUT_MARKER___";
        DEBUG(" CMD: " << cmd.toUtf8() + "\necho " + marker + "\n");
        m_shell->write(cmd.toUtf8() + "\necho " + marker + "\n");
        QByteArray output;
        while (!output.contains(marker)) {
            DEBUG("OUTPUT" << output);
            m_shell->waitForReadyRead();
            output.append(m_shell->readAllStandardOutput());
        }
        DEBUG("GOT2 " << output);
        LOG("Run command in shell:" << cmd << "output size:" << output.size());
        const int pos = output.indexOf(marker);
        if (pos >= 0)
            output = output.left(pos);
        DEBUG("CHOPPED2 " << output);
        return output;
    }

    bool isRunning() const { return m_shell; }
private:
    SshRemoteProcess *m_shell = nullptr;
};

// LinuxDevicePrivate

class LinuxDevicePrivate
{
public:
    explicit LinuxDevicePrivate(LinuxDevice *parent);
    ~LinuxDevicePrivate();

    CommandLine fullLocalCommandLine(const CommandLine &remoteCommand,
                                     TerminalMode terminalMode,
                                     bool hasDisplay) const;
    bool setupShell();
    bool runInShell(const CommandLine &cmd, const QByteArray &data = {});
    QByteArray outputForRunInShell(const QString &cmd);
    QByteArray outputForRunInShell(const CommandLine &cmd);

    LinuxDevice *q = nullptr;
    QThread m_shellThread;
    ShellThreadHandler *m_handler = nullptr;
    mutable QMutex m_shellMutex;
};

// LinuxDevice

LinuxDevice::LinuxDevice()
    : d(new LinuxDevicePrivate(this))
{
    setDisplayType(tr("Generic Linux"));
    setDefaultDisplayName(tr("Generic Linux Device"));
    setOsType(OsTypeLinux);

    addDeviceAction({tr("Deploy Public Key..."), [](const IDevice::Ptr &device, QWidget *parent) {
        if (auto d = PublicKeyDeploymentDialog::createDialog(device, parent)) {
            d->exec();
            delete d;
        }
    }});

    setOpenTerminal([this](const Environment &env, const FilePath &workingDir) {
        DeviceProcess * const proc = createProcess(nullptr);
        QObject::connect(proc, &DeviceProcess::finished, [proc] {
            if (!proc->errorString().isEmpty()) {
                Core::MessageManager::writeDisrupting(
                    tr("Error running remote shell: %1").arg(proc->errorString()));
            }
            proc->deleteLater();
        });
        QObject::connect(proc, &DeviceProcess::errorOccurred, [proc] {
            Core::MessageManager::writeDisrupting(tr("Error starting remote shell."));
            proc->deleteLater();
        });

        // It seems we cannot pass an environment to OpenSSH dynamically
        // without specifying an executable.
        if (env.size() > 0)
            proc->setCommand({"/bin/sh", {}});

        proc->setTerminalMode(TerminalMode::On);
        proc->setEnvironment(env);
        proc->setWorkingDirectory(workingDir);
        proc->start();
    });

    if (Utils::HostOsInfo::isAnyUnixHost()) {
        addDeviceAction({tr("Open Remote Shell"), [](const IDevice::Ptr &device, QWidget *) {
            device->openTerminal(Environment(), FilePath());
        }});
    }
}

LinuxDevice::~LinuxDevice()
{
    delete d;
}

IDeviceWidget *LinuxDevice::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis());
}

DeviceProcess *LinuxDevice::createProcess(QObject *parent) const
{
    return new LinuxDeviceProcess(sharedFromThis(), parent);
}

bool LinuxDevice::canAutoDetectPorts() const
{
    return true;
}

PortsGatheringMethod::Ptr LinuxDevice::portsGatheringMethod() const
{
    return LinuxPortsGatheringMethod::Ptr(new LinuxPortsGatheringMethod);
}

DeviceProcessList *LinuxDevice::createProcessListModel(QObject *parent) const
{
    return new LinuxDeviceProcessList(sharedFromThis(), parent);
}

DeviceTester *LinuxDevice::createDeviceTester() const
{
    return new GenericLinuxDeviceTester;
}

DeviceProcessSignalOperation::Ptr LinuxDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new RemoteLinuxSignalOperation(sshParameters()));
}

class LinuxDeviceEnvironmentFetcher : public DeviceEnvironmentFetcher
{
public:
    LinuxDeviceEnvironmentFetcher(const IDevice::ConstPtr &device)
        : m_reader(device)
    {
        connect(&m_reader, &Internal::RemoteLinuxEnvironmentReader::finished,
                this, &LinuxDeviceEnvironmentFetcher::readerFinished);
        connect(&m_reader, &Internal::RemoteLinuxEnvironmentReader::error,
                this, &LinuxDeviceEnvironmentFetcher::readerError);
    }

private:
    void start() override { m_reader.start(); }
    void readerFinished() { emit finished(m_reader.remoteEnvironment(), true); }
    void readerError() { emit finished(Utils::Environment(), false); }

    Internal::RemoteLinuxEnvironmentReader m_reader;
};

DeviceEnvironmentFetcher::Ptr LinuxDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr(new LinuxDeviceEnvironmentFetcher(sharedFromThis()));
}

QString LinuxDevice::userAtHost() const
{
    return sshParameters().userAtHost();
}

FilePath LinuxDevice::mapToGlobalPath(const FilePath &pathOnDevice) const
{
    if (pathOnDevice.needsDevice()) {
        // Already correct form, only sanity check it's ours...
        QTC_CHECK(handlesFile(pathOnDevice));
        return pathOnDevice;
    }
    FilePath result;
    result.setScheme("ssh");
    result.setHost(userAtHost());
    result.setPath(pathOnDevice.path());
    return result;
}

bool LinuxDevice::handlesFile(const FilePath &filePath) const
{
    return filePath.scheme() == "ssh" && filePath.host() == userAtHost();
}

CommandLine LinuxDevicePrivate::fullLocalCommandLine(const CommandLine &remoteCommand,
                                                     TerminalMode terminalMode,
                                                     bool hasDisplay) const
{
    Utils::CommandLine cmd{SshSettings::sshFilePath()};
    const SshConnectionParameters parameters = q->sshParameters();

    if (hasDisplay)
        cmd.addArg("-X");
    if (terminalMode != TerminalMode::Off)
        cmd.addArg("-tt");

    cmd.addArg("-q");
    // TODO: currently this drops shared connection (-o ControlPath=socketFilePath)
    cmd.addArgs(parameters.connectionOptions(SshSettings::sshFilePath()) << parameters.host());

    CommandLine remoteWithLocalPath = remoteCommand;
    FilePath executable = remoteWithLocalPath.executable();
    executable.setScheme({});
    executable.setHost({});
    remoteWithLocalPath.setExecutable(executable);
    cmd.addArg(remoteWithLocalPath.toUserOutput());
    return cmd;
}

void LinuxDevice::runProcess(QtcProcess &process) const
{
    QTC_ASSERT(!process.isRunning(), return);

    Utils::Environment env = process.hasEnvironment() ? process.environment()
                                                      : Utils::Environment::systemEnvironment();
    const bool hasDisplay = env.hasKey("DISPLAY") && (env.value("DISPLAY") != QString(":0"));
    if (SshSettings::askpassFilePath().exists()) {
        env.set("SSH_ASKPASS", SshSettings::askpassFilePath().toUserOutput());

        // OpenSSH only uses the askpass program if DISPLAY is set, regardless of the platform.
        if (!env.hasKey("DISPLAY"))
            env.set("DISPLAY", ":0");
    }
    process.setEnvironment(env);

    // Otherwise, ssh will ignore SSH_ASKPASS and read from /dev/tty directly.
    process.setDisableUnixTerminal();

    process.setCommand(d->fullLocalCommandLine(process.commandLine(), process.terminalMode(),
                                               hasDisplay));
    process.start();
}

LinuxDevicePrivate::LinuxDevicePrivate(LinuxDevice *parent)
    : q(parent)
{
    m_handler = new ShellThreadHandler();
    m_handler->moveToThread(&m_shellThread);
    QObject::connect(&m_shellThread, &QThread::finished, m_handler, &QObject::deleteLater);
    m_shellThread.start();
}

LinuxDevicePrivate::~LinuxDevicePrivate()
{
    m_shellThread.quit();
    m_shellThread.wait();
}

bool LinuxDevicePrivate::setupShell()
{
    bool ok = false;
    QMetaObject::invokeMethod(m_handler, [this, parameters = q->sshParameters()] {
        return m_handler->start(parameters);
    }, Qt::BlockingQueuedConnection, &ok);
    return ok;
}

bool LinuxDevicePrivate::runInShell(const CommandLine &cmd, const QByteArray &data)
{
    QMutexLocker locker(&m_shellMutex);
    DEBUG(cmd.toUserOutput());
    if (!m_handler->isRunning()) {
        const bool ok = setupShell();
        QTC_ASSERT(ok, return false);
    }

    bool ret = false;
    QMetaObject::invokeMethod(m_handler, [this, &cmd, &data] {
        return m_handler->runInShell(cmd, data);
    }, Qt::BlockingQueuedConnection, &ret);
    return ret;
}

QByteArray LinuxDevicePrivate::outputForRunInShell(const QString &cmd)
{
    QMutexLocker locker(&m_shellMutex);
    DEBUG(cmd);
    if (!m_handler->isRunning()) {
        const bool ok = setupShell();
        QTC_ASSERT(ok, return {});
    }

    QByteArray ret;
    QMetaObject::invokeMethod(m_handler, [this, &cmd] {
        return m_handler->outputForRunInShell(cmd);
    }, Qt::BlockingQueuedConnection, &ret);
    return ret;
}

QByteArray LinuxDevicePrivate::outputForRunInShell(const CommandLine &cmd)
{
    return outputForRunInShell(cmd.toUserOutput());
}

bool LinuxDevice::isExecutableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-x", path}});
}

bool LinuxDevice::isReadableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-r", path, "-a", "-f", path}});
}

bool LinuxDevice::isWritableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-w", path, "-a", "-f", path}});
}

bool LinuxDevice::isReadableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-r", path, "-a", "-d", path}});
}

bool LinuxDevice::isWritableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-w", path, "-a", "-d", path}});
}

bool LinuxDevice::isFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-f", path}});
}

bool LinuxDevice::isDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-d", path}});
}

bool LinuxDevice::createDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"mkdir", {"-p", path}});
}

bool LinuxDevice::exists(const FilePath &filePath) const
{
    DEBUG("filepath " << filePath.path());
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"test", {"-e", path}});
}

bool LinuxDevice::ensureExistingFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const QString path = filePath.path();
    return d->runInShell({"touch", {path}});
}

bool LinuxDevice::removeFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    return d->runInShell({"rm", {filePath.path()}});
}

bool LinuxDevice::removeRecursively(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(filePath.path().startsWith('/'), return false);

    const QString path = filePath.cleanPath().path();
    // We are expecting this only to be called in a context of build directories or similar.
    // Chicken out in some cases that _might_ be user code errors.
    QTC_ASSERT(path.startsWith('/'), return false);
    const int levelsNeeded = path.startsWith("/home/") ? 3 : 2;
    QTC_ASSERT(path.count('/') >= levelsNeeded, return false);

    return d->runInShell({"rm", {"-rf", "--", path}});
}

bool LinuxDevice::copyFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    return d->runInShell({"cp", {filePath.path(), target.path()}});
}

bool LinuxDevice::renameFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    return d->runInShell({"mv", {filePath.path(), target.path()}});
}

QDateTime LinuxDevice::lastModified(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%Y", filePath.path()}});
    const qint64 secs = output.toLongLong();
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(secs, Qt::UTC);
    return dt;
}

FilePath LinuxDevice::symLinkTarget(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const QByteArray output = d->outputForRunInShell({"readlink", {"-n", "-e", filePath.path()}});
    const QString out = QString::fromUtf8(output.data(), output.size());
    return output.isEmpty() ? FilePath() : filePath.withNewPath(out);
}

qint64 LinuxDevice::fileSize(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return -1);
    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%s", filePath.path()}});
    return output.toLongLong();
}

qint64 LinuxDevice::bytesAvailable(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return -1);
    CommandLine cmd("df", {"-k"});
    cmd.addArg(filePath.path());
    cmd.addArgs("|tail -n 1 |sed 's/  */ /g'|cut -d ' ' -f 4", CommandLine::Raw);
    const QByteArray output = d->outputForRunInShell(cmd.toUserOutput());
    bool ok = false;
    const qint64 size = output.toLongLong(&ok);
    if (ok)
        return size * 1024;
    return -1;
}

QFileDevice::Permissions LinuxDevice::permissions(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%a", filePath.path()}});
    const uint bits = output.toUInt(nullptr, 8);
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

bool LinuxDevice::setPermissions(const Utils::FilePath &filePath, QFileDevice::Permissions permissions) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    const int flags = int(permissions);
    return d->runInShell({"chmod", {QString::number(flags, 16), filePath.path()}});
}

void LinuxDevice::iterateDirectory(const FilePath &filePath,
                                   const std::function<bool(const FilePath &)> &callBack,
                                   const FileFilter &filter) const
{
    QTC_ASSERT(handlesFile(filePath), return);
    // if we do not have find - use ls as fallback
    const QByteArray output = d->outputForRunInShell({"ls", {"-1", "-b", "--", filePath.path()}});
    const QStringList entries = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
    FileUtils::iterateLsOutput(filePath, entries, filter, callBack);
}

QByteArray LinuxDevice::fileContents(const FilePath &filePath, qint64 limit, qint64 offset) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    QString args = "if=" + filePath.path() + " status=none";
    if (limit > 0 || offset > 0) {
        const qint64 gcd = std::gcd(limit, offset);
        args += QString(" bs=%1 count=%2 seek=%3").arg(gcd).arg(limit / gcd).arg(offset / gcd);
    }
    CommandLine cmd(FilePath::fromString("dd"), args, CommandLine::Raw);

    const QByteArray output = d->outputForRunInShell(cmd);
    DEBUG(output << output << QByteArray::fromHex(output));
    return output;
}

bool LinuxDevice::writeFileContents(const FilePath &filePath, const QByteArray &data) const
{
    QTC_ASSERT(handlesFile(filePath), return {});

// This following would be the generic Unix solution.
// But it doesn't pass input. FIXME: Why?
    return d->runInShell({"dd", {"of=" + filePath.path()}}, data);
}

namespace Internal {

// Factory

LinuxDeviceFactory::LinuxDeviceFactory()
    : IDeviceFactory(Constants::GenericLinuxOsType)
{
    setDisplayName(LinuxDevice::tr("Generic Linux Device"));
    setIcon(QIcon());
    setConstructionFunction(&LinuxDevice::create);
    setCreator([] {
        GenericLinuxDeviceConfigurationWizard wizard(Core::ICore::dialogParent());
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
}

} // namespace Internal
} // namespace RemoteLinux
