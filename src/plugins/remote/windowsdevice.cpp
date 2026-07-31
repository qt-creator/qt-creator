// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsdevice.h"

#include "powershellutils.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "sshdevicewizard.h"
#include "sshkeycreationdialog.h"
#include "windowsdevicetester.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <debugger/debuggerconstants.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitaspect.h>

#include <gocmdbridge/client/bridgedfileaccess.h>
#include <gocmdbridge/client/cmdbridgeclient.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/msvctoolchain.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/globaltasktree.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/guiutils.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDateTime>
#include <QLoggingCategory>
#include <QMutex>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <QUuid>
#include <QVersionNumber>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote {

static Q_LOGGING_CATEGORY(windowsDeviceLog, "qtc.remotewindows.device", QtWarningMsg)

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
            qCDebug(windowsDeviceLog) << "WindowsProcessInterface done, exit code"
                                      << result.m_exitCode << "stderr"
                                      << m_process.readAllRawStandardError();
            // 255 is ssh's own exit code for connection or authentication failures.
            if (result.m_exitCode == 255) {
                result.m_exitStatus = ProcessExitStatus::CrashExit;
                result.m_error = ProcessError::Crashed;
            }
            if (!m_envScript.isEmpty()) {
                m_envScript.removeFile();
                m_envScript.clear();
            }
            if (!m_runTempDir.isEmpty()) {
                m_runTempDir.removeRecursively();
                m_runTempDir.clear();
            }
            emit done(result);
        });
    }

private:
    void start() final;
    qint64 write(const QByteArray &data) final { return m_process.writeRaw(data); }
    void sendControlSignal(ControlSignal controlSignal) final;
    CommandLine fullLocalCommandLine();
    QString buildInteractiveRunRemoteCommand();

    IDevice::ConstPtr m_device;
    // A PowerShell script written to the device's temp dir to apply the build environment
    // before running the command (see fullLocalCommandLine); removed when the process is done.
    FilePath m_envScript;
    // A temp dir on the device holding the interactive-run orchestration script and its
    // output/exit files (see buildInteractiveRunRemoteCommand); removed when done.
    FilePath m_runTempDir;
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
    // remote command has already finished. For a one-shot Reader command with no input,
    // point ssh's stdin at the null device so the connection terminates promptly. A
    // streaming process (e.g. the CmdBridge, started in Writer mode) must keep stdin open.
    const bool useTerminal = m_setup.m_terminalMode != TerminalMode::Off || m_setup.m_ptyData;
    if (m_setup.m_processMode == ProcessMode::Reader && m_setup.m_writeData.isEmpty()
        && !useTerminal) {
        m_process.setStandardInputFile(QProcess::nullDevice());
    }

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

