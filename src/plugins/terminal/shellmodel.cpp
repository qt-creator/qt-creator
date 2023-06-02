// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "shellmodel.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>

#include <QFileIconProvider>
#include <QStandardPaths>

namespace Terminal::Internal {

using namespace Utils;

FilePaths availableShells()
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        FilePaths shells;

        FilePath comspec = FilePath::fromUserInput(qtcEnvironmentVariable("COMSPEC"));
        shells << comspec;

        if (comspec.fileName() != "cmd.exe") {
            FilePath cmd = FilePath::fromUserInput(QStandardPaths::findExecutable("cmd.exe"));
            shells << cmd;
        }

        FilePath powershell = FilePath::fromUserInput(
            QStandardPaths::findExecutable("powershell.exe"));
        if (powershell.exists())
            shells << powershell;

        FilePath bash = FilePath::fromUserInput(QStandardPaths::findExecutable("bash.exe"));
        if (bash.exists())
            shells << bash;

        FilePath git_bash = FilePath::fromUserInput(QStandardPaths::findExecutable("git.exe"));
        if (git_bash.exists())
            shells << git_bash.parentDir().parentDir().pathAppended("usr/bin/bash.exe");

        FilePath msys2_bash = FilePath::fromUserInput(QStandardPaths::findExecutable("msys2.exe"));
        if (msys2_bash.exists())
            shells << msys2_bash.parentDir().pathAppended("usr/bin/bash.exe");

        return shells;
    } else {
        FilePath shellsFile = FilePath::fromString("/etc/shells");
        const auto shellFileContent = shellsFile.fileContents();
        QTC_ASSERT_EXPECTED(shellFileContent, return {});

        QString shellFileContentString = QString::fromUtf8(*shellFileContent);

        // Filter out comments ...
        const QStringList lines
            = Utils::filtered(shellFileContentString.split('\n', Qt::SkipEmptyParts),
                              [](const QString &line) { return !line.trimmed().startsWith('#'); });

        // Convert lines to file paths ...
        const FilePaths shells = Utils::transform(lines, [](const QString &line) {
            return FilePath::fromUserInput(line.trimmed());
        });

        // ... and filter out non-existing shells.
        return Utils::filtered(shells, [](const FilePath &shell) { return shell.exists(); });
    }
}

struct ShellModelPrivate
{
    QList<ShellModelItem> localShells;
};

ShellModel::ShellModel(QObject *parent)
    : QObject(parent)
    , d(new ShellModelPrivate())
{
    QFileIconProvider iconProvider;

    const FilePaths shells = availableShells();
    for (const FilePath &shell : shells) {
        ShellModelItem item;
        item.icon = iconProvider.icon(shell.toFileInfo());
        item.name = shell.toUserOutput();
        item.openParameters.shellCommand = {shell, {}};
        d->localShells << item;
    }
}

ShellModel::~ShellModel() = default;

QList<ShellModelItem> ShellModel::local() const
{
    return d->localShells;
}

QList<ShellModelItem> ShellModel::remote() const
{
    const auto deviceCmds = Utils::Terminal::Hooks::instance().getTerminalCommandsForDevicesHook()();

    const QList<ShellModelItem> deviceItems = Utils::transform(
        deviceCmds, [](const Utils::Terminal::NameAndCommandLine &item) -> ShellModelItem {
            return ShellModelItem{item.name, {}, {item.commandLine, std::nullopt, std::nullopt}};
        });

    return deviceItems;
}

} // namespace Terminal::Internal
