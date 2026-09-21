// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/osspecificaspects.h>
#include <utils/result.h>

#include <QString>
#include <QStringList>

#include <optional>

namespace Wsl::Internal {

// The launcher every WSL device runs its commands through. Empty on a host
// without WSL, which is every host that is not Windows.
Utils::FilePath wslExecutable();

// wsl.exe answers in UTF-16LE unless WSL_UTF8 says otherwise, and builds that
// predate that variable answer in UTF-16LE regardless. The two are told apart
// by the data itself: a UTF-16LE answer opens with a byte order mark, and
// UTF-8 never contains a zero byte.
QString decodeWslOutput(const QByteArray &output);

// One name per line, as "wsl.exe --list --quiet" prints them. The quiet form
// carries no header and is not translated, unlike "--list --verbose".
QStringList parseDistributionList(const QString &output);

// The distributions installed on this host, and those of them that are
// running. A stopped one is started by the first command sent to it, so being
// stopped is not an error.
Utils::Result<QStringList> installedDistributions();
Utils::Result<QStringList> runningDistributions();

// The automount root WSL puts the Windows drives under when /etc/wsl.conf
// does not say.
inline constexpr char defaultMountRoot[] = "/mnt";

// An automount root in the one form the translations below expect: without a
// trailing separator, except for a root of "/", which is nothing but one.
// Leaves an empty root empty, which is how "ask the distribution" is spelled.
QString normalizeMountRoot(const QString &mountRoot);

// The automount root the distribution puts the Windows drives under, as
// /etc/wsl.conf spells it, normalized. defaultMountRoot when the file does
// not say, which is the default WSL itself applies.
QString parseMountRoot(const QString &wslConf);

// The "KEY=VALUE" lines "env" prints. A line carrying no "=" continues the
// value of the one before it, which is how a value with a newline in it
// comes out.
Utils::Environment parseEnvironment(const QString &envOutput);

// What the distribution answers about itself. Each starts it if it is not
// running, and each goes through wsl.exe rather than through the device, so
// that setting the device up can use them.
//
// A distribution without /etc/wsl.conf answers defaultMountRoot; an error
// means it could not be asked at all, which a caller must not remember as
// an answer.
Utils::Result<QString> detectMountRoot(const QString &distribution);
Utils::Result<Utils::OsArch> detectArchitecture(const QString &distribution);
Utils::Result<Utils::Environment> detectEnvironment(
    const QString &distribution, const QString &userName);

// The same environment with the PATH entries that point back at a Windows
// drive removed. WSL appends the host's PATH to the distribution's, so
// without this Qt Creator finds the host's own tools through the automount
// root and offers them as the distribution's.
Utils::Environment withoutWindowsPath(
    const Utils::Environment &environment, const QString &mountRoot);

// The automount path a Windows drive is reached at, "C:/src" -> "/mnt/c/src".
// Leaves a path naming no drive alone, which is what DeviceFileAccess expects
// of a mapping it cannot make.
QString hostDrivePathToWslPath(const QString &mountRoot, const QString &hostPath);

// Where a path on this Windows host shows up inside the distribution: a local
// drive under the automount root, and the distribution's own UNC share back at
// the path it shares. Empty when the distribution cannot see the path.
QString hostPathToWslPath(const QString &mountRoot,
                          const QString &distribution,
                          const Utils::FilePath &hostPath);

// The same for the bare path DeviceFileAccess::mapToDevicePath() is handed:
// FilePath::withNewMappedPath() drops the UNC host before the mapping sees
// it, so a path on the distribution's own share arrives decomposed, as
// "/<distribution>/<path in the distribution>". Leaves a path it can map
// nothing of alone, which is what DeviceFileAccess expects.
QString mappedHostPathToWslPath(const QString &mountRoot,
                                const QString &distribution,
                                const QString &hostPath);

// The way back: a path under the automount root is a Windows drive, anything
// else is only reachable through the UNC share the distribution exports.
QString wslPathToHostPath(const QString &mountRoot,
                          const QString &distribution,
                          const QString &wslPath);

// The wsl.exe command line that runs one command inside a distribution.
//
// An empty userName leaves the distribution its default user, an empty
// workingDirectory leaves wsl.exe its default directory, and an empty
// markerTemplate runs the command without one.
//
// wsl.exe reports nothing about the command it starts, so the command reports
// itself: it echoes the marker with its own pid, but only after its executable
// is found, so that a failed start is told apart from a command that failed.
// exec keeps the pid the shell already announced.
//
// env carries the environment in, having no wsl.exe option to do it with. It
// wraps the shell rather than the command inside it, so that the executable is
// looked for in the PATH the command is then run with, and not in the one the
// distribution starts its shells with.
//
// The command's arguments go in as they are, so they must already be quoted
// for the distribution's shell. A command whose executable is a path on the
// device is, since CommandLine quotes for the OS of its executable.
Utils::CommandLine wslCommandLine(
    const Utils::FilePath &wslExecutable,
    const QString &distribution,
    const QString &userName,
    const QString &workingDirectory,
    const Utils::CommandLine &command,
    const std::optional<Utils::Environment> &environment,
    const QString &markerTemplate);

} // namespace Wsl::Internal