CommandLine WindowsProcessInterface::fullLocalCommandLine()
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
    const QString args = remoteCommand.arguments();

    QString remote;

    // A GUI run must appear on the device's interactive desktop, not the (invisible) SSH
    // session; WindowsRunWorkerFactory flags such processes. The launch is orchestrated by a
    // staged script that starts the app in the interactive session and waits for it.
    if (m_setup.m_extraData.value(Constants::RunInInteractiveSession).toBool())
        remote = buildInteractiveRunRemoteCommand();

    // A process that carries an explicit environment (e.g. a build run with the kit's
    // build environment: MSVC vcvars putting INCLUDE/LIB in the env and the SDK bin on
    // PATH) needs that environment applied on the device, the way SshProcessInterface
    // prefixes "KEY=value cmd" on Unix. We do it through a PowerShell script that sets
    // $env:KEY for each entry, then runs the command and propagates its exit code. The
    // build environment is too large to inline on the remote command line (Windows caps it
    // around 32 KB), so the script is written to the device's temp directory and run via
    // "-File", mirroring how the MSVC vcvars environment capture stages a .bat there.
    // The streaming CmdBridge process (ProcessMode::Writer, binary protocol on stdin/stdout)
    // is excluded - a PowerShell wrapper would corrupt its byte stream; it keeps the plain
    // direct invocation below.
    const bool injectEnvironment = remote.isEmpty()
                                   && m_setup.m_processMode != ProcessMode::Writer
                                   && !m_setup.m_environment.toStringList().isEmpty();

    if (injectEnvironment) {
        const Environment &env = m_setup.m_environment;
        QString script;
        env.forEachEntry([&](const QString &key, const QString &value, bool enabled) {
            // Use SetEnvironmentVariable rather than "$env:KEY = ...": some Windows variable
            // names contain characters PowerShell cannot parse after "$env:", e.g. the "(x86)"
            // in "ProgramFiles(x86)" / "CommonProgramFiles(x86)".
            if (enabled && !key.trimmed().isEmpty() && !value.contains('\n')) {
                script += "[Environment]::SetEnvironmentVariable(" + psQuote(key) + ", "
                          + psQuote(env.expandVariables(value)) + ")\n";
            }
        });
        const FilePath workingDirectory = m_setup.rawWorkingDirectory();
        if (!workingDirectory.isEmpty())
            script += "Set-Location -LiteralPath " + psPath(workingDirectory) + "\n";
        // "--%" (the PowerShell stop-parsing token) passes the already-quoted Windows
        // arguments to the program verbatim, so PowerShell does not re-interpret them.
        script += "& " + psPath(remoteCommand.executable());
        if (!args.isEmpty())
            script += " --% " + args;
        script += "\nexit $LASTEXITCODE\n";

        // Stage the script in the device user's temp directory (taken from the build
        // environment, so no extra remote round-trip), then run it by path.
        QString tempDir = env.value("TEMP");
        if (tempDir.isEmpty())
            tempDir = env.value("TMP");
        if (tempDir.isEmpty())
            tempDir = "C:/Windows/Temp";
        tempDir.replace('\\', '/');
        const FilePath scriptPath = m_device->rootPath().withNewPath(tempDir)
                / ("qtc-run-" + QUuid::createUuid().toString(QUuid::Id128) + ".ps1");
        if (const Result<qint64> res = scriptPath.writeFileContents(script.toUtf8()); res) {
            m_envScript = scriptPath;
            remote = "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \""
                     + scriptPath.nativePath() + "\"";
        } else {
            // Could not stage the script; fall back to running without the environment.
            qCWarning(windowsDeviceLog) << "Failed to write env script" << scriptPath.toUserOutput()
                                        << ":" << res.error();
        }
    }

    if (remote.isEmpty()) {
        // Quote the executable for the remote (Windows) shell: native backslash path,
        // wrapped in double quotes when it contains spaces, e.g.
        // "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe".
        remote = remoteCommand.executable().nativePath();
        if (remote.contains(' '))
            remote = '"' + remote + '"';
        if (!args.isEmpty())
            remote += ' ' + args;

        // Apply the requested working directory. The PowerShell env script above does
        // this via Set-Location, but this direct invocation would otherwise run in
        // sshd's default working directory (the user's home directory), silently
        // ignoring the caller's setWorkingDirectory(). Wrap the command in
        // "cmd /d /s /c" ("/s": strip only the outer quotes) and prefix "cd /d".
        // A cmd.exe wrapper - unlike a PowerShell one - passes stdin/stdout handles
        // straight to the child, so binary streams of Writer-mode processes survive.
        const FilePath workingDirectory = m_setup.rawWorkingDirectory();
        if (!remote.isEmpty() && !workingDirectory.isEmpty()) {
            remote = "cmd /d /s /c \"cd /d \"" + workingDirectory.nativePath() + "\" & "
                     + remote + '"';
        }
    }

    if (!remote.isEmpty())
        cmd.addArg(remote);

    return cmd;
}

