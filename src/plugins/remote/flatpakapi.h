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

namespace Remote::Internal {

inline constexpr char defaultLauncher[] = "/usr/bin/flatpak-spawn";

bool isInsideFlatpakSandbox();

QString hostPathToSandboxPath(const QString &hostPath);
QString sandboxPathToHostPath(const QString &sandboxPath);

Utils::CommandLine hostCommandLine(
    const Utils::FilePath &launcher,
    const QString &workingDirectory,
    const Utils::CommandLine &command,
    const std::optional<Utils::Environment> &environment,
    const QString &markerTemplate,
    const QStringList &unsetNames = {});

Utils::Result<Utils::Environment> detectHostEnvironment(const Utils::FilePath &launcher);

} // namespace Remote::Internal
