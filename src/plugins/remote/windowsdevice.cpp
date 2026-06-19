// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsdevice.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "sshdevicewizard.h"
#include "sshkeycreationdialog.h"
#include "windowsdevicetester.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDateTime>
#include <QLoggingCategory>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QThread>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote {

static Q_LOGGING_CATEGORY(windowsDeviceLog, "qtc.remotewindows.device", QtWarningMsg)

// Helpers for running a single PowerShell command on the remote machine over SSH.
// We always go through PowerShell with -EncodedCommand: the script is base64 of its
// UTF-16LE representation, which side-steps all quoting issues across the local
// shell, ssh and the remote default shell (cmd.exe or PowerShell) layers.

static QString encodePowerShellCommand(const QString &script)
{
    QByteArray utf16le;
    utf16le.reserve(script.size() * 2);
    for (const QChar c : script) {
        const ushort u = c.unicode();
        utf16le.append(char(u & 0xff));
        utf16le.append(char((u >> 8) & 0xff));
    }
    return QString::fromLatin1(utf16le.toBase64());
}

// Quotes a string for use inside a single-quoted PowerShell literal.
static QString psQuote(const QString &str)
{
    QString result = str;
    result.replace('\'', "''");
    return '\'' + result + '\'';
}

static QString psPath(const FilePath &filePath)
{
    return psQuote(filePath.nativePath());
}

// Builds the local "ssh ... <host> <remoteCommand>" invocation. The remote command is
// passed raw (unquoted): the -EncodedCommand base64 has no spaces or shell metacharacters,
// and quoting it would be misinterpreted by the remote default shell (cmd.exe or PowerShell)
// that OpenSSH wraps the command in.
static CommandLine sshCommandLine(const SshParameters &ssh, const QString &remoteCommand)
{
    const FilePath sshBinary = sshSettings().sshFilePath();
    CommandLine cmd{sshBinary};
    // Note: no "-q". It would suppress ssh's own diagnostics (e.g. "No route to host"),
    // turning a dropped connection into a silent, empty result. ssh's stderr is ignored
    // on success anyway.
    cmd.addArgs(ssh.connectionOptions(sshBinary));
    cmd.addArg(ssh.host());
    cmd.addArg(remoteCommand);
    return cmd;
}

static Result<RunResult> runPowerShell(const SshParameters &ssh, const QString &script,
                                       const QByteArray &stdInData = {})
{
    qCDebug(windowsDeviceLog) << "Running PowerShell script:" << script;
    const QString remoteCommand = "powershell -NoProfile -NonInteractive -EncodedCommand "
                                  + encodePowerShellCommand(script);
    // Run ssh as a plain local process (no nested device routing). When we have no input
    // to stream, point stdin at the null device: Windows OpenSSH otherwise keeps the
    // session open waiting for stdin EOF, even after the remote command has finished.
    Process proc;
    SshParameters::setupSshEnvironment(&proc);
    if (stdInData.isEmpty())
        proc.setStandardInputFile(QProcess::nullDevice());
    else
        proc.setWriteData(stdInData);
    const CommandLine cmd = sshCommandLine(ssh, remoteCommand);
    qCDebug(windowsDeviceLog) << "Running:" << cmd.toUserOutput();
    proc.setCommand(cmd);
    proc.runBlocking(std::chrono::seconds(60));
    const RunResult result{proc.resultData().m_exitCode,
                           proc.readAllRawStandardOutput(),
                           proc.readAllRawStandardError()};
    qCDebug(windowsDeviceLog) << "  exit code:" << result.exitCode
                             << "stdout:" << result.stdOut << "stderr:" << result.stdErr;
    return result;
}

// WindowsProcessInterface

class WindowsProcessInterface final : public ProcessInterface
{
public:
    explicit WindowsProcessInterface(const IDevice::ConstPtr &device)
        : m_device(device)
    {
        connect(&m_process, &Process::started, this, [this] {
            emit started(m_process.processId());
        });
        connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
            emit readyRead(m_process.readAllRawStandardOutput(), {});
        });
        connect(&m_process, &Process::readyReadStandardError, this, [this] {
            emit readyRead({}, m_process.readAllRawStandardError());
        });
        connect(&m_process, &Process::done, this, [this] {
            ProcessResultData result = m_process.resultData();
            // 255 is ssh's own exit code for connection or authentication failures.
            if (result.m_exitCode == 255) {
                result.m_exitStatus = QProcess::CrashExit;
                result.m_error = QProcess::Crashed;
            }
            emit done(result);
        });
    }