// Builds the remote command for a GUI run: stages a PowerShell orchestrator on the device and
// returns the "powershell -File <script>" invocation to run over SSH (in the invisible session 0).
// Run in its default mode the script creates an interactive scheduled task (schtasks /it) that
// re-invokes it in "app" mode inside the logged-on user's desktop session, where it applies the
// run environment and starts the application (so its window is actually visible), waits for it,
// and reports the exit code back. Returns an empty string on staging failure (caller falls back).
QString WindowsProcessInterface::buildInteractiveRunRemoteCommand()
{
    const CommandLine remoteCommand = m_setup.m_commandLine;
    const Environment &env = m_setup.m_environment;

    QString envScript;
    env.forEachEntry([&](const QString &key, const QString &value, bool enabled) {
        if (enabled && !key.trimmed().isEmpty() && !value.contains('\n')) {
            envScript += "    [Environment]::SetEnvironmentVariable(" + psQuote(key) + ", "
                         + psQuote(env.expandVariables(value)) + ")\n";
        }
    });

    // Stage under C:\Users\Public: the application is launched by SYSTEM as the (possibly
    // different) desktop user, not the SSH user, so both the script and its output/exit files
    // must live where that user can read and write them.
    const QString id = QUuid::createUuid().toString(QUuid::Id128);
    const FilePath dir = m_device->rootPath().withNewPath("C:/Users/Public/qtc-run-" + id);
    if (const Result<> res = dir.ensureWritableDir(); !res) {
        qCWarning(windowsDeviceLog) << "Failed to create interactive-run dir"
                                    << dir.toUserOutput() << ":" << res.error();
        return {};
    }

    const FilePath scriptPath = dir / "run.ps1";
    const QString self = scriptPath.nativePath();
    const QString workingDir = m_setup.rawWorkingDirectory().isEmpty()
            ? QString() : m_setup.rawWorkingDirectory().nativePath();

    QString script;
    script += "param([string]$Mode = 'run')\n";
    script += "$ErrorActionPreference = 'SilentlyContinue'\n";
    script += "$out = " + psQuote((dir / "out.txt").nativePath()) + "\n";
    script += "$err = " + psQuote((dir / "err.txt").nativePath()) + "\n";
    script += "$done = " + psQuote((dir / "exit.txt").nativePath()) + "\n";
    script += "$started = " + psQuote((dir / "started.txt").nativePath()) + "\n";
    script += "$exe = " + psQuote(remoteCommand.executable().nativePath()) + "\n";
    script += "$tn = " + psQuote("qtc_run_" + id) + "\n";
    script += "$self = " + psQuote(self) + "\n\n";

    // "app" mode: runs inside the interactive session, applies the env and starts the app.
    // The first thing it does is drop a "started" marker, so the orchestrator can tell that the
    // task actually began running (as opposed to never launching, e.g. the target user is not
    // logged on) without having to wait for the application to exit.
    script += "if ($Mode -eq 'app') {\n";
    script += "    Set-Content -Path $started -Value 1\n";
    script += envScript;
    script += "    $a = @{ FilePath = $exe; PassThru = $true; Wait = $true;\n";
    script += "            RedirectStandardOutput = $out; RedirectStandardError = $err }\n";
    if (!workingDir.isEmpty())
        script += "    $a['WorkingDirectory'] = " + psQuote(workingDir) + "\n";
    if (!remoteCommand.arguments().isEmpty())
        script += "    $a['ArgumentList'] = " + psQuote(remoteCommand.arguments()) + "\n";
    script += "    $code = 1\n";
    script += "    try { $p = Start-Process @a; $code = $p.ExitCode }\n";
    script += "    catch { Add-Content -Path $err -Value ('qtc: failed to start the application: '"
              " + $_.Exception.Message) }\n";
    script += "    Set-Content -Path $done -Value $code\n";
    script += "    return\n";
    script += "}\n\n";

    // "sys" mode: runs as SYSTEM (via a scheduled task the run mode creates). Only SYSTEM may call
    // WTSQueryUserToken, so this is where we cross from the invisible SSH session 0 onto the user's
    // interactive desktop: resolve the desktop session (owner of explorer.exe - reliable on console
    // or RDP, and even for a logged-on-but-disconnected session), grab that user's token, and
    // CreateProcessAsUser the "app" mode into their session (winsta0\default) with their
    // environment. Failures are written to $err so the run mode can report them.
    script += "if ($Mode -eq 'sys') {\n";
    script += "    Add-Type -Namespace Qtc -Name Native -MemberDefinition @'\n";
    script += "[DllImport(\"wtsapi32.dll\", SetLastError=true)]\n";
    script += "public static extern bool WTSQueryUserToken(uint SessionId, out IntPtr phToken);\n";
    script += "[DllImport(\"wtsapi32.dll\", SetLastError=true)]\n";
    script += "public static extern int WTSEnumerateSessions(IntPtr hServer, int reserved, int version, out IntPtr ppSessionInfo, out int count);\n";
    script += "[DllImport(\"wtsapi32.dll\")]\n";
    script += "public static extern void WTSFreeMemory(IntPtr p);\n";
    script += "[StructLayout(LayoutKind.Sequential)]\n";
    script += "public struct WTS_SESSION_INFO { public uint SessionId; public IntPtr pWinStationName; public int State; }\n";
    script += "[DllImport(\"advapi32.dll\", SetLastError=true)]\n";
    script += "public static extern bool DuplicateTokenEx(IntPtr h, uint access, IntPtr attr, int imp, int type, out IntPtr phNew);\n";
    script += "[DllImport(\"userenv.dll\", SetLastError=true)]\n";
    script += "public static extern bool CreateEnvironmentBlock(out IntPtr env, IntPtr hToken, bool inherit);\n";
    script += "[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]\n";
    script += "public struct STARTUPINFO { public int cb; public string lpReserved; public string lpDesktop;\n";
    script += "  public string lpTitle; public int dwX; public int dwY; public int dwXSize; public int dwYSize;\n";
    script += "  public int dwXCountChars; public int dwYCountChars; public int dwFillAttribute; public int dwFlags;\n";
    script += "  public short wShowWindow; public short cbReserved2; public IntPtr lpReserved2;\n";
    script += "  public IntPtr hStdInput; public IntPtr hStdOutput; public IntPtr hStdError; }\n";
    script += "[StructLayout(LayoutKind.Sequential)]\n";
    script += "public struct PROCESS_INFORMATION { public IntPtr hProcess; public IntPtr hThread; public int dwPid; public int dwTid; }\n";
    script += "[DllImport(\"advapi32.dll\", SetLastError=true, CharSet=CharSet.Unicode)]\n";
    script += "public static extern bool CreateProcessAsUser(IntPtr hToken, string app, string cmd, IntPtr pa, IntPtr ta,\n";
    script += "  bool inherit, uint flags, IntPtr env, string cwd, ref STARTUPINFO si, out PROCESS_INFORMATION pi);\n";
    script += "'@\n";
    // Target the ACTIVE interactive session deterministically - the one a user is currently
    // looking at - rather than an arbitrary logged-on session (WTS_CONNECTSTATE_CLASS.WTSActive
    // is 0). If nothing is active, there is no visible desktop to run on, so report that.
    script += "    $sid = -1\n";
    script += "    $pInfo = [IntPtr]::Zero; $count = 0\n";
    script += "    if ([Qtc.Native]::WTSEnumerateSessions([IntPtr]::Zero, 0, 1, [ref]$pInfo, [ref]$count)) {\n";
    script += "        $sz = [Runtime.InteropServices.Marshal]::SizeOf([type]'Qtc.Native+WTS_SESSION_INFO')\n";
    script += "        for ($i = 0; $i -lt $count; $i++) {\n";
    script += "            $e = [Runtime.InteropServices.Marshal]::PtrToStructure("
              "[IntPtr]([int64]$pInfo + $i * $sz), [type]'Qtc.Native+WTS_SESSION_INFO')\n";
    script += "            if ($e.State -eq 0) { $sid = [int]$e.SessionId; break }\n";
    script += "        }\n";
    script += "        [Qtc.Native]::WTSFreeMemory($pInfo)\n";
    script += "    }\n";
    script += "    if ($sid -lt 0) { Add-Content -Path $err -Value 'qtc: no active interactive "
              "session on the device.'; return }\n";
    script += "    $le = { [Runtime.InteropServices.Marshal]::GetLastWin32Error() }\n";
    script += "    $tok = [IntPtr]::Zero\n";
    script += "    if (-not [Qtc.Native]::WTSQueryUserToken([uint32]$sid, [ref]$tok)) "
              "{ Add-Content -Path $err -Value ('qtc: WTSQueryUserToken failed err=' + (& $le)); return }\n";
    script += "    $dup = [IntPtr]::Zero\n";
    script += "    if (-not [Qtc.Native]::DuplicateTokenEx($tok, 0x02000000, [IntPtr]::Zero, 2, 1, [ref]$dup)) "
              "{ Add-Content -Path $err -Value ('qtc: DuplicateTokenEx failed err=' + (& $le)); return }\n";
    script += "    $envb = [IntPtr]::Zero\n";
    script += "    [Qtc.Native]::CreateEnvironmentBlock([ref]$envb, $dup, $false) | Out-Null\n";
    script += "    $si = New-Object 'Qtc.Native+STARTUPINFO'\n";
    script += "    $si.cb = [Runtime.InteropServices.Marshal]::SizeOf($si)\n";
    script += "    $si.lpDesktop = 'winsta0\\default'\n";
    script += "    $pi = New-Object 'Qtc.Native+PROCESS_INFORMATION'\n";
    // Use $PSHOME to locate powershell.exe (robust in the SYSTEM task, whose environment may lack
    // SystemRoot); CreateProcessAsUser needs a full, valid application path.
    script += "    $psexe = (Join-Path $PSHOME 'powershell.exe')\n";
    script += "    $cmd = '\"' + $psexe + '\" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"' "
              "+ $self + '\" -Mode app'\n";
    // CREATE_NO_WINDOW 0x08000000 | CREATE_UNICODE_ENVIRONMENT 0x00000400; cwd must be a valid dir.
    script += "    $ok = [Qtc.Native]::CreateProcessAsUser($dup, $psexe, $cmd, [IntPtr]::Zero, [IntPtr]::Zero, "
              "$false, 0x08000400, $envb, $PSHOME, [ref]$si, [ref]$pi)\n";
    script += "    if (-not $ok) { Add-Content -Path $err -Value ('qtc: CreateProcessAsUser failed err=' + (& $le)) }\n";
    script += "    return\n";
    script += "}\n\n";

    // Default ("run") mode: runs as the SSH user in the invisible session 0. It cannot reach the
    // interactive desktop directly, so it elevates to SYSTEM via a scheduled task that runs the
    // "sys" mode above. Reaching SYSTEM needs local-admin rights; without them, fail clearly.
    script += "if (-not ([Security.Principal.WindowsPrincipal]"
              "[Security.Principal.WindowsIdentity]::GetCurrent())"
              ".IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {\n";
    script += "    [Console]::Error.Write('qtc: showing a GUI run needs the device account to be a "
              "local administrator.')\n";
    script += "    exit 1\n";
    script += "}\n";
    script += "$tr = 'powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"'"
              " + $self + '\" -Mode sys'\n";
    script += "$create = schtasks /create /f /tn $tn /tr $tr /sc once /st 00:00 /ru SYSTEM 2>&1\n";
    script += "schtasks /run /tn $tn 2>&1 | Out-Null\n";
    // Wait for the app to actually begin (the "started" marker), but only briefly: if the launcher
    // never gets that far report a diagnostic (including anything it wrote to $err) instead of
    // hanging forever. Once it has started, wait as long as the application keeps running.
    script += "$deadline = (Get-Date).AddSeconds(20)\n";
    script += "while (-not (Test-Path $started) -and -not (Test-Path $done)"
              " -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }\n";
    script += "if (-not (Test-Path $started) -and -not (Test-Path $done)) {\n";
    script += "    schtasks /delete /f /tn $tn 2>&1 | Out-Null\n";
    script += "    $msg = 'qtc: the GUI run did not start.'\n";
    script += "    if (Test-Path $err) { $msg += ' ' + (Get-Content -Raw $err) }\n";
    script += "    [Console]::Error.Write($msg)\n";
    script += "    exit 1\n";
    script += "}\n";
    script += "while (-not (Test-Path $done)) { Start-Sleep -Milliseconds 300 }\n";
    script += "schtasks /delete /f /tn $tn 2>&1 | Out-Null\n";
    script += "if (Test-Path $out) { [Console]::Out.Write((Get-Content -Raw $out)) }\n";
    script += "if (Test-Path $err) { [Console]::Error.Write((Get-Content -Raw $err)) }\n";
    // The status travels back as the exit code of the SSH connection, which carries only its low
    // 8 bits. A Windows status code that is a multiple of 256 - which many crash and loader
    // failures are, e.g. 0xC0000100 - would arrive as 0, reporting a failed run as successful.
    // So pass ordinary exit codes through unchanged and turn anything that does not survive into
    // a plain failure that names the real status.
    script += "$raw = (Get-Content -Raw $done)\n";
    script += "if ($raw -notmatch '^\\s*-?\\d+\\s*$') {\n";
    script += "    [Console]::Error.Write('qtc: the application did not report an exit code.')\n";
    script += "    exit 1\n";
    script += "}\n";
    script += "$code = [int]$raw.Trim()\n";
    script += "if ($code -lt 0 -or $code -gt 255) {\n";
    script += "    [Console]::Error.Write(('qtc: the application terminated with status "
              "0x{0:X8}.' -f $code))\n";
    script += "    exit 1\n";
    script += "}\n";
    script += "exit $code\n";

    if (const Result<qint64> res = scriptPath.writeFileContents(script.toUtf8()); !res) {
        qCWarning(windowsDeviceLog) << "Failed to write interactive-run script"
                                    << scriptPath.toUserOutput() << ":" << res.error();
        dir.removeRecursively();
        return {};
    }

    m_runTempDir = dir;
    return "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + self + "\"";
}

