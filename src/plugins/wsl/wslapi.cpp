// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wslapi.h"

#include "wsltr.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/osspecificaspects.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Wsl::Internal {

static const char16_t wslLocalhostHost[] = u"wsl.localhost";
static const char16_t wslDollarHost[] = u"wsl$";

FilePath wslExecutable()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    static const FilePath wsl = [] {
        const FilePath found = Environment::systemEnvironment().searchInPath("wsl.exe");
        if (!found.isEmpty())
            return found;
        const QString systemRoot = qtcEnvironmentVariable("SystemRoot", "C:/Windows");
        const FilePath fallback
            = FilePath::fromUserInput(systemRoot + "/System32/wsl.exe");
        return fallback.exists() ? fallback : FilePath();
    }();
    return wsl;
}

// The mark is not whitespace, so it survives the trimming every caller does.
// It goes here, where every answer passes, rather than in each of them.
static QString withoutByteOrderMark(const QString &decoded)
{
    return decoded.startsWith(QChar(0xFEFF)) ? decoded.mid(1) : decoded;
}

QString decodeWslOutput(const QByteArray &output)
{
    // Either mark is enough on its own: a zero byte cannot occur in UTF-8,
    // and the byte order mark carries none, which an answer consisting of
    // nothing but the mark is made up of entirely.
    if (!output.startsWith("\xFF\xFE") && !output.contains('\0'))
        return withoutByteOrderMark(QString::fromUtf8(output));

    // Decoded a code unit at a time rather than through a cast, which would
    // read the bytes at an alignment they were never written at.
    QString result;
    result.reserve(output.size() / 2);
    for (int i = 0; i + 1 < output.size(); i += 2) {
        const char16_t unit = char16_t(uchar(output.at(i)))
                              | (char16_t(uchar(output.at(i + 1))) << 8);
        result.append(QChar(unit));
    }
    return withoutByteOrderMark(result);
}

QStringList parseDistributionList(const QString &output)
{
    QStringList result;
    for (const QString &line : output.split('\n')) {
        const QString name = line.trimmed();
        if (!name.isEmpty())
            result.append(name);
    }
    return result;
}

// The WSL service itself answers right away. A command sent into a
// distribution has to wait for it to be there: a stopped one is started
// first, and a cold start is a virtual machine boot.
static constexpr std::chrono::seconds serviceTimeout{10};
static constexpr std::chrono::seconds distributionTimeout{120};

// What wsl.exe wrote about a failure, decoded the way its answers are:
// which channel it uses depends on the version and on what went wrong, so
// both are taken, and neither is UTF-8 on the builds that ignore WSL_UTF8.
static QString decodedWslFailureOutput(const Process &process)
{
    const auto decoded = [](const QByteArray &raw) {
        return decodeWslOutput(raw).replace("\r\n", "\n").trimmed();
    };

    const QString stdErr = decoded(process.rawStdErr());
    const QString stdOut = decoded(process.rawStdOut());
    if (!stdErr.isEmpty() && !stdOut.isEmpty())
        return stdErr + '\n' + stdOut;
    return stdErr.isEmpty() ? stdOut : stdErr;
}

static Result<QString> runWsl(const QStringList &arguments,
                              std::chrono::seconds timeout = serviceTimeout)
{
    const FilePath wsl = wslExecutable();
    if (wsl.isEmpty())
        return ResultError(Tr::tr("No wsl.exe was found on this host."));

    Environment env = Environment::systemEnvironment();
    // Asks for UTF-8 instead of the UTF-16LE wsl.exe writes by default.
    // Builds that do not know the variable ignore it, which decodeWslOutput()
    // covers.
    env.set("WSL_UTF8", "1");

    Process process;
    process.setCommand({wsl, arguments});
    process.setEnvironment(env);
    process.runBlocking(timeout);

    if (process.result() != ProcessResult::FinishedWithSuccess) {
        // exitMessage() and not verboseExitMessage(): that one appends what
        // wsl.exe wrote as UTF-8, which it is not.
        QString message = Tr::tr("Running \"%1\" failed: %2")
                              .arg(process.commandLine().toUserOutput(), process.exitMessage());
        const QString output = decodedWslFailureOutput(process);
        if (!output.isEmpty())
            message += '\n' + output;
        return ResultError(message);
    }

    return decodeWslOutput(process.rawStdOut());
}

Result<QStringList> installedDistributions()
{
    const Result<QString> output = runWsl({"--list", "--quiet"});
    if (!output)
        return ResultError(output.error());
    return parseDistributionList(*output);
}

Result<QStringList> runningDistributions()
{
    const Result<QString> output = runWsl({"--list", "--quiet", "--running"});
    if (!output)
        return ResultError(output.error());
    return parseDistributionList(*output);
}

QString normalizeMountRoot(const QString &mountRoot)
{
    QString normalized = mountRoot;
    // A root of "/" is a separator and nothing else, so the strip stops
    // short of eating it.
    while (normalized.size() > 1 && normalized.endsWith('/'))
        normalized.chop(1);
    return normalized;
}