private:
    void start() final;
    qint64 write(const QByteArray &data) final { return m_process.writeRaw(data); }
    void sendControlSignal(ControlSignal controlSignal) final;
    CommandLine fullLocalCommandLine() const;

    IDevice::ConstPtr m_device;
    // Parented to this so it moves along when Process::waitForFinished() relocates the
    // interface to its blocking worker thread; otherwise nested blocking calls (e.g. a
    // device-rooted Process run via runBlocking) would never see the inner process's
    // events and would hang.
    Process m_process{this};
};

void WindowsProcessInterface::start()
{
    m_process.setProcessMode(m_setup.m_processMode);
    m_process.setTerminalMode(m_setup.m_terminalMode);
    m_process.setPtyData(m_setup.m_ptyData);
    m_process.setReaperTimeout(m_setup.m_reaperTimeout);
    m_process.setWriteData(m_setup.m_writeData);
    m_process.setExtraData(m_setup.m_extraData);

    // Windows OpenSSH keeps the session open until it sees EOF on stdin, even after the
    // remote command has already finished. When we have no input to stream, point ssh's
    // stdin at the null device so the connection terminates promptly.
    const bool useTerminal = m_setup.m_terminalMode != TerminalMode::Off || m_setup.m_ptyData;
    if (m_setup.m_writeData.isEmpty() && !useTerminal)
        m_process.setStandardInputFile(QProcess::nullDevice());

    SshParameters::setupSshEnvironment(&m_process);
    const CommandLine cmd = fullLocalCommandLine();
    qCDebug(windowsDeviceLog) << "WindowsProcessInterface::start, local command:"
                             << cmd.toUserOutput();
    m_process.setCommand(cmd);
    m_process.start();
}

void WindowsProcessInterface::sendControlSignal(ControlSignal controlSignal)
{
    // For now we only act on the local ssh process. Tracking and signalling the
    // remote process by pid is left for a later milestone.
    switch (controlSignal) {
    case ControlSignal::CloseWriteChannel: m_process.closeWriteChannel(); break;
    case ControlSignal::Terminate:         m_process.terminate();         break;
    case ControlSignal::Kill:              m_process.kill();              break;
    case ControlSignal::Interrupt:         m_process.interrupt();         break;
    case ControlSignal::KickOff:           m_process.kickoffProcess();    break;
    }
}

CommandLine WindowsProcessInterface::fullLocalCommandLine() const
{
    const FilePath sshBinary = sshSettings().sshFilePath();
    const SshParameters sshParameters = m_device->sshParameters();

    CommandLine cmd{sshBinary};
    cmd.addArg("-q");

    const bool useTerminal = m_setup.m_terminalMode != TerminalMode::Off || m_setup.m_ptyData;
    if (useTerminal)
        cmd.addArg("-tt");

    const QString forwardPort = m_setup.m_extraData.value(Constants::SshForwardPort).toString();
    if (!forwardPort.isEmpty()) {
        cmd.addArg("-L");
        cmd.addArg(QString("%1:localhost:%1").arg(forwardPort));
    }

    cmd.addArgs(sshParameters.connectionOptions(sshBinary));
    cmd.addArg(sshParameters.host());

    // Re-assemble the remote command without the "ssh://host" prefix. The remote
    // sshd hands the resulting string to the default shell, so a self-contained
    // "powershell -EncodedCommand ..." works regardless of which shell that is.
    const CommandLine remoteCommand = m_setup.m_commandLine;
    // Quote the executable for the remote (Windows) shell: native backslash path, wrapped
    // in double quotes when it contains spaces, e.g.
    // "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe".
    QString remote = remoteCommand.executable().nativePath();
    if (remote.contains(' '))
        remote = '"' + remote + '"';
    const QString args = remoteCommand.arguments();
    if (!args.isEmpty())
        remote += ' ' + args;

    // TODO: honor m_setup.rawWorkingDirectory(). A "cd" prefix would be shell-specific
    // (cmd.exe vs PowerShell), which contradicts the shell-agnostic approach used for file
    // access. Proper working-directory and arbitrary-command launching belong to the
    // run/debug milestone; for now the working directory is ignored.

    if (!remote.isEmpty())
        cmd.addArg(remote);

    return cmd;
}

// WindowsDeviceAccess

class WindowsDeviceAccess final : public DeviceFileAccess
{
public:
    explicit WindowsDeviceAccess(const SshParameters &ssh) : m_ssh(ssh) {}

protected:
    Result<bool> isExecutableFile(const FilePath &filePath) const override;
    Result<bool> isReadableFile(const FilePath &filePath) const override;
    Result<bool> isWritableFile(const FilePath &filePath) const override;
    Result<bool> isReadableDirectory(const FilePath &filePath) const override;
    Result<bool> isWritableDirectory(const FilePath &filePath) const override;
    Result<bool> isFile(const FilePath &filePath) const override;
    Result<bool> isDirectory(const FilePath &filePath) const override;
    Result<bool> isSymLink(const FilePath &filePath) const override;
    Result<> ensureExistingFile(const FilePath &filePath) const override;
    Result<> createDirectory(const FilePath &filePath) const override;
    Result<bool> exists(const FilePath &filePath) const override;
    Result<> removeFile(const FilePath &filePath) const override;
    Result<> removeRecursively(const FilePath &filePath) const override;
    Result<> copyFile(const FilePath &filePath, const FilePath &target) const override;
    Result<> renameFile(const FilePath &filePath, const FilePath &target) const override;

