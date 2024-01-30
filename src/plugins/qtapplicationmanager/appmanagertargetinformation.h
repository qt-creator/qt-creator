// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>

#include <cmakeprojectmanager/cmakebuildsystem.h>

namespace AppManager {
namespace Internal {

class Manifest
{
public:
    Utils::FilePath filePath;

    QString id;
    QString code;
    QString runtime;

    bool supportsDebugging() const { return isQmlRuntime() || isNativeRuntime(); }
    bool isQmlRuntime() const { return runtime.toLower() == "qml"; }
    bool isNativeRuntime() const { return runtime.toLower() == "native"; }

    Manifest() = default;
    Manifest(const Manifest &other) = default;
};

class TargetInformation final
{
public:
    Manifest manifest;
    Utils::FilePath runDirectory;
    Utils::FilePath packageFilePath;
    QString buildKey;
    QString displayName;
    QString displayNameUniquifier;
    QString cmakeBuildTarget;
    bool isBuiltin = false;

    bool isValid() const;

    TargetInformation() = default;
    TargetInformation(const ProjectExplorer::Target *target);

    static QList<TargetInformation> readFromProject(const ProjectExplorer::Target *target, const QString &buildKey = QString());
};

} // namespace Internal
} // namespace AppManager