// WindowsDeviceAccess

class WindowsDeviceAccess final : public DeviceFileAccess
{
public:
    explicit WindowsDeviceAccess(const SshParameters &ssh) : m_ssh(ssh) {}

    // Public wrapper around the (protected) deviceEnvironment(), so the CmdBridge deploy can query
    // the environment via a throwaway access without the device exposing this slow access publicly.
    Result<Environment> queryEnvironment() const { return deviceEnvironment(); }

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

// Confirms the remote host is Windows with PowerShell.
static Result<> probeWindows(const SshParameters &ssh)
{
    const Result<RunResult> res = runPowerShell(
        ssh, "[Console]::Out.Write('QTCWIN:' + [Environment]::OSVersion.Platform)");
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
}

static Result<OsArch> detectWindowsArchitecture(const Environment &env)
{
    const QString archStr = env.value("PROCESSOR_ARCHITECTURE").toUpper();
    if (archStr == "AMD64")
        return OsArchAMD64;
    if (archStr == "ARM64")
        return OsArchArm64;
    if (archStr == "X86")
        return OsArchX86;
    return ResultError(Tr::tr("Unsupported remote architecture \"%1\".").arg(archStr));
}

// Tries to bring up the fast Go CmdBridge on the device: copy the matching cmdbridge.exe to
// the device's temp directory (via sftp) and start it. Returns the bridge file access on
// success. Runs on a worker thread. (A base64-over-PowerShell-stdin push is far too slow for
// a multi-MB binary, so we use sftp, which ships with Windows OpenSSH.)
static Result<DeviceFileAccessPtr> deployCmdBridge(const SshParameters &ssh,
                                                   const FilePath &rootPath,
                                                   const std::function<void()> &errorExitHandler)
{
    if (qtcEnvironmentVariableIsSet("QTC_DISABLE_CMDBRIDGE"))
        return ResultError(QString("CmdBridge disabled via QTC_DISABLE_CMDBRIDGE."));

    // Query the device environment through a local (per-command) access rather than the device's
    // own file access, so setupFileAccess() need not expose that slow access publicly while the
    // device is still being probed - see the note there.
    const Result<Environment> envResult = WindowsDeviceAccess(ssh).queryEnvironment();
    if (!envResult)
        return ResultError(envResult.error());
    const Environment env = *envResult;

    const Result<OsArch> arch = detectWindowsArchitecture(env);
    if (!arch)
        return ResultError(arch.error());

    const Result<FilePath> localBridge = CmdBridge::Client::getCmdBridgePath(
        OsTypeWindows, *arch, Core::ICore::libexecPath());
    if (!localBridge)
        return ResultError(localBridge.error());

    QString tempDir = env.value("TEMP");
    if (tempDir.isEmpty())
        tempDir = env.value("TMP");
    if (tempDir.isEmpty())
        tempDir = "C:/Windows/Temp";
    tempDir.replace('\\', '/');

    const FilePath remoteBridge = rootPath.withNewPath(
        tempDir + "/qtc-cmdbridge-" + QUuid::createUuid().toString(QUuid::Id128) + ".exe");

    // Transfer the binary via sftp. Windows OpenSSH sftp wants an absolute remote path with a
    // leading slash before the drive letter, e.g. "/C:/Users/.../x.exe".
    const FilePath sftpBinary = sshSettings().sftpFilePath();
    if (sftpBinary.isEmpty())
        return ResultError(Tr::tr("No sftp client is configured."));

    CommandLine sftpCmd{sftpBinary};
    sftpCmd.addArgs(ssh.connectionOptions(sftpBinary));
    sftpCmd.addArgs({"-b", "-"}); // read the batch of commands from stdin
    sftpCmd.addArg(ssh.host());

    Process sftp;
    SshParameters::setupSshEnvironment(&sftp);
    sftp.setCommand(sftpCmd);
    sftp.setWriteData(QString("put \"%1\" \"/%2\"\n")
                          .arg(localBridge->path(), remoteBridge.path()).toUtf8());
    qCDebug(windowsDeviceLog) << "Deploying CmdBridge via sftp to" << remoteBridge.toUserOutput();
    sftp.runBlocking(std::chrono::seconds(60));
    if (sftp.result() != ProcessResult::FinishedWithSuccess) {
        return ResultError(Tr::tr("Failed to transfer the CmdBridge: %1")
                               .arg(sftp.exitMessage(Process::FailureMessageFormat::WithStdErr)));
    }

    auto fileAccess = std::make_shared<CmdBridge::FileAccess>(errorExitHandler);
    // deleteOnExit is false: the bridge cannot delete its own running .exe on Windows.
    const Result<> initResult = fileAccess->init(remoteBridge, env, /*deleteOnExit=*/false);
    if (!initResult)
        return ResultError(initResult.error());

    qCDebug(windowsDeviceLog) << "CmdBridge started on" << rootPath.toUserOutput();
    return DeviceFileAccessPtr(fileAccess);
}

// WindowsDevicePrivate

class WindowsDevicePrivate
{
public:
    explicit WindowsDevicePrivate(WindowsDevice *parent) : q(parent) {}