    Result<FilePathInfo> filePathInfo(const FilePath &filePath) const override;
    Result<QDateTime> lastModified(const FilePath &filePath) const override;
    Result<QFile::Permissions> permissions(const FilePath &filePath) const override;
    Result<> setPermissions(const FilePath &filePath, QFile::Permissions permissions) const override;
    Result<qint64> fileSize(const FilePath &filePath) const override;
    Result<qint64> bytesAvailable(const FilePath &filePath) const override;

    Result<> iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const override;

    Result<Environment> deviceEnvironment() const override;
    Result<QByteArray> fileContents(const FilePath &filePath, qint64 limit,
                                    qint64 offset) const override;
    Result<qint64> writeFileContents(const FilePath &filePath, const QByteArray &data) const override;

    bool supportsRemovingFiles() const override { return true; }

private:
    Result<RunResult> run(const QString &script, const QByteArray &stdInData = {}) const
    {
        return runPowerShell(m_ssh, script, stdInData);
    }

    // Runs a script that is expected to "exit 0" on true and "exit 1" on false.
    Result<bool> runBoolTest(const QString &condition) const;

    SshParameters m_ssh;
};

// Returns true if the file name has an extension that Windows considers executable.
static bool hasExecutableSuffix(const FilePath &filePath)
{
    static const QStringList suffixes = {"exe", "bat", "cmd", "com", "ps1"};
    return suffixes.contains(filePath.suffix().toLower());
}

Result<bool> WindowsDeviceAccess::runBoolTest(const QString &condition) const
{
    const QString script = QString("if (%1) { exit 0 } else { exit 1 }").arg(condition);
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    // The script only ever exits 0 (true) or 1 (false); anything else (e.g. ssh's 255
    // on a connection failure) is a real error, not a "false" answer.
    if (res->exitCode == 0)
        return true;
    if (res->exitCode == 1)
        return false;
    return ResultError(Tr::tr("Command failed (exit code %1): %2")
                           .arg(res->exitCode)
                           .arg(QString::fromUtf8(res->stdErr).trimmed()));
}

Result<bool> WindowsDeviceAccess::exists(const FilePath &filePath) const
{
    return runBoolTest(QString("Test-Path -LiteralPath %1").arg(psPath(filePath)));
}

Result<bool> WindowsDeviceAccess::isFile(const FilePath &filePath) const
{
    return runBoolTest(QString("Test-Path -LiteralPath %1 -PathType Leaf").arg(psPath(filePath)));
}

Result<bool> WindowsDeviceAccess::isDirectory(const FilePath &filePath) const
{
    if (filePath.isRootPath())
        return true;
    return runBoolTest(QString("Test-Path -LiteralPath %1 -PathType Container").arg(psPath(filePath)));
}

Result<bool> WindowsDeviceAccess::isReadableFile(const FilePath &filePath) const
{
    return isFile(filePath);
}

Result<bool> WindowsDeviceAccess::isReadableDirectory(const FilePath &filePath) const
{
    return isDirectory(filePath);
}

Result<bool> WindowsDeviceAccess::isWritableFile(const FilePath &filePath) const
{
    // Approximation: a file is writable if it exists and is not read-only.
    return runBoolTest(QString("$f = Get-Item -LiteralPath %1 -Force -ErrorAction SilentlyContinue; "
                               "$f -and -not $f.PSIsContainer -and -not $f.IsReadOnly")
                           .arg(psPath(filePath)));
}

Result<bool> WindowsDeviceAccess::isWritableDirectory(const FilePath &filePath) const
{
    // Approximation: writability of directories is not modelled in M1.
    return isDirectory(filePath);
}

Result<bool> WindowsDeviceAccess::isExecutableFile(const FilePath &filePath) const
{
    if (!hasExecutableSuffix(filePath))
        return false;
    return isFile(filePath);
}

Result<bool> WindowsDeviceAccess::isSymLink(const FilePath &filePath) const
{
    return runBoolTest(QString("$i = Get-Item -LiteralPath %1 -Force -ErrorAction SilentlyContinue; "
                               "$i -and ($i.Attributes -band [IO.FileAttributes]::ReparsePoint)")
                           .arg(psPath(filePath)));
}