// The automount root with exactly one separator after it, which a root of "/"
// already ends in.
static QString mountRootPrefix(const QString &mountRoot)
{
    const QString normalized = normalizeMountRoot(mountRoot);
    return normalized.endsWith('/') ? normalized : normalized + '/';
}

QString parseMountRoot(const QString &wslConf)
{
    QString section;
    for (const QString &line : wslConf.split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';'))
            continue;

        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            section = trimmed.mid(1, trimmed.size() - 2).trimmed().toLower();
            continue;
        }

        if (section != "automount")
            continue;

        const int equals = trimmed.indexOf('=');
        if (equals < 0)
            continue;
        if (trimmed.left(equals).trimmed().toLower() != "root")
            continue;

        QString value = trimmed.mid(equals + 1).trimmed();
        if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"'))
            value = value.mid(1, value.size() - 2);
        value = normalizeMountRoot(value);
        if (!value.isEmpty())
            return value;
    }

    return defaultMountRoot;
}

Environment parseEnvironment(const QString &envOutput)
{
    Environment environment{OsTypeLinux};

    QString key;
    QString value;
    const auto flush = [&environment, &key, &value] {
        if (!key.isEmpty())
            environment.set(key, value);
    };

    // "env" ends the last entry with a newline, and the empty tail that
    // leaves behind is not a continuation of it.
    QString output = envOutput;
    if (output.endsWith('\n'))
        output.chop(1);
    if (output.endsWith('\r'))
        output.chop(1);

    for (const QString &line : output.split('\n')) {
        const QString entry = line.endsWith('\r') ? line.chopped(1) : line;
        const int equals = entry.indexOf('=');
        // A name cannot contain "=", so a line without one before any name
        // character belongs to the value above it.
        if (equals <= 0) {
            if (!key.isEmpty())
                value += '\n' + entry;
            continue;
        }
        flush();
        key = entry.left(equals);
        value = entry.mid(equals + 1);
    }
    flush();

    return environment;
}

// Runs one command in the distribution through wsl.exe. Deliberately not
// through the device: this is what setting the device up needs, and a command
// run on the device asks it for the file access that is being set up.
static Result<QString> runInDistribution(
    const QString &distribution, const QString &userName, const QStringList &command)
{
    QStringList arguments{"--distribution", distribution};
    if (!userName.isEmpty())
        arguments << "--user" << userName;
    arguments << "--exec";
    arguments << command;
    return runWsl(arguments, distributionTimeout);
}

// Through /bin/sh rather than a full path to the program: /usr/bin is where
// a distribution with usrmerge keeps them, and Alpine and the older Debian
// images keep "uname" and "env" in /bin.
static Result<QString> runShellInDistribution(
    const QString &distribution, const QString &userName, const QString &script)
{
    return runInDistribution(distribution, userName, {"/bin/sh", "-c", script});
}

Result<QString> detectMountRoot(const QString &distribution)
{
    // "cat" exits nonzero for a file that is not there, so the fallback is
    // what makes a distribution without /etc/wsl.conf the default rather than
    // an error, and what is left really is one.
    const Result<QString> output
        = runShellInDistribution(distribution, {}, "cat /etc/wsl.conf 2>/dev/null || true");
    if (!output)
        return ResultError(output.error());
    return parseMountRoot(*output);
}

Result<OsArch> detectArchitecture(const QString &distribution)
{
    const Result<QString> output = runShellInDistribution(distribution, {}, "uname -m");
    if (!output)
        return ResultError(output.error());
    return osArchFromString(output->trimmed());
}

Result<Environment> detectEnvironment(const QString &distribution, const QString &userName)
{
    const Result<QString> output = runShellInDistribution(distribution, userName, "exec env");
    if (!output)
        return ResultError(output.error());
    return parseEnvironment(*output);
}

static bool isWslShare(const FilePath &path)
{
    if (path.scheme() != u"unc")
        return false;
    const QString host = path.host().toString().toLower();
    return host == QString::fromUtf16(wslLocalhostHost)
           || host == QString::fromUtf16(wslDollarHost);
}

// The path inside the distribution a decomposed share path names: the share
// is "//wsl.localhost/<distribution>/<path>", and decomposing it leaves the
// distribution as the first component of the path. Empty when the path names
// another distribution.
static QString pathInDistributionShare(const QString &distribution, const QString &path)
{
    const QString prefix = '/' + distribution;
    if (path.compare(prefix, Qt::CaseInsensitive) == 0)
        return "/";
    if (path.size() > prefix.size() && path.at(prefix.size()) == '/'
        && path.left(prefix.size()).compare(prefix, Qt::CaseInsensitive) == 0) {
        return path.mid(prefix.size());
    }
    return {};
}

// The part of an automounted path that follows the drive letter, and nothing
// when the path names no automounted drive. Exactly one letter names a drive:
// "/mnt/c" and "/mnt/c/src", but not "/mnt/cdrom", which is an ordinary
// directory that happens to live under the automount root. A root of "/" puts
// every path under itself, so the drive is what tells them apart there.
static std::optional<QString> automountedTail(const QString &mountRoot, const QString &path)
{
    const QString prefix = mountRootPrefix(mountRoot);
    if (!path.startsWith(prefix))
        return std::nullopt;

    const QString rest = path.mid(prefix.size());
    if (rest.isEmpty() || !rest.at(0).isLetter() || (rest.size() > 1 && rest.at(1) != '/'))
        return std::nullopt;

    return rest;
}

