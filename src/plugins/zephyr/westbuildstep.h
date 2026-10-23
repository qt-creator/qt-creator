// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <functional>

QT_BEGIN_NAMESPACE
class QObject;
class QString;
QT_END_NAMESPACE

namespace ProjectExplorer {
class BuildConfiguration;
class Project;
} // namespace ProjectExplorer

namespace Utils {
class CommandLine;
class FilePath;
} // namespace Utils

namespace Zephyr::Internal {

void setupWestBuildSteps();

Utils::FilePath westWorkingDirectory(const ProjectExplorer::Project *project);
void forgetProjectWorkspace(const Utils::FilePath &projectDir);
QString westBoard(const ProjectExplorer::BuildConfiguration *bc);
Utils::CommandLine westConfigureCommand(const ProjectExplorer::BuildConfiguration *bc);
std::function<void(bool success)> westConfigureRecorder(
    const ProjectExplorer::BuildConfiguration *bc);

#ifdef WITH_TESTS
QObject *createWestBuildStepTest();
QObject *createWestBoardsTest();
#endif

} // namespace Zephyr::Internal