Result<> WindowsDeviceAccess::ensureExistingFile(const FilePath &filePath) const
{
    const QString script = QString(
        "if (-not (Test-Path -LiteralPath %1)) { "
        "try { New-Item -ItemType File -Path %1 -ErrorAction Stop | Out-Null } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 } }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));
    return ResultOk;
}

Result<> WindowsDeviceAccess::createDirectory(const FilePath &filePath) const
{
    const QString script = QString(
        "try { New-Item -ItemType Directory -Force -Path %1 -ErrorAction Stop | Out-Null } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));
    return ResultOk;
}

Result<> WindowsDeviceAccess::removeFile(const FilePath &filePath) const
{
    const QString script = QString(
        "try { Remove-Item -LiteralPath %1 -Force -ErrorAction Stop } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));
    return ResultOk;
}

Result<> WindowsDeviceAccess::removeRecursively(const FilePath &filePath) const
{
    // Safety guard, mirroring UnixDeviceFileAccess: refuse to recursively remove
    // a drive root or a top-level directory. A valid target looks like
    // "c:/dir/..." with at least the drive, the root slash and two more levels.
    QTC_ASSERT(filePath.startsWithDriveLetter(), return ResultError(ResultAssert));
    QTC_ASSERT(!filePath.isRootPath(), return ResultError(ResultAssert));
    QTC_ASSERT(filePath.pathComponents().size() >= 4, return ResultError(ResultAssert));

    const QString script = QString(
        "try { Remove-Item -LiteralPath %1 -Recurse -Force -ErrorAction Stop } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));
    return ResultOk;
}

Result<> WindowsDeviceAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    const QString script = QString(
        "try { Copy-Item -LiteralPath %1 -Destination %2 -Force -ErrorAction Stop } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }")
        .arg(psPath(filePath), psPath(target));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
                               .arg(filePath.toUserOutput(), target.toUserOutput(),
                                    QString::fromUtf8(res->stdErr)));
    }
    return ResultOk;
}

Result<> WindowsDeviceAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    const QString script = QString(
        "try { Move-Item -LiteralPath %1 -Destination %2 -Force -ErrorAction Stop } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }")
        .arg(psPath(filePath), psPath(target));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed to rename file \"%1\" to \"%2\": %3")
                               .arg(filePath.toUserOutput(), target.toUserOutput(),
                                    QString::fromUtf8(res->stdErr)));
    }
    return ResultOk;
}

Result<qint64> WindowsDeviceAccess::fileSize(const FilePath &filePath) const
{
    const QString script = QString(
        "try { [Console]::Out.Write((Get-Item -LiteralPath %1 -Force -ErrorAction Stop).Length) } "
        "catch { exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(Tr::tr("Cannot determine size of \"%1\".").arg(filePath.toUserOutput()));
    bool ok = false;
    const qint64 size = QString::fromUtf8(res->stdOut).trimmed().toLongLong(&ok);
    if (!ok)
        return ResultError(Tr::tr("Cannot parse the size of \"%1\".").arg(filePath.toUserOutput()));
    return size;
}

Result<qint64> WindowsDeviceAccess::bytesAvailable(const FilePath &filePath) const
{
    const QString script = QString(
        "try { [Console]::Out.Write((Get-Item -LiteralPath %1 -Force -ErrorAction Stop).PSDrive.Free) } "
        "catch { exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(Tr::tr("Cannot determine free space for \"%1\".").arg(filePath.toUserOutput()));
    bool ok = false;
    const qint64 free = QString::fromUtf8(res->stdOut).trimmed().toLongLong(&ok);
    if (!ok)
        return ResultError(Tr::tr("Cannot parse the free space for \"%1\".").arg(filePath.toUserOutput()));
    return free;
}

Result<QDateTime> WindowsDeviceAccess::lastModified(const FilePath &filePath) const
{
    const QString script = QString(
        "try { [Console]::Out.Write((Get-Item -LiteralPath %1 -Force -ErrorAction Stop)"
        ".LastWriteTimeUtc.ToString('o')) } catch { exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(Tr::tr("Cannot determine modification time of \"%1\".")
                               .arg(filePath.toUserOutput()));
    return QDateTime::fromString(QString::fromUtf8(res->stdOut).trimmed(), Qt::ISODateWithMs);
}