    void setupFileAccess(const Continuation<> &cont);

    WindowsDevice *q = nullptr;
    QMutex m_systemDriveMutex;
    std::optional<QString> m_systemDrive;
};

void WindowsDevicePrivate::setupFileAccess(const Continuation<> &cont)
{
    QTC_ASSERT(QThread::isMainThread(),
               cont(ResultError(ResultAssert, "setupFileAccess called from wrong thread"));
               return);

    q->setIsTesting(true);
    // Do NOT expose a file access yet: the device is not confirmed reachable. Setting the slow
    // per-command access here would make an unreachable device look connected, so startup
    // validation (toolchains, Qt, kits) would hang on repeated 10s SSH timeouts. The public file
    // access is set only once probing/deploy on the worker thread below has finished (in the
    // continuation), so a down device stays without access and is skipped quickly.

    const SshParameters ssh = q->sshParameters();
    const FilePath rootPath = q->rootPath();
    const auto onBridgeExit = [id = q->id()] {
        QMetaObject::invokeMethod(DeviceManager::instance(), [id] {
            DeviceManager::setDeviceState(id, IDevice::DeviceDisconnected);
        });
    };

    qCDebug(windowsDeviceLog) << "setupFileAccess: probing" << rootPath.toUserOutput();
    QFuture<Result<DeviceFileAccessPtr>> future = Utils::asyncRun(
        [ssh, rootPath, onBridgeExit]() -> Result<DeviceFileAccessPtr> {
            if (const Result<> res = probeWindows(ssh); !res)
                return ResultError(res.error());
            // Prefer the fast CmdBridge; a null result means "keep the slow access".
            const Result<DeviceFileAccessPtr> bridge = deployCmdBridge(ssh, rootPath, onBridgeExit);
            if (bridge)
                return bridge;
            qCDebug(windowsDeviceLog) << "CmdBridge unavailable, using slow access:"
                                      << bridge.error();
            return DeviceFileAccessPtr();
        });
    future.then(q, [this, cont, ssh](const Result<DeviceFileAccessPtr> &res) {
        q->setIsTesting(false);
        if (!res) {
            q->setFileAccess(nullptr);
            q->setDeviceState(IDevice::DeviceDisconnected);
            cont(ResultError(res.error()));
        } else if (*res) {
            q->setFileAccess(*res);
            q->setDeviceState(IDevice::DeviceReadyToUse);
            cont(ResultOk);
        } else {
            q->setFileAccess(std::make_shared<WindowsDeviceAccess>(ssh));
            q->setDeviceState(IDevice::DeviceConnected);
            cont(ResultOk);
        }
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
    offerKitCreation();

    autoConnectOnStartup.setSettingsKey("AutoConnectOnStartup");
    autoConnectOnStartup.setDefaultValue(true);
    autoConnectOnStartup.setLabelText(Tr::tr("Auto-connect on startup"));
    autoConnectOnStartup.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

    cdbExtensionDirectory.setSettingsKey("CdbExtensionDirectory");
    cdbExtensionDirectory.setExpectedKind(PathChooserKind::Directory);
    cdbExtensionDirectory.setAllowPathFromDevice(true);
    cdbExtensionDirectory.setBaseDirectory(rootPath());
    cdbExtensionDirectory.setLabelText(Tr::tr("CDB extension directory:"));
    cdbExtensionDirectory.setToolTip(
        Tr::tr("Device directory holding the qtcreatorcdbext subdirectories, that is, the \"lib\" "
               "directory of a Qt Creator installed on the device. Required to debug the device's "
               "binaries with CDB."));

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

Result<OsArch> WindowsDevice::osArch() const
{
    const DeviceFileAccessPtr access = fileAccess();
    if (!access)
        return ResultError(Tr::tr("No file access for device \"%1\".").arg(displayName()));
    const Result<Environment> env = access->deviceEnvironment();
    return env ? detectWindowsArchitecture(*env) : ResultError(env.error());
}

FilePath WindowsDevice::rootPath() const
{
    QMutexLocker locker(&d->m_systemDriveMutex);
    if (!d->m_systemDrive) {
        if (const Result<Environment> env = systemEnvironmentWithError()) {
            d->m_systemDrive = env->value_or("SystemDrive", "C:/");
            if (!d->m_systemDrive->endsWith('/'))
                d->m_systemDrive->append('/');
        }
    }
    return FilePath::fromParts(u"ssh", userAtHostAndPort(), d->m_systemDrive.value_or("C:/"));
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
    const DeviceState state = deviceState();
    if (state == DeviceReadyToUse || state == DeviceConnected)
        cont(ResultOk);
    else
        d->setupFileAccess(cont);
}

void WindowsDevice::postLoad()
{
    // Connect on startup, as LinuxDevice does, so the device has file access from the start and
    // shows up in the device-aware file dialogs (and anywhere else that lists browsable devices)
    // without the user first opening its settings. setupFileAccess() installs the bootstrap file
    // access synchronously, so hasFileAccess() is true right away.
    if (!autoConnectOnStartup())
        return;

    tryToConnect({this, [this](const Result<> &res) {
        if (res)
            return;
        // Do not keep retrying a device that is not reachable; turn auto-connect off and tell
        // the user (they can re-enable it in the device settings).
        const QString message = Tr::tr("Auto-connection to device \"%1\" failed. "
                                       "Switching auto-connection off.").arg(displayName());
        qCWarning(windowsDeviceLog).noquote() << message << res.error();
        autoConnectOnStartup.setValue(false);
        QTC_ASSERT(QThread::isMainThread(), return);
        InfoBarEntry info(id().withPrefix("announce_"), message);
        info.setTitle(Tr::tr("Establishing a Connection"));
        info.setInfoType(InfoLabelType::Warning);
        Core::ICore::popupInfoBar()->addInfo(info);
        Core::MessageManager::writeSilently(message);
    }});
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

// Locates cdb.exe (from the Windows SDK "Debugging Tools for Windows") on the device, preferring
// the architecture matching the device but falling back to the others. Returns an empty path when
// no cdb.exe is found.
static FilePath findDeviceCdb(const WindowsDevice::Ptr &device)
{
    QStringList archDirs;
    if (const Result<OsArch> arch = device->osArch()) {
        switch (*arch) {
        case OsArchArm64: archDirs << "arm64"; break;
        case OsArchX86:   archDirs << "x86"; break;
        default:          archDirs << "x64"; break;
        }
    }
    for (const QString &fallback : {QString("x64"), QString("arm64"), QString("x86")}) {
        if (!archDirs.contains(fallback))
            archDirs << fallback;
    }
    const QStringList kitRoots = {
        "C:/Program Files (x86)/Windows Kits/10/Debuggers",
        "C:/Program Files/Windows Kits/10/Debuggers",
    };
    for (const QString &root : kitRoots) {
        for (const QString &arch : std::as_const(archDirs)) {
            const QString path = root + QLatin1Char('/') + arch + QLatin1String("/cdb.exe");
            const FilePath cdb = device->rootPath().withNewPath(path);
            if (cdb.isExecutableFile())
                return cdb;
        }
    }
    return {};
}

// Registers a CDB DebuggerItem for the device's cdb.exe (reused across kits and re-runs), so the
// generic kit creation's debugger kit aspect attaches it to the device's MSVC kits. The aspect
// selects a debugger by ABI (DebuggerItem::matchTarget), so the item is given the device's Windows
// ABI; cdb.exe debugs any MSVC target, so only the architecture and word width need to match.
// Does nothing when no cdb.exe is present on the device or it is already registered.
static void registerDeviceCdb(const WindowsDevice::Ptr &device,
                              const ProjectExplorer::ToolDetectionLogger &logger)
{
    const FilePath cdb = findDeviceCdb(device);
    if (cdb.isEmpty())
        return;
    if (Debugger::DebuggerItemManager::findByCommand(cdb).isValid())
        return;

    Abi::Architecture arch = Abi::X86Architecture;
    unsigned char width = 64;
    if (const Result<OsArch> osArch = device->osArch()) {
        switch (*osArch) {
        case OsArchArm64: arch = Abi::ArmArchitecture; width = 64; break;
        case OsArchX86:   arch = Abi::X86Architecture; width = 32; break;
        default:          break;
        }
    }
    // The MSVC flavor is not otherwise significant to the debugger ABI match; any non-MinGW
    // Windows flavor matches the detected MSVC kits.
    const Abi abi(arch, Abi::WindowsOS, Abi::WindowsMsvc2022Flavor, Abi::PEFormat, width);

    Debugger::DebuggerItem item;
    item.setCommand(cdb);
    item.setEngineType(Debugger::CdbEngineType);
    item.setAbis({abi});
    item.setUnexpandedDisplayName(Tr::tr("CDB for %1").arg(device->displayName()));
    item.setDetectionSource({DetectionSource::FromSystem, device->id().toString()});
    Debugger::DebuggerItemManager::registerDebugger(item);
    if (logger)
        logger.logTopLevel(Tr::tr("Registered CDB debugger \"%1\".").arg(cdb.toUserOutput()));
}

WindowsDeviceConfigurationWidget::WindowsDeviceConfigurationWidget(const IDevicePtr &device)
    : IDeviceWidget(device)
{
    auto windowsDevice = std::dynamic_pointer_cast<WindowsDevice>(device);
    QTC_ASSERT(windowsDevice, return);

    auto createKeyButton = new QPushButton(Tr::tr("Create New..."));
    SshParametersAspectContainer &ssh = device->sshParametersAspectContainer();

    using namespace Layouting;
    // clang-format off
    Form {
        ssh.host, ssh.port, ssh.hostKeyCheckingMode, st, br,
        ssh.timeout, st, br,
        ssh.userName, st, br,
        ssh.useKeyFile, st, br,
        ssh.privateKeyFile, createKeyButton, br,
        windowsDevice->autoConnectOnStartup, br,
        windowsDevice->cdbExtensionDirectory, br,
        device->deviceToolsGui(),
        device->autoDetectGui(),
    }.attachTo(this);
    // clang-format on

    connect(createKeyButton, &QAbstractButton::clicked,
            this, &WindowsDeviceConfigurationWidget::createNewKey);
    connect(device.get(), &AspectContainer::volatileValueChanged, this, &checkSettingsDirty);
    connect(&ssh, &AspectContainer::volatileValueChanged, this, &checkSettingsDirty);
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

void WindowsDevice::runAutoDetect(
    const ProjectExplorer::ToolDetectionLogger &logger, const std::function<void()> &onDone)
{
    if (logger)
        logger.logTopLevel(Tr::tr("Connecting..."));
    const std::weak_ptr<IDevice> weakSelf = shared_from_this();
    tryToConnect(Continuation<>([weakSelf, logger, onDone](const Result<> &res) {
        const IDevice::Ptr self = weakSelf.lock();
        if (!self)
            return;
        if (!res) {
            if (logger)
                logger.logTopLevel(Tr::tr("Connection failed: %1").arg(res.error()));
            onDone();
            return;
        }
        // Register the device's CDB (if present) before kit creation so the debugger kit aspect
        // attaches it: a debugger on the same device as the kit's build device is picked up
        // automatically. Remote CDB is not covered by the generic debugger detection.
        registerDeviceCdb(std::static_pointer_cast<WindowsDevice>(self), logger);
        self->requestToolDetection(self->toolSearchPaths(), logger);
        GlobalTaskTree::start(self->autoDetectDeviceToolsRecipe(logger), {}, onDone);
    }));
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
    setCombinedIcon(":/remotelinux/images/windowsdevicesmall.png",
                    ":/remotelinux/images/windowsdevice.png");
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
