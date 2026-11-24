// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/osspecificaspects.h>

#include <QString>
#include <QStringList>

#include <optional>

namespace Remote::Internal {

// Appends "[env ...] /bin/sh -c <script>" to launcher, where the script
// runs command. The script is quoted for the shell of launcher's host
// operating system, osType. A non-empty markerTemplate makes the script
// report its process id once the executable of command is found.
void addShellInvocation(
    Utils::CommandLine *launcher,
    const Utils::CommandLine &command,
    const std::optional<Utils::Environment> &environment,
    const QString &markerTemplate,
    Utils::OsType osType,
    const QStringList &unsetNames = {});

} // namespace Remote::Internal
