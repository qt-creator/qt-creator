// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shellcommand.h"

using namespace Utils;

namespace Remote::Internal {

void addShellInvocation(
    CommandLine *launcher,
    const CommandLine &command,
    const std::optional<Environment> &environment,
    const QString &markerTemplate,
    OsType osType,
    const QStringList &unsetNames)
{
    // Around the shell, not around the command inside it: the test below runs
    // in the shell, and has to look the executable up in the PATH the command
    // is then run with.
    if (environment) {
        launcher->addArg("env");
        for (const QString &name : unsetNames)
            launcher->addArgs({"-u", name});
        environment->forEachEntry(
            [launcher, &environment](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    launcher->addArg(key + '=' + environment->expandVariables(value));
            });
    }

    launcher->addArgs({"/bin/sh", "-c"});

    // The executable is quoted for the target's shell rather than the host's,
    // the arguments already are.
    CommandLine inner("exec");
    inner.addArg(command.executable().path(), OsTypeLinux);
    inner.addArgs(command.arguments(), CommandLine::Raw);

    if (markerTemplate.isEmpty()) {
        launcher->addCommandLineAsSingleArg(inner, osType);
        return;
    }

    // The command reports its process id itself, but only after its
    // executable is found, so that a failed start is told apart from a
    // command that failed. exec keeps the id.
    CommandLine testType({"type", {}});
    testType.addArg(command.executable().path(), OsTypeLinux);
    testType.addArgs(">/dev/null", CommandLine::Raw);

    CommandLine echo("echo");
    echo.addArgs(markerTemplate.arg("$$"), CommandLine::Raw);
    echo.addCommandLineWithAnd(inner);

    testType.addCommandLineWithAnd(echo);

    launcher->addCommandLineAsSingleArg(testType, osType);
}

} // namespace Remote::Internal
