// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "shellmodel.h"

#include "terminaltr.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QStandardPaths>

namespace Terminal::Internal {

using namespace Utils;

/*!
    Returns the command line as the user entered it, with macros unexpanded.
*/
CommandLine ShellProfile::unexpandedCommandLine() const
{
    return CommandLine{executable, arguments, CommandLine::Raw};
}

/*!
    Returns the command line to start the shell with.
*/
CommandLine ShellProfile::commandLine() const
{
    const MacroExpander *expander = globalMacroExpander();
    return CommandLine{expander->expand(executable), expander->expand(arguments), CommandLine::Raw};
}

FilePath ShellProfile::expandedWorkingDirectory() const
{
    return globalMacroExpander()->expand(workingDirectory);
}

/*!
    Adds the environment changes of the profile to \a environment.
*/
void ShellProfile::applyEnvironmentChanges(std::optional<Environment> &environment) const
{
    if (!environmentChanges.hasData())
        return;
    Environment result = environment.value_or(Environment{});
    environmentChanges.modifyEnvironment(result, globalMacroExpander());
    environment = result;
}

QIcon ShellProfile::icon() const
{
    QFileIconProvider iconProvider;
    return iconProvider.icon((iconPath.isEmpty() ? executable : iconPath).toFileInfo());
}

Utils::Terminal::OpenTerminalParameters ShellProfile::openParameters() const
{
    Utils::Terminal::OpenTerminalParameters parameters{commandLine()};
    applyEnvironmentChanges(parameters.environment);
    if (!workingDirectory.isEmpty())
        parameters.workingDirectory = expandedWorkingDirectory();
    parameters.icon = icon();
    return parameters;
}

QString systemShellProfileId()
{
    return QString("Detected:System");
}

static ShellProfile detectedProfile(const FilePath &executable,
                                    const QStringList &arguments = {},
                                    const QString &name = {},
                                    const FilePath &iconPath = {})
{
    ShellProfile profile;
    profile.executable = executable;
    profile.arguments = ProcessArgs::joinArgs(arguments, HostOsInfo::hostOs());
    profile.name = name.isEmpty() ? profile.unexpandedCommandLine().toUserOutput() : name;
    profile.iconPath = iconPath;
    profile.id = "Detected:" + profile.unexpandedCommandLine().toUserOutput();
    profile.isDetected = true;
    return profile;
}

/*!
    Returns the profile of the shell the user logs in with. Unlike
    detectedShellProfiles(), this does not look for the installed shells.
*/
ShellProfile systemShellProfile()
{
    ShellProfile profile;
    if (HostOsInfo::isWindowsHost()) {
        profile.executable = FilePath::fromUserInput(qtcEnvironmentVariable("COMSPEC"));
    } else {
        profile.executable = FilePath::fromUserInput(qtcEnvironmentVariable("SHELL"));
        if (!profile.executable.isExecutableFile())
            profile.executable = Environment::systemEnvironment().searchInPath("sh");
        profile.arguments = "-l";
    }
    profile.name = Tr::tr("System Shell");
    profile.id = systemShellProfileId();
    profile.isDetected = true;
    return profile;
}

static QSet<FilePath> msysPaths()
{
    QSet<FilePath> res;
    FilePath msys2 = FilePath::fromUserInput(QStandardPaths::findExecutable("msys2.exe"));
    if (msys2.exists())
        res.insert(msys2.parentDir());
    for (const QFileInfo &drive : QDir::drives()) {
        for (const QString &name : QStringList{"msys2", "msys32", "msys64"}) {
            msys2 = FilePath::fromString(drive.filePath()).pathAppended(name);
            if (msys2.pathAppended("msys2.exe").exists())
                res.insert(msys2);
        }
    }
    return res;
}

static QList<ShellProfile> detectInstalledShells()
{
    QList<ShellProfile> shells;
    if (HostOsInfo::isWindowsHost()) {
        const FilePath comspec = FilePath::fromUserInput(qtcEnvironmentVariable("COMSPEC"));
        shells << detectedProfile(comspec);

        if (comspec.fileName() != "cmd.exe") {
            FilePath cmd = FilePath::fromUserInput(QStandardPaths::findExecutable("cmd.exe"));
            shells << detectedProfile(cmd);
        }

        const FilePath powershell = FilePath::fromUserInput(
            QStandardPaths::findExecutable("powershell.exe"));

        if (powershell.exists())
            shells << detectedProfile(powershell);

        const FilePath sys_bash =
                FilePath::fromUserInput(QStandardPaths::findExecutable("bash.exe"));
        if (sys_bash.exists())
            shells << detectedProfile(sys_bash, {"--login"});

        const FilePath git_exe = FilePath::fromUserInput(QStandardPaths::findExecutable("git.exe"));
        if (git_exe.exists()) {
            FilePath git_bash = git_exe.parentDir().parentDir().pathAppended("bin/bash.exe");
            if (git_bash.exists()) {
                shells << detectedProfile(
                    git_bash,
                    {"--login"},
                    {},
                    git_bash.parentDir().parentDir().pathAppended("git-bash.exe"));
            }
        }

        const QStringList msys2_args{"-defterm", "-no-start", "-here"};
        for (const FilePath &msys2_path : msysPaths()) {
            const FilePath msys2_cmd = msys2_path.pathAppended("msys2_shell.cmd");
            for (const FilePath &type : msys2_path.dirEntries({{"*.ico"}})) {
                // Only list this type if it has anything in /bin
                QDirIterator it(type.path().replace(".ico", "/bin"), QDir::Files);
                if (!it.hasNext())
                    continue;
                shells << detectedProfile(msys2_cmd,
                                          msys2_args + QStringList{"-" + type.baseName()},
                                          type.toUserOutput().replace(".ico", ".exe"),
                                          type);
            }
        }
    } else {
        FilePath shellsFile = FilePath::fromString("/etc/shells");
        const auto shellFileContent = shellsFile.fileContents();
        QTC_ASSERT_RESULT(shellFileContent, return shells);

        QString shellFileContentString = QString::fromUtf8(*shellFileContent);

        // Filter out comments ...
        const QStringList lines
            = Utils::filtered(shellFileContentString.split('\n', Qt::SkipEmptyParts),
                              [](const QString &line) { return !line.trimmed().startsWith('#'); });

        // Convert lines to file paths ...
        const FilePaths paths = Utils::transform(lines, [](const QString &line) {
            return FilePath::fromUserInput(line.trimmed());
        });

        // ... and filter out non-existing shells.
        shells = Utils::transform(Utils::filtered(paths, &FilePath::exists),
                                  [](const FilePath &shell) { return detectedProfile(shell); });
    }
    return shells;
}

static QList<ShellProfile> detectShellProfiles()
{
    const ShellProfile system = systemShellProfile();
    QList<ShellProfile> profiles{system};
    for (const ShellProfile &shell : detectInstalledShells()) {
        if (shell.executable != system.executable)
            profiles << shell;
    }
    return profiles;
}

/*!
    Returns the shells found on this machine, the system shell first.

    The list is collected once per session.
*/
QList<ShellProfile> detectedShellProfiles()
{
    static const QList<ShellProfile> profiles = detectShellProfiles();
    return profiles;
}

QList<ShellModelItem> remoteShells()
{
    QList<ShellModelItem> result;

    ProjectExplorer::DeviceManager::forEachDevice(
        [&result](const std::shared_ptr<const ProjectExplorer::IDevice> &device) {
            if (device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
                result << ShellModelItem{device->displayName(), {CommandLine{device->rootPath()}}};
        });

    return result;
}

/*!
    Completes \a parameters for a new terminal.

    Without a shell of their own, they get the one of \a defaultProfile, unless
    the terminal is to start in a directory on another device. That terminal
    gets the device's shell instead, which is not decided here.

    The directory the terminal starts in is the one in \a parameters, then the
    one of \a defaultProfile if its shell is used, then \a defaultDirectory.
*/
Utils::Terminal::OpenTerminalParameters withDefaultShell(
    const Utils::Terminal::OpenTerminalParameters &parameters,
    const FilePath &defaultDirectory,
    const ShellProfile &defaultProfile)
{
    Utils::Terminal::OpenTerminalParameters result = parameters;
    const FilePath directory = parameters.workingDirectory.value_or(defaultDirectory);

    if (!result.shellCommand && directory.isLocal()) {
        result.shellCommand = defaultProfile.commandLine();
        if (!result.workingDirectory && !defaultProfile.workingDirectory.isEmpty())
            result.workingDirectory = defaultProfile.expandedWorkingDirectory();
        defaultProfile.applyEnvironmentChanges(result.environment);
        if (result.icon.isNull() && !defaultProfile.iconPath.isEmpty())
            result.icon = defaultProfile.icon();
    }

    if (!result.workingDirectory && !defaultDirectory.isEmpty()) {
        if (!result.shellCommand) {
            result.workingDirectory = defaultDirectory;
        } else if (result.shellCommand->executable().ensureReachable(defaultDirectory)) {
            result.workingDirectory
                = result.shellCommand->executable().withNewMappedPath(defaultDirectory);
        }
    }
    return result;
}

} // namespace Terminal::Internal