Result<QFile::Permissions> WindowsDeviceAccess::permissions(const FilePath &filePath) const
{
    const QString script = QString(
        "try { $i = Get-Item -LiteralPath %1 -Force -ErrorAction Stop; "
        "if ($i.PSIsContainer) { [Console]::Out.Write('0') } "
        "elseif ($i.IsReadOnly) { [Console]::Out.Write('1') } "
        "else { [Console]::Out.Write('0') } } catch { exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(Tr::tr("Cannot determine permissions of \"%1\".").arg(filePath.toUserOutput()));

    const bool readOnly = QString::fromUtf8(res->stdOut).trimmed() == "1";
    QFile::Permissions perms = QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup
                               | QFile::ReadOther;
    if (!readOnly)
        perms |= QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther;
    if (hasExecutableSuffix(filePath))
        perms |= QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
    return perms;
}

Result<> WindowsDeviceAccess::setPermissions(const FilePath &filePath,
                                             QFile::Permissions permissions) const
{
    const bool readOnly = !(permissions & QFile::WriteUser);
    const QString script = QString(
        "try { $i = Get-Item -LiteralPath %1 -Force -ErrorAction Stop; "
        "if (-not $i.PSIsContainer) { $i.IsReadOnly = $%2 } } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }")
        .arg(psPath(filePath), readOnly ? QString("true") : QString("false"));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));
    return ResultOk;
}

// Builds a FilePathInfo from the type ('D'/'F'), size and modification time of a
// single entry. Permission and write flags are approximated for M1.
static FilePathInfo makeFilePathInfo(const FilePath &filePath, QChar type, qint64 size,
                                     const QDateTime &lastModified)
{
    FilePathInfo info;
    info.fileSize = size;
    info.lastModified = lastModified;

    const auto addExecutablePerms = [](FilePathInfo::FileFlags &flags) {
        flags |= FilePathInfo::ExeOwnerPerm;
        flags |= FilePathInfo::ExeUserPerm;
        flags |= FilePathInfo::ExeGroupPerm;
        flags |= FilePathInfo::ExeOtherPerm;
    };

    FilePathInfo::FileFlags flags;
    flags |= FilePathInfo::ExistsFlag;
    flags |= FilePathInfo::LocalDiskFlag;
    flags |= FilePathInfo::ReadOwnerPerm;
    flags |= FilePathInfo::ReadUserPerm;
    flags |= FilePathInfo::ReadGroupPerm;
    flags |= FilePathInfo::ReadOtherPerm;
    flags |= FilePathInfo::WriteOwnerPerm;
    flags |= FilePathInfo::WriteUserPerm;
    if (type == 'D') {
        flags |= FilePathInfo::DirectoryType;
        addExecutablePerms(flags);
    } else {
        flags |= FilePathInfo::FileType;
        if (hasExecutableSuffix(filePath))
            addExecutablePerms(flags);
    }
    info.fileFlags = flags;
    return info;
}

Result<FilePathInfo> WindowsDeviceAccess::filePathInfo(const FilePath &filePath) const
{
    if (filePath.isRootPath()) {
        return makeFilePathInfo(filePath, 'D', 0, QDateTime::currentDateTimeUtc());
    }

    const QString script = QString(
        "try { $i = Get-Item -LiteralPath %1 -Force -ErrorAction Stop; "
        "$t = if ($i.PSIsContainer) {'D'} else {'F'}; "
        "$s = if ($i.PSIsContainer) {0} else {$i.Length}; "
        "[Console]::Out.Write(('{0}|{1}|{2}' -f $t,$s,$i.LastWriteTimeUtc.ToString('o'))) } "
        "catch { exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    // The script exits 1 (via catch) only when the item does not exist; any other
    // non-zero code (e.g. ssh's 255 on a dropped connection) is a real error.
    if (res->exitCode == 1)
        return FilePathInfo(); // Does not exist.
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Cannot determine information about \"%1\" (exit code %2): %3")
                               .arg(filePath.toUserOutput())
                               .arg(res->exitCode)
                               .arg(QString::fromUtf8(res->stdErr).trimmed()));
    }

    const QStringList parts = QString::fromUtf8(res->stdOut).trimmed().split('|');
    if (parts.size() < 3)
        return FilePathInfo();

    const QDateTime dt = QDateTime::fromString(parts.at(2), Qt::ISODateWithMs);
    return makeFilePathInfo(filePath, parts.at(0).at(0), parts.at(1).toLongLong(), dt);
}

