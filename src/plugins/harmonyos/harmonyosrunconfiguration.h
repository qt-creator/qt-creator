// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>

#include <utils/filepath.h>

namespace HarmonyOs::Internal {

// The application bundle name from the generated project's AppScope/app.json5.
QString bundleName(const Utils::FilePath &buildDir, const QString &buildKey);

// Where harmonydeployqt generates the HAP project, which is beside the application target.
Utils::FilePath generatedProjectDir(const Utils::FilePath &buildDir, const QString &buildKey);

// What harmonydeployqt was told, beside the application target. Empty before the first
// build.
Utils::FilePath deploymentSettings(const Utils::FilePath &buildDir, const QString &buildKey);

// The library the build produced, as harmonydeployqt was told about it. Empty when the
// deployment settings are not there yet.
Utils::FilePath applicationLibrary(const Utils::FilePath &buildDir, const QString &buildKey);

// What a project needs in its package beyond what harmonydeployqt stages, written by the
// project's own build into "<target>-harmonyos-extras.json".
class HarmonyOsExtras
{
public:
    Utils::FilePaths resourceDirectories;
    Utils::FilePaths nativePackageFiles;
    QStringList launchArguments;
};

HarmonyOsExtras harmonyOsExtras(const Utils::FilePath &buildDir, const QString &buildKey);

void setupHarmonyOsRunSupport();

} // namespace HarmonyOs::Internal
