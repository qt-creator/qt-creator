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

Utils::FilePath wslExecutable();

QString decodeWslOutput(const QByteArray &output);

QStringList parseDistributionList(const QString &output);

Utils::Result<QStringList> installedDistributions();
Utils::Result<QStringList> runningDistributions();

inline constexpr char defaultMountRoot[] = "/mnt";

QString normalizeMountRoot(const QString &mountRoot);

QString parseMountRoot(const QString &wslConf);

Utils::Environment parseEnvironment(const QString &envOutput);

Utils::Result<QString> detectMountRoot(const QString &distribution);
Utils::Result<Utils::OsArch> detectArchitecture(const QString &distribution);
Utils::Result<Utils::Environment> detectEnvironment(
    const QString &distribution, const QString &userName);

Utils::Environment withoutWindowsPath(
    const Utils::Environment &environment, const QString &mountRoot);

QString hostDrivePathToWslPath(const QString &mountRoot, const QString &hostPath);

QString hostPathToWslPath(const QString &mountRoot,
                          const QString &distribution,
                          const Utils::FilePath &hostPath);

QString mappedHostPathToWslPath(const QString &mountRoot,
                                const QString &distribution,
                                const QString &hostPath);

QString wslPathToHostPath(const QString &mountRoot,
                          const QString &distribution,
                          const QString &wslPath);

Utils::CommandLine wslCommandLine(
    const Utils::FilePath &wslExecutable,
    const QString &distribution,
    const QString &userName,
    const QString &workingDirectory,
    const Utils::CommandLine &command,
    const std::optional<Utils::Environment> &environment,
    const QString &markerTemplate);

} // namespace Wsl::Internal