Result<> WindowsDeviceAccess::iterateDirectory(
    const FilePath &filePath,
    const FilePath::IterateDirCallback &callBack,
    const FileFilter &filter) const
{
    // Match the requested name filters against the file name, mirroring the
    // wildcard handling of the Unix 'ls' code path.
    const QList<QRegularExpression> nameRegexps = transform(filter.nameFilters,
        [](const QString &filter) {
            QRegularExpression re(QRegularExpression::wildcardToRegularExpression(filter));
            QTC_CHECK(re.isValid());
            return re;
        });
    const auto nameMatches = [&nameRegexps](const QString &fileName) {
        for (const QRegularExpression &re : nameRegexps) {
            if (re.match(fileName).hasMatch())
                return true;
        }
        return nameRegexps.isEmpty();
    };

    const bool withInfo = callBack.index() == 1;
    const auto emitEntry = [&](const FilePath &entry, const FilePathInfo &info) {
        if (!nameMatches(entry.fileName()))
            return IterationPolicy::Continue;
        if (withInfo)
            return std::get<1>(callBack)(entry, info);
        return std::get<0>(callBack)(entry);
    };

    // The artificial device root "/" lists the available file-system drives.
    if (filePath.path() == "/") {
        const Result<RunResult> res = run(
            "Get-PSDrive -PSProvider FileSystem | ForEach-Object { [Console]::Out.Write($_.Name + \"`n\") }");
        if (!res)
            return ResultError(res.error());
        const QStringList names = QString::fromUtf8(res->stdOut).split('\n', Qt::SkipEmptyParts);
        for (const QString &name : names) {
            const FilePath drive = filePath.withNewPath(name.trimmed().toLower() + ":/");
            const FilePathInfo info = makeFilePathInfo(drive, 'D', 0, QDateTime::currentDateTimeUtc());
            if (emitEntry(drive, info) == IterationPolicy::Stop)
                break;
        }
        return ResultOk;
    }

    const QString script = QString(
        "$ErrorActionPreference = 'SilentlyContinue'; "
        "Get-ChildItem -Force -LiteralPath %1 | ForEach-Object { "
        "$t = if ($_.PSIsContainer) {'D'} else {'F'}; "
        "$s = if ($_.PSIsContainer) {0} else {$_.Length}; "
        "[Console]::Out.Write(('{0}|{1}|{2}|{3}' -f $t,$s,$_.LastWriteTimeUtc.ToString('o'),$_.FullName) + \"`n\") }")
        .arg(psPath(filePath));
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());

    const QStringList lines = QString::fromUtf8(res->stdOut).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        // Format: "<type>|<size>|<iso-utc>|<full-path>" where the path may contain '|'.
        const int firstSep = line.indexOf('|');
        const int secondSep = line.indexOf('|', firstSep + 1);
        const int thirdSep = line.indexOf('|', secondSep + 1);
        if (firstSep < 0 || secondSep < 0 || thirdSep < 0)
            continue;

        const QChar type = line.at(0);
        const qint64 size = line.mid(firstSep + 1, secondSep - firstSep - 1).toLongLong();
        const QDateTime dt = QDateTime::fromString(line.mid(secondSep + 1, thirdSep - secondSep - 1),
                                                   Qt::ISODateWithMs);
        const QString nativeName = line.mid(thirdSep + 1);

        const FilePath entry = filePath.withNewPath(QString(nativeName).replace('\\', '/'));
        const FilePathInfo info = makeFilePathInfo(entry, type, size, dt);
        if (emitEntry(entry, info) == IterationPolicy::Stop)
            break;
    }
    return ResultOk;
}

Result<Environment> WindowsDeviceAccess::deviceEnvironment() const
{
    const Result<RunResult> res = run(
        "Get-ChildItem Env: | ForEach-Object { [Console]::Out.Write(($_.Name + '=' + $_.Value) + \"`n\") }");
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0)
        return ResultError(Tr::tr("Cannot read the device environment."));

    // Strip CR so values (e.g. used to build paths) don't end up with a trailing '\r'.
    QString out = QString::fromUtf8(res->stdOut);
    out.remove('\r');
    const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
    qCDebug(windowsDeviceLog) << "deviceEnvironment: parsed" << lines.size() << "entries";
    return Environment(lines, OsTypeWindows);
}

Result<QByteArray> WindowsDeviceAccess::fileContents(const FilePath &filePath, qint64 limit,
                                                     qint64 offset) const
{
    // Note: this reads the whole file into PowerShell memory and then base64-slices the
    // requested range. That is simple and binary-safe, but a ranged read of a very large
    // file may exhaust memory or hit the runPowerShell() timeout. A streamed
    // FileStream.Seek/Read would avoid that and is left as a future improvement.
    const QString script = QString(
        "try { $b = [IO.File]::ReadAllBytes(%1); "
        "$off = %2; $len = %3; "
        "if ($len -le 0) { $len = $b.Length - $off }; "
        "if ($off -gt $b.Length) { $off = $b.Length }; "
        "if ($off + $len -gt $b.Length) { $len = $b.Length - $off }; "
        "[Console]::Out.Write([Convert]::ToBase64String($b, $off, $len)) } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }")
        .arg(psPath(filePath)).arg(offset).arg(limit);
    const Result<RunResult> res = run(script);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed reading file \"%1\": %2")
                               .arg(filePath.toUserOutput(), QString::fromUtf8(res->stdErr)));
    }
    return QByteArray::fromBase64(QByteArray(res->stdOut).trimmed());
}

