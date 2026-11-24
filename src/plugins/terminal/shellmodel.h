// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/terminalhooks.h>

#include <QIcon>
#include <QList>
#include <QString>

namespace Terminal::Internal {

struct ShellModelItem
{
    QString name;
    Utils::Terminal::OpenTerminalParameters openParameters;
};

class ShellProfile
{
public:
    Utils::CommandLine unexpandedCommandLine() const;
    Utils::CommandLine commandLine() const;
    Utils::FilePath expandedWorkingDirectory() const;
    void applyEnvironmentChanges(std::optional<Utils::Environment> &environment) const;
    QIcon icon() const;
    Utils::Terminal::OpenTerminalParameters openParameters() const;

    friend bool operator==(const ShellProfile &, const ShellProfile &) = default;

    QString id;
    QString name;
    Utils::FilePath executable;
    QString arguments;
    Utils::EnvironmentChanges environmentChanges;
    Utils::FilePath workingDirectory;
    Utils::FilePath iconPath;
    bool isDetected = false;
};

QString systemShellProfileId();
ShellProfile systemShellProfile();
QList<ShellProfile> detectedShellProfiles();
QList<ShellModelItem> remoteShells();

Utils::Terminal::OpenTerminalParameters withDefaultShell(
    const Utils::Terminal::OpenTerminalParameters &parameters,
    const Utils::FilePath &defaultDirectory,
    const ShellProfile &defaultProfile);

} // namespace Terminal::Internal
