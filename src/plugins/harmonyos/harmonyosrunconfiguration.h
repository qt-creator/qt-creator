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

// Where the libraries the application links against are, as harmonydeployqt was told.
Utils::FilePaths libraryDirectories(const Utils::FilePath &deploymentSettings);

// The file a DT_NEEDED entry names, in the directories above. A versioned name falls back
// to the unversioned one, which is what the libraries built for the platform carry.
Utils::FilePath findLibrary(const QString &name, const Utils::FilePaths &directories);

// What the application needs beside itself, and the name the runner caches that set under.
// An installed package carries the Qt it was built with and nothing else, so an application
// built against another one - which is every application a Qt Creator on the device builds,
// because the Qt it installs lands in its own storage - has to be given it.
class QtLibraries
{
public:
    QString tag;
    Utils::FilePaths files;
};

// The transitive DT_NEEDED closure of the application, as far as the directories can answer
// it. What they cannot is the device's own business, exactly as in packaging. The tag covers
// what the files are as well as which, so a Qt that was rebuilt is a different one.
QtLibraries qtLibraries(const Utils::FilePath &library, const Utils::FilePaths &directories);

// What a project needs in its package beyond what harmonydeployqt stages, written by the
// project's own build into "<target>-harmonyos-extras.json".
class HarmonyOsExtras
{
public:
    Utils::FilePaths resourceDirectories;
    Utils::FilePaths nativePackageFiles;
    QStringList launchArguments;
    // Schemes of the implicit wants the package answers.
    QStringList launchSchemes;
    // Whether the native package is to carry a toolchain for building on the device,
    // which only Qt Creator's own package has a use for.
    bool deviceToolchain = false;
};

HarmonyOsExtras harmonyOsExtras(const Utils::FilePath &buildDir, const QString &buildKey);

void setupHarmonyOsRunSupport();

} // namespace HarmonyOs::Internal