Result<qint64> WindowsDeviceAccess::writeFileContents(const FilePath &filePath,
                                                      const QByteArray &data) const
{
    const QString script = QString(
        "try { $in = ([Console]::In.ReadToEnd()).Trim(); "
        "[IO.File]::WriteAllBytes(%1, [Convert]::FromBase64String($in)) } "
        "catch { [Console]::Error.Write($_.Exception.Message); exit 1 }").arg(psPath(filePath));
    const Result<RunResult> res = run(script, data.toBase64());
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed writing file \"%1\": %2")
                               .arg(filePath.toUserOutput(), QString::fromUtf8(res->stdErr)));
    }
    return data.size();
}

// WindowsDevicePrivate

class WindowsDevicePrivate
{
public:
    explicit WindowsDevicePrivate(WindowsDevice *parent) : q(parent) {}

    void setupFileAccess(const Continuation<> &cont);

    WindowsDevice *q = nullptr;
};

void WindowsDevicePrivate::setupFileAccess(const Continuation<> &cont)
{
    QTC_ASSERT(QThread::isMainThread(),
               cont(ResultError(ResultAssert, "setupFileAccess called from wrong thread"));
               return);

    q->setFileAccess(nullptr);
    // Make the process interface usable even though we are not "connected" yet.
    q->setIsTesting(true);

    qCDebug(windowsDeviceLog) << "setupFileAccess: probing" << q->rootPath().toUserOutput();
    QFuture<Result<>> future = Utils::asyncRun([ssh = q->sshParameters()]() -> Result<> {
        qCDebug(windowsDeviceLog) << "setupFileAccess: running probe command...";
        const Result<RunResult> res = runPowerShell(
            ssh, "[Console]::Out.Write('QTCWIN:' + [Environment]::OSVersion.Platform)");
        qCDebug(windowsDeviceLog) << "setupFileAccess: probe command returned, ok =" << bool(res);
        if (!res)
            return ResultError(res.error());
        if (res->exitCode != 0) {
            return ResultError(Tr::tr("Failed to run PowerShell on the device: %1")
                                   .arg(QString::fromUtf8(res->stdErr)));
        }
        if (!QString::fromUtf8(res->stdOut).contains("QTCWIN:Win32NT")) {
            return ResultError(Tr::tr("The remote host does not appear to be a Windows machine "
                                      "with PowerShell available."));
        }
        return ResultOk;
    });
    future.then(q, [this, cont](const Result<> &res) {
        qCDebug(windowsDeviceLog) << "setupFileAccess: continuation, ok =" << bool(res)
                                 << (res ? QString() : res.error());
        q->setIsTesting(false);
        if (res) {
            q->setFileAccess(std::make_shared<WindowsDeviceAccess>(q->sshParameters()));
            q->setDeviceState(IDevice::DeviceReadyToUse);
        } else {
            q->setFileAccess(nullptr);
            q->setDeviceState(IDevice::DeviceDisconnected);
        }
        cont(res);
        DeviceManager::instance()->deviceUpdated(q->id());
    });
    Utils::futureSynchronizer()->addFuture(future);
}

// WindowsDevice

WindowsDevice::WindowsDevice()
    : d(new WindowsDevicePrivate(this))
{
    setupId(IDevice::ManuallyAdded, Utils::Id());
    setDisplayType(Tr::tr("Remote Windows"));
    setOsType(OsTypeWindows);
    setDefaultDisplayName(Tr::tr("Remote Windows Device"));
    setType(Constants::GenericWindowsOsType);
    setMachineType(IDevice::Hardware);
    setFreePorts(PortList::fromString(QLatin1String("10000-10100")));

    SshParameters sshParams;
    sshParams.setTimeout(10);
    setDefaultSshParameters(sshParams);
}

WindowsDevice::~WindowsDevice()
{
    delete d;
}

QString WindowsDevice::userAtHost() const
{
    return sshParameters().userAtHost();
}

QString WindowsDevice::userAtHostAndPort() const
{
    return sshParameters().userAtHostAndPort();
}

FilePath WindowsDevice::rootPath() const
{
    return FilePath::fromParts(u"ssh", userAtHostAndPort(), u"/");
}

Result<> WindowsDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"ssh" && filePath.host() == userAtHostAndPort())
        return ResultOk;
    return IDevice::handlesFile(filePath);
}

ProcessInterface *WindowsDevice::createProcessInterface() const
{
    return new WindowsProcessInterface(shared_from_this());
}

