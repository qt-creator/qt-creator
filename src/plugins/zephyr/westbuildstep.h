// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QtGlobal>

#include <functional>

QT_BEGIN_NAMESPACE
class QObject;
class QString;
QT_END_NAMESPACE

namespace ProjectExplorer {
class BuildConfiguration;
class Kit;
class Project;
} // namespace ProjectExplorer

namespace Utils {
class CommandLine;
class Environment;
class FilePath;
class Id;
} // namespace Utils

namespace Zephyr::Internal {

void setupWestBuildSteps();

Utils::FilePath westWorkingDirectory(const ProjectExplorer::Project *project);
void forgetProjectWorkspace(const Utils::FilePath &projectDir);
Utils::FilePath onBuildDevice(const ProjectExplorer::Kit *kit, const Utils::FilePath &path);
Utils::FilePath westRunDirectory(const ProjectExplorer::BuildConfiguration *bc);
Utils::FilePath westExecutable(const Utils::FilePath &workingDir,
                               const Utils::Environment &env);
QString westBoard(const ProjectExplorer::BuildConfiguration *bc);
Utils::CommandLine westConfigureCommand(const ProjectExplorer::BuildConfiguration *bc);
std::function<void(bool success)> westConfigureRecorder(
    const ProjectExplorer::BuildConfiguration *bc);

#ifdef WITH_TESTS
QObject *createWestBuildStepTest();
QObject *createWestBoardsTest();
ProjectExplorer::IDevicePtr createWestTestDevice(Utils::Id id,
                                                 const Utils::FilePath &binDir,
                                                 const QString &root);
#endif

} // namespace Zephyr::Internal
