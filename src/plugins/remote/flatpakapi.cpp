// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flatpakapi.h"

#include "remotelinuxtr.h"
#include "shellcommand.h"

#include <utils/qtcprocess.h>

#include <QStringList>

using namespace Utils;

namespace Remote::Internal {

bool isInsideFlatpakSandbox()
{
    static const bool inside = FilePath::fromString("/.flatpak-info").exists();
    return inside;
}

// The host's operating system directories, which Flatpak shows below
// /run/host because the runtime occupies their places in the sandbox.
static const QStringList &hostOsDirectories()
{
    static const QStringList dirs{"bin", "etc", "lib", "lib32", "lib64", "sbin", "usr"};
    return dirs;
}

// Directories of the same name exist on both sides of the sandbox and hold
// different things. The sandbox does not see the host's at all.
static const QStringList &sandboxPrivateDirectories()
{
    static const QStringList dirs{"app", "boot", "efi", "proc", "root", "run", "tmp", "var"};
    return dirs;
}

static QString topDirectory(const QString &path)
{
    const int end = path.indexOf('/', 1);
    return end < 0 ? path.mid(1) : path.mid(1, end - 1);
}

static bool isBelow(const QString &path, const QString &dir)
{
    return path == dir || path.startsWith(dir + '/');
}

// Fedora Atomic and its relatives keep the home directories in /var/home,
// and Flatpak shares /var/home and removable media in /run/media at the same path.
static bool isSharedBelowPrivate(const QString &path)
{
    return isBelow(path, "/var/home") || isBelow(path, "/run/media");
}

static bool isPrivateInSandbox(const QString &path)
{
    return sandboxPrivateDirectories().contains(topDirectory(path)) && !isSharedBelowPrivate(path);
}

QString hostPathToSandboxPath(const QString &hostPath)
{
    if (!hostPath.startsWith('/') || hostPath == "/")
        return {};

    if (hostOsDirectories().contains(topDirectory(hostPath)))
        return "/run/host" + hostPath;

    if (isPrivateInSandbox(hostPath))
        return {};

    return hostPath;
}

QString sandboxPathToHostPath(const QString &sandboxPath)
{
    if (!sandboxPath.startsWith('/') || sandboxPath == "/")
        return {};

    static const QString runHost = "/run/host";
    if (sandboxPath.startsWith(runHost + '/')) {
        const QString onHost = sandboxPath.mid(runHost.size());
        return hostOsDirectories().contains(topDirectory(onHost)) ? onHost : QString();
    }

    if (hostOsDirectories().contains(topDirectory(sandboxPath)) || isPrivateInSandbox(sandboxPath))
        return {};

    return sandboxPath;
}

CommandLine hostCommandLine(
    const FilePath &launcher,
    const QString &workingDirectory,
    const CommandLine &command,
    const std::optional<Environment> &environment,
    const QString &markerTemplate,
    const QStringList &unsetNames)
{
    CommandLine hostCmd{launcher, {"--host"}};

    if (!workingDirectory.isEmpty())
        hostCmd.addArg("--directory=" + workingDirectory);

    addShellInvocation(&hostCmd, command, environment, markerTemplate, OsTypeLinux, unsetNames);
    return hostCmd;
}

// Not through the device: a command run on the device asks it for the file
// access that is being set up.
static Result<QString> runShellOnHost(const FilePath &launcher, const QString &script)
{
    Process process;
    process.setCommand({launcher, {"--host", "/bin/sh", "-c", script}});
    process.runBlocking(std::chrono::seconds(10));

    if (process.result() != ProcessResult::FinishedWithSuccess) {
        return ResultError(Tr::tr("Running \"%1\" failed: %2")
                               .arg(process.commandLine().toUserOutput(),
                                    process.verboseExitMessage()));
    }

    return process.cleanedStdOut();
}

Result<Environment> detectHostEnvironment(const FilePath &launcher)
{
    const Result<QString> output = runShellOnHost(launcher, "exec env -0");
    if (!output)
        return ResultError(output.error());
    return Environment(output->split(QChar(0), Qt::SkipEmptyParts), OsTypeLinux);
}

} // namespace Remote::Internal