void WindowsDevice::tryToConnect(const Continuation<> &cont) const
{
    d->setupFileAccess(cont);
}

// WindowsDeviceConfigurationWidget

class WindowsDeviceConfigurationWidget final : public IDeviceWidget
{
public:
    explicit WindowsDeviceConfigurationWidget(const IDevicePtr &device);

private:
    void createNewKey();
    void updateDeviceFromUi() override {}
};

// Runs toolchain auto-detection for the device (MSVC, over SSH) and registers the results.
// Kept to toolchains only for now; Qt detection and kit creation come later via the
// generic kitDetectionRecipe.
static void detectAndRegisterToolchains(const IDevice::Ptr &device)
{
    const FilePaths searchPaths = {device->rootPath()};
    const Toolchains known = ToolchainManager::toolchains();
    qCDebug(windowsDeviceLog) << "Starting toolchain detection for"
                              << device->rootPath().toUserOutput();
    QFuture<Toolchains> future = Utils::asyncRun([device, searchPaths, known]() -> Toolchains {
        ToolchainDetector detector(known, device, searchPaths);
        Toolchains found;
        for (ToolchainFactory *factory : ToolchainFactory::allToolchainFactories()) {
            const Toolchains detected = factory->autoDetect(detector);
            for (Toolchain *tc : detected) {
                if (tc->isValid())
                    found.append(tc);
                else
                    delete tc;
            }
        }
        return found;
    });
    future.then(device.get(), [](const Toolchains &found) {
        const Toolchains registered = ToolchainManager::registerToolchains(found);
        qCDebug(windowsDeviceLog) << "Registered" << registered.size() << "of" << found.size()
                                  << "detected toolchain(s)";
    });
    Utils::futureSynchronizer()->addFuture(future);
}

WindowsDeviceConfigurationWidget::WindowsDeviceConfigurationWidget(const IDevicePtr &device)
    : IDeviceWidget(device)
{
    auto windowsDevice = std::dynamic_pointer_cast<WindowsDevice>(device);
    QTC_ASSERT(windowsDevice, return);

    auto createKeyButton = new QPushButton(Tr::tr("Create New..."));
    auto autoDetectButton = new QPushButton(Tr::tr("Run Auto-Detection Now"));

    connect(autoDetectButton, &QPushButton::clicked, this, [windowsDevice, autoDetectButton] {
        autoDetectButton->setEnabled(false);
        windowsDevice->tryToConnect(
            {windowsDevice.get(),
             [windowsDevice, autoDetectButton = QPointer<QWidget>(autoDetectButton)](
                 const Result<> &res) {
                 if (autoDetectButton)
                     autoDetectButton->setEnabled(true);
                 if (res)
                     detectAndRegisterToolchains(windowsDevice);
             }});
    });

    SshParametersAspectContainer &ssh = device->sshParametersAspectContainer();

    using namespace Layouting;
    // clang-format off
    Form {
        ssh.host, ssh.port, ssh.hostKeyCheckingMode, st, br,
        ssh.timeout, st, br,
        ssh.userName, st, br,
        ssh.useKeyFile, st, br,
        ssh.privateKeyFile, createKeyButton, br,
        Row { autoDetectButton, st },
    }.attachTo(this);
    // clang-format on

    connect(createKeyButton, &QAbstractButton::clicked,
            this, &WindowsDeviceConfigurationWidget::createNewKey);
    connect(&device->sshParametersAspectContainer(), &AspectContainer::volatileValueChanged,
            this, &markSettingsDirty);

    installMarkSettingsDirtyTriggerRecursively(this);
}

void WindowsDeviceConfigurationWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        device()->sshParametersAspectContainer().privateKeyFile.setValue(dialog.privateKeyFilePath());
}

IDeviceWidget *WindowsDevice::createWidget()
{
    return new WindowsDeviceConfigurationWidget(shared_from_this());
}

DeviceTester *WindowsDevice::createDeviceTester()
{
    return new WindowsDeviceTester(shared_from_this());
}

// WindowsDeviceFactory

namespace Internal {

WindowsDeviceFactory::WindowsDeviceFactory()
    : IDeviceFactory(Constants::GenericWindowsOsType)
{
    setDisplayName(Tr::tr("Remote Windows Device"));
    setIcon(QIcon());
    setQuickCreationAllowed(true);
    setConstructionFunction(&WindowsDevice::create);
    setCreator([]() -> IDevice::Ptr {
        auto device = WindowsDevice::create();
        SshDeviceWizard wizard(Tr::tr("New Remote Windows Device Configuration Setup"),
                               IDevice::Ptr(device));
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });
    setExecutionTypeId(Constants::ExecutionType);
}

} // namespace Internal
} // namespace Remote