Environment withoutWindowsPath(const Environment &environment, const QString &mountRoot)
{
    QStringList kept;
    const QStringList entries = environment.value("PATH").split(':', Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        if (!automountedTail(mountRoot, entry))
            kept.append(entry);
    }

    Environment result = environment;
    result.set("PATH", kept.join(':'));
    return result;
}

QString hostDrivePathToWslPath(const QString &mountRoot, const QString &hostPath)
{
    if (hostPath.size() < 2 || !hostPath.at(0).isLetter() || hostPath.at(1) != ':')
        return hostPath;

    const QString rest = hostPath.mid(2);
    return mountRootPrefix(mountRoot) + hostPath.at(0).toLower() + rest;
}

QString mappedHostPathToWslPath(const QString &mountRoot,
                                const QString &distribution,
                                const QString &hostPath)
{
    // A path in the distribution that opens with a directory named after the
    // distribution is indistinguishable from its own share, and loses: that
    // share is what localSource() hands out for every path outside the
    // automount root, so it is the one that turns up here.
    const QString inShare = pathInDistributionShare(distribution, hostPath);
    if (!inShare.isEmpty())
        return inShare;

    return hostDrivePathToWslPath(mountRoot, hostPath);
}

QString hostPathToWslPath(const QString &mountRoot,
                          const QString &distribution,
                          const FilePath &hostPath)
{
    if (isWslShare(hostPath)) {
        // The server is in the host part, so what is left of the share is the
        // distribution and the path in it.
        return pathInDistributionShare(distribution, hostPath.path());
    }

    if (!hostPath.isLocal())
        return {};

    const QString path = hostPath.path();
    if (path.size() < 2 || !path.at(0).isLetter() || path.at(1) != ':')
        return {};

    return hostDrivePathToWslPath(mountRoot, path);
}

QString wslPathToHostPath(const QString &mountRoot,
                          const QString &distribution,
                          const QString &wslPath)
{
    if (const std::optional<QString> rest = automountedTail(mountRoot, wslPath)) {
        const QString drive = QString(rest->at(0).toUpper()) + ':';
        const QString tail = rest->mid(1);
        return drive + (tail.isEmpty() ? QString('/') : tail);
    }

    return "//" + QString::fromUtf16(wslLocalhostHost) + '/' + distribution + wslPath;
}

// What CommandLine::addCommandLineAsArgs(Raw) does, except that the
// executable is quoted for the distribution rather than for the host: the
// shell that parses it is the distribution's, and the arguments already are.
static void addLinuxCommandLineAsArgs(CommandLine *cmd, const CommandLine &inner)
{
    cmd->addArg(inner.executable().path(), OsTypeLinux);
    cmd->addArgs(inner.arguments(), CommandLine::Raw);
}

CommandLine wslCommandLine(
    const FilePath &wslExecutable,
    const QString &distribution,
    const QString &userName,
    const QString &workingDirectory,
    const CommandLine &command,
    const std::optional<Environment> &environment,
    const QString &markerTemplate)
{
    CommandLine wslCmd{wslExecutable, {"--distribution", distribution}};

    if (!userName.isEmpty())
        wslCmd.addArgs({"--user", userName});

    if (!workingDirectory.isEmpty())
        wslCmd.addArgs({"--cd", workingDirectory});

    // --exec, not --: with "--" wsl.exe hands the rest to the default shell,
    // which parses the script a second time and expands what the quoting was
    // there to protect.
    wslCmd.addArg("--exec");

    // Around the shell, not around the command inside it: the test below runs
    // in the shell, and has to look the executable up in the PATH the command
    // is then run with. By name, not by path, so that wsl.exe finds it
    // wherever the distribution keeps it. These are arguments of wsl.exe, so
    // the host quotes them, unlike everything that goes into the script.
    if (environment) {
        wslCmd.addArg("env");
        environment->forEachEntry(
            [&wslCmd, &environment](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    wslCmd.addArg(key + '=' + environment->expandVariables(value));
            });
    }

    wslCmd.addArgs({"/bin/sh", "-c"});

    CommandLine inner("exec");
    addLinuxCommandLineAsArgs(&inner, command);

    if (markerTemplate.isEmpty()) {
        wslCmd.addCommandLineAsSingleArg(inner);
        return wslCmd;
    }

    CommandLine testType({"type", {}});
    testType.addArg(command.executable().path(), OsTypeLinux);
    testType.addArgs(">/dev/null", CommandLine::Raw);

    CommandLine echo("echo");
    echo.addArgs(markerTemplate.arg("$$"), CommandLine::Raw);
    echo.addCommandLineWithAnd(inner);

    testType.addCommandLineWithAnd(echo);

    wslCmd.addCommandLineAsSingleArg(testType);
    return wslCmd;
}

} // namespace Wsl::Internal
